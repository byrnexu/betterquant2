/*!
 * \file TDSrvRiskPluginTrdSymbolList.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#include "TDSrvRiskPluginTrdSymbolList.hpp"

#include "AssetsMgr.hpp"
#include "OrdMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginConst.hpp"
#include "TDSrvRiskPluginUtil.hpp"
#include "TrdSymbolListMgr.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

boost::dll::fs::path TDSrvRiskPluginTrdSymbolList::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginTrdSymbolList::TDSrvRiskPluginTrdSymbolList(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {}

int TDSrvRiskPluginTrdSymbolList::doOnRiskCtrlConfChg(const std::string& step,
                                                      const Doc& doc,
                                                      const std::string& conf,
                                                      std::uint32_t combNo,
                                                      std::uint32_t threadNo) {
  auto& trdSymbolListMgr = getTDSrv()
                               ->getRiskCtrlModuleComb()[combNo]
                               ->getTrdSymbolListMgrGroup()[threadNo];
  const auto [statusCode, statusMsg] = trdSymbolListMgr->update(doc);
  if (statusCode != 0) {
    L_W(logger(), statusMsg);
    return statusCode;
  }

  L_I(logger(), "Step of {} recv {}", step, conf);
  return 0;
}

std::tuple<int, std::string> TDSrvRiskPluginTrdSymbolList::doOnOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  auto& trdSymbolListMgr = getTDSrv()
                               ->getRiskCtrlModuleComb()[combNo]
                               ->getTrdSymbolListMgrGroup()[threadNo];

  //! 如果在黑名单里，那么就算在白名单里也禁止下单
  if (trdSymbolListMgr->inTheBlackList(order)) {
    const auto statusMsg =
        fmt::format("Cur symbol {} in black list.", order->getSymbolInfo());
    L_W(logger(), "[{}] {} ", name(), statusMsg);
    const auto details = MakeRiskCtrlTriggerDetails(
        RISK_NAME_IN_BLACK_LIST, SCODE_TD_SRV_RISK_IN_BLACK_LIST, statusMsg,
        order);
    saveTriggerInfoToDB(RISK_NAME_IN_BLACK_LIST,
                        SCODE_TD_SRV_RISK_IN_BLACK_LIST, statusMsg, details);
    return {SCODE_TD_SRV_RISK_IN_BLACK_LIST, details};
  }

  //! 只有白名单存在，且不在白名单里的情况下禁止下单
  if (trdSymbolListMgr->whiteListExistsAndNotInIt(order)) {
    const auto statusMsg =
        fmt::format("White list exists but cur symbol {} not in white list.",
                    order->getSymbolInfo());
    L_W(logger(), "[{}] {} ", name(), statusMsg);
    const auto details = MakeRiskCtrlTriggerDetails(
        RISK_NAME_NOT_IN_WHITE_LIST, SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST,
        statusMsg, order);
    saveTriggerInfoToDB(RISK_NAME_NOT_IN_WHITE_LIST,
                        SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST, statusMsg,
                        details);
    return {SCODE_TD_SRV_RISK_NOT_IN_WHITE_LIST, details};
  }

  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginTrdSymbolList::doOnCancelOrder(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginTrdSymbolList::doOnOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

std::tuple<int, std::string> TDSrvRiskPluginTrdSymbolList::doOnCancelOrderRet(
    const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo) {
  return {0, ""};
}

}  // namespace bq::td::srv
