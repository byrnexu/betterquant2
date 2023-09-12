/*!
 * \file TDSrvRiskPluginUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/20
 *
 * \brief
 */

#include "db/DBE.hpp"
#include "db/TBLRiskCtrlTriggerInfo.hpp"
#include "def/Def.hpp"
#include "def/OrderInfoIF.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Pch.hpp"
#include "util/Random.hpp"

namespace bq {

// clang-format off
/*
{
  "msgName": "triggerRiskCtrl",
	"name": "OrderAmtEachTime",
	"statusCode": 10003,
	"statusMsg": "Trigger risk ctrl [no: 10003; name: OrderAmtEachTime; step: acctId; target: OrderAmtEachTime; condition: acctId=10000&side=Bid; value: 1000000; msInterval: 2657896652; tsQueSize: 0]. [1111111000 > 1000000] 3286969224994457149",
	"orderInfo": {
		"productGrpId": 0,
		"productId": 0,
		"userId": 1,
		"acctGrpId": 0,
		"acctId": 10000,
		"trdAcctId": 100000,
		"stgGrpId": 0,
		"stgId": 1,
		"stgInstId": 1,
		"algoId": 0,
		"orderId": 3286969224994457149,
		"exchOrderId": "",
		"parentOrderId": 0,
		"marketCode": "SSE",
		"symbolType": "CN_MainBoard",
		"symbolCode": "600600",
		"exchSymbolCode": "600600",
		"side": "Bid",
		"posDirection": "Both",
		"posSide": "Both",
		"orderPrice": 1111111.0,
		"orderSize": 1000.0,
		"parValue": 0,
		"orderType": "Limit",
		"orderTypeExtra": "Normal",
		"closeTDayStg": "RejectCloseTDay",
		"orderTime": 1682064669699409,
		"fee": 0.0,
		"feeCurrency": "CNY",
		"dealSize": 0.0,
		"avgDealPrice": 0.0,
		"lastTradeId": "",
		"lastDealPrice": 0.0,
		"lastDealSize": 0.0,
		"lastDealTime": 946684800000000,
		"orderStatus": "Created",
		"statusCode": 0
	}
}
*/
// clang-format on
//
std::string MakeRiskCtrlTriggerDetails(const std::string& name, int statusCode,
                                       const std::string& statusMsg,
                                       const OrderInfoSPtr& orderInfo) {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key(MSG_NAME);
  writer.String(MSG_TRIGGER_RISK_CTRL.c_str());
  writer.Key("name");
  writer.String(name.c_str());
  writer.Key("statusCode");
  writer.Int(statusCode);
  writer.Key("statusMsg");
  writer.String(statusMsg.c_str());
  writer.EndObject();

  std::string details;
  details.reserve(2048);
  details = strBuf.GetString();
  details[details.size() - 1] = ',';

  details.append(R"("orderInfo":)");
  details.append(orderInfo->toJson());
  details.append("}");

  return details;
}

std::string MakeSqlOfRiskCtrlTriggerInfo(const std::string& name,
                                         int statusCode,
                                         const std::string& statusMsg,
                                         const std::string& details) {
  // clang-format off
  const auto sql = fmt::format(
      "INSERT INTO {} ("
      "`name`,"
      "`statusCode`,"
      "`statusMsg`,"
      "`details`"
      ")"
      "VALUES"
      "("
      "'{}',"  // name
      " {} ,"  // statusCode
      "'{}',"  // statusMsg
      "'{}' "  // details
      "); ",
      TBLRiskCtrlTriggerInfo::TableName,
      name,
      statusCode,
      statusMsg,
      details
      );
  return sql;
  // clang-format on
}

std::string MakeSqlOfRiskCtrlTriggerInfo(const std::string& name,
                                         int statusCode,
                                         const std::string& statusMsg,
                                         const OrderInfoSPtr& orderInfo) {
  const auto details =
      MakeRiskCtrlTriggerDetails(name, statusCode, statusMsg, orderInfo);
  return MakeSqlOfRiskCtrlTriggerInfo(name, statusCode, statusMsg, details);
}

}  // namespace bq
