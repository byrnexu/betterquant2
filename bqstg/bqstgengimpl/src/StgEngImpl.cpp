/*!
 * \file StgEngImpl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgEngImpl.hpp"

#include "AlgoMgr.hpp"
#include "CommonIPCData.hpp"
#include "DynCandleSvc.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "PosMgrOfStgInst.hpp"
#include "SHMIPC.hpp"
#include "StgEngConst.hpp"
#include "StgEngDef.hpp"
#include "StgEngUtil.hpp"
#include "StgInstTaskHandlerImpl.hpp"
#include "SysInstructionSvc.hpp"
#include "WebSrvTaskHandler.hpp"
#include "db/DBE.hpp"
#include "db/DBEngConst.hpp"
#include "db/TBLMonitorOfStgInstInfo.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "db/TBLOrderInfo.hpp"
#include "db/TBLPosInfo.hpp"
#include "db/TBLStgInfo.hpp"
#include "db/TBLStgInstInfo.hpp"
#include "db/TBLStgLoggerInfo.hpp"
#include "def/AssetInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfStg.hpp"
#include "def/Def.hpp"
#include "def/Pnl.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "def/SyncTask.hpp"
#include "tdeng/TDEngConnpool.hpp"
#include "tdeng/TDEngConst.hpp"
#include "tdeng/TDEngParam.hpp"
#include "util/AcctInfoCache.hpp"
#include "util/File.hpp"
#include "util/Literal.hpp"
#include "util/LoggerUtil.hpp"
#include "util/MarketDataCache.hpp"
#include "util/MarketDataCond.hpp"
#include "util/ProductInfoCache.hpp"
#include "util/Random.hpp"
#include "util/ScheduleTaskBundle.hpp"
#include "util/Scheduler.hpp"
#include "util/StgInfoCache.hpp"
#include "util/StgLoggerInfo.hpp"
#include "util/String.hpp"
#include "util/SubMgr.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/TopicMgr.hpp"
#include "util/TrdAcctInfoCache.hpp"
#include "util/Util.hpp"

namespace bq::stg {

StgEngImpl::StgEngImpl(const std::string& configFilename)
    : SvcBase(configFilename) {}

int StgEngImpl::prepareInit() {
  syncTaskGroup_.reserve(1024);

  try {
    config_ = YAML::LoadFile(configFilename_);
  } catch (const std::exception& e) {
    std::cerr << fmt::format("Open config filename {} failed. [{}]",
                             configFilename_, e.what())
              << std::endl;
    return SCODE_STG_ENG_INVALID_CONFIG_FILENAME;
  }

  stgId_ = getConfig()["stgId"].as<StgId>();
  appName_ = fmt::format("Stg-{}", getStgId());
  rootDirOfStgPrivateData_ = fmt::format(
      "{}/{}", getConfig()["rootDirOfStgPrivateData"].as<std::string>(),
      getStgId());

  const auto ret = InitLogger(configFilename_);
  if (ret != 0) {
    const auto statusMsg =
        fmt::format("Init stg {} failed because of init logger failed. {}",
                    getStgId(), configFilename_);
    std::cerr << statusMsg << std::endl;
    return ret;
  }

  if (auto ret = initDBEng(); ret != 0) {
    logError("[{}] Init stg {} failed because of init dbeng failed.",
             {appName_, std::to_string(getStgId())});
    return ret;
  }
  getDBEng()->start();

  //! dftStgInstInfo_用于日志输出接口，有一些日志输出和策略无关，比如数据库的启动情况等
  initDftStgInstInfo();

  return 0;
}

int StgEngImpl::initDBEng() {
  const auto dbEngParam = SetParam(db::DEFAULT_DB_ENG_PARAM,
                                   getConfig()["dbEngParam"].as<std::string>());
  int retOfMakeDBEng = 0;
  std::tie(retOfMakeDBEng, dbEng_) = db::MakeDBEng(
      dbEngParam, [this](db::DBTaskSPtr& dbTask, const StringSPtr& dbExecRet) {
        logDebug("[{}] Exec sql finished. [{}] [exec result = {}]",
                 {appName_, dbTask->toStr(), *dbExecRet});
      });
  if (retOfMakeDBEng != 0) {
    logError("[{}] Init dbeng failed. {}", {appName_, dbEngParam});
    return retOfMakeDBEng;
  }

  if (auto retOfInit = getDBEng()->init(); retOfInit != 0) {
    logError("[{}] Init dbeng failed. {}", {appName_, dbEngParam});
    return retOfInit;
  }

  return 0;
}

void StgEngImpl::initDftStgInstInfo() {
  //! 第一时间获取dftStgInstInfo_，使得后面的日志可以使用它
  const auto sql =
      fmt::format("SELECT * from {} where stgId = {} and stgInstId = 1",
                  TBLStgInstInfo::TableName, stgId_);
  const auto [statusCode, execRet] = syncExecSql(sql);
  Doc doc;
  doc.Parse(execRet.data());
  dftStgInstInfo_ = std::make_shared<StgInstInfo>();
  dftStgInstInfo_->stgId_ = getStgId();
  dftStgInstInfo_->stgInstId_ = 1;
  dftStgInstInfo_->userId_ = doc["recordSetGroup"][0][0]["userId"].GetUint();
  logInfo("[{}] Get default stg inst info. [{}]",
          {appName_, getDftStgInstInfo()->toStr()});
}

int StgEngImpl::doInit() {
  //! 初始化策略实例监控模块
  initTBLMonitorOfStgInstInfo();

  //! 初始化代码表监控模块
  initTBLMonitorOfSymbolInfo();

  if (const auto ret = initTDEng(); ret != 0) {
    logError("Do init failed because of init tdeng failed.",
             getDftStgInstInfo());
    return ret;
  }

  //! 系统预设人工干预指令处理模块，比如撤销实例层面所有订单的指令
  sysInstructionSvc_ = std::make_shared<SysInstructionSvc>(this);

  //! 算法单管理模块
  algoMgr_ = std::make_shared<algo::AlgoMgr>(this);
  if (const auto ret = algoMgr_->init(); ret != 0) {
    logError("Do init failed because of algomgr failed.", getDftStgInstInfo());
    return ret;
  }

  productInfoCache_ = std::make_shared<ProductInfoCache>(getDBEng());
  stgInfoCache_ = std::make_shared<StgInfoCache>(getDBEng());
  acctInfoCache_ = std::make_shared<AcctInfoCache>(getDBEng());
  trdAcctInfoCache_ = std::make_shared<TrdAcctInfoCache>(getDBEng());

  //! 市场行情也就是实时价格缓存
  marketDataCache_ = std::make_shared<MarketDataCache>();

  //! 来自web服务的请求处理，比如人工报撤单请求、人工干预指令的处理
  webSrvTaskHandler_ = std::make_shared<WebSrvTaskHandler>(this);

  //! 动态k线生成模块
  dynCandle_ = std::make_shared<DynCandleSvc>(this);

  initSubMgr();
  initTopicMgr();

  initOrdMgr();
  initPosMgr();

  initStgInstTaskDispatcher();

  //! 初始化交易服务客户端
  initSHMCliOfTDSrv();

  //! 初始化风控子系统客户端
  initSHMCliOfRiskMgr();

  //! 初始化web服务客户端
  initSHMCliOfWebSrv();

  //! 初始化动态k线生成模块
  dynCandle_->init();

  //! 为了确保策略引擎提供的定时任务api触发时间点准确性，用独立定时器处理也就是
  //! 独立线程处理定时任务
  scheduleTaskBundleOfTimer_ = std::make_shared<ScheduleTaskBundle>();
  initScheduleTaskBundleOfTimer();
  scheduleTaskBundleExecutorOfTimer_ = std::make_shared<Scheduler>(
      fmt::format("{}OfTimer", getAppName()),
      [this]() {
        auto scheduleTaskBundle = getScheduleTaskBundleOfTimer();
        ExecScheduleTaskBundle(scheduleTaskBundle);
      },
      MilliSecInterval(1));

  //! 处理其他定时任务的定时器
  scheduleTaskBundle_ = std::make_shared<ScheduleTaskBundle>();
  initScheduleTaskBundle();
  scheduleTaskBundleExecutor_ = std::make_shared<Scheduler>(
      getAppName(), [this]() { ExecScheduleTaskBundle(scheduleTaskBundle_); },
      MilliSecInterval(1));

  return 0;
}

int StgEngImpl::initTDEng() {
  const auto tdEngParamInStrFmt = SetParam(
      tdeng::DEFAULT_TDENG_PARAM, getConfig()["tdEngParam"].as<std::string>());
  const auto [ret, tdEngParam] = tdeng::MakeTDEngParam(tdEngParamInStrFmt);
  if (ret != 0) {
    logError("Init tdeng failed. {}", {tdEngParamInStrFmt},
             getDftStgInstInfo());
    return ret;
  }

  tdEngConnpool_ = std::make_shared<tdeng::TDEngConnpool>(tdEngParam);
  if (const auto statusCode = tdEngConnpool_->init(); statusCode != 0) {
    logError("Init tdeng failed. {}", {tdEngParamInStrFmt},
             getDftStgInstInfo());
    return -1;
  }

  return 0;
}

void StgEngImpl::initTBLMonitorOfSymbolInfo() {
  const auto milliSecIntervalOfTBLMonitorOfSymbolInfo =
      getConfig()["milliSecIntervalOfTBLMonitorOfSymbolInfo"]
          .as<std::uint32_t>();

  const auto prefixOfSql =
      fmt::format("SELECT * from {}", TBLSymbolInfo::TableName);
  const auto condOfSql =
      getConfig()["tblMonitorOfSymbolInfo"].as<std::string>();
  std::string sql;
  if (condOfSql.empty()) {
    sql = fmt::format("{};", prefixOfSql);
  } else {
    sql = fmt::format("{} WHERE {};", prefixOfSql, condOfSql);
  }

  const auto monitorSymbolTableChanges =
      getConfig()["monitorSymbolTableChanges"].as<bool>(true);
  const auto enableMonitoring = monitorSymbolTableChanges
                                    ? db::EnableMonitoring::True
                                    : db::EnableMonitoring::False;

  tblMonitorOfSymbolInfo_ = std::make_shared<db::TBLMonitorOfSymbolInfo>(
      getDBEng(), milliSecIntervalOfTBLMonitorOfSymbolInfo, sql, nullptr,
      enableMonitoring);
}

void StgEngImpl::initTBLMonitorOfStgInstInfo() {
  const auto cbOnStgInstInfoChg = [this](const auto& tblRecSetAdd,
                                         const auto& tblRecSetDel,
                                         const auto& tblRecSetChg) {
    for (const auto tblRec : *tblRecSetAdd) {
      const auto stgInstId = tblRec.second->getRecWithAllFields()->stgInstId;
      auto asynTask = MakeStgSignal(MSG_ID_ON_STG_INST_ADD, stgInstId);
      stgInstTaskDispatcher_->dispatch(asynTask);
    }
    for (const auto tblRec : *tblRecSetDel) {
      const auto stgInstId = tblRec.second->getRecWithAllFields()->stgInstId;
      auto asynTask = MakeStgSignal(MSG_ID_ON_STG_INST_DEL, stgInstId);
      stgInstTaskDispatcher_->dispatch(asynTask);
    }
    for (const auto tblRec : *tblRecSetChg) {
      const auto stgInstId = tblRec.second->getRecWithAllFields()->stgInstId;
      auto asynTask = MakeStgSignal(MSG_ID_ON_STG_INST_CHG, stgInstId);
      stgInstTaskDispatcher_->dispatch(asynTask);
    }
  };

  const auto milliSecIntervalOfTBLMonitorOfStgInstInfo =
      getConfig()["milliSecIntervalOfTBLMonitorOfStgInstInfo"]
          .as<std::uint32_t>();
  const auto sql = fmt::format(
      "SELECT a.`productId`, a.`stgId`, a.`stgName`, "
      "a.`userIdOfAuthor`, b.`stgInstId`, b.`stgInstParams`, b.`stgInstName`, "
      "b.`userId`, b.`isDel` FROM {} a, {} b WHERE "
      "a.`stgId` = {} AND a.`stgId` = b.`stgId` AND b.`isDel` = 0; ",
      TBLStgInfo::TableName, TBLStgInstInfo::TableName, getStgId());
  tblMonitorOfStgInstInfo_ = std::make_shared<db::TBLMonitorOfStgInstInfo>(
      getDBEng(), milliSecIntervalOfTBLMonitorOfStgInstInfo, sql,
      cbOnStgInstInfoChg);
}

void StgEngImpl::initSubMgr() {
  //!
  //! 订阅的消息结构体里没有StgInstId字段，因此在这里找到订阅者也就是StgInstId，
  //! 然后再生成带有StgInstId的asyncTask再分发。
  //!
  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
    if (shmHeader->msgId_ == MSG_ID_ON_MD_LAST_PRICE) {
      //! 这里收到的是所有品种的LastPrice，所以不dispatch，而是在handle里先过滤下
      dynCandle_->handle(shmBuf, shmBufLen);
    }

    //! 发送一份行情给算法交易引擎，注意这里是所有行情，在algoMgr中会过滤为订阅的行情
    auto shmIPCTask = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
    getAlgoMgr()->handle(shmIPCTask);

    //! 发送行情给所有的订阅此行情的策略实例
    const auto subscriberGroup =
        subMgr_->getSubscriberGroupByTopicHash(shmHeader->topicHash_);
    for (auto stgInstId : subscriberGroup) {
      auto asyncTask = std::make_shared<SHMIPCAsyncTask>(shmIPCTask, stgInstId);
      stgInstTaskDispatcher_->dispatch(asyncTask);
    }
  };

  subMgr_ = std::make_shared<SubMgr>(appName_, onSHMDataRecv);
}

void StgEngImpl::initTopicMgr() {
  topicMgr_ = std::make_shared<TopicMgr>(
      TopicMgrRole::Cli, [this](const auto& anyData) {
        //! 行情服务的 mdSvcIdentity = @MD@
        const auto mdSvcIdentity =
            fmt::format("{}{}{}", SEP_OF_SHM_SVC,  //
                        TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_SHM_SVC);
        //! json格式的订阅信息
        const auto subscriber2TopicGroupInJsonFmt =
            std::any_cast<std::string>(anyData);
        //! json格式的订阅信息的长度
        const auto shmBufLen =
            sizeof(CommonIPCData) + subscriber2TopicGroupInJsonFmt.size() + 1;
        //! 将订阅信息同步给所有的服务
        const auto addr2SHMCliGroup = subMgr_->getSHMCliGroup();
        for (const auto& addr2SHMCliGroup : addr2SHMCliGroup) {
          //! 如果不是行情服务(比如风控)，那么跳过
          if (!boost::contains(addr2SHMCliGroup.first, mdSvcIdentity)) {
            continue;
          }
          //! 遍历所有行情服务端同步订阅信息
          addr2SHMCliGroup.second->asyncSendMsgWithZeroCopy(
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

void StgEngImpl::initSHMCliOfTDSrv() {
  //! 获取addr
  const auto stgEngChannelOfTDSrv =
      getConfig()["stgEngChannelOfTDSrv"].as<std::string>();
  const auto addr =
      fmt::format("{}{}{}", appName_, SEP_OF_SHM_SVC, stgEngChannelOfTDSrv);

  //! 定义共享内存数据到达的回报
  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
    StgInstId stgInstId = 1;
    //! 除了以下类型的消息，其他类型的消息(比如MSG_ID_ON_STG_REG)都在1号实例的线程中触发回调
    if (shmHeader->msgId_ == MSG_ID_ON_ORDER_RET ||
        shmHeader->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      stgInstId = static_cast<const OrderInfo*>(shmBuf)->stgInstId_;
    }

    //! 发送一份将来自交易服务的消息至算法交易引擎
    auto shmIPCTask = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
    getAlgoMgr()->handle(shmIPCTask);

    //! 将来自交易服务的消息发给策略实例处理
    auto asyncTask = std::make_shared<SHMIPCAsyncTask>(shmIPCTask, stgInstId);
    stgInstTaskDispatcher_->dispatch(asyncTask);
  };

  shmCliOfTDSrv_ = std::make_shared<SHMCli>(addr, onSHMDataRecv);
  shmCliOfTDSrv_->setClientChannel(getStgId());
}

void StgEngImpl::initSHMCliOfRiskMgr() {
  //! 获取addr
  const auto stgEngChannelOfRiskMgr =
      getConfig()["stgEngChannelOfRiskMgr"].as<std::string>();
  const auto addr =
      fmt::format("{}{}{}", appName_, SEP_OF_SHM_SVC, stgEngChannelOfRiskMgr);

  //! 定义共享内存数据到达的回报
  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
    StgInstId stgInstId = 1;
    //! 除了以下类型的消息，其他类型的消息都在1号实例的线程中触发回调
    if (shmHeader->msgId_ == MSG_ID_ON_ORDER_RET ||
        shmHeader->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      stgInstId = static_cast<const OrderInfo*>(shmBuf)->stgInstId_;
    }
    auto asyncTask = std::make_shared<SHMIPCAsyncTask>(
        std::make_shared<SHMIPCTask>(shmBuf, shmBufLen), stgInstId);
    stgInstTaskDispatcher_->dispatch(asyncTask);
  };

  shmCliOfRiskMgr_ = std::make_shared<SHMCli>(addr, onSHMDataRecv);
  shmCliOfRiskMgr_->setClientChannel(getStgId());
}

void StgEngImpl::initSHMCliOfWebSrv() {
  const auto stgEngChannelOfWebSrv =
      getConfig()["stgEngChannelOfWebSrv"].as<std::string>();
  const auto addr =
      fmt::format("{}{}{}", appName_, SEP_OF_SHM_SVC, stgEngChannelOfWebSrv);

  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    webSrvTaskHandler_->handleTask(shmBuf, shmBufLen);
  };

  shmCliOfWebSrv_ = std::make_shared<SHMCli>(addr, onSHMDataRecv);
  //! 订阅 web srv 的 stgId channel
  shmCliOfWebSrv_->setClientChannel(getStgId());
}

void StgEngImpl::initOrdMgr() {
  const auto filled = magic_enum::enum_integer(OrderStatus::Filled);
  ordMgr_ = std::make_shared<StgOrdMgr>();
  const auto sql = fmt::format(
      "SELECT * FROM {} WHERE `stgId` = {} AND `orderStatus` < {}; ",
      TBLOrderInfo::TableName, getStgId(), filled);
  getOrdMgr()->init(getConfig(), getDBEng(), sql);
}

void StgEngImpl::initPosMgr() {
  const auto sql = fmt::format("SELECT * FROM {} WHERE `stgId` = {}",
                               TBLPosInfo::TableName, getStgId());
  posMgr_ = std::make_shared<StgPosMgr>();
  getPosMgr()->init(getConfig(), getDBEng(), sql);
}

int StgEngImpl::initStgInstTaskDispatcher() {
  const auto stgInstTaskDispatcherParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               getConfig()["stgInstTaskDispatcherParam"].as<std::string>());
  const auto [ret, stgInstTaskDispatcherParam] =
      MakeTaskDispatcherParam(stgInstTaskDispatcherParamInStrFmt);
  if (ret != 0) {
    logError("[{}] Init stginst taskdispatcher failed. {}",
             {appName_, stgInstTaskDispatcherParamInStrFmt},
             getDftStgInstInfo());
    return ret;
  }

  //! 因为stgInstId是从1开始，threadNo是从0开始，所以stgInstId - 1
  const auto getThreadForAsyncTask = [](const auto& asyncTask,
                                        auto taskSpecificThreadPoolSize) {
    const auto stgInstId = std::any_cast<StgInstId>(asyncTask->arg_);
    return (stgInstId - 1) % taskSpecificThreadPoolSize;
  };

  const auto handleAsyncTask = [this](auto& asyncTask) {
    stgInstTaskHandler_->handleAsyncTask(asyncTask);
  };

  //!
  //! 订阅的消息和ON_STG_START等事件的StgInstId获取的来源不一样，因此无法在统一
  //! 的函数里创建asyncTask，所以在得到task的地方生成asyncTask。
  //!
  stgInstTaskDispatcher_ = std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
      stgInstTaskDispatcherParam, nullptr, getThreadForAsyncTask,
      handleAsyncTask);
  stgInstTaskDispatcher_->init();

  return ret;
}

void StgEngImpl::initScheduleTaskBundleOfTimer() {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxScheduleTaskBundleOfTimer_);
    scheduleTaskBundleOfTimer_->emplace_back(std::make_shared<ScheduleTask>(
        "timerOfFixedTs",
        [this]() {
          execTaskBunndleOfTimer();
          return true;
        },
        ExecAtStartup::True, MilliSecInterval(1)));
  }
}

void StgEngImpl::execTaskBunndleOfTimer() {
  //! 将时间从 YYYY-MM-DDTHH:MM:SS 格式转换为 Y2020M01D23h10m15s20
  auto convertLocalTs = [](auto& localTsInStrFmt) {
    localTsInStrFmt[04] = 'M';
    localTsInStrFmt[07] = 'D';
    localTsInStrFmt[10] = 'h';
    localTsInStrFmt[13] = 'm';
    localTsInStrFmt[16] = 's';
    localTsInStrFmt = "Y" + localTsInStrFmt;
  };

  //! clone taskOfFixedTimeGroup_
  std::vector<TaskOfFixedTimeSPtr> taskOfFixedTimeGroup;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTaskOfFixedTimeGroup_);
    std::for_each(std::begin(taskOfFixedTimeGroup_),
                  std::end(taskOfFixedTimeGroup_),
                  [&taskOfFixedTimeGroup](const auto& rec) {
                    taskOfFixedTimeGroup.emplace_back(
                        std::make_shared<TaskOfFixedTime>(*rec));
                  });
  }

  const auto oneSec = boost::posix_time::seconds(1);
  const auto now = boost::posix_time::second_clock::universal_time();
  if (prevExecTaskBunndleOfTimer_ == boost::posix_time::not_a_date_time) {
    prevExecTaskBunndleOfTimer_ = now;
  }

  //! 从上次检测时间到现在之间的所有时间点有没有被触发的task
  for (auto ts = prevExecTaskBunndleOfTimer_; ts <= now; ts += oneSec) {
    for (const auto& task : taskOfFixedTimeGroup) {
      //! 根据时区转换为本地时间
      const auto localTs = ts + boost::posix_time::hours(task->timeZone_);
      //! 本地时间转换为YYYY-MM-DDTHH:MM:SS格式
      auto localTsInStrFmt = boost::posix_time::to_iso_extended_string(localTs);
      convertLocalTs(localTsInStrFmt);

      if (boost::contains(localTsInStrFmt, task->execTime_)) {
        auto asynTask = MakeStgSignal(  //
            MSG_ID_ON_STG_INST_TIMER, task->stgInstId_, task->timerName_);
        stgInstTaskDispatcher_->dispatch(asynTask);
      }
    }
  }

  prevExecTaskBunndleOfTimer_ = now + oneSec;
}

void StgEngImpl::initScheduleTaskBundle() {
  //! 定时检测产品信息
  const auto milliSecIntervalOfReloadProduct =
      getConfig()["milliSecIntervalOfReloadProduct"].as<std::uint32_t>(10000);
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "reloadProductInfoCache",
      [this]() {
        productInfoCache_->reload();
        return true;
      },
      ExecAtStartup::True, milliSecIntervalOfReloadProduct));

  //! 定时检测策略信息
  const auto milliSecIntervalOfReloadStg =
      getConfig()["milliSecIntervalOfReloadStg"].as<std::uint32_t>(10000);
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "reloadStgInfoCache",
      [this]() {
        stgInfoCache_->reload();
        return true;
      },
      ExecAtStartup::True, milliSecIntervalOfReloadStg));

  //! 定时检测账户信息
  const auto milliSecIntervalOfReloadAcct =
      getConfig()["milliSecIntervalOfReloadAcct"].as<std::uint32_t>(10000);
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "reloadAcctInfoCache",
      [this]() {
        acctInfoCache_->reload();
        return true;
      },
      ExecAtStartup::True, milliSecIntervalOfReloadAcct));

  //! 定时检测交易账户信息
  const auto milliSecIntervalOfReloadTrdAcct =
      getConfig()["milliSecIntervalOfReloadTrdAcct"].as<std::uint32_t>(5000);
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "reloadTrdAcctInfoCache",
      [this]() {
        trdAcctInfoCache_->reload();
        return true;
      },
      ExecAtStartup::True, milliSecIntervalOfReloadTrdAcct));

  //! 定时发送策略注册信息
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "sendStgReg",
      [this]() {
        sendStgReg();
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(5000)));

  //! 定时同步任务，包括入库和同步消息到风控子系统
  const auto milliSecIntervalOfSyncTask =
      getConfig()["milliSecIntervalOfSyncTask"].as<std::uint32_t>();
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "syncTask",
      [this]() {
        handleSyncTaskGroup();
        return true;
      },
      ExecAtStartup::False, milliSecIntervalOfSyncTask));

  //! 往前端定时发送health情况
  scheduleTaskBundle_->emplace_back(std::make_shared<ScheduleTask>(
      "healthCheck",
      [this]() {
        logInfo("[{}] Health Check: OK.", {appName_}, getDftStgInstInfo(),
                NotifyToTerminal::True);
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(60000)));
}

int StgEngImpl::doRun() {
  if (stgInstTaskHandler_ == nullptr) {
    logError(
        "[{}] Start failed because of "
        "StgInstTaskHandler is null, please install first.",
        {appName_}, getDftStgInstInfo());
    return SCODE_STG_INST_TASK_HANDLER_NOT_INSTALL;
  }

  if (auto ret = tblMonitorOfStgInstInfo_->start(); ret != 0) {
    logError(
        "[{}] Start failed because of start table monitor of stg inst info "
        "faailed.",
        {appName_}, getDftStgInstInfo());
    return ret;
  }

  if (const auto [ret, stgInstInfo] =
          tblMonitorOfStgInstInfo_->getStgInstInfo(1);
      ret != 0) {
    logError("[{}] The stg must have at least one instance with an id of 1.",
             {appName_}, getDftStgInstInfo());
    return SCODE_STG_MUST_HAVE_STG_INST_1;
  }

  if (const auto [ret, stgInstInfo] =
          tblMonitorOfStgInstInfo_->getStgInstInfo(0);
      stgInstInfo != nullptr) {
    logError("[{}] The id of the stg instance must start from 1", {appName_},
             getDftStgInstInfo());
    return SCODE_STG_INST_ID_MUST_START_FROM_1;
  }

  if (auto ret = tblMonitorOfSymbolInfo_->start(); ret != 0) {
    logError(
        "[{}] Start failed because of start monitor of symbol info failed.",
        {appName_}, getDftStgInstInfo());
    return ret;
  }

  algoMgr_->start();
  dynCandle_->start();

  subMgr_->start();
  topicMgr_->start();

  stgInstTaskDispatcher_->start();
  shmCliOfTDSrv_->start();
  shmCliOfRiskMgr_->start();
  shmCliOfWebSrv_->start();

  resetBarrierOfStgStartSignal();
  sendStgStartSignal();
  getBarrierOfStgStartSignal()->get_future().wait();
  sendStgInstStartSignal();

  sendStgReg();

  if (const auto ret = scheduleTaskBundleExecutorOfTimer_->start(); ret != 0) {
    logError("Start scheduler of fixed ts failed.", getDftStgInstInfo());
    return ret;
  }

  if (const auto ret = scheduleTaskBundleExecutor_->start(); ret != 0) {
    logError("Start scheduler of multi task failed.", getDftStgInstInfo());
    return ret;
  }

  return 0;
}

void StgEngImpl::sendStgStartSignal() {
  const StgInstId stgInstId = 1;
  auto asynTask = MakeStgSignal(MSG_ID_ON_STG_START, stgInstId);
  stgInstTaskDispatcher_->dispatch(asynTask);
}

void StgEngImpl::sendStgInstStartSignal() {
  const auto stgInstIdGroup = getTBLMonitorOfStgInstInfo()->getStgInstIdGroup();
  for (const auto stgInstId : stgInstIdGroup) {
    auto asynTask = MakeStgSignal(MSG_ID_ON_STG_INST_START, stgInstId);
    stgInstTaskDispatcher_->dispatch(asynTask);
  }
}

void StgEngImpl::sendStgReg() {
  shmCliOfTDSrv_->asyncSendReqWithZeroCopy(
      [&](void* shmBufOfReq) {
        auto stgReg = static_cast<StgReg*>(shmBufOfReq);
      },
      MSG_ID_ON_STG_REG, sizeof(StgReg));

  shmCliOfRiskMgr_->asyncSendReqWithZeroCopy(
      [&](void* shmBufOfReq) {
        auto stgReg = static_cast<StgReg*>(shmBufOfReq);
      },
      MSG_ID_ON_STG_REG, sizeof(StgReg));
}

void StgEngImpl::cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                                    SyncToRiskMgr syncToRiskMgr,
                                    SyncToDB syncToDB) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    syncTaskGroup_.emplace_back(
        std::make_shared<SyncTask>(msgId, task, syncToRiskMgr, syncToDB));
  }
}

void StgEngImpl::handleSyncTaskGroup() {
  std::vector<SyncTaskSPtr> syncTaskGroup;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    std::swap(syncTaskGroup, syncTaskGroup_);
  }

  if (syncTaskGroup.size() > 100) {
    logWarn("Too many unprocessed task of sync. [num = {}]",
            {std::to_string(syncTaskGroup.size())}, getDftStgInstInfo());
  }

  for (const auto& rec : syncTaskGroup) {
    if (rec->syncToRiskMgr_ == SyncToRiskMgr::False) continue;
    if (rec->msgId_ == MSG_ID_ON_ORDER ||  //
        rec->msgId_ == MSG_ID_ON_ORDER_RET ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      const auto orderInfo = std::any_cast<OrderInfoSPtr>(rec->task_);
      shmCliOfRiskMgr_->asyncSendMsgWithZeroCopy(
          [&](void* shmBufOfReq) {
            InitMsgBodyExt(shmBufOfReq, *orderInfo);
            logDebug("Send order info to risk mgr. {}",
                     {static_cast<OrderInfo*>(shmBufOfReq)->toShortStr()},
                     getDftStgInstInfo());
          },
          rec->msgId_, orderInfo->size());
    } else {
      logWarn("Unhandled task of sync to riskmgr. {} - {}",
              {std::to_string(rec->msgId_), GetMsgName(rec->msgId_)},
              getDftStgInstInfo());
    }
  }

  for (const auto& rec : syncTaskGroup) {
    if (rec->syncToDB_ == SyncToDB::False) continue;
    if (rec->msgId_ == MSG_ID_ON_ORDER ||  //
        rec->msgId_ == MSG_ID_ON_ORDER_RET ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      const auto orderInfo = std::any_cast<OrderInfoSPtr>(rec->task_);
      const auto identity = GET_RAND_STR();
      const auto sql = orderInfo->getSqlOfUSPOrderInfoUpdate();
      const auto [ret, execRet] = getDBEng()->asyncExec(identity, sql);
      if (ret != 0) {
        logWarn("Sync order info to db failed. [{}]", {sql},
                getDftStgInstInfo());
      }

    } else {
      logWarn("Unhandled task of sync to db. {} - {}",
              {std::to_string(rec->msgId_), GetMsgName(rec->msgId_)},
              getDftStgInstInfo());
    }
  }
}

void StgEngImpl::doExit(const boost::system::error_code* ec, int signalNum) {
  scheduleTaskBundleExecutor_->stop();
  scheduleTaskBundleExecutorOfTimer_->stop();
  shmCliOfWebSrv_->stop();
  shmCliOfRiskMgr_->stop();
  shmCliOfTDSrv_->stop();
  sendStgInstStopSignal();
  sendStgStopSignal();
  stgInstTaskDispatcher_->stop();
  topicMgr_->stop();
  subMgr_->stop();
  dynCandle_->stop();
  algoMgr_->stop();
  tblMonitorOfSymbolInfo_->stop();
  tblMonitorOfStgInstInfo_->stop();
  tdEngConnpool_->uninit();
  getDBEng()->stop();
}

void StgEngImpl::sendStgStopSignal() {
  const StgInstId stgInstId = 1;
  auto asynTask = MakeStgSignal(MSG_ID_ON_STG_STOP, stgInstId);
  stgInstTaskDispatcher_->dispatch(asynTask);
}

void StgEngImpl::sendStgInstStopSignal() {
  const auto stgInstIdGroup = getTBLMonitorOfStgInstInfo()->getStgInstIdGroup();
  for (const auto stgInstId : stgInstIdGroup) {
    auto asynTask = MakeStgSignal(MSG_ID_ON_STG_INST_STOP, stgInstId);
    stgInstTaskDispatcher_->dispatch(asynTask);
  }
}

//! 因为 Bid + Long 无法确定 Open or Close，但是 Bid + Open 可以确定 Long，
//! 所以入参为 Side + PosDirection，而不是 Side + PosSide
std::tuple<int, OrderId> StgEngImpl::order(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    const std::string& symbolCode, Side side, PosDirection posDirection,
    Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
    CloseTDayStg closeTDayStg, AlgoId algoId,
    const SimedTDInfoSPtr& simedTDInfo) {
  std::string simedTDInfoInJsonFmt = "";
  if (simedTDInfo != nullptr) {
    simedTDInfoInJsonFmt = ConvertSimedTDInfoToJsonFmt(simedTDInfo);
  }
  if (simedTDInfoInJsonFmt.size() > MAX_SIMED_TD_INFO - 1) {
    return {SCODE_STG_INVALID_SIMED_TD_INFO_SIZE, 0};
  }

  const auto [statusCode, acctId] = trdAcctInfoCache_->getAcctId(trdAcctId);
  if (statusCode != 0) {
    return {statusCode, 0};
  }

  const auto acctGrpId = acctInfoCache_->getAcctGrpId(acctId);
  const auto productGrpId =
      productInfoCache_->getProductGrpId(stgInstInfo->productId_);
  const auto stgGrpId = stgInfoCache_->getStgGrpId(stgInstInfo->stgId_);

  auto orderInfo = MakeOrderInfo(stgInstInfo, productGrpId, stgGrpId, acctGrpId,
                                 acctId, trdAcctId, marketCode, symbolCode,
                                 side, posDirection, orderPrice, orderSize,
                                 closeTDayStg, algoId, simedTDInfoInJsonFmt);

  return order(orderInfo);
}

std::tuple<int, OrderId> StgEngImpl::order(
    StgInstId stgInstId, MarketCode marketCode, const std::string& symbolCode,
    Side side, PosDirection posDirection, Decimal orderPrice, Decimal orderSize,
    TrdAcctId trdAcctId, CloseTDayStg closeTDayStg, AlgoId algoId,
    OrderId orderId, const SimedTDInfoSPtr& simedTDInfo) {
  int statusCode = 0;
  StgInstInfoSPtr stgInstInfo = nullptr;

  std::tie(statusCode, stgInstInfo) =
      tblMonitorOfStgInstInfo_->getStgInstInfo(stgInstId);
  if (!stgInstInfo) {
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(statusCode), GetStatusMsg(statusCode)},
            getDftStgInstInfo());
    return {statusCode, 0};
  }

  std::string simedTDInfoInJsonFmt = "";
  if (simedTDInfo != nullptr) {
    simedTDInfoInJsonFmt = ConvertSimedTDInfoToJsonFmt(simedTDInfo);
  }
  if (simedTDInfoInJsonFmt.size() > MAX_SIMED_TD_INFO - 1) {
    return {SCODE_STG_INVALID_SIMED_TD_INFO_SIZE, 0};
  }

  AcctId acctId = 0;
  std::tie(statusCode, acctId) = trdAcctInfoCache_->getAcctId(trdAcctId);
  if (statusCode != 0) {
    return {statusCode, 0};
  }

  const auto acctGrpId = acctInfoCache_->getAcctGrpId(acctId);
  const auto productGrpId =
      productInfoCache_->getProductGrpId(stgInstInfo->productId_);
  const auto stgGrpId = stgInfoCache_->getStgGrpId(stgInstInfo->stgId_);

  auto orderInfo = MakeOrderInfo(stgInstInfo, productGrpId, stgGrpId, acctGrpId,
                                 acctId, trdAcctId, marketCode, symbolCode,
                                 side, posDirection, orderPrice, orderSize,
                                 closeTDayStg, algoId, simedTDInfoInJsonFmt);
  orderInfo->orderId_ = orderId;

  return order(orderInfo);
}

std::tuple<int, OrderId> StgEngImpl::order(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    const std::string& symbolCode, Side side, Decimal orderPrice,
    Decimal orderSize, TrdAcctId trdAcctId, CloseTDayStg closeTDayStg,
    AlgoId algoId, const SimedTDInfoSPtr& simedTDInfo) {
  std::string simedTDInfoInJsonFmt = "";
  if (simedTDInfo != nullptr) {
    simedTDInfoInJsonFmt = ConvertSimedTDInfoToJsonFmt(simedTDInfo);
  }
  if (simedTDInfoInJsonFmt.size() > MAX_SIMED_TD_INFO - 1) {
    return {SCODE_STG_INVALID_SIMED_TD_INFO_SIZE, 0};
  }

  const auto [statusCode, acctId] = trdAcctInfoCache_->getAcctId(trdAcctId);
  if (statusCode != 0) {
    return {statusCode, 0};
  }

  const auto acctGrpId = acctInfoCache_->getAcctGrpId(acctId);
  const auto productGrpId =
      productInfoCache_->getProductGrpId(stgInstInfo->productId_);
  const auto stgGrpId = stgInfoCache_->getStgGrpId(stgInstInfo->stgId_);

  auto orderInfo = MakeOrderInfo(
      stgInstInfo, productGrpId, stgGrpId, acctGrpId, acctId, trdAcctId,
      marketCode, symbolCode, side, PosDirection::Others, orderPrice, orderSize,
      closeTDayStg, algoId, simedTDInfoInJsonFmt);

  return order(orderInfo);
}

std::tuple<int, OrderId> StgEngImpl::order(
    StgInstId stgInstId, MarketCode marketCode, const std::string& symbolCode,
    Side side, Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
    CloseTDayStg closeTDayStg, AlgoId algoId, OrderId orderId,
    const SimedTDInfoSPtr& simedTDInfo) {
  int statusCode = 0;
  StgInstInfoSPtr stgInstInfo = nullptr;

  std::tie(statusCode, stgInstInfo) =
      tblMonitorOfStgInstInfo_->getStgInstInfo(stgInstId);
  if (!stgInstInfo) {
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(statusCode), GetStatusMsg(statusCode)},
            getDftStgInstInfo());
    return {statusCode, 0};
  }

  std::string simedTDInfoInJsonFmt = "";
  if (simedTDInfo != nullptr) {
    simedTDInfoInJsonFmt = ConvertSimedTDInfoToJsonFmt(simedTDInfo);
  }
  if (simedTDInfoInJsonFmt.size() > MAX_SIMED_TD_INFO - 1) {
    return {SCODE_STG_INVALID_SIMED_TD_INFO_SIZE, 0};
  }

  AcctId acctId = 0;
  std::tie(statusCode, acctId) = trdAcctInfoCache_->getAcctId(trdAcctId);
  if (statusCode != 0) {
    return {statusCode, 0};
  }

  const auto acctGrpId = acctInfoCache_->getAcctGrpId(acctId);
  const auto productGrpId =
      productInfoCache_->getProductGrpId(stgInstInfo->productId_);
  const auto stgGrpId = stgInfoCache_->getStgGrpId(stgInstInfo->stgId_);

  auto orderInfo = MakeOrderInfo(
      stgInstInfo, productGrpId, stgGrpId, acctGrpId, acctId, trdAcctId,
      marketCode, symbolCode, side, PosDirection::Others, orderPrice, orderSize,
      closeTDayStg, algoId, simedTDInfoInJsonFmt);
  orderInfo->orderId_ = orderId;

  return order(orderInfo);
}

std::tuple<int, OrderId> StgEngImpl::order(OrderInfoSPtr& orderInfo) {
  //! 如果下单没有指定MarketCode，那么尝试从acctInfoCache中获取
  if (orderInfo->marketCode_ == MarketCode::Others) {
    int retOfGetMarketCodeAndSymbolType = 0;
    std::tie(retOfGetMarketCodeAndSymbolType, orderInfo->marketCode_,
             orderInfo->symbolType_) =
        acctInfoCache_->getMarketCodeAndSymbolType(orderInfo->acctId_);
    if (retOfGetMarketCodeAndSymbolType != 0) {
      orderInfo->orderStatus_ = OrderStatus::Failed;
      orderInfo->statusCode_ = retOfGetMarketCodeAndSymbolType;
      logWarn("[{}] Order failed. [{} - {}] {}",
              {appName_, std::to_string(orderInfo->statusCode_),
               GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
              tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                  orderInfo->stgInstId_));
      return {retOfGetMarketCodeAndSymbolType, 0};
    }
  }

  //! 如果orderInfo->marketCode_还是没能获取，那么提示非法marketCode
  if (orderInfo->marketCode_ == MarketCode::Others) {
    orderInfo->orderStatus_ = OrderStatus::Failed;
    orderInfo->statusCode_ = SCODE_STG_INVALID_MARKET_CODE;
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(orderInfo->statusCode_),
             GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
            tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                orderInfo->stgInstId_));
    return {orderInfo->statusCode_, 0};
  }

  //! 从数据库查找symbol信息
  const auto [retOfGetSym, recSymbolInfo] =
      getTBLMonitorOfSymbolInfo()->getRecSymbolInfoBySymbolCode(
          GetMarketName(orderInfo->marketCode_), orderInfo->symbolCode_);
  if (retOfGetSym != 0) {
    orderInfo->orderStatus_ = OrderStatus::Failed;
    orderInfo->statusCode_ = retOfGetSym;
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(orderInfo->statusCode_),
             GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
            tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                orderInfo->stgInstId_));
    return {retOfGetSym, 0};
  }

  //! 如果symbolType还没有正确的值，那么从symbol信息获取
  if (orderInfo->symbolType_ == SymbolType::Others) {
    const auto symbolTypeVal =
        magic_enum::enum_cast<SymbolType>(recSymbolInfo->symbolType);
    if (symbolTypeVal.has_value()) {
      orderInfo->symbolType_ = symbolTypeVal.value();
    } else {
      orderInfo->orderStatus_ = OrderStatus::Failed;
      orderInfo->statusCode_ = SCODE_STG_INVALID_SYMBOL_TYPE_IN_DB;
      logWarn("[{}] Order failed. [{} - {}] {}",
              {appName_, std::to_string(orderInfo->statusCode_),
               GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
              tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                  orderInfo->stgInstId_));
      return {orderInfo->statusCode_, 0};
    }
  }

  //! 获取策略实例信息
  const auto [retOfGetStgInst, stgInstInfo] =
      tblMonitorOfStgInstInfo_->getStgInstInfo(orderInfo->stgInstId_);
  if (retOfGetStgInst != 0) {
    orderInfo->orderStatus_ = OrderStatus::Failed;
    orderInfo->statusCode_ = retOfGetStgInst;
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(orderInfo->statusCode_),
             GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
            tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                orderInfo->stgInstId_));
    return {retOfGetStgInst, 0};
  }

  //! 现货将PosDirection和PosSide都置为Both，其他根据Ask和Bid确定空或者多
  const auto statusCode = InitPosSide(orderInfo);
  if (statusCode != 0) {
    return {statusCode, 0};
  }

  //! 当前版本将收费币种设为CNY
  if (orderInfo->symbolType_ == SymbolType::CN_MainBoard ||
      orderInfo->symbolType_ == SymbolType::CN_SecondBoard ||
      orderInfo->symbolType_ == SymbolType::CN_StartupBoard ||
      orderInfo->symbolType_ == SymbolType::CN_TechBoard ||
      orderInfo->symbolType_ == SymbolType::CN_Futures ||
      orderInfo->symbolType_ == SymbolType::CN_FuturesOptions ||
      orderInfo->symbolType_ == SymbolType::CN_SpotOptions) {
    strncpy(orderInfo->feeCurrency_, CN_OFFICIAL_CURRENCY.c_str(),
            sizeof(orderInfo->feeCurrency_) - 1);
  }

  if (orderInfo->orderId_ == 0) {
    orderInfo->orderId_ = GET_RAND_INT();
  }
  orderInfo->parValue_ = recSymbolInfo->parValue;
  strncpy(orderInfo->exchSymbolCode_, recSymbolInfo->exchSymbolCode.c_str(),
          sizeof(orderInfo->exchSymbolCode_) - 1);
  orderInfo->orderStatus_ = OrderStatus::Created;

  //! 因为orderInfo最后进入cacheSyncTaskGroup，不再由StgEngImpl控制，所以这里
  //! 必须是DeepClone::True
  if (const auto ret =
          getOrdMgr()->add<LockFunc::True, DeepClone::True>(orderInfo);
      ret != 0) {
    orderInfo->orderStatus_ = OrderStatus::Failed;
    orderInfo->statusCode_ = ret;
    logWarn("[{}] Order failed. [{} - {}] {}",
            {appName_, std::to_string(orderInfo->statusCode_),
             GetStatusMsg(orderInfo->statusCode_), orderInfo->toShortStr()},
            tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                orderInfo->stgInstId_));
    return {ret, 0};
  }

//! time used 1.62 us amd cpu
#ifdef PERF_TEST
  EXEC_PERF_TEST("Order", orderInfo->orderTime_, 100, 10);
#endif

  shmCliOfTDSrv_->asyncSendMsgWithZeroCopy(
      [this, &orderInfo](void* shmBufOfReq) {
        InitMsgBodyExt(shmBufOfReq, *orderInfo);
#ifndef OPT_LOG
        logInfo("Send order {}",
                {static_cast<OrderInfo*>(shmBufOfReq)->toShortStr()},
                tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                    orderInfo->stgInstId_));
#endif
      },
      MSG_ID_ON_ORDER, orderInfo->size());

  cacheSyncTaskGroup(MSG_ID_ON_ORDER, orderInfo, SyncToRiskMgr::True,
                     SyncToDB::True);

//! time used 5.96 us amd cpu
#ifdef PERF_TEST
  EXEC_PERF_TEST("Order", orderInfo->orderTime_, 100, 10);
#endif
  return {0, orderInfo->orderId_};
}

int StgEngImpl::cancelOrder(OrderId orderId) {
  //! 因为orderInfo会在其他线程被改动，因此这里克隆一个快照出来
  const auto [statusCode, orderInfo] =
      getOrdMgr()->getOrderInfo<LockFunc::True, DeepClone::True>(orderId);
  if (statusCode != 0) {
    logWarn("[{}] Cancel order {} failed. [{} - {}]",
            {appName_, std::to_string(orderId), std::to_string(statusCode),
             GetStatusMsg(statusCode)},
            getDftStgInstInfo());
    return statusCode;
  }

  shmCliOfTDSrv_->asyncSendMsgWithZeroCopy(
      [&](void* shmBufOfReq) {
        InitMsgBodyExt(shmBufOfReq, *orderInfo);
        logInfo("Send cancel order {}",
                {static_cast<OrderInfo*>(shmBufOfReq)->toShortStr()},
                tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                    orderInfo->stgInstId_));
      },
      MSG_ID_ON_CANCEL_ORDER, orderInfo->size());

  cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER, orderInfo, SyncToRiskMgr::True,
                     SyncToDB::False);

  return 0;
}

std::vector<OrderId> StgEngImpl::cancelAllOrderOfStg() {
  const auto ret = getOrdMgr()->getOrderIdGroupOfStg<LockFunc::True>();
  for (auto orderId : ret) {
    cancelOrder(orderId);
  }
  return ret;
}

std::vector<OrderId> StgEngImpl::cancelAllOrderOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) {
  const auto ret = getOrdMgr()->getOrderIdGroupOfStgInst<LockFunc::True>(
      stgInstInfo->stgInstId_);
  for (auto orderId : ret) {
    cancelOrder(orderId);
  }
  return ret;
}

std::vector<OrderId> StgEngImpl::cancelAllOrderOfStgInst(StgInstId stgInsId) {
  const auto ret =
      getOrdMgr()->getOrderIdGroupOfStgInst<LockFunc::True>(stgInsId);
  for (auto orderId : ret) {
    cancelOrder(orderId);
  }
  return ret;
}

std::vector<OrderId> StgEngImpl::cancelAllOrderOfAlgo(AlgoId algoId) {
  const auto ret = getOrdMgr()->getOrderIdGroupOfAlgo<LockFunc::True>(algoId);
  for (auto orderId : ret) {
    cancelOrder(orderId);
  }
  return ret;
}

//! algoName仅仅是一个助记词
std::tuple<int, AlgoId> StgEngImpl::algoOrder(
    const StgInstInfoSPtr& stgInstInfo, const std::string& algoType,
    const std::string& algoName, std::uint32_t lifetime,
    const std::string& algoParamsInJsonFmt) {
  return algoMgr_->algoOrder(stgInstInfo, algoType, algoName, lifetime,
                             algoParamsInJsonFmt);
}

int StgEngImpl::cancelAlgoOrder(AlgoId algoId) {
  return algoMgr_->cancelAlgoOrder(algoId);
}

std::string StgEngImpl::getProgressOfAlgoOrder(AlgoId algoId) {
  return algoMgr_->getProgressOfAlgoOrder(algoId);
}

int StgEngImpl::sub(StgInstId subscriber, const std::string& topic) {
  if (boost::contains(topic, magic_enum::enum_name(MDType::DynCandle))) {
    //! 动态k线本地生成，所以特殊处理
    return dynCandle_->sub(subscriber, topic);
  } else {
    const auto statusCode = subMgr_->sub(subscriber, topic);
    const auto s = fmt::format("{}-{}", getAppName(), subscriber);
    topicMgr_->add(s, topic);
    return statusCode;
  }
}

int StgEngImpl::unSub(StgInstId subscriber, const std::string& topic) {
  if (boost::contains(topic, magic_enum::enum_name(MDType::DynCandle))) {
    //! 动态k线本地生成，所以特殊处理
    return dynCandle_->unSub(subscriber, topic);
  } else {
    const auto statusCode = subMgr_->unSub(subscriber, topic);
    const auto s = fmt::format("{}-{}", getAppName(), subscriber);
    topicMgr_->remove(s, topic);
    return statusCode;
  }
}

std::tuple<int, std::string> StgEngImpl::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t tsBegin, std::uint64_t tsEnd) {
  const auto [statusCode, marketDataCond] = GetMarketDataCondFromTopic(topic);
  if (statusCode != 0) return {statusCode, ""};
  return queryHisMDBetween2Ts(
      stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo(),
      marketDataCond->marketCode_, marketDataCond->symbolType_,
      marketDataCond->symbolCode_, marketDataCond->mdType_, tsBegin, tsEnd,
      marketDataCond->ext_);
}

std::tuple<int, std::string> StgEngImpl::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext) {
  std::string addr;
  //! prefix = http://localhost/v1/QueryHisMD/between
  const auto prefix =
      fmt::format(prefixOfQueryHisMDBetween,
                  getConfig()["webSrv"].as<std::string>("localhost"));
  if (mdType == MDType::Candle) {
    if (ext.empty()) {
      addr = fmt::format("{}/{}/{}/{}/{}?tsBegin={}&tsEnd={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), tsBegin, tsEnd);
    } else {
      addr = fmt::format("{}/{}/{}/{}/{}?tsBegin={}&tsEnd={}&freq={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), tsBegin, tsEnd, ext);
    }
  } else {
    addr = fmt::format("{}/{}/{}/{}/{}?tsBegin={}&tsEnd={}", prefix,
                       GetMarketName(marketCode),
                       magic_enum::enum_name(symbolType), symbolCode,
                       magic_enum::enum_name(mdType), tsBegin, tsEnd);
  }

  const auto timeoutOfQueryHisMD =
      getConfig()["timeoutOfQueryHisMD"].as<std::uint32_t>(60000);
  cpr::Response rsp =
      cpr::Get(cpr::Url{addr}, cpr::Timeout(timeoutOfQueryHisMD));
  if (rsp.status_code != cpr::status::HTTP_OK) {
    const auto statusMsg =
        fmt::format("Query his market data between 2 ts failed. [{}:{}] {} {}",
                    rsp.status_code, rsp.reason, rsp.text, rsp.url.str());
    logWarn(statusMsg,
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
    return {SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED, ""};
  } else {
    logInfo("Send http req success. {}", {addr},
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
  }

  return {0, rsp.text};
}

std::tuple<int, std::string> StgEngImpl::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  const auto [statusCode, marketDataCond] = GetMarketDataCondFromTopic(topic);
  if (statusCode != 0) return {statusCode, ""};
  return querySpecificNumOfHisMDBeforeTs(
      stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo(),
      marketDataCond->marketCode_, marketDataCond->symbolType_,
      marketDataCond->symbolCode_, marketDataCond->mdType_, ts, num,
      marketDataCond->ext_);
}

std::tuple<int, std::string> StgEngImpl::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  std::string addr;
  //! prefix = http://localhost/v1/QueryHisMD/before
  const auto prefix =
      fmt::format(prefixOfQueryHisMDBefore,
                  getConfig()["webSrv"].as<std::string>("localhost"));
  if (mdType == MDType::Candle) {
    if (ext.empty()) {
      addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), ts, num);
    } else {
      addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}&freq={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), ts, num, ext);
    }
  } else {
    addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}", prefix,
                       GetMarketName(marketCode),
                       magic_enum::enum_name(symbolType), symbolCode,
                       magic_enum::enum_name(mdType), ts, num);
  }

  const auto timeoutOfQueryHisMD =
      getConfig()["timeoutOfQueryHisMD"].as<std::uint32_t>(60000);
  cpr::Response rsp =
      cpr::Get(cpr::Url{addr}, cpr::Timeout(timeoutOfQueryHisMD));
  if (rsp.status_code != cpr::status::HTTP_OK) {
    const auto statusMsg = fmt::format(
        "Query specific num of his market data before ts failed. [{}:{}] {} {}",
        rsp.status_code, rsp.reason, rsp.text, rsp.url.str());
    logWarn(statusMsg,
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
    return {SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED, ""};
  } else {
    logInfo("Send http req success. {}", {addr},
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
  }

  return {0, rsp.text};
}

std::tuple<int, std::string> StgEngImpl::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  const auto [statusCode, marketDataCond] = GetMarketDataCondFromTopic(topic);
  if (statusCode != 0) return {statusCode, ""};
  return querySpecificNumOfHisMDAfterTs(
      stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo(),
      marketDataCond->marketCode_, marketDataCond->symbolType_,
      marketDataCond->symbolCode_, marketDataCond->mdType_, ts, num,
      marketDataCond->ext_);
}

std::tuple<int, std::string> StgEngImpl::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  std::string addr;
  //! prefix = http://localhost/v1/QueryHisMD/after
  const auto prefix =
      fmt::format(prefixOfQueryHisMDAfter,
                  getConfig()["webSrv"].as<std::string>("localhost"));
  if (mdType == MDType::Candle) {
    if (ext.empty()) {
      addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), ts, num);
    } else {
      addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}&freq={}", prefix,
                         GetMarketName(marketCode),
                         magic_enum::enum_name(symbolType), symbolCode,
                         magic_enum::enum_name(mdType), ts, num, ext);
    }
  } else {
    addr = fmt::format("{}/{}/{}/{}/{}?ts={}&num={}", prefix,
                       GetMarketName(marketCode),
                       magic_enum::enum_name(symbolType), symbolCode,
                       magic_enum::enum_name(mdType), ts, num);
  }

  const auto timeoutOfQueryHisMD =
      getConfig()["timeoutOfQueryHisMD"].as<std::uint32_t>(60000);
  cpr::Response rsp =
      cpr::Get(cpr::Url{addr}, cpr::Timeout(timeoutOfQueryHisMD));
  if (rsp.status_code != cpr::status::HTTP_OK) {
    const auto statusMsg = fmt::format(
        "Query specific num of his market data after ts failed. [{}:{}] {} {}",
        rsp.status_code, rsp.reason, rsp.text, rsp.url.str());
    logWarn(statusMsg,
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
    return {SCODE_STG_SEND_HTTP_REQ_TO_QUERY_HIS_MD_FAILED, ""};
  } else {
    logInfo("Send http req success. {}", {addr},
            stgInstInfo != nullptr ? stgInstInfo : getDftStgInstInfo());
  }

  return {0, rsp.text};
}

void StgEngImpl::installStgInstTimer(StgInstId stgInstId,
                                     const std::string& timerName,
                                     const std::string& execTime,
                                     std::uint32_t timeZone) {
  const auto timerNameWithInstId =
      fmt::format("{}{}-{}", PREFIX_OF_TIMER_NAME, stgInstId, timerName);
  const auto task = std::make_shared<TaskOfFixedTime>(
      stgInstId, timerNameWithInstId, execTime, timeZone);
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTaskOfFixedTimeGroup_);
    taskOfFixedTimeGroup_.emplace_back(task);
  }
}

void StgEngImpl::installStgInstTimer(StgInstId stgInstId,
                                     const std::string& timerName,
                                     ExecAtStartup execAtStartUp,
                                     std::uint32_t milliSecInterval,
                                     std::uint64_t maxExecTimes) {
  //! 确保每个实例的定时器不重名，这样移除定时器的时候不会移除其他实例的定时器
  const auto timerNameWithInstId =
      fmt::format("{}{}-{}", PREFIX_OF_TIMER_NAME, stgInstId, timerName);
  logInfo(
      "Install a timer named {} "
      "with a trigger interval of {} ms and a max exec times of {}.",
      {timerNameWithInstId, std::to_string(milliSecInterval),
       std::to_string(maxExecTimes)},
      tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(stgInstId));

  const auto callback = [stgInstId, timerNameWithInstId, this]() {
    auto asynTask =
        MakeStgSignal(MSG_ID_ON_STG_INST_TIMER, stgInstId, timerNameWithInstId);
    stgInstTaskDispatcher_->dispatch(asynTask);
    return true;
  };

  //! 安装定时调用callback的定时任务
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxScheduleTaskBundleOfTimer_);
    scheduleTaskBundleOfTimer_->emplace_back(std::make_shared<ScheduleTask>(
        timerNameWithInstId, callback, execAtStartUp, milliSecInterval,
        maxExecTimes));
    if (scheduleTaskBundleOfTimer_->size() % 100 == 0) {
      logWarn(
          "[{}] Size of scheduleTaskBundleOfTimer is {}",
          {getAppName(), std::to_string(scheduleTaskBundleOfTimer_->size())},
          tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(stgInstId));
    }
  }
}

void StgEngImpl::uninstallStgInstTimer(StgInstId stgInstId,
                                       const std::string& timerName) {
  const auto timerNameWithInstId =
      fmt::format("{}{}-{}", PREFIX_OF_TIMER_NAME, stgInstId, timerName);

  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTaskOfFixedTimeGroup_);
    std::ext::erase_if(
        *scheduleTaskBundleOfTimer_,  //
        [&, this](const auto& scheduleTask) {
          if (scheduleTask->scheduleTaskName_ == timerNameWithInstId) {
            const auto statusMsg = fmt::format(
                "[{}] uninstall scheduleTask {} of interval. "
                "[execTimes: {}, maxExecTimes: {}]",
                getAppName(), scheduleTask->scheduleTaskName_,
                scheduleTask->execTimes_, scheduleTask->maxExecTimes_);
            logInfo(statusMsg,
                    tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                        stgInstId));
            return true;
          } else {
            return false;
          }
        });

    std::ext::erase_if(
        taskOfFixedTimeGroup_,  //
        [&, this](const auto& taskOfFixedTime) {
          if (taskOfFixedTime->timerName_ == timerNameWithInstId) {
            const auto statusMsg =
                fmt::format("[{}] uninstall scheduleTask {} of fixed time. ",
                            getAppName(), taskOfFixedTime->timerName_);
            logInfo(statusMsg,
                    tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(
                        stgInstId));
            return true;
          } else {
            return false;
          }
        });
  }
}

std::tuple<int, OrderInfoSPtr> StgEngImpl::getOrderInfo(OrderId orderId) const {
  return ordMgr_->getOrderInfo<LockFunc::True, DeepClone::True>(orderId);
}

std::vector<OrderInfoSPtr> StgEngImpl::getUnclosedOrderInfoGroup(
    const StgInstInfoSPtr& stgInstInfo) {
  const auto ret =
      getOrdMgr()->getOrderInfoGroupOfStgInst<LockFunc::True, DeepClone::True>(
          stgInstInfo);
  return ret;
}

PosOfStgInstGroupSPtr StgEngImpl::getPosOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) {
  const auto posMgrOfStgInst =
      std::make_shared<PosMgrOfStgInst>(stgInstInfo, getPosMgr(), getOrdMgr());
  const auto ret = posMgrOfStgInst->get();
  return ret;
}

bool StgEngImpl::saveStgPrivateData(StgInstId stgInstId,
                                    const std::string& jsonStr) {
  try {
    if (!boost::filesystem::exists(rootDirOfStgPrivateData_)) {
      boost::filesystem::create_directories(rootDirOfStgPrivateData_);
    }
  } catch (const std::exception& e) {
    logWarn(
        "Save stg private data failed "
        "because of create directories {} failed. {}",
        {rootDirOfStgPrivateData_, e.what()},
        tblMonitorOfStgInstInfo_->getStgInstInfo<StgInstInfoSPtr>(stgInstId));
    return false;
  }
  const auto filename =
      fmt::format("{}/{}.dat", rootDirOfStgPrivateData_, stgInstId);
  bool ret = OverwriteStrToFile(filename, jsonStr);
  return ret;
}

std::string StgEngImpl::loadStgPrivateData(StgInstId stgInstId) {
  const auto filename =
      fmt::format("{}/{}.dat", rootDirOfStgPrivateData_, stgInstId);
  const auto filecont = LoadFileContToStr(filename);
  return filecont;
}

std::tuple<int, std::string> StgEngImpl::syncExecSql(const std::string& sql) {
  const auto identity = GET_RAND_STR();
  logDebug("Try to exec sql {}.", {sql}, getDftStgInstInfo());
  const auto [ret, execRet] = dbEng_->syncExec(identity, sql);
  if (ret != 0) {
    logWarn("Exec sql failed. [{}] [{}]", {sql, execRet}, getDftStgInstInfo());
    return {ret, execRet};
  }
  return {ret, execRet};
}

void StgEngImpl::saveToDB(const PnlSPtr& pnl) {
  const auto identity = GET_RAND_STR();
  const auto sql = pnl->getSqlOfInsert();
  const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
  if (ret != 0) {
    logWarn("Insert pnl to db failed. [{}]", {sql}, getDftStgInstInfo());
  }
}

ScheduleTaskBundleSPtr StgEngImpl::getScheduleTaskBundleOfTimer() {
  auto ret = std::make_shared<ScheduleTaskBundle>();
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxScheduleTaskBundleOfTimer_);

    //! 移除失效的定时任务，避免队列过大
    std::ext::erase_if(
        *scheduleTaskBundleOfTimer_,  //
        [this](const auto& scheduleTask) {
          if (scheduleTask->execTimes_ == scheduleTask->maxExecTimes_) {
            const auto statusMsg = fmt::format(
                "[{}] scheduleTask {} finished. "
                "[execTimes: {}; maxExecTimes: {}]",
                getAppName(), scheduleTask->scheduleTaskName_,
                scheduleTask->execTimes_, scheduleTask->maxExecTimes_);
            logInfo(statusMsg, getDftStgInstInfo());
            return true;
          } else {
            return false;
          }
        });

    ret->assign(std::begin(*scheduleTaskBundleOfTimer_),
                std::end(*scheduleTaskBundleOfTimer_));
  }
  return ret;
};

void StgEngImpl::logTrace(const std::string& fmt,
                          const std::vector<std::string>& args,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::TRACE, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::TRACE, stgInstInfo, data);
  }
}

void StgEngImpl::logDebug(const std::string& fmt,
                          const std::vector<std::string>& args,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::DEBUG, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::DEBUG, stgInstInfo, data);
  }
}

void StgEngImpl::logInfo(const std::string& fmt,
                         const std::vector<std::string>& args,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::INFO, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::INFO, stgInstInfo, data);
  }
}

void StgEngImpl::logWarn(const std::string& fmt,
                         const std::vector<std::string>& args,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::WARN, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::WARN, stgInstInfo, data);
  }
}

void StgEngImpl::logError(const std::string& fmt,
                          const std::vector<std::string>& args,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::ERROR, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::ERROR, stgInstInfo, data);
  }
}

void StgEngImpl::logCritical(const std::string& fmt,
                             const std::vector<std::string>& args,
                             const StgInstInfoSPtr& stgInstInfo,
                             NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::CRITICAL, fmt, args);
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::CRITICAL, stgInstInfo, data);
  }
}

void StgEngImpl::logTrace(const std::string& fmt,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::TRACE, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::TRACE, stgInstInfo, data);
  }
}

void StgEngImpl::logDebug(const std::string& fmt,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::DEBUG, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::DEBUG, stgInstInfo, data);
  }
}

void StgEngImpl::logInfo(const std::string& fmt,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::INFO, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::INFO, stgInstInfo, data);
  }
}

void StgEngImpl::logWarn(const std::string& fmt,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::WARN, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::WARN, stgInstInfo, data);
  }
}

void StgEngImpl::logError(const std::string& fmt,
                          const StgInstInfoSPtr& stgInstInfo,
                          NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::ERROR, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::ERROR, stgInstInfo, data);
  }
}

void StgEngImpl::logCritical(const std::string& fmt,
                             const StgInstInfoSPtr& stgInstInfo,
                             NotifyToTerminal notifyToTerminal) {
  const auto data = log(LogLevel::CRITICAL, fmt, {});
  if (notifyToTerminal == NotifyToTerminal::True && stgInstInfo != nullptr) {
    StgLoggerInfoInsert(dbEng_, LogLevel::CRITICAL, stgInstInfo, data);
  }
}

}  // namespace bq::stg
