/*!
 * \file TDSrvRiskPluginCloseTDayStg.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginCloseTDayStg.hpp"

#include "AssetsMgr.hpp"
#include "OrdMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginConst.hpp"
#include "TDSrvRiskPluginUtil.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/OpenedContractGroup.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

boost::dll::fs::path TDSrvRiskPluginCloseTDayStg::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginCloseTDayStg::TDSrvRiskPluginCloseTDayStg(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {}

std::tuple<int, std::string> TDSrvRiskPluginCloseTDayStg::doOnOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  initOpenedContractGroup(combNo, threadNo);

  if (order->closeTDayStg_ == CloseTDayStg::AllowCloseTDay) {
    return {0, ""};
  }

  //! 这个逻辑是为了避免平今的出现，因此只有平仓单才需要此风控
  if (order->posDirection_ != PosDirection::Close) {
    return {0, ""};
  }

  //! 如果不是下面几个市场，不需要处理是否平今的逻辑
  if (order->marketCode_ != MarketCode::CZCE &&
      order->marketCode_ != MarketCode::DCE &&
      order->marketCode_ != MarketCode::CFFEX) {
    return {0, ""};
  }

  switch (order->closeTDayStg_) {
    //! 有挂单就拒单，彻底杜绝平今
    case CloseTDayStg::RejectCloseTDay: {
      //! 有挂单就拒单，有当日开仓更加要拒绝
      if (openedContractGroup_->haveOpenedTDay<LockFunc::False>(order.get()) ==
          OpenedTDay::True) {
        const auto statusMsg =
            fmt::format("Reject order because of exists open contracts. {}",
                        order->getSymbolInfo());
        L_W(logger(), statusMsg);
        const auto details = MakeRiskCtrlTriggerDetails(
            RISK_NAME_CLOSE_TDAY_CTRL, SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY,
            statusMsg, order);
        saveTriggerInfoToDB(RISK_NAME_CLOSE_TDAY_CTRL,
                            SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY, statusMsg,
                            details);
        return {SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY, details};
      }

      //! 有挂单就拒单，彻底杜绝平今
      const auto& ordMgr = getTDSrv()
                               ->getRiskCtrlModuleComb()[combNo]
                               ->getOrdMgrGroup()[threadNo];
      if (ordMgr->existsOpenPendingOrders<LockFunc::False>(order)) {
        const auto statusMsg = fmt::format(
            "Reject order because of exists open pending orders. {}",
            order->getSymbolInfo());
        L_W(logger(), statusMsg);
        const auto details = MakeRiskCtrlTriggerDetails(
            RISK_NAME_CLOSE_TDAY_CTRL,
            SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS, statusMsg, order);
        saveTriggerInfoToDB(RISK_NAME_CLOSE_TDAY_CTRL,
                            SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS,
                            statusMsg, details);
        return {SCODE_TD_SRV_RISK_EXISTS_OPEN_PENDING_ORDERS, details};
      }

    } break;

    //! 有开仓的情况下才拒单，这样如果有在途挂开仓单的话可能也会导致平今
    case CloseTDayStg::RejectUntilOpenTDay: {
      if (openedContractGroup_->haveOpenedTDay<LockFunc::False>(order.get()) ==
          OpenedTDay::True) {
        const auto statusMsg =
            fmt::format("Reject order because of exists open contracts. {}",
                        order->getSymbolInfo());
        L_W(logger(), statusMsg);
        const auto details = MakeRiskCtrlTriggerDetails(
            RISK_NAME_CLOSE_TDAY_CTRL, SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY,
            statusMsg, order);
        saveTriggerInfoToDB(RISK_NAME_CLOSE_TDAY_CTRL,
                            SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY, statusMsg,
                            details);
        return {SCODE_TD_SRV_RISK_EXISTS_OPEN_TDAY, details};
      }
    } break;

    default:
      L_I(logger(), "[{}] Invalid order stg in order. {}", name(),
          order->toShortStr());
      break;
  }

  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginCloseTDayStg::doOnCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

//!
//! 因为卖平和卖开都会修改 posMgr 的 Ask + Short 字段，因此 Ask + Short 无法体现
//! 是开还是平，所以 posMgr 无法体现是否开了今仓，因此在回报回来的时候如果有开今
//! 仓成功的话，那么在这里记录
//!
std::tuple<int, std::string> TDSrvRiskPluginCloseTDayStg::doOnOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  initOpenedContractGroup(combNo, threadNo);
  //! 如果有当日开仓，那么保存账号和标的信息
  const auto statusMsg =
      openedContractGroup_->saveOpenedContract<LockFunc::False>(
          order.get(), WriteLog::False);
  if (!statusMsg.empty()) {
    LOG_I(statusMsg);
  }
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginCloseTDayStg::doOnCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

void TDSrvRiskPluginCloseTDayStg::initOpenedContractGroup(
    std::uint32_t combNo, std::uint32_t threadNo) {
  if (openedContractGroup_ == nullptr) {
    openedContractGroup_ = std::make_shared<OpenedContractGroup>();
    const auto& segment = getTDSrv()
                              ->getRiskCtrlModuleComb()[combNo]
                              ->getSegmentGroup()[threadNo];
    const auto msgGroup = openedContractGroup_->load(segment);
    for (const auto& msg : msgGroup) {
      LOG_I(msg);
    }
  }
}

}  // namespace bq::td::srv
