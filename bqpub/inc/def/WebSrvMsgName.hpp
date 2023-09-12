/*!
 * \file WebSrvMsgName.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/11
 *
 * \brief
 */

#pragma once

#include "util/PchBase.hpp"

namespace bq {

const static char* MSG_NAME = "msgName";

const static std::string MSG_TRIGGER_RISK_CTRL = "triggerRiskCtrl";

const static std::string MSG_SESSION_CHECK = "sessionCheck";
const static std::string MSG_EXEC_SQL = "execSql";

const static std::string MSG_ORDER = "order";
const static std::string MSG_CANCEL_ORDER = "cancelOrder";
const static std::string MSG_ORDER_RET = "orderRet";
const static std::string MSG_CANCEL_ORDER_RET = "cancelOrderRet";

const static std::string MSG_REGISTER = "register";

const static std::string MSG_SUB = "sub";
const static std::string MSG_UN_SUB = "unSub";

const static std::string MSG_MARKET_DATA = "marketData";

const static std::string MSG_START_STG = "startStg";
const static std::string MSG_STOP_STG = "stopStg";
const static std::string MSG_GET_STG_STATUS = "getStgStatus";
const static std::string MSG_QUERY_PNL = "queryPnl";

}  // namespace bq
