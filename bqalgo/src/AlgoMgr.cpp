/*!
 * \file AlgoMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 *
 * AlgoMgr不停的给自己维护的algoOrder推送onTimer还有行情和交易消息，algoOrder
 * 内部会维护自己的状态，当algoMgr发现algoOrder为结束状态的时候，会移除algoOrder
 * 信息，被移除的algoOrder也不会再收到onTimer以及行情和交易的信息
 *
 */

#include "AlgoMgr.hpp"

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "AlgoOrder.hpp"
#include "AlgoSmartOrder.hpp"
#include "AlgoTWAP.hpp"
#include "AlgoTWAPFSM.hpp"
#include "SHMIPCTask.hpp"
#include "StgEngImpl.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfTD.hpp"
#include "util/BQUtil.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/StdExt.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::algo {

AlgoMgr::AlgoMgr(stg::StgEngImpl* stgEngImpl) : stgEngImpl_(stgEngImpl) {}

int AlgoMgr::init() {
  const auto statusCode = initTaskDispatacher();
  if (statusCode != 0) {
    return statusCode;
  }

  usIntervalOfTaskHandler_ =
      getStgEng()
          ->getConfig()["algoMgr"]["usIntervalOfTaskHandler"]
          .as<std::uint32_t>(1);

  return statusCode;
}

int AlgoMgr::initTaskDispatacher() {
  const auto algoMgrParamInStrFmt = SetParam(
      DEFAULT_TASK_DISPATCHER_PARAM,
      getStgEng()
          ->getConfig()["algoMgr"]["taskDispatcherParam"]
          .as<std::string>(
              "moduleName=algoMgrTaskDispatcherParam; "
              "numOfUnprocessedTaskAlert=1000; taskRandAllocThreadPoolSize=0; "
              "taskSpecificThreadPoolSize=1"));
  const auto [ret, dynCandleSvcParam] =
      MakeTaskDispatcherParam(algoMgrParamInStrFmt);
  if (ret != 0) {
    getStgEng()->logError("[ALGO] Init failed. {}", {algoMgrParamInStrFmt},
                          getStgEng()->getDftStgInstInfo());
    return ret;
  }

  taskDispatcher_ =
      std::make_shared<TaskDispatcher<SHMIPCTaskSPtr, BlockType::NoBlock>>(
          dynCandleSvcParam,
          [this](const auto& task) {
            const auto asyncTask =
                std::make_shared<SHMIPCAsyncTask>(task, std::any());
            return std::make_tuple(0, asyncTask);
          },
          [](auto& asyncTask, auto taskSpecificThreadPoolSize) {
            return ThreadNo(0);
          },
          [this](auto& asyncTask) { handleAsyncTask(asyncTask); });

  taskDispatcher_->init();

  return 0;
}

void AlgoMgr::start() {
  getStgEng()->logInfo("[ALGO] Start algo order manager.",
                       getStgEng()->getDftStgInstInfo());
  taskDispatcher_->start();
}

void AlgoMgr::stop() {
  taskDispatcher_->stop();
  getStgEng()->logInfo("[ALGO] Stop algo order manager.",
                       getStgEng()->getDftStgInstInfo());
}

std::tuple<int, AlgoId> AlgoMgr::algoOrder(
    const StgInstInfoSPtr& stgInstInfo, const std::string& algoType,
    const std::string& algoName, std::uint32_t lifetime,
    const std::string& algoParamsInJsonFmt) {
  auto jsonStr = RemoveWhitespaceAndNewlines(algoParamsInJsonFmt);
  //! 创建算法单实例
  AlgoOrderSPtr algoOrder = nullptr;
  if (algoType == AlgoTypeTWAP) {
    algoOrder = std::make_shared<TWAP>(  //
        this, stgInstInfo, algoType, algoName, lifetime);

  } else if (algoType == AlgoTypeSmartOrder) {
    algoOrder = std::make_shared<SmartOrder>(  //
        this, stgInstInfo, algoType, algoName, lifetime);

  } else {
    getStgEng()->logInfo("[ALGO] Invalid algo type {} of algo order {}. {}",
                         {algoType, algoName, (jsonStr)}, stgInstInfo);
    return {SCODE_ALGO_INVALID_ALGO_TYPE, 0};
  }

  //! 初始化算法单参数，此时algoOrder未加入AlgoMgr，algoOrder没有竞争，init是安全的
  if (const auto statusCode = algoOrder->init(jsonStr); statusCode != 0) {
    return {statusCode, 0};
  } else {
    getStgEng()->logInfo("[ALGO] Init params of algo order success. [{}] {}",
                         {algoOrder->toStr(), (jsonStr)}, stgInstInfo);
  }

  //! 订阅行情
  const auto [statusCode, topicGroup] = algoOrder->sub(jsonStr);
  if (statusCode != 0) {
    return {statusCode, 0};
  } else {
    //! 保存topic的订阅者信息
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2AlgoOrderGroup_);
    for (const auto& topic : topicGroup) {
      const auto internalTopic = convertTopic(topic);
      const auto topicHash =
          XXH3_64bits(internalTopic.c_str(), internalTopic.size());
      topicHash2AlgoOrderGroup_[topicHash].emplace_back(algoOrder);
    }
  }

  //! 保存算法单实例
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoId2AlgoOrderGroup_);
    algoId2AlgoOrderGroup_.emplace(algoOrder->getAlgoId(), algoOrder);
  }

  //! 算法单实例状态设为已启动
  algoOrder->setAlgoStatus(AlgoStatus::Started);

  getStgEng()->logInfo("[ALGO] Start algo order. [{}] {}",
                       {algoOrder->toStr(), (jsonStr)}, stgInstInfo);

  return {0, algoOrder->getAlgoId()};
}

int AlgoMgr::cancelAlgoOrder(AlgoId algoId,
                             TryToCancelAllOrders tryToCancelAllOrders) {
  const auto [statusCode, algoOrder] = releaseAlgoOrderInAlgoMgr(algoId);
  if (statusCode != 0) {
    return statusCode;
  }

  //! 撤销所有在途订单
  if (tryToCancelAllOrders == TryToCancelAllOrders::True) {
    getStgEng()->logInfo(
        "[ALGO] Cancel order of algo. [{}] {}",
        {algoOrder->toStr(), (algoOrder->getAlgoParamsInJsonFmt())},
        algoOrder->getStgInstInfo());
    algoOrder->cancelAllOrders();
  }

  algoOrder->setAlgoStatus(AlgoStatus::Canceled);
  getStgEng()->logInfo(
      "[ALGO] Cancel algo order. [{}] {}",
      {algoOrder->toStr(), (algoOrder->getAlgoParamsInJsonFmt())},
      algoOrder->getStgInstInfo());

  return 0;
}

std::string AlgoMgr::getProgressOfAlgoOrder(AlgoId algoId) {
  //! TODO
  return "";
}

void AlgoMgr::handle(SHMIPCTaskSPtr& shmIPCTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(shmIPCTask->data_);
  if (shmHeader->msgId_ == MSG_ID_ON_ORDER_RET ||
      shmHeader->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
    const auto orderInfo = static_cast<const OrderInfo*>(shmIPCTask->data_);
    if (orderInfo->algoId_ != 0) {
      taskDispatcher_->dispatch(shmIPCTask);
    }

  } else if (shmHeader->msgId_ == MSG_ID_ON_MD_TRADES ||
             shmHeader->msgId_ == MSG_ID_ON_MD_ORDERS ||
             shmHeader->msgId_ == MSG_ID_ON_MD_TICKERS ||
             shmHeader->msgId_ == MSG_ID_ON_MD_CANDLE ||
             shmHeader->msgId_ == MSG_ID_ON_MD_BOOKS ||
             shmHeader->msgId_ == MSG_ID_ON_MD_BID1_ASK1 ||
             shmHeader->msgId_ == MSG_ID_ON_MD_LAST_PRICE ||
             shmHeader->msgId_ == MSG_ID_ON_MD_DYN_CANDLE) {
    const auto topicHash = shmHeader->topicHash_;
    if (topicHash == 0) {
      const auto addrOfMDHeader =
          static_cast<const char*>(shmIPCTask->data_) + sizeof(SHMHeader);
      const auto mdHeader = reinterpret_cast<const MDHeader*>(addrOfMDHeader);
      getStgEng()->logInfo("[ALGO] Recv market data of topic hash 0. {}",
                           {mdHeader->toStr()},
                           getStgEng()->getDftStgInstInfo());
      return;
    }

    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2AlgoOrderGroup_);
      const auto iter = topicHash2AlgoOrderGroup_.find(topicHash);
      if (iter == std::end(topicHash2AlgoOrderGroup_)) {
        //! 说明没有算法单订阅当前行情
        return;
      }
    }

    taskDispatcher_->dispatch(shmIPCTask);
  }
}

void AlgoMgr::handleAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask) {
  if (asyncTask) {
    //! 处理行情和交易
    onAsyncTask(asyncTask);
  } else {
    onTimer();
  }
}

void AlgoMgr::onAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_ORDER_RET: {
      const auto orderInfo =
          static_cast<const OrderInfo*>(asyncTask->task_->data_);
      const auto algoId = orderInfo->algoId_;
      AlgoOrderSPtr algoOrder = nullptr;
      {
        std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoId2AlgoOrderGroup_);
        auto iter = algoId2AlgoOrderGroup_.find(algoId);
        if (iter != std::end(algoId2AlgoOrderGroup_)) {
          algoOrder = iter->second;
        }
      }
      if (algoOrder) algoOrder->onOrderRet(orderInfo);
    } break;

    case MSG_ID_ON_CANCEL_ORDER_RET: {
      const auto orderInfo =
          static_cast<const OrderInfo*>(asyncTask->task_->data_);
      const auto algoId = orderInfo->algoId_;
      AlgoOrderSPtr algoOrder = nullptr;
      {
        std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoId2AlgoOrderGroup_);
        auto iter = algoId2AlgoOrderGroup_.find(algoId);
        if (iter != std::end(algoId2AlgoOrderGroup_)) {
          algoOrder = iter->second;
        }
      }
      if (algoOrder) algoOrder->onCancelOrderRet(orderInfo);
    } break;

    case MSG_ID_ON_MD_TRADES: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto trades = static_cast<const Trades*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onTrades(trades);
      }
    } break;

    case MSG_ID_ON_MD_ORDERS: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto orders = static_cast<const Orders*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onOrders(orders);
      }
    } break;

    case MSG_ID_ON_MD_TICKERS: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto tickers = static_cast<const Tickers*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onTickers(tickers);
      }
    } break;

    case MSG_ID_ON_MD_CANDLE: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto candle = static_cast<const Candle*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onCandle(candle);
      }
    } break;

    case MSG_ID_ON_MD_BOOKS: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto books = static_cast<const Books*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onBooks(books);
      }
    } break;

    case MSG_ID_ON_MD_BID1_ASK1: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto bid1ask1 =
          static_cast<const Bid1Ask1*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onBid1Ask1(bid1ask1);
      }
    } break;

    case MSG_ID_ON_MD_LAST_PRICE: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto lastPrice =
          static_cast<const LastPrice*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onLastPrice(lastPrice);
      }
    } break;

    case MSG_ID_ON_MD_DYN_CANDLE: {
      const auto algoOrderGroup =
          getAlgoOrderGroupOfSubTopic(shmHeader->topicHash_);
      const auto candle = static_cast<const Candle*>(asyncTask->task_->data_);
      for (auto& algoOrder : algoOrderGroup) {
        algoOrder->onDynCandle(candle);
      }
    } break;

    default:
      break;
  }
}

AlgoMgr::AlgoOrderGroup AlgoMgr::getAlgoOrderGroupOfSubTopic(
    std::uint64_t topicHash) {
  AlgoOrderGroup algoOrderGroup;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2AlgoOrderGroup_);
    const auto iter = topicHash2AlgoOrderGroup_.find(topicHash);
    if (iter != std::end(topicHash2AlgoOrderGroup_)) {
      algoOrderGroup = iter->second;
    } else {
      return AlgoOrderGroup();
    }
  }
  return algoOrderGroup;
}

void AlgoMgr::onTimer() {
  decltype(algoId2AlgoOrderGroup_) algoId2AlgoOrderGroupOfShallowCopy;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoId2AlgoOrderGroup_);
    algoId2AlgoOrderGroupOfShallowCopy = algoId2AlgoOrderGroup_;
  }

  for (const auto& rec : algoId2AlgoOrderGroupOfShallowCopy) {
    auto& algoOrder = rec.second;
    algoOrder->onTimer();

    //! 如果算法单超时，那么释放相应的资源
    const auto algoStatus = algoOrder->getAlgoStatus();
    if (algoStatus == AlgoStatus::OutOfTime ||
        algoStatus == AlgoStatus::ExecFailed ||
        algoStatus == AlgoStatus::Finished) {
      releaseAlgoOrderInAlgoMgr(algoOrder->getAlgoId());
      getStgEng()->logInfo(
          "[ALGO] Release algo order in algo mgr "
          "because of algo status is {}. [{}]",
          {ENUM_TO_STR(algoStatus), algoOrder->toStr()},
          algoOrder->getStgInstInfo());
    }
  }

  //! 空转后才sleep
  if (usIntervalOfTaskHandler_ != 0) {
    std::this_thread::sleep_for(
        std::chrono::microseconds(usIntervalOfTaskHandler_));
  }
}

std::tuple<int, AlgoOrderSPtr> AlgoMgr::releaseAlgoOrderInAlgoMgr(
    AlgoId algoId) {
  //! 移除算法单实例
  AlgoOrderSPtr algoOrder = nullptr;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoId2AlgoOrderGroup_);
    auto iter = algoId2AlgoOrderGroup_.find(algoId);
    if (iter != std::end(algoId2AlgoOrderGroup_)) {
      algoOrder = iter->second;
      algoId2AlgoOrderGroup_.erase(iter);
    } else {
      getStgEng()->logInfo(
          "[ALGO] Algo id {} not exists when release algo order.",
          {std::to_string(algoId)}, getStgEng()->getDftStgInstInfo());
      return {SCODE_ALGO_ID_NOT_EXISTS, algoOrder};
    }
  }

  //! 取消订阅行情
  algoOrder->unSub(algoOrder->getAlgoParamsInJsonFmt());

  //! 移除topic的订阅者信息
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2AlgoOrderGroup_);
    for (auto& rec : topicHash2AlgoOrderGroup_) {
      auto& algoIdGroup = rec.second;
      std::ext::erase_if(algoIdGroup, [algoId](auto algoOrder) {
        if (algoOrder->getAlgoId() == algoId) return true;
        return false;
      });
    }
  }

  return {0, algoOrder};
}

}  // namespace bq::algo
