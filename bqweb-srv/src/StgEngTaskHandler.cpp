/*!
 * \file StgEngTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/14
 *
 * \brief
 */

#include "StgEngTaskHandler.hpp"

#include "CommonIPCData.hpp"
#include "ReqBody2CallbackGroup.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCTask.hpp"
#include "SHMIPCTopicName.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "UserId2WSConnGroup.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/Util.hpp"

namespace bq {

StgEngTaskHandler::StgEngTaskHandler(WebSrv* webSrv) : webSrv_(webSrv) {}

void StgEngTaskHandler::handleAsyncTask(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_ORDER_RET:
      handleMsgIdOnOrderRet(asyncTask);
      break;
    case MSG_ID_ON_CANCEL_ORDER_RET:
      handleMsgIdOnCancelOrderRet(asyncTask);
      break;
    case MSG_ID_ON_MANUAL_ORDER_RET:
      handleMsgIdOnManualOrderRet(asyncTask);
      break;
    case MSG_ID_ON_MANUAL_CANCEL_ORDER_RET:
      handleMsgIdOnManualCancelOrderRet(asyncTask);
      break;
    default:
      LOG_W("Unable to process msgId {} - {}.", shmHeader->msgId_,
            GetMsgName(shmHeader->msgId_));
      break;
  }
}

void StgEngTaskHandler::handleMsgIdOnOrderRet(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto commonIPCData = MakeMsgSPtrByTask<CommonIPCData>(asyncTask->task_);
  LOG_I("Recv order ret {}", commonIPCData->data_);

  Doc doc;
  std::string orderInfoInJsonFmt = commonIPCData->data_;
  doc.Parse(orderInfoInJsonFmt.data());

  const auto userId = doc["userId"].GetUint();
  const auto wsConn =
      WebSrv::get_mutable_instance().getUserId2WSConnGroup()->getWSConn(userId);
  if (wsConn == nullptr) {
    LOG_W("Can not get ws conn of userid {} when handle rsp of order. {}",
          userId, orderInfoInJsonFmt);
    return;
  }

  std::string rsp = fmt::format(R"({{"msgName":"{}")", MSG_ORDER_RET);
  orderInfoInJsonFmt[0] = ',';
  rsp.append(orderInfoInJsonFmt);
  wsConn->send(rsp);
}

void StgEngTaskHandler::handleMsgIdOnCancelOrderRet(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto commonIPCData = MakeMsgSPtrByTask<CommonIPCData>(asyncTask->task_);
  LOG_I("Recv cancel order ret {}", commonIPCData->data_);

  Doc doc;
  std::string orderInfoInJsonFmt = commonIPCData->data_;
  doc.Parse(orderInfoInJsonFmt.data());

  const auto userId = doc["userId"].GetUint();
  const auto wsConn =
      WebSrv::get_mutable_instance().getUserId2WSConnGroup()->getWSConn(userId);
  if (wsConn == nullptr) {
    LOG_W(
        "Can not get ws conn of userid {} when handle rsp of cancel order. {}",
        userId, orderInfoInJsonFmt);
    return;
  }

  std::string rsp = fmt::format(R"({{"msgName":"{}")", MSG_CANCEL_ORDER_RET);
  orderInfoInJsonFmt[0] = ',';
  rsp.append(orderInfoInJsonFmt);
  wsConn->send(rsp);
}

//! 手工下单应答
void StgEngTaskHandler::handleMsgIdOnManualOrderRet(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto commonIPCData = MakeMsgSPtrByTask<CommonIPCData>(asyncTask->task_);
  LOG_I("Recv manual order ret {}", commonIPCData->data_);
  forwardRspToClient(commonIPCData->data_);
}

//! 手工撤单应答
void StgEngTaskHandler::handleMsgIdOnManualCancelOrderRet(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto commonIPCData = MakeMsgSPtrByTask<CommonIPCData>(asyncTask->task_);
  LOG_I("Recv manual cancel order ret {}", commonIPCData->data_);
  forwardRspToClient(commonIPCData->data_);
}

void StgEngTaskHandler::forwardRspToClient(const std::string& rspInJsonFmt) {
  Doc doc;
  doc.Parse(rspInJsonFmt.data());

  const auto statusCode = doc["statusCode"].GetInt();
  const auto reqBody = doc["reqBody"].GetString();

  const auto callback =
      WebSrv::get_mutable_instance().getReqBody2CallbackGroup()->getCallback(
          reqBody);
  if (callback != nullptr) {
    const auto resp =
        MakeCommonHttpResp(statusCode, GetStatusMsg(statusCode), "", reqBody);
    callback(resp);
    return;
  }
}

}  // namespace bq
