/*!
 * \file BQTraderSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */

#include "BQTraderSpi.hpp"

#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDGatewayOfCTP.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfCTPUtil.hpp"
#include "TDSvcUtil.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/BQUtilOfCTP.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/Logger.hpp"
#include "util/MapRelOfCliIdAndOrdId.hpp"
#include "util/String.hpp"

using namespace std;

namespace bq::td::svc::ctp {

BQTraderSpi::BQTraderSpi(TDSvcOfCN *tdSvc, CThostFtdcTraderApi *api)
    : tdSvc_(tdSvc), api_(api) {
  appID_ = CONFIG["api"]["appID"].as<std::string>();
  authCode_ = CONFIG["api"]["authCode"].as<std::string>();
  brokerID_ = CONFIG["api"]["brokerID"].as<std::string>();
  userID_ = CONFIG["api"]["userID"].as<std::string>();
  password_ = CONFIG["api"]["password"].as<std::string>();
  frontAddr_ = CONFIG["api"]["frontAddr"].as<std::string>();

  for (std::size_t i = 0; i < CONFIG["marketCodeGroup"].size(); ++i) {
    const auto marketName = CONFIG["marketCodeGroup"][i].as<std::string>();
    const auto marketCode = bq::GetMarketCode(marketName);
    marketCodeGroup_.emplace_back(marketCode);
  }
}

BQTraderSpi::~BQTraderSpi() {}

//! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void BQTraderSpi::OnFrontConnected() {
  CThostFtdcReqAuthenticateField auth;
  memset(&auth, 0, sizeof(auth));

  strncpy(auth.AppID, appID_.c_str(), sizeof(auth.AppID) - 1);
  strncpy(auth.AuthCode, authCode_.c_str(), sizeof(auth.AuthCode) - 1);
  strncpy(auth.BrokerID, brokerID_.c_str(), sizeof(auth.BrokerID) - 1);
  strncpy(auth.UserID, userID_.c_str(), sizeof(auth.UserID) - 1);

  int ret = api_->ReqAuthenticate(&auth, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} try to authenticate failed. [ret = {}]",
          userID_, brokerID_, ret);
    return;
  }
}

//! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动
//! 重新连接，客户端可不做处理。
void BQTraderSpi::OnFrontDisconnected(int nReason) {
  LOG_W("Front disconnected. [{} - {}]", nReason,
        GetFrontDisConnectErrorMsg(nReason));
}

//! 心跳超时警告。当长时间未收到报文时，该方法被调用。
void BQTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
  LOG_W("Heartbeat delay warning. [timeElapse: {}]", nTimeLapse);
}

//! 客户端认证响应
void BQTraderSpi::OnRspAuthenticate(
    CThostFtdcRspAuthenticateField *pRspAuthenticateField,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("User {} of broker {} authenticate to {} failed. [{} - {}]", userID_,
          brokerID_, frontAddr_, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  CThostFtdcReqUserLoginField loginReq;
  memset(&loginReq, 0, sizeof(loginReq));

  strncpy(loginReq.BrokerID, brokerID_.c_str(), sizeof(loginReq.BrokerID) - 1);
  strncpy(loginReq.UserID, userID_.c_str(), sizeof(loginReq.UserID) - 1);
  strncpy(loginReq.Password, password_.c_str(), sizeof(loginReq.Password) - 1);

  int ret = api_->ReqUserLogin(&loginReq, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} try to login {} failed. [ret = {}]", userID_,
          brokerID_, frontAddr_, ret);
    return;
  }
}

//! 登录请求响应
void BQTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                 CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("User {} of broker {} login to {} failed. [{} - {}]",
          pRspUserLogin->UserID, pRspUserLogin->BrokerID, frontAddr_,
          pRspInfo->ErrorID, CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  if (!pRspUserLogin->TradingDay || strlen(pRspUserLogin->TradingDay) == 0) {
    LOG_E(
        "User {} of broker {} login to {} failed "
        "because of invalid trading day.",
        pRspUserLogin->UserID, pRspUserLogin->BrokerID, frontAddr_);
    return;
  }

  const auto tradingDay = api_->GetTradingDay();
  tdSvc_->setTradingDay(pRspUserLogin->TradingDay);
  LOG_I("User {} of broker {} login to {} success. [tradingDay: {}]",
        pRspUserLogin->UserID, pRspUserLogin->BrokerID, frontAddr_,
        pRspUserLogin->TradingDay);

  const auto tdGateway =
      std::dynamic_pointer_cast<TDGatewayOfCTP>(tdSvc_->getTDGateway());
  tdGateway->setSessionId(pRspUserLogin->FrontID);
  tdGateway->setFrontId(pRspUserLogin->SessionID);

  CThostFtdcSettlementInfoConfirmField sConfirm = {};
  const auto ret = api_->ReqSettlementInfoConfirm(&sConfirm, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} try confirm settlement failed. [ret = {}]",
          userID_, brokerID_, ret);
    return;
  }
}

void BQTraderSpi::OnRspSettlementInfoConfirm(
    CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("User {} of broker {} confirm settlement from {} failed. [{} - {}]",
          userID_, brokerID_, frontAddr_, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }
}

//! 登出请求响应
void BQTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {}

//! 错误应答
void BQTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("Recv rsp of error from {}. [{} - {}]", frontAddr_, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 报单录入请求响应
void BQTraderSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                                   CThostFtdcRspInfoField *pRspInfo,
                                   int nRequestID, bool bIsLast) {
  //! 如果报单录入没有错误，那么直接返回
  if (!pRspInfo || pRspInfo->ErrorID == 0) {
    return;
  }

  ClientId clientId = 0;
  if (pInputOrder && pInputOrder->OrderRef[0] != '\0') {
    clientId = CONV(ClientId, pInputOrder->OrderRef);
  } else {
    LOG_W(
        "Handle rsp of order failed "
        "beause pInputOrder is null or OrderRef in pInputOrder is empty.");
    return;
  }

  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ =
      tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(clientId);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rsp of order failed "
        "beause of can't find client id {} in client id info.",
        clientId);
    return;
  }
  tdSvc_->getMapRelOfCliIdAndOrdId()->remove(clientId);

  //! 订单拒单状态
  orderInfoFromExch->orderStatus_ = OrderStatus::Failed;

  //! 获取拒单原因
  const auto externalStatusCode = fmt::format("{}", pRspInfo->ErrorID);
  const auto externalStatusMsg =
      CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8");
  orderInfoFromExch->statusCode_ =
      tdSvc_->getExternalStatusCodeCache()->getAndSetStatusCodeIfNotExists(
          tdSvc_->getApiName(), 0, externalStatusCode, externalStatusMsg, -1);
  LOG_W("Order failed. [{} - {}] {}", externalStatusCode, externalStatusMsg,
        orderInfoFromExch->toShortStr());

  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void *shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo *>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

//! 报单录入错误回报
void BQTraderSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                                      CThostFtdcRspInfoField *pRspInfo) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("Order failed. [{} - {}]", pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 报单通知
void BQTraderSpi::OnRtnOrder(CThostFtdcOrderField *pInputOrder) {
  ClientId clientId = 0;
  if (pInputOrder && pInputOrder->OrderRef[0] != '\0') {
    clientId = CONV(ClientId, pInputOrder->OrderRef);
  } else {
    LOG_W(
        "Handle rtn of order failed "
        "beause pInputOrder is null or OrderRef in pInputOrder is empty.");
    return;
  }

  //! 因为exchOrderId_多个交易所会重复，回报中ExchangeID不一定有值，所以下面根
  //! 据orderId来更新订单
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ =
      tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(clientId);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of order failed "
        "beause of can't find client id {} in client id info.",
        clientId);
    return;
  }

  //! 根据报单响应情况来处理
  switch (pInputOrder->OrderStatus) {
    case THOST_FTDC_OST_NoTradeQueueing: {
      //! 订单确认状态，表示订单被交易所接受
      orderInfoFromExch->orderStatus_ = OrderStatus::ConfirmedByExch;
      strcpy(orderInfoFromExch->exchOrderId_, pInputOrder->OrderSysID);
      LOG_I("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(orderInfoFromExch->orderStatus_),
            orderInfoFromExch->orderId_);
      break;
    }

    //! 全成状态在成交回报里处理，所以此状态肯定只会推送一次
    case THOST_FTDC_OST_AllTraded: {
      //! 全成状态，为了保证不乱序，所有逻辑在成交回报里处理
      LOG_T("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(OrderStatus::Filled),
            orderInfoFromExch->orderId_);
      return;
    }

    //! 如果委托回报里能得到已成数量和成交均价，那么在这里发送部成部撤状态，如果
    //! 委托回报里得不到已成数量或者成交均价，部成部撤回报由查询订单信息功能返回，
    //! 这个状态就是返回慢一点，仓位什么的由于有成交回报还是能及时计算的
    case THOST_FTDC_OST_PartTradedNotQueueing: {
      //! 部成部撤状态在委托回报或查询订单线程处理，所以此状态也肯定只会推送一次
      LOG_T("Recv rtn order {} of order id {}. ",
            magic_enum::enum_name(OrderStatus::PartialFilledCanceled),
            orderInfoFromExch->orderId_);
      break;
    }

    case THOST_FTDC_OST_Canceled: {
      if (pInputOrder->OrderSubmitStatus == THOST_FTDC_OSS_InsertRejected) {
        //! 订单已被拒绝状态，表示订单到达已完结状态
        orderInfoFromExch->orderStatus_ = OrderStatus::Failed;
        const auto externalStatusMsg =
            CodeConvert(pInputOrder->StatusMsg, "gb2312", "utf8");
        const auto hashOfExternalStatusMsg =
            XXH3_64bits(externalStatusMsg.c_str(), externalStatusMsg.size());
        const auto externalStatusCode =
            fmt::format("{}", hashOfExternalStatusMsg);
        orderInfoFromExch->statusCode_ =
            tdSvc_->getExternalStatusCodeCache()
                ->getAndSetStatusCodeIfNotExists(tdSvc_->getApiName(), 0,
                                                 externalStatusCode,
                                                 externalStatusMsg, -1);
        LOG_W("Recv rtn order {} of order id {}. {}",
              magic_enum::enum_name(orderInfoFromExch->orderStatus_),
              orderInfoFromExch->orderId_, externalStatusMsg);
      } else {
        //! 订单全部撤单状态，表示订单到达已完结状态
        orderInfoFromExch->orderStatus_ = OrderStatus::Canceled;
        LOG_I("Recv rtn order {} of order id {}. ",
              magic_enum::enum_name(orderInfoFromExch->orderStatus_),
              orderInfoFromExch->orderId_);
      }
      //! 防止乱序，填上exchOrderId_，防止订单在这里直接完结，ConfirmedByExch状
      //! 态找不到订单更新exchOrderId_
      strcpy(orderInfoFromExch->exchOrderId_, pInputOrder->OrderSysID);
      tdSvc_->getMapRelOfCliIdAndOrdId()->remove(clientId);
      break;
    }

    default:
      //! 其他订单状态这里不做任何处理
      LOG_I("Recv unhandled order status {}. {}", pInputOrder->OrderStatus,
            orderInfoFromExch->toShortStr());
      return;
  }

  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void *shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo *>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

//! 成交通知
void BQTraderSpi::OnRtnTrade(CThostFtdcTradeField *pTrade) {
  ClientId clientId = 0;
  if (pTrade && pTrade->OrderRef[0] != '\0') {
    clientId = CONV(ClientId, pTrade->OrderRef);
  } else {
    LOG_W(
        "Handle rtn of trade failed "
        "beause pTrade is null or OrderRef in pTrade is empty.");
    return;
  }

  //! 因为exchOrderId_多个交易所会重复，回报中ExchangeID不一定有值，所以下面根
  //! 据orderId来更新订单
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ =
      tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(clientId);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of trade failed "
        "beause of can't find client id {} in client id info,"
        "PartialFilledCanceled status may be arrive first.",
        clientId);
    return;
  }

  //! 防止乱序，填上exchOrderId_，防止订单在这里进入全成状态直接完结，
  //! ConfirmedByExch状态找不到订单更新exchOrderId_
  strcpy(orderInfoFromExch->exchOrderId_, pTrade->OrderSysID);
  strcpy(orderInfoFromExch->lastTradeId_, pTrade->TradeID);
  orderInfoFromExch->lastDealPrice_ = pTrade->Price;
  orderInfoFromExch->lastDealSize_ = pTrade->Volume;
  orderInfoFromExch->lastDealTime_ =
      CalcTs(pTrade->TradeDate, pTrade->TradeTime, 0) -
      CHINA_TIME_ZONE_OFFSET_IN_US;

  LOG_I("Recv rtn trade. {}", orderInfoFromExch->toShortStr());

  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }

  //! 不能在委托回报里删除clientId，委托回报里删除的话，成交回报里根据clientId查
  //! 找orderId就需要到relOfCliIdAndOrdIdOfClosedGroup_里获取了，不划算
  if (orderInfoUpdated->orderStatus_ == OrderStatus::Filled) {
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(clientId);
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void *shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo *>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

//! 撤单操作请求响应
void BQTraderSpi::OnRspOrderAction(
    CThostFtdcInputOrderActionField *pInputOrderAction,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("Order cancel failed. [{} - {}]", pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 撤单操作错误回报
void BQTraderSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
                                      CThostFtdcRspInfoField *pRspInfo) {
  //! 如果撤单操作没有错误，那么直接退出
  if (!pRspInfo || pRspInfo->ErrorID == 0) {
    return;
  }

  //! 如果撤单操作出错，那么返回撤单失败信息
  const auto exchOrderId = pOrderAction->OrderSysID;
  const auto [statusCode, orderInfo] =
      tdSvc_->getOrdMgr()->getOrderInfo<LockFunc::True, DeepClone::True>(
          marketCodeGroup_, exchOrderId, QryFromClosedOrder::True);
  if (statusCode != 0) {
    LOG_W(
        "Handle cancel order ret failed because of "
        "can't find exch order id {} in ordmgr.",
        exchOrderId);
    return;
  }

  const auto externalStatusCode = fmt::format("{}", pRspInfo->ErrorID);
  const auto externalStatusMsg =
      CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8");
  orderInfo->statusCode_ =
      tdSvc_->getExternalStatusCodeCache()->getAndSetStatusCodeIfNotExists(
          tdSvc_->getApiName(), 0, externalStatusCode, externalStatusMsg, -1);

  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void *shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfo);
#ifndef OPT_LOG
        LOG_I("Send cancel order ret. {}",
              static_cast<OrderInfo *>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_CANCEL_ORDER_RET, orderInfo->size());

  // not sync to db
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER_RET, orderInfo,
                             SyncToRiskMgr::True, SyncToDB::False);

  LOG_W("Cancel order failed. [{} - {}] {}", externalStatusCode,
        externalStatusMsg, orderInfo->toShortStr());
}

//! 请求查询报单响应
void BQTraderSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("User {} of broker {} query order from {} failed. [{} - {}]", userID_,
          brokerID_, frontAddr_, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  if (!pOrder) {
    LOG_W(
        "User {} of broker {} query order from {} failed "
        "because of result is null.",
        userID_, brokerID_, frontAddr_);
    return;
  }

  //! 根据clientId获取orderId_
  const auto clientId = CONV(ClientId, pOrder->OrderRef);
  auto orderInfoFromExch = std::make_shared<OrderInfo>();
  orderInfoFromExch->orderId_ =
      tdSvc_->getMapRelOfCliIdAndOrdId()->getOrderId(clientId);
  if (orderInfoFromExch->orderId_ == 0) {
    LOG_W(
        "Handle rtn of order failed "
        "beause of can't find client id {} in client id info.",
        clientId);
    return;
  }

  //! 获取必要字段
  strcpy(orderInfoFromExch->exchOrderId_, pOrder->OrderSysID);
  orderInfoFromExch->dealSize_ = pOrder->VolumeTraded;
  orderInfoFromExch->avgDealPrice_ = pOrder->LimitPrice;
  orderInfoFromExch->orderStatus_ = GetOrderStatus(pOrder->OrderStatus);
  if (orderInfoFromExch->orderStatus_ == OrderStatus::Others) {
    LOG_W(
        "Handle rsp of query order id {} failed "
        "beause of receive invalid order status {}.",
        orderInfoFromExch->orderId_, pOrder->OrderStatus);
    return;
  }
  LOG_I("Query order info. {}", orderInfoFromExch->toShortStr());

  const auto secAgoTheOrderCouldBeSync =
      CONFIG["secAgoTheOrderCouldBeSync"].as<std::uint64_t>();
  if (!CheckIfUpdateOrderInfo(tdSvc_, orderInfoFromExch,
                              secAgoTheOrderCouldBeSync)) {
    return;
  }

  //! 根据orderInfoFromExch更新OrdMgr中的订单状态
  const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
      tdSvc_->getOrdMgr()
          ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
              orderInfoFromExch, tdSvc_->getNextNoUsedToCalcPos(),
              tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());
  if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
    return;
  }
  LOG_W(
      "Order is updated by task of sync unclosed order info. "
      "\norderInfoFromExch: {}\norderInfoUpdated: {}",
      orderInfoFromExch->toShortStr(), orderInfoUpdated->toShortStr());

  if (orderInfoUpdated->closed()) {
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(clientId);
  }

  //! 发送回报
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void *shmBuf) {
        InitMsgBodyExt(shmBuf, *orderInfoUpdated);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo *>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

  //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                             SyncToRiskMgr::True, SyncToDB::True);
}

void BQTraderSpi::OnRspQryInvestorPosition(
    CThostFtdcInvestorPositionField *pInvestorPosition,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("User {} of broker {} query position from {} failed. [{} - {}]",
          userID_, brokerID_, frontAddr_, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  if (!pInvestorPosition) {
    LOG_W(
        "User {} of broker {} query position from {} failed "
        "because of result is null.",
        userID_, brokerID_, frontAddr_);
    return;
  }

  LOG_I("{} - {} YdPosition: {}; Position: {}; ExchangeMargin: {}",
        GetMarketName(GetMarketCode(pInvestorPosition->ExchangeID)),
        pInvestorPosition->InstrumentID, pInvestorPosition->YdPosition,
        pInvestorPosition->Position, pInvestorPosition->ExchangeMargin);
}

}  // namespace bq::td::svc::ctp
