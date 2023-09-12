/*!
 * \file StgInstTaskHandlerImpl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgInstTaskHandlerImpl.hpp"

#include "CommonIPCData.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "SHMCli.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCTask.hpp"
#include "SHMIPCUtil.hpp"
#include "StgEngImpl.hpp"
#include "StgEngUtil.hpp"
#include "SysInstructionSvc.hpp"
#include "db/TBLMonitorOfStgInstInfo.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfAssets.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfStg.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StgConst.hpp"
#include "util/Datetime.hpp"
#include "util/MarketDataCache.hpp"
#include "util/PosSnapshot.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::stg {

StgInstTaskHandlerImpl::StgInstTaskHandlerImpl(
    StgEngImpl* stgEng,
    const StgInstTaskHandlerBundle& stgInstTaskHandlerBundle)
    : stgEng_(stgEng), stgInstTaskHandlerBundle_(stgInstTaskHandlerBundle) {}

void StgInstTaskHandlerImpl::handleAsyncTask(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto stgInstId = std::any_cast<StgInstId>(asyncTask->arg_);
  const auto [ret, stgInstInfo] =
      getStgEngImpl()->getTBLMonitorOfStgInstInfo()->getStgInstInfo(stgInstId);
  if (ret == 0) {
    handleAsyncTaskImpl(stgInstInfo, asyncTask);
  } else {
    stgEng_->logWarn(
        "Get stg inst info of {} - {} failed. ",
        {std::to_string(stgEng_->getStgId()), std::to_string(stgInstId)},
        stgEng_->getDftStgInstInfo());
  }
}

void StgInstTaskHandlerImpl::handleAsyncTaskImpl(
    const StgInstInfoSPtr& stgInstInfo, const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto data = asyncTask->task_->data_;
  const auto msgId = static_cast<const SHMHeader*>(data)->msgId_;
  switch (msgId) {
    case MSG_ID_ON_STG_MANUAL_INTERVENTION: {
      const auto commonIPCData =
          static_cast<const CommonIPCData*>(asyncTask->task_->data_);
      stgEng_->logInfo("Recv manual intervention: {}", {commonIPCData->data_},
                       stgInstInfo);
      const auto notContinueToHandleInstr = beforeOnStgManualIntervention(
          stgInstInfo, std::string(commonIPCData->data_));
      if (notContinueToHandleInstr) {
        return;
      }
      if (stgInstTaskHandlerBundle_.onStgManualIntervention_) {
        stgInstTaskHandlerBundle_.onStgManualIntervention_(stgInstInfo,
                                                           commonIPCData);
      }
    } break;

    case MSG_ID_ON_PUSH_TOPIC: {
      const auto commonIPCData =
          static_cast<const CommonIPCData*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onPushTopic_) {
        stgInstTaskHandlerBundle_.onPushTopic_(stgInstInfo, commonIPCData);
      }
    } break;

    case MSG_ID_ON_ORDER_RET: {
      const auto ordRet =
          static_cast<const OrderInfo*>(asyncTask->task_->data_);
      stgEng_->logInfo("Recv order ret {}", {ordRet->toShortStr()},
                       stgInstInfo);
      beforeOnOrderRet(stgInstInfo, ordRet);
      if (stgInstTaskHandlerBundle_.onOrderRet_) {
        stgInstTaskHandlerBundle_.onOrderRet_(stgInstInfo, ordRet);
      }
    } break;

    case MSG_ID_ON_CANCEL_ORDER_RET: {
      const auto ordRet =
          static_cast<const OrderInfo*>(asyncTask->task_->data_);
      stgEng_->logInfo("Recv cancel order ret {}", {ordRet->toShortStr()},
                       stgInstInfo);
      beforeOnCancelOrderRet(stgInstInfo, ordRet);
      if (stgInstTaskHandlerBundle_.onCancelOrderRet_) {
        stgInstTaskHandlerBundle_.onCancelOrderRet_(stgInstInfo, ordRet);
      }
    } break;

    case MSG_ID_ON_ALGO_ORDER: {
      const auto commonIPCData =
          static_cast<const CommonIPCData*>(asyncTask->task_->data_);
      stgEng_->logInfo("Recv on algo order {}", {commonIPCData->data_},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onAlgoOrder_) {
        stgInstTaskHandlerBundle_.onAlgoOrder_(stgInstInfo, commonIPCData);
      }
    } break;

    case MSG_ID_ON_MD_TRADES: {
      const auto trades = static_cast<const Trades*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onTrades_) {
        stgInstTaskHandlerBundle_.onTrades_(stgInstInfo, trades);
      }
    } break;

    case MSG_ID_ON_MD_ORDERS: {
      const auto orders = static_cast<const Orders*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onOrders_) {
        stgInstTaskHandlerBundle_.onOrders_(stgInstInfo, orders);
      }
    } break;

    case MSG_ID_ON_MD_TICKERS: {
      const auto tickers = static_cast<const Tickers*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onTickers_) {
        stgInstTaskHandlerBundle_.onTickers_(stgInstInfo, tickers);
      }
      stgEng_->getMarketDataCache()->cache(std::make_shared<Tickers>(*tickers));
    } break;

    case MSG_ID_ON_MD_CANDLE: {
      const auto candle = static_cast<const Candle*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onCandle_) {
        stgInstTaskHandlerBundle_.onCandle_(stgInstInfo, candle);
      }
    } break;

    case MSG_ID_ON_MD_BOOKS: {
      const auto books = static_cast<const Books*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onBooks_) {
        stgInstTaskHandlerBundle_.onBooks_(stgInstInfo, books);
      }
    } break;

    case MSG_ID_ON_MD_BID1_ASK1: {
      const auto bid1ask1 =
          static_cast<const Bid1Ask1*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onBid1Ask1_) {
        stgInstTaskHandlerBundle_.onBid1Ask1_(stgInstInfo, bid1ask1);
      }
    } break;

    case MSG_ID_ON_MD_LAST_PRICE: {
      const auto lastPrice =
          static_cast<const LastPrice*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onLastPrice_) {
        stgInstTaskHandlerBundle_.onLastPrice_(stgInstInfo, lastPrice);
      }
    } break;

    case MSG_ID_ON_MD_DYN_CANDLE: {
      const auto candle = static_cast<const Candle*>(asyncTask->task_->data_);
      if (stgInstTaskHandlerBundle_.onDynCandle_) {
        stgInstTaskHandlerBundle_.onDynCandle_(stgInstInfo, candle);
      }
    } break;

    case MSG_ID_ON_STG_START:
      stgEng_->logInfo("On stg {} start trigged. ",
                       {std::to_string(stgInstInfo->stgId_)}, stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgStart_) {
        stgInstTaskHandlerBundle_.onStgStart_();
      }
      getStgEngImpl()->getBarrierOfStgStartSignal()->set_value();
      break;

    case MSG_ID_ON_STG_INST_START:
      stgEng_->logInfo("On stg inst start trigged. {}", {stgInstInfo->toStr()},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstStart_) {
        stgInstTaskHandlerBundle_.onStgInstStart_(stgInstInfo);
      }
      break;

    case MSG_ID_ON_STG_STOP:
      stgEng_->logInfo("On stg {} stop trigged. ",
                       {std::to_string(stgInstInfo->stgId_)}, stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgStop_) {
        stgInstTaskHandlerBundle_.onStgStop_();
      }
      break;

    case MSG_ID_ON_STG_INST_STOP:
      stgEng_->logInfo("On stg inst stop trigged. {}", {stgInstInfo->toStr()},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstStop_) {
        stgInstTaskHandlerBundle_.onStgInstStop_(stgInstInfo);
      }
      break;

    case MSG_ID_ON_STG_INST_ADD:
      stgEng_->logInfo("On stg inst add trigged. {}", {stgInstInfo->toStr()},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstAdd_) {
        stgInstTaskHandlerBundle_.onStgInstAdd_(stgInstInfo);
      }
      break;

    case MSG_ID_ON_STG_INST_DEL:
      stgEng_->logInfo("On stg inst del trigged. {}", {stgInstInfo->toStr()},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstDel_) {
        stgInstTaskHandlerBundle_.onStgInstDel_(stgInstInfo);
      }
      break;

    case MSG_ID_ON_STG_INST_CHG:
      stgEng_->logInfo("On stg inst chg trigged. {}", {stgInstInfo->toStr()},
                       stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstChg_) {
        stgInstTaskHandlerBundle_.onStgInstChg_(stgInstInfo);
      }
      break;

    case MSG_ID_ON_STG_INST_TIMER: {
      const auto commonIPCData =
          static_cast<const CommonIPCData*>(asyncTask->task_->data_);
      auto timerName = std::string(commonIPCData->data_);
      const auto prefix =
          fmt::format("{}{}-", PREFIX_OF_TIMER_NAME, stgInstInfo->stgInstId_);
      //! 确保install定时器和回调里出来的timeName是一样的
      boost::erase_first(timerName, prefix);
      stgEng_->logDebug("On timer of {} stg inst {}. {}",
                        {std::to_string(stgInstInfo->stgInstId_), timerName,
                         stgInstInfo->toStr()},
                        stgInstInfo);
      if (stgInstTaskHandlerBundle_.onStgInstTimer_) {
        stgInstTaskHandlerBundle_.onStgInstTimer_(stgInstInfo, timerName);
      }
    } break;

    case MSG_ID_ON_STG_REG:
      stgEng_->logDebug("On stg reg trigged. ", stgInstInfo);
      break;

    case MSG_ID_POS_UPDATE_OF_ACCT_ID: {
      const auto posUpdateOfAcctIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfAcctIdForPub>(asyncTask->task_);
      const auto posUpdateOfAcctId =
          MakePosUpdateOfAcctId(posUpdateOfAcctIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posUpdateOfAcctId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosUpdateOfAcctId_) {
        stgInstTaskHandlerBundle_.onPosUpdateOfAcctId_(stgInstInfo,
                                                       posSnapshot);
      }
    } break;

    case MSG_ID_POS_SNAPSHOT_OF_ACCT_ID: {
      const auto posUpdateOfAcctIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfAcctIdForPub>(asyncTask->task_);
      const auto posSnapshotOfAcctId =
          MakePosUpdateOfAcctId(posUpdateOfAcctIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posSnapshotOfAcctId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosSnapshotOfAcctId_) {
        stgInstTaskHandlerBundle_.onPosSnapshotOfAcctId_(stgInstInfo,
                                                         posSnapshot);
      }
    } break;

    case MSG_ID_POS_UPDATE_OF_STG_ID: {
      const auto posUpdateOfStgIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfStgIdForPub>(asyncTask->task_);
      const auto posUpdateOfStgId =
          MakePosUpdateOfStgId(posUpdateOfStgIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posUpdateOfStgId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosUpdateOfStgId_) {
        stgInstTaskHandlerBundle_.onPosUpdateOfStgId_(stgInstInfo, posSnapshot);
      }
    } break;

    case MSG_ID_POS_SNAPSHOT_OF_STG_ID: {
      const auto posUpdateOfStgIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfStgIdForPub>(asyncTask->task_);
      const auto posSnapshotOfStgId =
          MakePosUpdateOfStgId(posUpdateOfStgIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posSnapshotOfStgId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosSnapshotOfStgId_) {
        stgInstTaskHandlerBundle_.onPosSnapshotOfStgId_(stgInstInfo,
                                                        posSnapshot);
      }
    } break;

    case MSG_ID_POS_UPDATE_OF_STG_INST_ID: {
      const auto posUpdateOfStgInstIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfStgInstIdForPub>(asyncTask->task_);
      const auto posUpdateOfStgInstId =
          MakePosUpdateOfStgInstId(posUpdateOfStgInstIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posUpdateOfStgInstId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosUpdateOfStgInstId_) {
        stgInstTaskHandlerBundle_.onPosUpdateOfStgInstId_(stgInstInfo,
                                                          posSnapshot);
      }
    } break;

    case MSG_ID_POS_SNAPSHOT_OF_STG_INST_ID: {
      const auto posUpdateOfStgInstIdForPub =
          MakeMsgSPtrByTask<PosUpdateOfStgInstIdForPub>(asyncTask->task_);
      const auto posSnapshotOfStgInstId =
          MakePosUpdateOfStgInstId(posUpdateOfStgInstIdForPub);
      const auto posSnapshot = std::make_shared<PosSnapshot>(
          std::move(posSnapshotOfStgInstId), stgEng_->getMarketDataCache());
      if (stgInstTaskHandlerBundle_.onPosSnapshotOfStgInstId_) {
        stgInstTaskHandlerBundle_.onPosSnapshotOfStgInstId_(stgInstInfo,
                                                            posSnapshot);
      }
    } break;

    case MSG_ID_ASSETS_UPDATE: {
      const auto assetsUpdateForPub =
          MakeMsgSPtrByTask<AssetsUpdateForPub>(asyncTask->task_);
      const auto assetsUpdate = MakeAssetsUpdate(assetsUpdateForPub);
      if (stgInstTaskHandlerBundle_.onAssetsUpdate_) {
        stgInstTaskHandlerBundle_.onAssetsUpdate_(stgInstInfo, assetsUpdate);
      }
    } break;

    case MSG_ID_ASSETS_SNAPSHOT: {
      const auto assetsUpdateForPub =
          MakeMsgSPtrByTask<AssetsUpdateForPub>(asyncTask->task_);
      const auto assetsSnapshot = MakeAssetsUpdate(assetsUpdateForPub);
      if (stgInstTaskHandlerBundle_.onAssetsSnapshot_) {
        stgInstTaskHandlerBundle_.onAssetsSnapshot_(stgInstInfo,
                                                    assetsSnapshot);
      }
    } break;

    default: {
      const auto msgId = static_cast<const SHMHeader*>(data)->msgId_;
      stgEng_->logInfo(
          "On msg {} - {} of stg inst {}. {}",
          {std::to_string(msgId), GetMsgName(msgId),
           std::to_string(stgInstInfo->stgInstId_), stgInstInfo->toStr()},
          stgInstInfo);
      if (stgInstTaskHandlerBundle_.onOtherStgInstTask_) {
        stgInstTaskHandlerBundle_.onOtherStgInstTask_(stgInstInfo, asyncTask);
      }
    } break;
  }
}

void StgInstTaskHandlerImpl::beforeOnOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {
  //!
  //! syncToDB动作已经在tdSrv做了，orderInfo都会到策略层被用户使用，所以这里都复
  //! 制一个副本。
  //!
  const auto orderRet = std::make_shared<OrderInfo>(*orderInfo);

  const auto [isTheOrderCanBeUsedCalcPos, orderInfoInOrdMgr] =
      getStgEngImpl()->getOrdMgr()->updateByOrderInfoFromTDGW<LockFunc::True>(
          orderRet);

  if (isTheOrderCanBeUsedCalcPos == IsTheOrderCanBeUsedCalcPos::True) {
    getStgEngImpl()->getPosMgr()->updateByOrderInfoFromTDGW<LockFunc::True>(
        orderRet);
  }

  //! 手拍单转发到web服务
  if (stgInstInfo->stgId_ == STG_OF_MANUAL) {
    const auto orderInfoInJsonFmt = orderRet->toJson();
    const auto shmBufLen =
        sizeof(CommonIPCData) + orderInfoInJsonFmt.size() + 1;
    stgEng_->getSHMCliOfWebSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          auto commonIPCData = static_cast<CommonIPCData*>(shmBuf);
          memcpy(commonIPCData->data_, orderInfoInJsonFmt.c_str(),
                 orderInfoInJsonFmt.size());
          commonIPCData->dataLen_ = orderInfoInJsonFmt.size() + 1;
        },
        MSG_ID_ON_ORDER_RET, shmBufLen);
  }
}

bool StgInstTaskHandlerImpl::beforeOnStgManualIntervention(
    const StgInstInfoSPtr& stgInstInfo, const std::string& instruction) {
  const auto notContinueToHandleInstr =
      stgEng_->getSysInstructionSvc()->handle(stgInstInfo, instruction);
  return notContinueToHandleInstr;
}

void StgInstTaskHandlerImpl::beforeOnCancelOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {
  //! 手拍单转发到web服务
  if (stgInstInfo->stgId_ == STG_OF_MANUAL) {
    const auto orderInfoInJsonFmt = orderInfo->toJson();
    const auto shmBufLen =
        sizeof(CommonIPCData) + orderInfoInJsonFmt.size() + 1;
    stgEng_->getSHMCliOfWebSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          auto commonIPCData = static_cast<CommonIPCData*>(shmBuf);
          memcpy(commonIPCData->data_, orderInfoInJsonFmt.c_str(),
                 orderInfoInJsonFmt.size());
          commonIPCData->dataLen_ = orderInfoInJsonFmt.size() + 1;
        },
        MSG_ID_ON_CANCEL_ORDER_RET, shmBufLen);
  }
}

}  // namespace bq::stg
