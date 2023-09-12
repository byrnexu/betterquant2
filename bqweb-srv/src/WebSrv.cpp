/*!
 * \file WebSrv.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/20
 *
 * \brief
 */

#include "WebSrv.hpp"

#include <drogon/drogon.h>

#include "CacheOfDBRet.hpp"
#include "CommonIPCData.hpp"
#include "Config.hpp"
#include "ReqBody2CallbackGroup.hpp"
#include "SHMCli.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCTask.hpp"
#include "SHMSrv.hpp"
#include "SessionTable.hpp"
#include "StgEngTaskHandler.hpp"
#include "StgMgr.hpp"
#include "TopicHandler.hpp"
#include "UserId2WSConnGroup.hpp"
#include "WebSrvConst.hpp"
#include "db/DBE.hpp"
#include "def/PosInfo.hpp"
#include "tdeng/TDEngConnpool.hpp"
#include "tdeng/TDEngConst.hpp"
#include "tdeng/TDEngParam.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"
#include "util/Random.hpp"
#include "util/ScheduleTaskBundle.hpp"
#include "util/Scheduler.hpp"
#include "util/StdExt.hpp"
#include "util/String.hpp"
#include "util/SubMgr.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/TopicMgr.hpp"

namespace bq {

int WebSrv::prepareInit() {
  RandomStr::get_mutable_instance().init();
  RandomInt::get_mutable_instance().init();

  //! 创建会话表
  sessionTable_ = std::make_shared<SessionTable>();

  if (const auto ret = Config::get_mutable_instance().init(configFilename_);
      ret != 0) {
    const auto statusMsg = fmt::format("Prepare init failed.");
    std::cerr << statusMsg << std::endl;
    return ret;
  }

  if (const auto ret = InitLogger(configFilename_); ret != 0) {
    const auto statusMsg =
        fmt::format("Init td srv failed because of init logger failed. {}",
                    configFilename_);
    std::cerr << statusMsg << std::endl;
    return ret;
  }

  return 0;
}

int WebSrv::doInit() {
  if (const auto ret = initDBEng(); ret != 0) {
    LOG_E("Do init failed. ");
    return ret;
  }

  if (const auto ret = initTDEng(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  //! 创建策略启停管理模块
  stgMgr_ = std::make_shared<StgMgr>(this);

  initSubMgr();
  initTopicMgr();
  //! topicHandler主要用于处理订阅到的行情和仓位信息，将行情推送到订阅者，将仓位
  //! 信息保存下来用于客户端查询
  topicHandler_ = std::make_shared<TopicHandler>(this);

  //! 请求和应答回调的对应关系
  reqBody2CallbackGroup_ = std::make_shared<ReqBody2CallbackGroup>(this);

  //! userId和WSConn的对应关系
  userId2WSConnGroup_ = std::make_shared<UserId2WSConnGroup>(this);

  cacheOfDBRet_ = std::make_shared<CacheOfDBRet>();
  cacheOfDBRet_->init();

  //! 主要处理来自策略引擎的手工下单应答、手工撤单应答，非手工报撤也接收并推送
  stgEngTaskHandler_ = std::make_shared<StgEngTaskHandler>(this);
  if (const auto ret = initStgEngTaskDispatcher(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  initSHMSrv();

  initScheduleTaskBundle();
  scheduleTaskBundleExecutor_ = std::make_shared<Scheduler>(
      APP_NAME, [this]() { ExecScheduleTaskBundle(scheduleTaskBundle_); },
      1 * 1000);

  return 0;
}

int WebSrv::initStgEngTaskDispatcher() {
  const auto stgEngTaskDispatcherParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               CONFIG["stgEngTaskDispatcherParam"].as<std::string>());
  const auto [ret, stgEngTaskDispatcherParam] =
      MakeTaskDispatcherParam(stgEngTaskDispatcherParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init taskdispatcher failed. {}", stgEngTaskDispatcherParamInStrFmt);
    return ret;
  }

  const auto makeAsyncTask = [](const auto& task) {
    const auto asyncTask = std::make_shared<SHMIPCAsyncTask>(task, std::any());
    return std::make_tuple(0, asyncTask);
  };

  const auto getThreadForAsyncTask = [](const auto& asyncTask,
                                        auto taskSpecificThreadPoolSize) {
    return ThreadNo(0);
  };

  const auto handleAsyncTask = [this](auto& asyncTask) {
    stgEngTaskHandler_->handleAsyncTask(asyncTask);
  };

  stgEngTaskDispatcher_ = std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
      stgEngTaskDispatcherParam, makeAsyncTask, getThreadForAsyncTask,
      handleAsyncTask);
  stgEngTaskDispatcher_->init();

  return ret;
}

void WebSrv::initSHMSrv() {
  const auto stgEngChannel =
      fmt::format("{}@{}", APP_NAME, CONFIG["stgEngChannel"].as<std::string>());
  shmSrvOfStgEng_ = std::make_shared<SHMSrv>(
      stgEngChannel, [this](const auto* shmBuf, std::size_t shmBufLen) {
        auto task = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
        stgEngTaskDispatcher_->dispatch(task);
      });
}

void WebSrv::initSubMgr() {
  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    topicHandler_->handleData(shmBuf, shmBufLen);
  };
  subMgr_ = std::make_shared<SubMgr>(APP_NAME, onSHMDataRecv);
  subMgr_->sub(0, "RISK@PubChannel@Trade@PosInfo@All");
}

void WebSrv::initTopicMgr() {
  topicMgr_ = std::make_shared<TopicMgr>(
      TopicMgrRole::Cli, [this](const auto& anyData) {
        //! mdSvcIdentity = "@MD@"
        const auto mdSvcIdentity =
            fmt::format("{}{}{}", SEP_OF_SHM_SVC, TOPIC_PREFIX_OF_MARKET_DATA,
                        SEP_OF_SHM_SVC);
        const auto subscriber2TopicGroupInJsonFmt =
            std::any_cast<std::string>(anyData);
        const auto addr2SHMCliGroup = subMgr_->getSHMCliGroup();
        const auto shmBufLen =
            sizeof(CommonIPCData) + subscriber2TopicGroupInJsonFmt.size() + 1;
        for (const auto& addr2SHMCli : addr2SHMCliGroup) {
          //! 如果不是行情服务(比如风控)，那么跳过
          if (!boost::contains(addr2SHMCli.first, mdSvcIdentity)) {
            continue;
          }
          //! 遍历所有行情服务端同步订阅信息
          addr2SHMCli.second->asyncSendMsgWithZeroCopy(
              [&](void* shmBuf) {
                auto commonIPCData = static_cast<CommonIPCData*>(shmBuf);
                memcpy(commonIPCData->data_,
                       subscriber2TopicGroupInJsonFmt.c_str(),
                       subscriber2TopicGroupInJsonFmt.size());
                commonIPCData->dataLen_ =
                    subscriber2TopicGroupInJsonFmt.size() + 1;
              },
              MSG_ID_SYNC_SUB_INFO, shmBufLen);
        }
      });
}

int WebSrv::initDBEng() {
  const auto dbEngParam = SetParam(db::DEFAULT_DB_ENG_PARAM,
                                   CONFIG["dbEngParam"].as<std::string>());
  int retOfMakeDBEng = 0;
  std::tie(retOfMakeDBEng, dbEng_) = db::MakeDBEng(
      dbEngParam, [](db::DBTaskSPtr& dbTask, const StringSPtr& dbExecRet) {
        LOG_D("Exec sql finished. [{}] [exec result = {}]", dbTask->toStr(),
              *dbExecRet);
      });
  if (retOfMakeDBEng != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfMakeDBEng;
  }

  if (auto retOfInit = getDBEng()->init(); retOfInit != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfInit;
  }

  return 0;
}

int WebSrv::initTDEng() {
  const auto tdEngParamInStrFmt = SetParam(
      tdeng::DEFAULT_TDENG_PARAM, CONFIG["tdEngParam"].as<std::string>());
  const auto [ret, tdEngParam] = tdeng::MakeTDEngParam(tdEngParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init failed. {}", tdEngParamInStrFmt);
    return ret;
  }

  tdEngConnpool_ = std::make_shared<tdeng::TDEngConnpool>(tdEngParam);
  if (const auto statusCode = tdEngConnpool_->init(); statusCode != 0) {
    LOG_E("Init failed. {}", tdEngParamInStrFmt);
    return -1;
  }

  return 0;
}

void WebSrv::initScheduleTaskBundle() {
  scheduleTaskBundle_ = std::make_shared<ScheduleTaskBundle>();
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "removeExpiredSession",
      [&, this]() {
        LOG_D("Try to remove expired session.");
        const auto thresholdForSessionTimeout =
            CONFIG["thresholdForSessionTimeout"].as<std::uint32_t>(8 * 3600);
        getSessionTable()->removeExpiredSession(thresholdForSessionTimeout);
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(60000)));
}

int WebSrv::doRun() {
  LOG_I("Start web srv.");
  getDBEng()->start();
  stgMgr_->start();
  stgEngTaskDispatcher_->start();
  subMgr_->start();
  shmSrvOfStgEng_->start();
  startDrogon();
  if (const auto ret = scheduleTaskBundleExecutor_->start(); ret != 0) {
    LOG_E("Start scheduler of multi task failed.");
    return ret;
  }
  return 0;
}

void WebSrv::startDrogon() {
  LOG_I("Start thunderbolt.");
  threadDrogon_ = std::make_shared<std::thread>([]() {
    drogon::app().loadConfigFile("config/bqweb-srv/config.json");
    drogon::app().disableSigtermHandling();
    //! 以下代码解决cors跨域问题，这样就不用每隔应答都设置"Access-Control-Allow-Origin"了
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr& req,
           const drogon::HttpResponsePtr& resp) {
          LOG_D("Register post handling advice.");
          resp->addHeader("Access-Control-Allow-Origin", "*");
          resp->addHeader("Access-Control-Allow-Methods", "*");
          resp->addHeader("Access-Control-Allow-Headers", "*");
          resp->addHeader("Access-Control-Max-Age", "86400");
        });
    drogon::app().run();
  });
}

void WebSrv::doExit(const boost::system::error_code* ec, int signalNum) {
  scheduleTaskBundleExecutor_->stop();
  stopDrogon();
  LOG_I("Stop thunderbolt.");
  stgEngTaskDispatcher_->stop();
  stgMgr_->stop();
  shmSrvOfStgEng_->stop();
  subMgr_->stop();
  tdEngConnpool_->uninit();
  getDBEng()->stop();
  LOG_I("Stop web srv.");
}

void WebSrv::stopDrogon() {
  drogon::app().quit();
  if (threadDrogon_->joinable()) {
    threadDrogon_->join();
  }
}

int WebSrv::sub(UserId userId, const std::string& topic) {
  LOG_I("User {} sub topic {}.", userId, topic);
  const auto statusCode = subMgr_->sub(userId, topic);
  const auto s = fmt::format("{}-{}", APP_NAME, userId);
  topicMgr_->add(s, topic);
  return statusCode;
}

int WebSrv::unSub(UserId userId, const std::string& topic) {
  LOG_I("User {} unSub topic {}.", userId, topic);
  const auto statusCode = subMgr_->unSub(userId, topic);
  const auto s = fmt::format("{}-{}", APP_NAME, userId);
  topicMgr_->remove(s, topic);
  return statusCode;
}

void WebSrv::unSubAllTopic(UserId userId) {
  LOG_I("User {} unSub all topic.", userId);
  subMgr_->unSubAllTopic(userId);
}

void WebSrv::setPosSnapshotOfAll(const PosSnapshotSPtr& value) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxPosSnapshot_);
    posSnapshot_ = value;
  }
}

PosSnapshotSPtr WebSrv::getPosSnapshotOfAll() const {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxPosSnapshot_);
    return posSnapshot_;
  }
}

std::tuple<int, Key2PnlGroupSPtr> WebSrv::queryPnlGroupBy(
    const std::string& groupCond) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxPosSnapshot_);
    if (!posSnapshot_) {
      return {0, std::make_shared<Key2PnlGroup>()};
    }
    const auto [statusCode, key2PnlGroup] = posSnapshot_->queryPnlGroupBy(
        groupCond, CN_OFFICIAL_CURRENCY, CN_OFFICIAL_CURRENCY,
        CN_OFFICIAL_CURRENCY);
    return {statusCode, key2PnlGroup};
  }
}

}  // namespace bq
