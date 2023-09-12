/*!
 * \file TDSrvRiskPluginSelfTradeCtrl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginSelfTradeCtrl.hpp"

#include "AssetsMgr.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "RiskCtrlUtil.hpp"
#include "SelfTradeCtrlRangeMgr.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginConst.hpp"
#include "TDSrvRiskPluginUtil.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

boost::dll::fs::path TDSrvRiskPluginSelfTradeCtrl::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginSelfTradeCtrl::TDSrvRiskPluginSelfTradeCtrl(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {}

int TDSrvRiskPluginSelfTradeCtrl::doLoad() {
  maxSelfTradeTriggerTimesAllowed_ =
      node_["maxSelfTradeTriggerTimesAllowed"].as<std::uint32_t>();
  return 0;
}

int TDSrvRiskPluginSelfTradeCtrl::doOnRiskCtrlConfChg(const std::string& step,
                                                      const Doc& doc,
                                                      const std::string& conf,
                                                      std::uint32_t combNo,
                                                      std::uint32_t threadNo) {
  auto& selfTradeCtrlRangeMgr = getTDSrv()
                                    ->getRiskCtrlModuleComb()[combNo]
                                    ->getSelfTradeCtrlRangeMgrGroup()[threadNo];
  const auto [statusCode, statusMsg] = selfTradeCtrlRangeMgr->update(doc);
  if (statusCode != 0) {
    L_W(logger(), statusMsg);
    return statusCode;
  }

  L_I(logger(), "Step of {} recv {}", step, conf);
  return 0;
}

std::tuple<int, std::string> TDSrvRiskPluginSelfTradeCtrl::doOnOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  if (order->marketCode_ != MarketCode::SHFE &&
      order->marketCode_ != MarketCode::CZCE &&
      order->marketCode_ != MarketCode::DCE &&
      order->marketCode_ != MarketCode::CFFEX &&
      order->marketCode_ != MarketCode::INE) {
    return {0, ""};
  }

  const auto& selfTradeCtrlRangeMgr =
      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getSelfTradeCtrlRangeMgrGroup()[threadNo];

  //! selfTradeCtrlRange = acctId=10000&trdAcctId=100000
  const auto [inTheSelfTradeCtrlList, selfTradeCtrlRange] =
      selfTradeCtrlRangeMgr->inTheSelfTradeCtrlList(order);
  if (!inTheSelfTradeCtrlList) {
    return {0, ""};
  }

  findOrCtorPendingOrderGroup(combNo, threadNo);

  const auto sym = fmt::format(
      "{}-{}-{}-{}", selfTradeCtrlRange, GetMarketName(order->marketCode_),
      magic_enum::enum_name(order->symbolType_), order->symbolCode_);

  //! 如果没有挂单价格信息
  auto& idx = pendingOrderGroup_->get<TagHashOfSym>();
  const auto hashOfSym = XXH3_64bits(sym.data(), sym.size());
  const auto iter = idx.find(hashOfSym);
  if (iter == std::end(idx)) {
    cachePendingOrders(order, combNo, threadNo, sym, hashOfSym);
    return {0, ""};
  }

  //! 如果已有挂单价格信息
  const auto& pendingOrder = *iter;
  const auto [statusCode, details] =
      handleSymHasPendingOrders(order, combNo, threadNo, sym, pendingOrder);
  if (statusCode != 0) {
    return {statusCode, details};
  }

  return {0, ""};
}

void TDSrvRiskPluginSelfTradeCtrl::cachePendingOrders(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo,
    const std::string& sym, std::uint64_t hashOfSym) {
  auto& segment =
      getTDSrv()->getRiskCtrlModuleComb()[combNo]->getSegmentGroup()[threadNo];

  switch (order->side_) {
    case Side::Bid: {
      auto pendingOrder = bip::make_managed_shared_ptr(
          segment->construct<PendingOrder>(bip::anonymous_instance)(),
          *segment);

      pendingOrder->hashOfSym_ = hashOfSym;
      pendingOrder->maxBidPrice_ = order->orderPrice_;

      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash(
              [pendingOrderGroup = this->pendingOrderGroup_, pendingOrder]() {
                pendingOrderGroup->emplace(pendingOrder);
              });

      L_I(logger(), "[{}] Create pending order price info: {} {}", name(), sym,
          pendingOrder->toStr());

    } break;

    case Side::Ask: {
      auto pendingOrder = bip::make_managed_shared_ptr(
          segment->construct<PendingOrder>(bip::anonymous_instance)(),
          *segment);

      pendingOrder->hashOfSym_ = hashOfSym;
      pendingOrder->minAskPrice_ = order->orderPrice_;

      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash(
              [pendingOrderGroup = this->pendingOrderGroup_, pendingOrder]() {
                pendingOrderGroup->emplace(pendingOrder);
              });

      L_I(logger(), "[{}] Create pending order price info: {} {}", name(), sym,
          pendingOrder->toStr());

    } break;

    default:
      L_W(logger(), "[{}] Invalid side in order info.", name(),
          order->toShortStr());
      break;
  }
}

std::tuple<int, std::string>
TDSrvRiskPluginSelfTradeCtrl::handleSymHasPendingOrders(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo,
    const std::string& sym,
    const bip::shared_ptr<PendingOrder, VoidAlloc, DelType>& pendingOrder) {
  switch (order->side_) {
    //! 如果是多单
    case Side::Bid: {
      //! 如果买入价格大于等于挂单中的最小卖出价格，会导致自成交
      if (DEC::GE(order->orderPrice_, pendingOrder->minAskPrice_)) {
        if (pendingOrder->selfTradeTriggerTimes_ >=
            maxSelfTradeTriggerTimesAllowed_) {
          //! 如果目前自成交触发次数已经超过大于等于配置中的允许次数
          const auto statusMsg =
              fmt::format("Trigger self trade ctrl of bid: {} ", sym);
          L_W(logger(), "[{}] {} ", name(), statusMsg);
          const auto details = MakeRiskCtrlTriggerDetails(
              RISK_NAME_SELF_TRADE_CTRL, SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID,
              statusMsg, order);
          saveTriggerInfoToDB(RISK_NAME_SELF_TRADE_CTRL,
                              SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID, statusMsg,
                              details);
          return {SCODE_TD_SRV_RISK_SELF_TRADE_OF_BID, details};

        } else {
          getTDSrv()
              ->getRiskCtrlModuleComb()[combNo]
              ->getRiskCtrlStatusUpdatersGroup()[threadNo]
              ->stash(
                  [pendingOrder]() { ++pendingOrder->selfTradeTriggerTimes_; });
          L_I(logger(),
              "[{}] Add 1 to the num of self trade risk ctrl triggers. {} {}",
              name(), sym, pendingOrder->toStr());
        }

      } else {
        //! 如果下单价格大于挂单中的最大买入价格，那么更新挂单最大买入价格
        if (DEC::GT(order->orderPrice_, pendingOrder->maxBidPrice_)) {
          getTDSrv()
              ->getRiskCtrlModuleComb()[combNo]
              ->getRiskCtrlStatusUpdatersGroup()[threadNo]
              ->stash([pendingOrder, orderPrice = order->orderPrice_]() {
                pendingOrder->maxBidPrice_ = orderPrice;
              });
          L_I(logger(), "[{}] Update pending order max bid price info: {} {}",
              name(), sym, pendingOrder->toStr());
        }
      }

    } break;

    //! 如果是空单
    case Side::Ask: {
      //! 如果卖出价格小于等于挂单中的最大买入价格，会导致自成交
      if (DEC::GE(pendingOrder->maxBidPrice_, order->orderPrice_)) {
        if (pendingOrder->selfTradeTriggerTimes_ >=
            maxSelfTradeTriggerTimesAllowed_) {
          //! 如果目前自成交触发次数已经超过大于等于配置中的允许次数
          const auto statusMsg =
              fmt::format("Trigger self trade ctrl of ask: {} ", sym);
          L_W(logger(), "[{}] {} ", name(), statusMsg);
          const auto details = MakeRiskCtrlTriggerDetails(
              RISK_NAME_SELF_TRADE_CTRL, SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK,
              statusMsg, order);
          saveTriggerInfoToDB(RISK_NAME_SELF_TRADE_CTRL,
                              SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK, statusMsg,
                              details);
          return {SCODE_TD_SRV_RISK_SELF_TRADE_OF_ASK, details};

        } else {
          getTDSrv()
              ->getRiskCtrlModuleComb()[combNo]
              ->getRiskCtrlStatusUpdatersGroup()[threadNo]
              ->stash(
                  [pendingOrder]() { ++pendingOrder->selfTradeTriggerTimes_; });
          L_I(logger(),
              "[{}] Add 1 to the num of self trade risk ctrl triggers. {} {}",
              name(), sym, pendingOrder->toStr());
        }

      } else {
        //! 如果下单价格小于挂单中的最小卖出价格，那么更新挂单最小卖出价格
        if (DEC::GT(pendingOrder->minAskPrice_, order->orderPrice_)) {
          getTDSrv()
              ->getRiskCtrlModuleComb()[combNo]
              ->getRiskCtrlStatusUpdatersGroup()[threadNo]
              ->stash([pendingOrder, orderPrice = order->orderPrice_]() {
                pendingOrder->minAskPrice_ = orderPrice;
              });
          L_I(logger(), "[{}] Update pending order max ask price info: {} {}",
              name(), sym, pendingOrder->toStr());
        }
      }

    } break;

    default:
      L_W(logger(), "[{}] Invalid side in order info.", name(),
          order->toShortStr());
      break;
  }

  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginSelfTradeCtrl::doOnCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginSelfTradeCtrl::doOnOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  //! 如果订单未完结，不做任何处理
  if (order->notClosed()) {
    return {0, ""};
  }

  if (order->marketCode_ != MarketCode::SHFE &&
      order->marketCode_ != MarketCode::CZCE &&
      order->marketCode_ != MarketCode::DCE &&
      order->marketCode_ != MarketCode::CFFEX &&
      order->marketCode_ != MarketCode::INE) {
    return {0, ""};
  }

  const auto& selfTradeCtrlRangeMgr =
      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getSelfTradeCtrlRangeMgrGroup()[threadNo];

  //! selfTradeCtrlRange = acctId=10000&trdAcctId=100000
  const auto [inTheSelfTradeCtrlList, selfTradeCtrlRange] =
      selfTradeCtrlRangeMgr->inTheSelfTradeCtrlList(order);
  if (!inTheSelfTradeCtrlList) {
    return {0, ""};
  }

  findOrCtorPendingOrderGroup(combNo, threadNo);

  //! 如果订单已完结
  const auto sym = fmt::format(
      "{}-{}-{}-{}", selfTradeCtrlRange, GetMarketName(order->marketCode_),
      magic_enum::enum_name(order->symbolType_), order->symbolCode_);

  auto& idx = pendingOrderGroup_->get<TagHashOfSym>();
  const auto hashOfSym = XXH3_64bits(sym.data(), sym.size());
  const auto iter = idx.find(hashOfSym);
  if (iter == std::end(idx)) {
    L_W(logger(), "[{}] No pending order was found while handle order ret.",
        name(), order->toShortStr());
    return {0, ""};
  }

  const auto& pendingOrder = *iter;

  switch (order->side_) {
    //! 如果是多单
    case Side::Bid:
      //! 因为包含了maxBidPrice_和minAskPrice_，所以即使其中之一重置，也不能删除
      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash([pendingOrder]() {
            pendingOrder->maxBidPrice_ = std::numeric_limits<Decimal>::min();
          });
      L_I(logger(), "[{}] Reset pending order max bid price info: {} {}",
          name(), sym, pendingOrder->toStr());
      break;

    //! 如果是空单
    case Side::Ask:
      //! 因为包含了maxBidPrice_和minAskPrice_，所以即使其中之一重置，也不能删除
      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash([pendingOrder]() {
            pendingOrder->minAskPrice_ = std::numeric_limits<Decimal>::max();
          });
      L_I(logger(), "[{}] Reset pending order max ask price info: {} {}",
          name(), sym, pendingOrder->toStr());
      break;

    default:
      L_W(logger(), "[{}] Invalid side in order info.", name(),
          order->toShortStr());
      break;
  }

  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginSelfTradeCtrl::doOnCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

void TDSrvRiskPluginSelfTradeCtrl::findOrCtorPendingOrderGroup(
    std::uint32_t combNo, std::uint32_t threadNo) {
  auto& segment =
      getTDSrv()->getRiskCtrlModuleComb()[combNo]->getSegmentGroup()[threadNo];
  if (pendingOrderGroup_ == nullptr) {
    pendingOrderGroup_ = segment->find_or_construct<PendingOrderGroup>(
        NAME_OF_PENDING_ORDER_GROUP)(
        PendingOrderGroup::ctor_args_list(),
        segment->get_allocator<PendingOrderGroup>());
  }
}

}  // namespace bq::td::srv
