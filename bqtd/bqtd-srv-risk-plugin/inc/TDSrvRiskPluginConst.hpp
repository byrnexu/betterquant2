/*!
 * \file TDSrvRiskPluginDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

const static std::string PLUGIN_CLOST_TDAY_STG = "risk-plugin-close-tday-stg";
const static std::string PLUGIN_SELF_TRADE_CTRL = "risk-plugin-self-trade-ctrl";
const static std::string PLUGIN_FLOW_CTRL_PLUS = "risk-plugin-flow-ctrl-plus";
const static std::string PLUGIN_FLOW_CTRL = "risk-plugin-flow-ctrl";
const static std::string PLUGIN_TRD_SYMBOL_LIST = "risk-plugin-trd-symbol-list";
const static std::string PLUGIN_PNL_MONITOR = "risk-plugin-pnl-monitor";

const static std::string RISK_NAME_CLOSE_TDAY_CTRL = "CloseTDayCtrl";
const static std::string RISK_NAME_SELF_TRADE_CTRL = "SelfTradeCtrl";
const static std::string RISK_NAME_PNL_MONITOR = "PnlMonitor";
const static std::string RISK_NAME_IN_BLACK_LIST = "InBlackList";
const static std::string RISK_NAME_NOT_IN_WHITE_LIST = "NotInWhiteList";

}  // namespace bq
