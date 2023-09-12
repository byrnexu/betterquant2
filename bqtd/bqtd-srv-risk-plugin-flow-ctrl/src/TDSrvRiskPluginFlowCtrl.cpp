/*!
 * \file TDSrvRiskPluginFlowCtrl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginFlowCtrl.hpp"

#include "AssetsMgr.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "TDSrv.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"

namespace bq::td::srv {

boost::dll::fs::path TDSrvRiskPluginFlowCtrl::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginFlowCtrl::TDSrvRiskPluginFlowCtrl(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {
  flowCtrlSvc_ = std::make_shared<FlowCtrlSvc>(node_);
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrl::doOnOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  const auto taskName = fmt::format("{}-{}", order->acctId_,
                                    GetMsgName(order->shmHeader_.msgId_));
  if (flowCtrlSvc_->exceedFlowCtrl(taskName)) {
    return {SCODE_TD_SRV_RISK_EXCEED_FLOW_CTRL, ""};
  }
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrl::doOnCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrl::doOnOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrl::doOnCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

}  // namespace bq::td::srv
