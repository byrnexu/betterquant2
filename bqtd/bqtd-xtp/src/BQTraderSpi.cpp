/*!
 * \file BQTraderSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/31
 *
 * \brief
 */

#include "BQTraderSpi.hpp"

#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDGatewayOfXTP.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfXTPUtil.hpp"
#include "TDSvcUtil.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/Logger.hpp"
#include "util/MapRelOfCliIdAndOrdId.hpp"

using namespace std;

namespace bq::td::svc::xtp {

BQTraderSpi::BQTraderSpi(TDSvcOfCN* tdSvc, TraderApi* api)
    : tdSvc_(tdSvc), api_(api) {
  for (std::size_t i = 0; i < CONFIG["marketCodeGroup"].size(); ++i) {
    const auto marketName = CONFIG["marketCodeGroup"][i].as<std::string>();
    const auto marketCode = bq::GetMarketCode(marketName);
    marketCodeGroup_.emplace_back(marketCode);
  }
}

BQTraderSpi::~BQTraderSpi() {}

void BQTraderSpi::OnDisconnected(uint64_t sessionId, int reason) {
  LOG_W("Disconnect from trader server. session id {} reason {}", sessionId,
        reason);

  std::uint64_t newSessionId = 0;
  while (newSessionId == 0) {
    newSessionId = api_->Login(
        CONFIG["api"]["traderServerIP"].as<std::string>().c_str(),
        CONFIG["api"]["traderServerPort"].as<std::uint32_t>(),
        CONFIG["api"]["traderUsername"].as<std::string>().c_str(),
        CONFIG["api"]["traderPassword"].as<std::string>().c_str(),  //
        XTP_PROTOCOL_TCP, CONFIG["api"]["localIP"].as<std::string>().c_str());
    if (newSessionId == 0) {
      //! 登录失败，获取错误信息
      XTPRI* errorInfo = api_->GetApiLastError();
      LOG_E("Login to trader server {}:{} failed. [{} - {}]",
            CONFIG["api"]["traderServerIP"].as<std::string>(),
            CONFIG["api"]["traderServerPort"].as<std::string>(),
            errorInfo->error_id, errorInfo->error_msg);

      //! 等待10s以后再次连接，可修改此等待时间，建议不要小于3s
      std::this_thread::sleep_for(std::chrono::seconds(
          CONFIG["api"]["secIntervalOfReconnect"].as<std::uint32_t>()));
    }
  };

  //! 获取sessionId
  const auto tdGateway =
      std::dynamic_pointer_cast<TDGatewayOfXTP>(tdSvc_->getTDGateway());
  tdGateway->setSessionId(newSessionId);

  //! 重连成功，获取交易日
  const auto tradingDay = api_->GetTradingDay();
  tdSvc_->setTradingDay(tradingDay);
  LOG_I("User {} login to {}:{} success. [tradingDay: {}; sessionId: {}]",
        CONFIG["api"]["traderUsername"].as<std::string>(),
        CONFIG["api"]["traderServerIP"].as<std::string>(),
        CONFIG["api"]["traderServerPort"].as<std::string>(), tradingDay,
        newSessionId);
}

void BQTraderSpi::OnOrderEvent(XTPOrderInfo* orderInfo, XTPRI* errorInfo,
                               uint64_t sessionId) {
  //! 根据clientId获取orderInfoFromExch->orderId_
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ = tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(
      orderInfo->order_client_id);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of order failed "
        "beause of can't find client id {} in client id info.",
        orderInfo->order_client_id);
    return;
  }

  //! 根据报单响应情况来处理
  switch (orderInfo->order_status) {
    case XTP_ORDER_STATUS_NOTRADEQUEUEING: {
      //! 订单确认状态，表示订单被交易所接受
      orderInfoFromExch->orderStatus_ = OrderStatus::ConfirmedByExch;
      strcpy(orderInfoFromExch->exchOrderId_,
             fmt::format("{}", orderInfo->order_xtp_id).c_str());
      LOG_I("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(orderInfoFromExch->orderStatus_),
            orderInfoFromExch->orderId_);
      break;
    }

    //! 全成状态在成交回报里处理，所以此状态肯定只会推送一次
    case XTP_ORDER_STATUS_ALLTRADED: {
      //! 全成状态，为了保证不乱序，所有逻辑在成交回报里处理
      LOG_T("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(OrderStatus::Filled),
            orderInfoFromExch->orderId_);
      return;
    }

    /*
     *
     * 如果委托回报里能得到已成数量和成交均价，那么在这里发送部成部撤状态，如果
     * 委托回报里得不到已成数量或者成交均价，部成部撤回报由查询订单信息功能返回，
     * 这个状态就是返回慢一点，仓位什么的由于有成交回报还是能及时计算的部成部撤
     * 状态要么在委托回报要么在查询订单线程里处理，所以此状态肯定只会推送一次在
     * 这里填上成交数量和均价，那么乱序的情况也就是这个状态先到将订单完结，成交
     * 回报后到的情形下状态机也能正常处理。
     *
     */
    case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING: {
      orderInfoFromExch->orderStatus_ = OrderStatus::PartialFilledCanceled;
      //! 防止乱序，填上exchOrderId_，防止订单在这里直接完结，ConfirmedByExch状
      //! 态找不到订单更新exchOrderId_
      strcpy(orderInfoFromExch->exchOrderId_,
             fmt::format("{}", orderInfo->order_xtp_id).c_str());
      //! 计算已成数量
      orderInfoFromExch->dealSize_ = orderInfo->qty_traded;
      //! 计算成交均价
      const auto dealAmt = orderInfo->trade_amount;
      if (!DEC::ZERO(orderInfoFromExch->dealSize_)) {
        orderInfoFromExch->avgDealPrice_ =
            dealAmt / orderInfoFromExch->dealSize_;
      } else {
        //! 理论不可能到这里，如果真的到了这里，那么不处理，等查询未完结订单过程来处理
        LOG_W("Recv invalid dealSize {} in rtn order {} of order id {}. {}",
              orderInfoFromExch->dealSize_,
              magic_enum::enum_name(orderInfoFromExch->orderStatus_),
              orderInfoFromExch->orderId_, orderInfoFromExch->toShortStr());
        return;
      }
      tdSvc_->getMapRelOfCliIdAndOrdId()->remove(orderInfo->order_client_id);
      LOG_I("Recv rtn order {} of order id {}. {}",
            magic_enum::enum_name(orderInfoFromExch->orderStatus_),
            orderInfoFromExch->orderId_, orderInfoFromExch->toShortStr());
      break;
    }

    case XTP_ORDER_STATUS_CANCELED: {
      //! 订单全部撤单状态，表示订单到达已完结状态
      orderInfoFromExch->orderStatus_ = OrderStatus::Canceled;
      //! 防止乱序，填上exchOrderId_，防止订单在这里直接完结，ConfirmedByExch状
      //! 态找不到订单更新exchOrderId_
      strcpy(orderInfoFromExch->exchOrderId_,
             fmt::format("{}", orderInfo->order_xtp_id).c_str());
      tdSvc_->getMapRelOfCliIdAndOrdId()->remove(orderInfo->order_client_id);
      LOG_I("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(orderInfoFromExch->orderStatus_),
            orderInfoFromExch->orderId_);
      break;
    }

    case XTP_ORDER_STATUS_REJECTED: {
      //! 订单拒单状态
      orderInfoFromExch->orderStatus_ = OrderStatus::Failed;

      //! 防止乱序，填上exchOrderId_，防止订单在这里直接完结，ConfirmedByExch状
      //! 态找不到订单更新exchOrderId_
      strcpy(orderInfoFromExch->exchOrderId_,
             fmt::format("{}", orderInfo->order_xtp_id).c_str());

      //! 拒单是终态，所以移除MapRelOfCliIdAndOrdId中的记录
      tdSvc_->getMapRelOfCliIdAndOrdId()->remove(orderInfo->order_client_id);

      //! 获取拒单原因
      if (errorInfo && (errorInfo->error_id != 0)) {
        //! 说明有错误导致拒单
        const auto externalStatusCode = fmt::format("{}", errorInfo->error_id);
        const auto externalStatusMsg = errorInfo->error_msg;
        orderInfoFromExch->statusCode_ =
            tdSvc_->getExternalStatusCodeCache()
                ->getAndSetStatusCodeIfNotExists(
                    tdSvc_->getApiName(), /* UserID = */ 0, externalStatusCode,
                    externalStatusMsg, -1);
        LOG_W("Recv rtn order {} of of order id {}. [{} - {}] {} - {}",
              magic_enum::enum_name(orderInfoFromExch->orderStatus_),
              orderInfoFromExch->orderId_, orderInfoFromExch->statusCode_,
              GetStatusMsg(orderInfoFromExch->statusCode_), externalStatusCode,
              externalStatusMsg);
      }
      break;
    }

    default:
      //! 其他订单状态这里不做任何处理
      LOG_W("Recv unhandled order status {}. {}", orderInfo->order_status,
            orderInfoFromExch->toShortStr());
      return;
  }

  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

//! 成交回报
void BQTraderSpi::OnTradeEvent(XTPTradeReport* tradeInfo, uint64_t sessionId) {
  //! 根据clientId获取orderInfoFromExch->orderId_
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ = tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(
      tradeInfo->order_client_id);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of trade failed "
        "beause of can't find client id {} in client id info.",
        tradeInfo->order_client_id);
    return;
  }

  //! 防止乱序，填上exchOrderId_，防止订单在这里进入全成状态直接完结，
  //! ConfirmedByExch状态找不到订单更新exchOrderId_
  strcpy(orderInfoFromExch->exchOrderId_,
         fmt::format("{}", tradeInfo->order_xtp_id).c_str());

  //! 填充成交回报必要的4个字段
  strcpy(orderInfoFromExch->lastTradeId_,
         fmt::format("{}", tradeInfo->exec_id).c_str());
  orderInfoFromExch->lastDealPrice_ = tradeInfo->price;
  orderInfoFromExch->lastDealSize_ = tradeInfo->quantity;
  orderInfoFromExch->lastDealTime_ =
      ConvertDatetimeToTs(tradeInfo->trade_time) - CHINA_TIME_ZONE_OFFSET_IN_US;

  LOG_I("Recv rtn trade. {}", orderInfoFromExch->toShortStr());

  //! 根据orderInfoFromExch更新OrdMgr中的订单
  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }

  //! 不能在委托回报里删除clientId，委托回报里删除的话，成交回报里根据clientId查
  //! 找orderId就需要到relOfCliIdAndOrdIdOfClosedGroup_里获取了，不划算
  if (orderInfoUpdated->orderStatus_ == OrderStatus::Filled) {
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(tradeInfo->order_client_id);
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

void BQTraderSpi::OnCancelOrderError(XTPOrderCancelInfo* cancelInfo,
                                     XTPRI* errorInfo, uint64_t sessionId) {
  //! 如果errorInfo不合法或者撤单没有错误，那么退出
  if (!errorInfo || (errorInfo->error_id <= 0)) {
    return;
  }

  //! 获取exchOrderId_
  const auto exchOrderId = fmt::format("{}", cancelInfo->order_xtp_id);

  //! 因为撤单错误应答没有clientId所以只能根据marketCodeGroup_和exchOrderId_
  //! 获取orderInfo
  const auto [statusCode, orderInfo] =
      tdSvc_->getOrdMgr()->getOrderInfo<LockFunc::True, DeepClone::True>(
          marketCodeGroup_, exchOrderId, QryFromClosedOrder::True);
  if (statusCode != 0 || !orderInfo) {
    LOG_W(
        "Handle cancel order ret failed because of "
        "can't find order info of exchOrderId {} in ordmgr.",
        cancelInfo->order_xtp_id);
    return;
  }

  //! 获取orderInfo->exchOrderId_
  strcpy(orderInfo->exchOrderId_, exchOrderId.c_str());

  //! 获取来自柜台的错误码和错误信息
  const auto externalStatusCode = fmt::format("{}", errorInfo->error_id);
  const auto externalStatusMsg = errorInfo->error_msg;
  orderInfo->statusCode_ =
      tdSvc_->getExternalStatusCodeCache()->getAndSetStatusCodeIfNotExists(
          tdSvc_->getApiName(), /* UserID = */ 0, externalStatusCode,
          externalStatusMsg, -1);

  //! 发送撤单错误应答
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfo);
#ifndef OPT_LOG
        LOG_I("Send cancel order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_CANCEL_ORDER_RET, orderInfo->size());

  // not sync to db
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER_RET, orderInfo,
                             SyncToRiskMgr::True, SyncToDB::False);

  LOG_W("Cancel order failed. [{} - {}] {}", externalStatusCode,
        externalStatusMsg, orderInfo->toShortStr());
}

void BQTraderSpi::OnQueryOrderEx(XTPOrderInfoEx* orderInfo, XTPRI* errorInfo,
                                 int requestId, bool isLast,
                                 uint64_t sessionId) {
  if (errorInfo && (errorInfo->error_id != 0)) {
    LOG_W("Query order info failed. [{} - {}]", errorInfo->error_id,
          errorInfo->error_msg);
    return;
  }

  //! 以下逻辑确保被同步的订单状态已经变化一段时间，
  //! 尽可能避免同步订单和回报同时触发
  const auto secAgoTheOrderCouldBeSync =
      CONFIG["secAgoTheOrderCouldBeSync"].as<int>(30);
  //! secOffOfUpdateTimeInOrd表示订单中update_time中处于该日期的第几秒
  const auto secOffOfUpdateTimeInOrd =
      orderInfo->update_time == 0
          ? 0
          : GetTotalSecFromDatetime(orderInfo->update_time);
  if (!CheckIfUpdateOrderInfo(secOffOfUpdateTimeInOrd,
                              secAgoTheOrderCouldBeSync)) {
    return;
  }

  //! 根据clientId获取orderId_
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ = tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(
      orderInfo->order_client_id);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of order failed "
        "beause of can't find client id {} in client id info.",
        orderInfo->order_client_id);
    return;
  }

  //! 获取必要字段
  strcpy(orderInfoFromExch->exchOrderId_,
         fmt::format("{}", orderInfo->order_xtp_id).c_str());
  orderInfoFromExch->dealSize_ = orderInfo->qty_traded;
  const auto dealAmt = orderInfo->trade_amount;
  if (!DEC::ZERO(orderInfoFromExch->dealSize_)) {
    orderInfoFromExch->avgDealPrice_ = dealAmt / orderInfoFromExch->dealSize_;
  }
  orderInfoFromExch->orderStatus_ = GetOrderStatus(orderInfo->order_status);
  if (orderInfoFromExch->orderStatus_ == OrderStatus::Others) {
    LOG_W(
        "Handle rsp of query order id {} failed "
        "beause of receive invalid order status {}.",
        orderInfoFromExch->orderId_, orderInfo->order_status);
    return;
  }
  LOG_D("Query order info. {}", orderInfoFromExch->toShortStr());

  //! 根据orderInfoFromExch更新OrdMgr中的订单状态
  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }
  LOG_W(
      "Order is updated by task of sync unclosed order info. "
      "\norderInfoFromExch: {}\norderInfoUpdated:  {}",
      orderInfoFromExch->toShortStr(), orderInfoUpdated->toShortStr());

  if (orderInfoUpdated->closed()) {
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(orderInfo->order_client_id);
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

void BQTraderSpi::OnQueryPosition(XTPQueryStkPositionRsp* pos, XTPRI* errorInfo,
                                  int requestId, bool isLast,
                                  uint64_t sessionId) {
  if (errorInfo && (errorInfo->error_id != 0)) {
    LOG_W("Query position info failed. [{} - {}]", errorInfo->error_id,
          errorInfo->error_msg);
    return;
  }
  LOG_I("symbolCode: {}; totalPos: {}; PrePos: {}; sellablePos: {}",
        pos->ticker, pos->total_qty, pos->yesterday_position,
        pos->sellable_qty);
}

void BQTraderSpi::OnQueryAsset(XTPQueryAssetRsp* asset, XTPRI* errorInfo,
                               int requestId, bool isLast, uint64_t sessionId) {
  if (errorInfo && (errorInfo->error_id != 0)) {
    LOG_W("Query position info failed. [{} - {}]", errorInfo->error_id,
          errorInfo->error_msg);
    return;
  }
  LOG_I("total: {}; available: {}; securityAsset: {}", asset->total_asset,
        asset->buying_power, asset->security_asset);
}

}  // namespace bq::td::svc::xtp
