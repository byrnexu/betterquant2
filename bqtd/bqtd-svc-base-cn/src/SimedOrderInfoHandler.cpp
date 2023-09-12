/*!
 * \file SimedOrderInfoHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/01
 *
 * \brief
 */

#include "SimedOrderInfoHandler.hpp"

#include "Config.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "SHMIPC.hpp"
#include "TDSvcDef.hpp"
#include "TDSvcOfCN.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "db/TBLSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/OrderInfoIF.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/FeeUtil.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"

namespace bq::td::svc {

SimedOrderInfoHandler::SimedOrderInfoHandler(TDSvcOfCN* tdSvc) : tdSvc_(tdSvc) {
  milliSecIntervalOfSimOrderStatus_ =
      CONFIG["simedMode"]["milliSecIntervalOfSimOrderStatus"].as<std::uint32_t>(
          0);
}

void SimedOrderInfoHandler::simOnOrder(OrderInfoSPtr& ordReq) {
  const auto [statusCode, simedTDInfo] = MakeSimedTDInfo(ordReq->extData_);
  if (statusCode != 0) {
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = statusCode;
    LOG_W("Handle order in simed td mode failed. [{} - {}] {}", statusCode,
          GetStatusMsg(statusCode), ordReq->toShortStr());
    tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
          LOG_I("Send simed order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        MSG_ID_ON_ORDER_RET, ordReq->size());

    //! 复制一个副本，不然因为是异步入库，如果有后续状态的话，入库的都是同一个ordReq
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET,
                               std::make_shared<OrderInfo>(*ordReq),
                               SyncToRiskMgr::True, SyncToDB::True);
    return;
  }
  simOnOrder(ordReq, simedTDInfo);
}

void SimedOrderInfoHandler::simOnOrder(OrderInfoSPTr& ordReq,
                                       const SimedTDInfoSPtr& simedTDInfo) {
  switch (simedTDInfo->orderStatus_) {
    //! 模拟交易所确认状态
    case OrderStatus::ConfirmedByExch:
      simOnOrderConfirmedByExch(ordReq, simedTDInfo);
      break;

    //! 模拟部分成交状态
    case OrderStatus::PartialFilled: {
      //! symbolInfo在getFeeCurrency用于获取手续费币种
      const auto symbolInfo = simOnOrderConfirmedByExch(ordReq, simedTDInfo);
      if (symbolInfo) {
        simOnOrderPartialFilled(ordReq, simedTDInfo, symbolInfo);
      }
    } break;

    //! 模拟全部成交状态
    case OrderStatus::Filled: {
      //! symbolInfo在getFeeCurrency用于获取手续费币种
      const auto symbolInfo = simOnOrderConfirmedByExch(ordReq, simedTDInfo);
      if (symbolInfo) {
        simOnOrderFilled(ordReq, simedTDInfo, symbolInfo);
      }
    } break;

    //! 模拟废单
    case OrderStatus::Failed:
      simOnOrderFailed(ordReq, simedTDInfo);
      break;

    default:
      assert(1 == 2 && "Unhandled order status");
      break;
  }
}

db::symbolInfo::RecordSPtr SimedOrderInfoHandler::simOnOrderConfirmedByExch(
    OrderInfoSPTr& ordReq, const SimedTDInfoSPtr& simedTDInfo) {
  //! 获取marketCode和symbolInfo
  const auto marketCode = GetMarketName(ordReq->marketCode_);
  const auto [statusCode, symbolInfo] =
      tdSvc_->getTBLMonitorOfSymbolInfo()->getRecSymbolInfoBySymbolCode(
          marketCode, ordReq->symbolCode_);
  if (statusCode != SCODE_SUCCESS) {
    LOG_W(
        "Handle on order confirmed by exch in simed td mode failed because of "
        "query symbol info of {} - {} failed. [{} - {}]",
        marketCode, ordReq->symbolCode_, statusCode, GetStatusMsg(statusCode));
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = statusCode;
    tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
          LOG_I("Send simed order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        MSG_ID_ON_ORDER_RET, ordReq->size());
    //! 复制一个副本，不然因为是异步入库，如果有后续状态的话，入库的都是同一个ordReq
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET,
                               std::make_shared<OrderInfo>(*ordReq),
                               SyncToRiskMgr::True, SyncToDB::True);
    return nullptr;
  }

  //! 填写回报必要的字段
  ordReq->orderStatus_ = OrderStatus::ConfirmedByExch;
  ordReq->statusCode_ = SCODE_SUCCESS;
  strcpy(ordReq->exchOrderId_, fmt::format("{}", GET_RAND_INT()).c_str());
  ordReq->dealSize_ = 0;
  ordReq->avgDealPrice_ = 0;
  ordReq->lastTradeId_[0] = '\0';
  ordReq->lastDealPrice_ = 0;
  ordReq->lastDealSize_ = 0;
  ordReq->lastDealTime_ = GetTotalUSSince1970();

  const auto feeCurrency = getFeeCurrency(ordReq);
  strncpy(ordReq->feeCurrency_, feeCurrency.c_str(),
          sizeof(ordReq->feeCurrency_) - 1);

  //! 更新OrdMgr中的纪录
  const auto [isTheOrderInfoUpdated, orderInfoInOrdMgr] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              ordReq, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());

  //! 发送下单确认应答
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoInOrdMgr);
#ifndef OPT_LOG
        LOG_I("Send simed order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr->size());

  //! 复制一个副本，不然因为是异步入库，如果有后续状态的话，入库的都是同一个ordReq
  //! 这里不用复制了，orderInfoInOrdMgr就是一个副本。
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr,
                             SyncToRiskMgr::True, SyncToDB::True);

  return symbolInfo;
}

void SimedOrderInfoHandler::simOnOrderFilled(
    OrderInfoSPTr& ordReq, const SimedTDInfoSPtr& simedTDInfo,
    const db::symbolInfo::RecordSPtr& symbolInfo) {
  //! 填写必要的字段
  ordReq->orderStatus_ = OrderStatus::PartialFilled;
  ordReq->statusCode_ = SCODE_SUCCESS;
  ordReq->exchOrderId_[0] = '\0';

  for (std::size_t i = 0; i < simedTDInfo->transDetailGroup_.size(); ++i) {
    //! 每个回报之间有间隔
    if (milliSecIntervalOfSimOrderStatus_ != 0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(milliSecIntervalOfSimOrderStatus_));
    }

    //! 最后一条模拟成交，委托状态修改为 Filled
    if (i == simedTDInfo->transDetailGroup_.size() - 1) {
      ordReq->orderStatus_ = OrderStatus::Filled;
    }

    //! 填写lastTrade相关的4个字段
    const auto lastTradeId = fmt::format("{}", GET_RAND_INT());
    strncpy(ordReq->lastTradeId_, lastTradeId.c_str(),
            sizeof(ordReq->lastTradeId_) - 1);
    if (ordReq->side_ == Side::Bid) {
      ordReq->lastDealSize_ =
          ordReq->orderSize_ * simedTDInfo->transDetailGroup_[i]->filledPer_;
      ordReq->lastDealPrice_ =
          ordReq->orderPrice_ -
          ordReq->orderPrice_ * simedTDInfo->transDetailGroup_[i]->slippage_;
    } else {
      ordReq->lastDealSize_ = ordReq->orderSize_ *
                              simedTDInfo->transDetailGroup_[i]->filledPer_ *
                              -1;
      ordReq->lastDealPrice_ =
          ordReq->orderPrice_ +
          ordReq->orderPrice_ * simedTDInfo->transDetailGroup_[i]->slippage_;
    }
    ordReq->lastDealTime_ = GetTotalUSSince1970();

    //! 计算成交均价
    const auto prevDealAmt = ordReq->avgDealPrice_ * ordReq->dealSize_;
    const auto lastDealAmt = ordReq->lastDealPrice_ * ordReq->lastDealSize_;
    const auto totalDealAmt = prevDealAmt + lastDealAmt;
    const auto totalDealSize = ordReq->dealSize_ + ordReq->lastDealSize_;
    if (!DEC::ZERO(totalDealSize)) {
      ordReq->avgDealPrice_ = totalDealAmt / totalDealSize;
    }
    //! 成交数量
    ordReq->dealSize_ = totalDealSize;

    //! fee在updateByOrderInfoFromExch中计算
    //! feeCurrency_ simOnOrderConfirmedByExch 中已经填写

    //! 根据ordReq更新OrdMgr中的纪录
    const auto [isTheOrderInfoUpdated, orderInfoInOrdMgr] =
        tdSvc_->getOrdMgr()
            ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
                ordReq, tdSvc_->getNextNoUsedToCalcPos(),
                tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());

    //! 发送回报
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *orderInfoInOrdMgr);
#ifndef OPT_LOG
          LOG_I("Send simed order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr->size());

    //! 复制一个副本，不然因为是异步入库，如果有后续状态的话，入库的都是同一个ordReq
    //! 这里不用复制了，orderInfoInOrdMgr就是一个副本。
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr,
                               SyncToRiskMgr::True, SyncToDB::True);
  }
}

void SimedOrderInfoHandler::simOnOrderPartialFilled(
    OrderInfoSPTr& ordReq, const SimedTDInfoSPtr& simedTDInfo,
    const db::symbolInfo::RecordSPtr& symbolInfo) {
  //! 填写必要的字段
  ordReq->orderStatus_ = OrderStatus::PartialFilled;
  ordReq->statusCode_ = SCODE_SUCCESS;
  ordReq->exchOrderId_[0] = '\0';

  for (std::size_t i = 0; i < simedTDInfo->transDetailGroup_.size(); ++i) {
    //! 每个回报之间有间隔
    if (milliSecIntervalOfSimOrderStatus_ != 0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(milliSecIntervalOfSimOrderStatus_));
    }

    //! 填写lastTrade相关的4个字段
    const auto lastTradeId = fmt::format("{}", GET_RAND_INT());
    strncpy(ordReq->lastTradeId_, lastTradeId.c_str(),
            sizeof(ordReq->lastTradeId_) - 1);
    if (ordReq->side_ == Side::Bid) {
      ordReq->lastDealSize_ =
          ordReq->orderSize_ * simedTDInfo->transDetailGroup_[i]->filledPer_;
      ordReq->lastDealPrice_ =
          ordReq->orderPrice_ -
          ordReq->orderPrice_ * simedTDInfo->transDetailGroup_[i]->slippage_;
    } else {
      ordReq->lastDealSize_ = ordReq->orderSize_ *
                              simedTDInfo->transDetailGroup_[i]->filledPer_ *
                              -1;
      ordReq->lastDealPrice_ =
          ordReq->orderPrice_ +
          ordReq->orderPrice_ * simedTDInfo->transDetailGroup_[i]->slippage_;
    }
    ordReq->lastDealTime_ = GetTotalUSSince1970();

    //! 计算成交均价
    const auto prevDealAmt = ordReq->avgDealPrice_ * ordReq->dealSize_;
    const auto lastDealAmt = ordReq->lastDealPrice_ * ordReq->lastDealSize_;
    const auto totalDealAmt = prevDealAmt + lastDealAmt;
    const auto totalDealSize = ordReq->dealSize_ + ordReq->lastDealSize_;
    if (!DEC::ZERO(totalDealSize)) {
      ordReq->avgDealPrice_ = totalDealAmt / totalDealSize;
    }
    //! 成交数量
    ordReq->dealSize_ = totalDealSize;

    //! fee在updateByOrderInfoFromExch中计算
    //! feeCurrency_ simOnOrderConfirmedByExch 中已经填写

    //! 根据ordReq更新OrdMgr中的纪录
    const auto [isTheOrderInfoUpdated, orderInfoInOrdMgr] =
        tdSvc_->getOrdMgr()
            ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
                ordReq, tdSvc_->getNextNoUsedToCalcPos(),
                tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());

    //! 发送回报
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *orderInfoInOrdMgr);
#ifndef OPT_LOG
          LOG_I("Send simed order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr->size());

    //! 复制一个副本，不然因为是异步入库，如果有后续状态的话，入库的都是同一个ordReq
    //! 这里不用复制了，orderInfoInOrdMgr就是一个副本。
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr,
                               SyncToRiskMgr::True, SyncToDB::True);
  }
}

void SimedOrderInfoHandler::simOnOrderFailed(
    OrderInfoSPTr& ordReq, const SimedTDInfoSPtr& simedTDInfo) {
  //! 填写废单模拟的必要字段
  ordReq->orderStatus_ = OrderStatus::Failed;
  ordReq->statusCode_ = SCODE_EXTERNAL_SYS_ORDER_REJECTED_MIN;

  //! 移除OrdMgr中的记录
  tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);

  //! 发送废单回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
        LOG_I("Send simed order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, ordReq->size());

  //! 创建同步任务
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq, SyncToRiskMgr::True,
                             SyncToDB::True);
}

void SimedOrderInfoHandler::simOnCancelOrder(OrderInfoSPtr& ordReq) {
  //! 获取 OrdMgr 中的订单(下单后马上撤单的情况下，传过来的 ordReq 可能不正确)
  int statusCode = 0;
  OrderInfoSPtr orderInfoInOrdMgr{nullptr};
  std::tie(statusCode, orderInfoInOrdMgr) =
      tdSvc_->getOrdMgr()->getOrderInfo<LockFunc::True, DeepClone::False>(
          ordReq->orderId_);
  if (statusCode != 0 || orderInfoInOrdMgr == nullptr) {
    ordReq->statusCode_ = -1;
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
          LOG_I("Send simed order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());
    return;
  }

  //! 确定委托状态是全撤还是部成部撤
  orderInfoInOrdMgr->orderStatus_ = DEC::ZERO(orderInfoInOrdMgr->dealSize_)
                                        ? OrderStatus::Canceled
                                        : OrderStatus::PartialFilledCanceled;

  //! 订单已经是完结状态，移除OrdMgr中的纪录
  tdSvc_->getOrdMgr()->remove<LockFunc::True>(orderInfoInOrdMgr->orderId_);

  //! 发送撤单应答
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoInOrdMgr);
#ifndef OPT_LOG
        LOG_I("Send simed order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr->size());

  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr,
                             SyncToRiskMgr::True, SyncToDB::True);
}

}  // namespace bq::td::svc
