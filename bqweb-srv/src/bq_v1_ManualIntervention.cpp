/*!
 * \file bq_v1_ManualIntervention.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_ManualIntervention.hpp"

#include "CommonIPCData.hpp"
#include "Config.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMSrv.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"

using namespace bq::v1;

void ManualIntervention::manualIntervention(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, StgId stgId,
    StgInstId stgInstId) const {
  LOG_I("Recv manual intervention of stg {} - {}.", stgId, stgInstId)

  Doc doc;
  std::string reqBody = std::string(req->body());
  if (doc.Parse(reqBody.data()).HasParseError()) {
    const auto statusCode = SCODE_WEB_SRV_INVALID_BODY_IN_REQ;
    const auto statusMsg =
        ("Parse request body failed. {0} [offset {1}] {2}",
         GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), reqBody);
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);
    return;
  }

  LOG_I("Recv original http request. {}", reqBody)

  auto forwardReqBody = reqBody;
  if (!forwardReqBody.empty()) {
    if (forwardReqBody == "{}") {
      forwardReqBody = "}";
    } else {
      forwardReqBody[0] = ',';
    }
  } else {
    forwardReqBody = "}";
  }
  //! 因为客户端发送过来的干预请求的json结构中没有stgId和stgInstId，所以在这里追
  //! 加上去，使得策略引擎可以得到stgId和stgInstId
  auto data = fmt::format(R"({{"stgId":{},"stgInstId":{})", stgId, stgInstId);
  data = data + forwardReqBody;

  LOG_I("Recv http request. {}", data)

  WebSrv::get_mutable_instance().getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
      [&](void *shmBuf) {
        auto commonIPCData = static_cast<CommonIPCData *>(shmBuf);
        memcpy(commonIPCData->data_, data.c_str(), data.size());
      },
      stgId, MSG_ID_ON_STG_MANUAL_INTERVENTION,
      sizeof(CommonIPCData) + data.size() + 1);

  LOG_I("Forward http request. {}", data)

  const auto resp = MakeCommonHttpResp(SCODE_SUCCESS, "", "", reqBody);
  callback(resp);

  LOG_D("Send http response. [{} - {}]", SCODE_SUCCESS,
        GetStatusMsg(SCODE_SUCCESS));
}
