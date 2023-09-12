/*!
 * \file TDSvcOfXTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDGatewayOfXTP.hpp"

#include "BQTraderSpi.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfXTPUtil.hpp"
#include "def/OrderInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/Logger.hpp"
#include "util/MapRelOfCliIdAndOrdId.hpp"

namespace bq::td::svc::xtp {

int TDGatewayOfXTP::doInit() {
  XTP_LOG_LEVEL logLevel = XTP_LOG_LEVEL_INFO;
  api_ = TraderApi::CreateTraderApi(
      CONFIG["api"]["clientId"].as<std::uint8_t>(),
      CONFIG["api"]["loggerPath"].as<std::string>().c_str(), logLevel);

  //! 设定与交易服务器交互的超时时间，设定与交易服务器交互的超时时间单位为秒，
  //! 默认是15s，调试时可以设定大点
  api_->SetHeartBeatInterval(
      CONFIG["api"]["secIntervalOfHeartBeat"].as<std::uint32_t>());

  //! 设定公共流传输方式
  api_->SubscribePublicTopic(XTP_TERT_QUICK);

  //! 设定用户的开发代码，在XTP申请开户时，由xtp运营人员提供
  api_->SetSoftwareKey(CONFIG["api"]["softwareKey"].as<std::string>().c_str());

  //! 设定软件版本号，用户可视自身需要在户自定义（仅可使用如下字符0-9，a-z，A-Z，.）
  api_->SetSoftwareVersion("1.0.0");

  //! 创建Spi类实例
  auto spi = new BQTraderSpi(tdSvc_, api_);
  if (!spi) {
    LOG_W("Create spi failed.");
    return -1;
  }

  //! 注册Spi
  api_->RegisterSpi(spi);

  //! 登录交易服务器
  sessionId_ = api_->Login(
      CONFIG["api"]["traderServerIP"].as<std::string>().c_str(),
      CONFIG["api"]["traderServerPort"].as<std::uint32_t>(),
      CONFIG["api"]["traderUsername"].as<std::string>().c_str(),
      CONFIG["api"]["traderPassword"].as<std::string>().c_str(),  //
      XTP_PROTOCOL_TCP, CONFIG["api"]["localIP"].as<std::string>().c_str());
  if (0 == sessionId_) {
    //! 登录失败，获取失败原因
    XTPRI* errorInfo = api_->GetApiLastError();
    LOG_E("Login to trader server {}:{} failed. [{} - {}]",
          CONFIG["api"]["traderServerIP"].as<std::string>(),
          CONFIG["api"]["traderServerPort"].as<std::string>(),
          errorInfo->error_id, errorInfo->error_msg);
    return -1;
  }

  //! 获取交易日
  const auto tradingDay = api_->GetTradingDay();
  tdSvc_->setTradingDay(tradingDay);
  LOG_I("User {} login to {}:{} success. [tradingDay: {}; sessionId: {}]",
        CONFIG["api"]["traderUsername"].as<std::string>(),
        CONFIG["api"]["traderServerIP"].as<std::string>(),
        CONFIG["api"]["traderServerPort"].as<std::string>(), tradingDay,
        sessionId_);

  return 0;
}

std::tuple<int, std::string> TDGatewayOfXTP::doOrder(
    const OrderInfoSPtr& orderInfo) {
  //! 生成订单
  XTPOrderInsertInfo order;
  memset(&order, 0, sizeof(XTPOrderInsertInfo));

  order.order_client_id = tdSvc_->getMapRelOfCliIdAndOrdId()->genClientId();
  tdSvc_->getMapRelOfCliIdAndOrdId()->add(order.order_client_id,
                                          orderInfo->orderId_);
  strncpy(order.ticker, orderInfo->exchSymbolCode_, XTP_TICKER_LEN - 1);
  order.market = GetExchMarketCode(orderInfo->marketCode_);
  order.price = orderInfo->orderPrice_;
  order.quantity = orderInfo->orderSize_;
  order.price_type = XTP_PRICE_LIMIT;
  order.side = GetExchSide(orderInfo->side_);
  order.position_effect = XTP_POSITION_EFFECT_INIT;
  order.business_type = XTP_BUSINESS_TYPE_CASH;

  //! 发送订单
  const auto exchOrderId = api_->InsertOrder(&order, sessionId_);
  if (exchOrderId == 0) {
    //! 报单失败，-1使得订单完结
    const auto errorInfo = api_->GetApiLastError();
    const auto externalStatusCode = fmt::format("{}", errorInfo->error_id);
    const auto externalStatusMsg = errorInfo->error_msg;
    tdSvc_->getMapRelOfCliIdAndOrdId()->remove(order.order_client_id);
    LOG_W("Insert order {} by api failed. {} - {}", orderInfo->orderId_,
          externalStatusCode, externalStatusMsg);
    return {SCODE_TD_SVC_ORDER_FAILED_BY_API, fmt::format("{}", exchOrderId)};
  }

  //! 报单请求发送成功
  LOG_I("Send req of insert order {} of clientId {} success. ",
        orderInfo->orderId_, order.order_client_id);
  return {0, fmt::format("{}", exchOrderId)};
}

int TDGatewayOfXTP::doCancelOrder(const OrderInfoSPtr& orderInfo) {
  //! 因为撤单要根据exchOrderId_来进行，所以还没有得到exchOrderId_之前返回撤单失败
  if (orderInfo->exchOrderId_[0] == '\0') {
    LOG_W("Cancel order {} failed because exch order id is empty.",
          orderInfo->orderId_);
    orderInfo->statusCode_ = SCODE_TD_SVC_EXCH_ORDER_ID_IS_EMPTY;
    return orderInfo->statusCode_;
  }

  //! 根据exchOrderId_撤单
  const auto exchOrderId = CONV(std::uint64_t, orderInfo->exchOrderId_);
  const auto exchCancelOrderId = api_->CancelOrder(exchOrderId, sessionId_);
  if (exchCancelOrderId == 0) {
    //! 撤单失败，获取失败原因
    const auto errorInfo = api_->GetApiLastError();
    const auto externalStatusCode = fmt::format("{}", errorInfo->error_id);
    const auto externalStatusMsg = errorInfo->error_msg;
    orderInfo->statusCode_ = SCODE_TD_SVC_ORDER_CANCEL_FAILED_BY_API;
    LOG_W("Cancel order {} by api failed. {} - {}", orderInfo->orderId_,
          externalStatusCode, externalStatusMsg);
    return orderInfo->statusCode_;
  }

  //! 撤单请求发送成功
  LOG_I("Send req of cancel order {} success. ", orderInfo->orderId_);
  return 0;
}

void TDGatewayOfXTP::doSyncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto orderInfoInOrdMgr = std::any_cast<OrderInfoSPtr>(asyncTask->arg_);
  //! exchOrderId_为0的订单根本就没有被发往交易所，直接按照废单处理
  if (orderInfoInOrdMgr->exchOrderId_[0] == '\0') {
    orderInfoInOrdMgr->orderStatus_ = OrderStatus::Failed;
    orderInfoInOrdMgr->statusCode_ = SCODE_TD_SVC_ORDER_NOT_SENT_TO_RMT_SRV;

    //! 根据orderInfoInOrdMgr更新OrdMgr中的订单状态
    const auto [isTheOrderInfoUpdated, orderInfoUpdated] =
        tdSvc_->getOrdMgr()
            ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
                orderInfoInOrdMgr, tdSvc_->getNextNoUsedToCalcPos(),
                tdSvc_->getFeeInfoCache());
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

  //! 获取exchOrderId，根据exchOrderId查询订单情况
  const auto exchOrderId = CONV(std::uint64_t, orderInfoInOrdMgr->exchOrderId_);
  const auto ret =
      api_->QueryOrderByXTPIDEx(exchOrderId, sessionId_, ++requestId_);
  if (ret != 0) {
    const auto errorInfo = api_->GetApiLastError();
    const auto externalStatusCode = errorInfo->error_id;
    const auto externalStatusMsg = errorInfo->error_msg;
    LOG_W("Sync unclosed order info by api failed. {} - {} {}",
          externalStatusCode, externalStatusMsg,
          orderInfoInOrdMgr->toShortStr());
    return;
  }
  LOG_I("Send req of query unclosed order info success. {} ",
        orderInfoInOrdMgr->toShortStr());
  return;
}

void TDGatewayOfXTP::doSyncAssetsSnapshot() {
  if (const auto ret = api_->QueryPosition(nullptr, sessionId_, ++requestId_);
      ret != 0) {
    const auto errorInfo = api_->GetApiLastError();
    const auto externalStatusCode = errorInfo->error_id;
    const auto externalStatusMsg = errorInfo->error_msg;
    LOG_W("Query position info by api failed. {} - {}", externalStatusCode,
          externalStatusMsg);
    return;
  }
  LOG_I("Send req of query position info success. ");

  if (const auto ret = api_->QueryAsset(sessionId_, ++requestId_); ret != 0) {
    const auto errorInfo = api_->GetApiLastError();
    const auto externalStatusCode = errorInfo->error_id;
    const auto externalStatusMsg = errorInfo->error_msg;
    LOG_W("Query position info by api failed. {} - {}", externalStatusCode,
          externalStatusMsg);
    return;
  }
  LOG_I("Send req of query assets info success. ");

  return;
}

void TDGatewayOfXTP::doTestOrder() {}
void TDGatewayOfXTP::doTestCancelOrder() {}

}  // namespace bq::td::svc::xtp
