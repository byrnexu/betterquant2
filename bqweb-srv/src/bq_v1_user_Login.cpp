/*!
 * \file bq_v1_Login.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_user_Login.hpp"

#include "CommonIPCData.hpp"
#include "Config.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMSrv.hpp"
#include "SessionTable.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "db/DBE.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"

using namespace bq::v1::user;

void Login::login(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  LOG_I("Recv login request.");

  Doc doc;
  std::string reqBody = std::string(req->body());
  if (doc.Parse(reqBody.data()).HasParseError()) {
    const auto statusCode = SCODE_WEB_SRV_INVALID_BODY_IN_REQ;
    const auto statusMsg = fmt::format(
        "Parse request body failed. {} [offset {}]",
        GetParseError_En(doc.GetParseError()), doc.GetErrorOffset());
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);

    return;
  }

  LOG_I("Recv http request. {}", reqBody);

  if (!doc.HasMember("username") || !doc["username"].IsString() ||
      !doc.HasMember("password") || !doc["password"].IsString()) {
    const auto statusCode = SCODE_WEB_SRV_INVALID_BODY_IN_REQ;
    const auto statusMsg = fmt::format(
        "Parse request body failed because of invalid username or password.");
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);

    return;
  }

  const auto identity = GET_RAND_STR();
  const auto username = doc["username"].GetString();
  const auto password = doc["password"].GetString();
  const auto sql =
      fmt::format("CALL uspCheckPassword('{}', '{}');", username, password);
  const auto [ret, execRet] =
      WebSrv::get_mutable_instance().getDBEng()->syncExec(identity, sql);
  if (ret != 0) {
    const auto statusCode = SCODE_WEB_SRV_EXEC_DB_CMD_FAILED;
    const auto statusMsg =
        fmt::format("Handle login request failed. {}", execRet);
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);

    return;
  }

  Doc docOfDBExecRet;
  docOfDBExecRet.Parse(execRet.data());
  if (docOfDBExecRet["recordSetGroup"].Size() == 0 ||
      docOfDBExecRet["recordSetGroup"][0].Size() == 0) {
    const auto statusCode = SCODE_WEB_SRV_INVALID_USERNAME_OR_PASSWORD;
    const auto statusMsg = fmt::format("Handle login request failed. [{} - {}]",
                                       statusCode, GetStatusMsg(statusCode));
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);

    return;
  }

  const auto userId =
      docOfDBExecRet["recordSetGroup"][0][0]["userId"].GetUint();

  //! 目前的逻辑是重复登录会把前一用户踢下线
  auto &sessionTable = WebSrv::get_mutable_instance().getSessionTable();
  sessionTable->removeSession(userId);
  const auto session = sessionTable->addSession(userId, username, password);

  const auto rspBody = session->toJson();
  const auto resp =
      MakeCommonHttpResp(SCODE_SUCCESS, "", session->toJson(), reqBody);
  callback(resp);

  LOG_D("Send http response. [{} - {}] {}", SCODE_SUCCESS,
        GetStatusMsg(SCODE_SUCCESS), rspBody)
}
