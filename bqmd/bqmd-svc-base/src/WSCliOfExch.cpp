/*!
 * \file WSCliOfExch.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "WSCliOfExch.hpp"

#include "Config.hpp"
#include "ConnMetadata.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvc.hpp"
#include "RspParser.hpp"
#include "SubAndUnSubSvc.hpp"
#include "TopicGroupMustSubMaint.hpp"
#include "WSCli.hpp"
#include "WSTask.hpp"
#include "WebConst.hpp"
#include "WebParam.hpp"
#include "def/MDWSCliAsyncTaskArg.hpp"
#include "util/BQMDUtil.hpp"
#include "util/Datetime.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/String.hpp"

namespace bq::md::svc {

WSCliOfExch::WSCliOfExch(MDSvc* mdSvc)
    : mdSvc_(mdSvc),
      topicGroupNeedMaint_(std::make_shared<TopicGroupNeedMaint>()) {}

int WSCliOfExch::init() {
  if (const auto ret = initWSCli(); ret != 0) {
    return ret;
  }
  if (const auto ret = initTaskDispatcher(); ret != 0) {
    return ret;
  }
  taskDispatcher_->init();
  return 0;
}

int WSCliOfExch::initWSCli() {
  const auto wsParamInStrFmt =
      SetParam(web::DEFAULT_WS_PARAM, CONFIG["wsParam"].as<std::string>());
  const auto [ret, wsParam] = web::MakeWSParam(wsParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init wscli failed. {}", wsParamInStrFmt);
    return ret;
  }

  wsCli_ = std::make_shared<web::WSCli>(
      wsParam,
      [this](auto* wsCli, const auto& connMetadata, const auto& msg) {
        OnWSCliMsg(wsCli, connMetadata, msg);
      },
      [this](auto* wsCli, const auto& connMetadata) {
        OnWSCliOpen(wsCli, connMetadata);
      },
      nullptr, nullptr, mdSvc_->getPingPongSvc());

  return 0;
}

int WSCliOfExch::initTaskDispatcher() {
  const auto wsTaskDispatcherParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               CONFIG["wsTaskDispatcherParam"].as<std::string>());
  const auto [ret, wsTaskDispatcherParam] =
      MakeTaskDispatcherParam(wsTaskDispatcherParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init taskdispatcher failed. {}", wsTaskDispatcherParamInStrFmt);
    return ret;
  }

  const auto getThreadForAsyncTask =
      [](const WSCliAsyncTaskSPtr& asyncTask,
         std::uint32_t taskSpecificThreadPoolSize) {
        const auto arg = std::any_cast  //
            <WSCliAsyncTaskArgSPtr>(asyncTask->arg_);

        switch (arg->wsMsgType_) {
          case MsgType::Books:
            return ThreadNo(1);

          case MsgType::Trades:
          case MsgType::Tickers:
          case MsgType::Candle:
            return ThreadNo(0);

          default:
            return ThreadNo(0);
        }
        return ThreadNo(0);
      };

  taskDispatcher_ = std::make_shared<TaskDispatcher<web::TaskFromSrvSPtr>>(
      wsTaskDispatcherParam,
      [this](const auto& task) { return makeAsyncTask(task); },
      getThreadForAsyncTask,
      [this](auto& asyncTask) { handleAsyncTask(asyncTask); });

  return ret;
}

int WSCliOfExch::start() {
  taskDispatcher_->start();
  const auto ret = wsCli_->start();
  if (ret != 0) {
    LOG_E("Start WSCliOfExch failed.");
    return ret;
  }
  return ret;
}

void WSCliOfExch::stop() {
  wsCli_->stop();
  taskDispatcher_->stop();
}

void WSCliOfExch::OnWSCliOpen(web::WSCli* wsCli,
                              const web::ConnMetadataSPtr& connMetadata) {
  //! 这里可以用来清除增量订单簿缓存
  onBeforeOpen(wsCli, connMetadata);
  //! 清除已订阅列表以便重新发起订阅
  mdSvc_->getTopicGroupMustSubMaint()->clearTopicGroupAlreadySub();
  //! 重置已经生成的流控数据
  mdSvc_->getFlowCtrlSvc()->reset();
}

void WSCliOfExch::OnWSCliMsg(web::WSCli* wsCli,
                             const web::ConnMetadataSPtr& connMetadata,
                             const web::MsgSPtr& msg) {
  //! 零拷贝模式可在此处直接将消息转换并写入内存
  auto task = std::make_shared<web::TaskFromSrv>(wsCli, connMetadata, msg);
  taskDispatcher_->dispatch(task);
}

std::tuple<int, WSCliAsyncTaskSPtr> WSCliOfExch::makeAsyncTask(
    const web::TaskFromSrvSPtr& task) {
  //! 此处生成asyncTask方便getThreadForAsyncTask函数将其分流到不同的线程
  const auto asyncTaskArg = MakeWSCliAsyncTaskArg(task);
  if (asyncTaskArg == nullptr) {
    LOG_W("WSCli msg parse failed. {}", task->msg_->get_payload());
    return {-1, nullptr};
  }
  const auto asyncTask = std::make_shared<WSCliAsyncTask>(task, asyncTaskArg);
  return std::make_tuple(0, asyncTask);
}

void WSCliOfExch::handleAsyncTask(WSCliAsyncTaskSPtr& asyncTask) {
  const auto asyncTaskArg =
      std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);

  std::string topic = "";

  switch (asyncTaskArg->wsMsgType_) {
    case MsgType::Trades:
      topic = handleMDTrades(asyncTask);
      break;

    case MsgType::Tickers:
      topic = handleMDTickers(asyncTask);
      break;

    case MsgType::Candle:
      topic = handleMDCandle(asyncTask);
      break;

    case MsgType::Books:
      topic = handleMDBooks(asyncTask);
      break;

    default:
      handleMDOthers(asyncTask);
      break;
  }

  if (!topic.empty()) {
    updateActiveTimeOfTopic(topic);
  }

#ifdef PERF_TEST
  //!
  //! Total num: 10000; avg: 15; med: 9; min: 2; max: 367  # rapidjson
  //! Total num: 10000; avg: 14; med: 8; min: 1; max: 374
  //! Total num: 10000; avg: 14; med: 8; min: 2; max: 349
  //! Total num: 10000; avg: 16; med: 8; min: 2; max: 1129
  //! Total num: 10000; avg: 12; med: 8; min: 2; max: 319
  //! Total num: 10000; avg: 13; med: 8; min: 2; max: 291
  //! Total num: 10000; avg: 13; med: 8; min: 3; max: 436
  //! Total num: 10000; avg: 16; med: 9; min: 2; max: 474
  //!
  //! Total num: 10000; avg: 12; med: 7; min: 2; max: 349  # yyjson
  //! Total num: 10000; avg: 11; med: 7; min: 2; max: 1325
  //! Total num: 10000; avg: 15; med: 8; min: 1; max: 527
  //! Total num: 10000; avg: 11; med: 6; min: 1; max: 364
  //! Total num: 10000; avg: 14; med: 8; min: 1; max: 455
  //! Total num: 10000; avg: 14; med: 8; min: 1; max: 554
  //! Total num: 10000; avg: 12; med: 7; min: 1; max: 316
  //! Total num: 10000; avg: 20; med: 8; min: 1; max: 2172
  //!
  //! Total num: 100000; avg: 751 ; med: 15; min: 1; max: 54317  # rapidjson
  //! Total num: 100000; avg: 757 ; med: 13; min: 1; max: 100756 # yyjson
  //! Total num: 100000; avg: 1022; med: 12; min: 0; max: 91384  #
  //! boost::contains
  //!
  EXEC_PERF_TEST("HandleAsyncTask", asyncTask->task_->localTs_, 10000, 100);
  //!
#endif

  if (!topic.empty() && mdSvc_->saveMarketData()) {
    mdSvc_->getMDStorageSvc()->handle(asyncTask);
  }
}

void WSCliOfExch::handleMDOthers(WSCliAsyncTaskSPtr& asyncTask) {
  if (isSubOrUnSubRet(asyncTask)) {
    //! 通过rspParaser从应答中获取需要重新订阅或者取消订阅的列表
    const auto topicGroupNeedMaint =
        mdSvc_->getRspParser()->getTopicGroupForSubOrUnSubAgain(asyncTask);
    for (const auto& topic : topicGroupNeedMaint->topicGroupNeedSub_) {
      mdSvc_->getTopicGroupMustSubMaint()->removeTopicForSubAgain(topic);
    }
    for (const auto& topic : topicGroupNeedMaint->topicGroupNeedUnSub_) {
      mdSvc_->getTopicGroupMustSubMaint()->addTopicForUnSubAgain(topic);
    }
  }
}

void WSCliOfExch::updateActiveTimeOfTopic(const std::string& topic) {
  const auto now = GetTotalMSSince1970();
  TopicGroupNeedMaintSPtr topicGroupNeedMaintOfCopy = nullptr;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicGroupNeedMaint_);
    if (now - topicGroupNeedMaint_->checkTs_ <= 5000) {
      topicGroupNeedMaint_->topic2ActiveTimeGroup_[topic] = now;
    } else {
      topicGroupNeedMaintOfCopy = topicGroupNeedMaint_;
      topicGroupNeedMaint_ = std::make_shared<TopicGroupNeedMaint>(now);
    }
  }
  if (topicGroupNeedMaintOfCopy != nullptr) {
    mdSvc_->getSubAndUnSubSvc()->getTaskDispatcher()->dispatch(
        topicGroupNeedMaintOfCopy);
  }
}

}  // namespace bq::md::svc
