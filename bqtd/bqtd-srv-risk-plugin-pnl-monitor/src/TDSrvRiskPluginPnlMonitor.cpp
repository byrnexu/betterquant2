/*!
 * \file TDSrvRiskPluginPnlMonitor.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginPnlMonitor.hpp"

#include "AssetsMgr.hpp"
#include "OrdMgr.hpp"
#include "PnlMonitorRangeMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
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

boost::dll::fs::path TDSrvRiskPluginPnlMonitor::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginPnlMonitor::TDSrvRiskPluginPnlMonitor(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {}

int TDSrvRiskPluginPnlMonitor::doLoad() {
  secDelayOfPrice_ = node_["secDelayOfPrice"].as<std::uint32_t>(UINT32_MAX);
  return 0;
}

int TDSrvRiskPluginPnlMonitor::doOnRiskCtrlConfChg(const std::string& step,
                                                   const Doc& doc,
                                                   const std::string& conf,
                                                   std::uint32_t combNo,
                                                   std::uint32_t threadNo) {
  auto& pnlMonitorRangeMgr = getTDSrv()
                                 ->getRiskCtrlModuleComb()[combNo]
                                 ->getPnlMonitorRangeMgrGroup()[threadNo];
  const auto [statusCode, statusMsg] = pnlMonitorRangeMgr->update(doc);
  if (statusCode != 0) {
    L_W(logger(), statusMsg);
    return statusCode;
  }

  L_I(logger(), "Step of {} recv {}", step, conf);
  return 0;
}

std::tuple<int, std::string> TDSrvRiskPluginPnlMonitor::doOnOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  if (order->symbolType_ == SymbolType::CN_MainBoard ||
      order->symbolType_ == SymbolType::CN_SecondBoard ||
      order->symbolType_ == SymbolType::CN_StartupBoard ||
      order->symbolType_ == SymbolType::CN_TechBoard ||
      order->symbolType_ == SymbolType::Spot) {
    //! 如果是现货卖出，不管盈亏如何都是允许的
    if (order->side_ == Side::Ask) {
      return {0, ""};
    }
  } else if (order->symbolType_ == SymbolType::CN_Futures) {
    //! 如果是合约平仓，不管盈亏如何都是允许的
    if (order->posDirection_ == PosDirection::Close ||
        order->posDirection_ == PosDirection::CloseTDay ||
        order->posDirection_ == PosDirection::CloseYDay) {
      return {0, ""};
    }
  } else {
    return {0, ""};
  }

  auto& pnlMonitorRangeMgr = getTDSrv()
                                 ->getRiskCtrlModuleComb()[combNo]
                                 ->getPnlMonitorRangeMgrGroup()[threadNo];

  const auto [statusCode, statusMsg] =
      pnlMonitorRangeMgr->checkIfTriggerRiskCtrl(order, secDelayOfPrice_);
  if (statusCode != 0) {
    L_W(logger(), "[{}] {} ", name(), statusMsg);
    const auto details = MakeRiskCtrlTriggerDetails(
        RISK_NAME_PNL_MONITOR, statusCode, statusMsg, order);
    saveTriggerInfoToDB(RISK_NAME_PNL_MONITOR, statusCode, statusMsg, details);
    return {statusCode, details};
  }

  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginPnlMonitor::doOnCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginPnlMonitor::doOnOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginPnlMonitor::doOnCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

}  // namespace bq::td::srv
