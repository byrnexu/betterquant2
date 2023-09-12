/*!
 * \file TDSvcOfCTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDGatewayOfCTP.hpp"

#include "BQTraderSpi.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfCTPUtil.hpp"
#include "def/OrderInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/Logger.hpp"
#include "util/MapRelOfCliIdAndOrdId.hpp"

namespace bq::td::svc::ctp {

int TDGatewayOfCTP::doInit() {
  brokerId_ = CONFIG["api"]["brokerID"].as<std::string>();
  userId_ = CONFIG["api"]["userID"].as<std::string>();

  const auto flowPath = CONFIG["api"]["flowPath"].as<std::string>();
  if (!boost::filesystem::exists(flowPath)) {
    boost::filesystem::create_directories(flowPath);
  }
  api_ = CThostFtdcTraderApi::CreateFtdcTraderApi(flowPath.c_str());

  const auto spi = new BQTraderSpi(tdSvc_, api_);
  api_->RegisterSpi(spi);

  api_->RegisterFront(
      const_cast<char*>(CONFIG["api"]["frontAddr"].as<std::string>().c_str()));
  api_->SubscribePrivateTopic(THOST_TERT_QUICK);
  api_->SubscribePublicTopic(THOST_TERT_QUICK);
  api_->Init();

  return 0;
}

std::tuple<int, std::string> TDGatewayOfCTP::doOrder(
    const OrderInfoSPtr& orderInfo) {
  //! 创建下单请求
  CThostFtdcInputOrderField order = {};
  memset(&order, 0, sizeof(CThostFtdcInputOrderField));

  //! 生成clientId
  const auto clientId = tdSvc_->getMapRelOfCliIdAndOrdId()->genClientId();
  strcpy(order.OrderRef, fmt::format("{}", clientId).c_str());
  tdSvc_->getMapRelOfCliIdAndOrdId()->add(clientId, orderInfo->orderId_);

  strcpy(order.BrokerID, brokerId_.c_str());
  strcpy(order.InvestorID, userId_.c_str());
  strcpy(order.InstrumentID, orderInfo->exchSymbolCode_);

  order.OrderPriceType = THOST_FTDC_OPT_LimitPrice;

  int statusCode = 0;
  std::tie(statusCode, order.Direction, order.CombOffsetFlag[0]) =
      GetExchSideInfo(orderInfo);

  order.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;  // 投机

  order.LimitPrice = orderInfo->orderPrice_;
  order.VolumeTotalOriginal = orderInfo->orderSize_;

  order.TimeCondition = THOST_FTDC_TC_GFD;                // 当日有效
  order.VolumeCondition = THOST_FTDC_VC_AV;               // 任何数量
  order.MinVolume = 1;                                    // 最小成交数量
  order.ContingentCondition = THOST_FTDC_CC_Immediately;  // 触发条件：立即
  order.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;  // 强平原因: 非强平

  //! 发送下单请求
  const auto ret = api_->ReqOrderInsert(&order, ++requestId_);
  if (ret != 0) {
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(clientId);
    LOG_W("Insert order {} by api failed. [ret = {}]", orderInfo->orderId_,
          ret);
    return {SCODE_TD_SVC_ORDER_FAILED_BY_API, ""};
  }

  //! 下单请求发送成功
  LOG_I("Send req of insert order {} of clientId {} success. ",
        orderInfo->orderId_, order.OrderRef);
  return {0, ""};
}

int TDGatewayOfCTP::doCancelOrder(const OrderInfoSPtr& orderInfo) {
  //! 订单尚未得到交易所的确认，返回撤单失败
  if (orderInfo->exchOrderId_[0] == '\0') {
    LOG_W("Cancel order {} failed because exch order id is empty.",
          orderInfo->orderId_);
    orderInfo->statusCode_ = SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY;
    return orderInfo->statusCode_;
  }

  //! 创建撤单请求
  CThostFtdcInputOrderActionField req = {};
  const auto exchange = GetExchange(orderInfo->marketCode_);
  strcpy(req.BrokerID, brokerId_.c_str());
  strcpy(req.InvestorID, userId_.c_str());
  strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID) - 1);
  strncpy(req.OrderSysID, orderInfo->exchOrderId_, sizeof(req.OrderSysID) - 1);
  req.ActionFlag = THOST_FTDC_AF_Delete;

  //! 发送撤单请求
  const auto ret = api_->ReqOrderAction(&req, ++requestId_);
  if (ret != 0) {
    //! 撤单请求失败，获取失败原因
    orderInfo->statusCode_ = SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API;
    LOG_W("Cancel order {} by api failed. [ret = {}]", orderInfo->orderId_,
          ret);
    return orderInfo->statusCode_;
  }

  //! 撤单请求发送成功
  LOG_I("Send req of cancel order {} success. ", orderInfo->orderId_);
  return 0;
}

void TDGatewayOfCTP::doSyncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) {
  OrderInfoSPtr orderInfoInOrdMgr = nullptr;
  if (asyncTask->arg_.has_value()) {
    orderInfoInOrdMgr = std::any_cast<OrderInfoSPtr>(asyncTask->arg_);
  }

  if (orderInfoInOrdMgr) {
    //! exchOrderId_为0的订单根本就没有被发往交易所，直接按照废单处理
    if (orderInfoInOrdMgr->exchOrderId_[0] == '\0') {
      orderInfoInOrdMgr->orderStatus_ = OrderStatus::Failed;
      orderInfoInOrdMgr->statusCode_ = SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV;

      const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
          tdSvc_->getOrdMgr()
              ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
                  orderInfoInOrdMgr, tdSvc_->getNextNoUsedToCalcPos(),
                  tdSvc_->getFeeInfoCache(), tdSvc_->getOpenedContractGroup());
      if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
        return;
      }

      //! 发送回报
      tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
          [&](void* shmBuf) {
            InitMsgBodyExt(shmBuf, *orderInfoUpdated);
            LOG_I("Send order ret. {}",
                  static_cast<OrderInfo*>(shmBuf)->toShortStr());
          },
          MSG_ID_ON_ORDER_RET, orderInfoUpdated->size());

      //! 这里入库的订单都经过updateByOrderInfoFromExch的检查，都是相对合法的订单
      tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoUpdated,
                                 SyncToRiskMgr::True, SyncToDB::True);
      return;
    }
  }

  //! 生成包括交易所和交易所订单号的订单查询请求
  CThostFtdcQryOrderField req = {};
  strcpy(req.BrokerID, brokerId_.c_str());
  strcpy(req.InvestorID, userId_.c_str());
  if (orderInfoInOrdMgr) {
    const auto exchange = GetExchange(orderInfoInOrdMgr->marketCode_);
    strncpy(req.ExchangeID, exchange.c_str(), sizeof(req.ExchangeID) - 1);
    strncpy(req.OrderSysID, orderInfoInOrdMgr->exchOrderId_,
            sizeof(req.OrderSysID) - 1);
  }

  //! 发送订单查询请求
  const auto ret = api_->ReqQryOrder(&req, ++requestId_);
  const auto orderId = orderInfoInOrdMgr != nullptr
                           ? fmt::format(" {}", orderInfoInOrdMgr->orderId_)
                           : "";
  if (ret == 0) {
    LOG_I("Send req of query order {} by api success. ", orderId);
  } else {
    LOG_W("Send req Of query order {} by api failed. [ret = {}]", orderId, ret);
    return;
  }

  return;
}

void TDGatewayOfCTP::doSyncAssetsSnapshot() {
  CThostFtdcQryInvestorPositionField req = {};
  const auto ret = api_->ReqQryInvestorPosition(&req, ++requestId_);
  if (ret != 0) {
    LOG_W("Query assets info by api failed. [ret = {}]", ret);
    return;
  }
  LOG_I("Send req of query assets info success. ");
  return;
}

void TDGatewayOfCTP::doTestOrder() {}
void TDGatewayOfCTP::doTestCancelOrder() {}

}  // namespace bq::td::svc::ctp
