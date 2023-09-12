/*!
 * \file WebSrvTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/12
 *
 * \brief
 */

#include "WebSrvTaskHandler.hpp"

#include "CommonIPCData.hpp"
#include "SHMIPC.hpp"
#include "StgEngImpl.hpp"
#include "StgEngUtil.hpp"
#include "db/TBLMonitorOfStgInstInfo.hpp"
#include "util/Logger.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::stg {

WebSrvTaskHandler::WebSrvTaskHandler(StgEngImpl* stgEng) : stgEng_(stgEng) {}

void WebSrvTaskHandler::handleTask(const void* buf, std::size_t bufLen) {
  const auto msgId = static_cast<const SHMHeader*>(buf)->msgId_;
  switch (msgId) {
    case MSG_ID_ON_STG_MANUAL_INTERVENTION:
      handleStgManualIntervention(buf, bufLen);
      break;
    case MSG_ID_ON_STG_MANUAL_ORDER:
      handleStgManualOrder(buf, bufLen);
      break;
    case MSG_ID_ON_STG_MANUAL_CANCEL_ORDER:
      handleStgManualCancelOrder(buf, bufLen);
      break;
    default:
      stgEng_->logWarn("Unhandled msgId {} - {}.",
                       {std::to_string(msgId), GetMsgName(msgId)},
                       stgEng_->getDftStgInstInfo());
      break;
  }
}

void WebSrvTaskHandler::handleStgManualIntervention(const void* buf,
                                                    std::size_t bufLen) {
  const auto [statusCode, stgInstId] =
      GetStgInstId(static_cast<const CommonIPCData*>(buf));
  if (statusCode != 0) {
    stgEng_->logWarn("Get invalid stgInstId from common ipc data of {} - {}.",
                     {std::to_string(MSG_ID_ON_STG_MANUAL_INTERVENTION),
                      GetMsgName(MSG_ID_ON_STG_MANUAL_INTERVENTION)},
                     stgEng_->getDftStgInstInfo());
    return;
  }

  stgEng_->logInfo("Dispatch manual intervention: {}",
                   {static_cast<const CommonIPCData*>(buf)->data_},
                   stgEng_->getDftStgInstInfo());

  auto asyncTask = std::make_shared<SHMIPCAsyncTask>(
      std::make_shared<SHMIPCTask>(buf, bufLen), stgInstId);
  stgEng_->getStgInstTaskDispatcher()->dispatch(asyncTask);
}

void WebSrvTaskHandler::handleStgManualOrder(const void* buf,
                                             std::size_t bufLen) {
  const auto reqBody = static_cast<const CommonIPCData*>(buf)->data_;
  stgEng_->logInfo("Recv manual order: {}", {reqBody},
                   stgEng_->getDftStgInstInfo());

  Doc doc;
  doc.Parse(reqBody);

  const auto stgInstId = doc["stgInstId"].GetUint();
  const auto trdAcctId = doc["trdAcctId"].GetUint();

  const auto marketCodeInStrFmt = doc["marketCode"].GetString();
  const auto marketCode =
      magic_enum::enum_cast<MarketCode>(marketCodeInStrFmt).value();

  const auto symbolCode = doc["symbolCode"].GetString();

  const auto sideInStrFmt = doc["side"].GetString();
  const auto side = magic_enum::enum_cast<Side>(sideInStrFmt).value();

  const auto posDirectionInStrFmt = doc["posDirection"].GetString();
  const auto posDirection =
      magic_enum::enum_cast<PosDirection>(posDirectionInStrFmt).value();

  const auto orderPrice = CONV(Decimal, doc["orderPrice"].GetString());
  const auto orderSize = CONV(Decimal, doc["orderSize"].GetString());

  const auto orderId = CONV(std::uint64_t, doc["orderId"].GetString());

  CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay;
  if (doc.HasMember("closeTDayStg") && doc["closeTDayStg"].IsString()) {
    const auto closeTDayStgInStrFmt = doc["closeTDayStg"].GetString();
    closeTDayStg =
        magic_enum::enum_cast<CloseTDayStg>(closeTDayStgInStrFmt).value();
  }

  const auto [statusCode, id] = stgEng_->order(
      stgInstId, marketCode, symbolCode, side, posDirection, orderPrice,
      orderSize, trdAcctId, closeTDayStg, 0, orderId);

  sendRspTOWebSrv(MSG_ID_ON_MANUAL_ORDER_RET, statusCode, reqBody);
}

void WebSrvTaskHandler::handleStgManualCancelOrder(const void* buf,
                                                   std::size_t bufLen) {
  const auto reqBody = static_cast<const CommonIPCData*>(buf)->data_;
  stgEng_->logInfo("Recv manual cancel order: {}", {reqBody},
                   stgEng_->getDftStgInstInfo());

  Doc doc;
  doc.Parse(reqBody);

  const auto orderId = CONV(std::uint64_t, doc["orderId"].GetString());
  const auto statusCode = stgEng_->cancelOrder(orderId);

  sendRspTOWebSrv(MSG_ID_ON_MANUAL_CANCEL_ORDER_RET, statusCode, reqBody);
}

void WebSrvTaskHandler::sendRspTOWebSrv(MsgId msgId, int statusCode,
                                        const std::string& reqBody) {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("statusCode");
  writer.Int(statusCode);
  writer.Key("reqBody");
  writer.String(reqBody.c_str());
  writer.EndObject();

  const std::string rsp = strBuf.GetString();
  stgEng_->logInfo("Send msg of {} - {} to web srv. {}",
                   {std::to_string(msgId), GetMsgName(msgId), rsp},
                   stgEng_->getDftStgInstInfo());

  const auto shmBufLen = sizeof(CommonIPCData) + rsp.size() + 1;
  stgEng_->getSHMCliOfWebSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto commonIPCData = static_cast<CommonIPCData*>(shmBuf);
        memcpy(commonIPCData->data_, rsp.c_str(), rsp.size());
        commonIPCData->dataLen_ = rsp.size() + 1;
      },
      msgId, shmBufLen);
}

}  // namespace bq::stg
