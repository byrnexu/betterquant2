/*!
 * \file bq_v1_user_Info.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_user_Info.hpp"

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

void Info::info(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback,
                std::string &&token) const {
  //
  const auto reqBody = MakeRestfulReqInfo(req);
  LOG_I("Recv get user info request. {}", reqBody);

  const auto &sessionTable = WebSrv::get_mutable_instance().getSessionTable();
  const auto session = sessionTable->getSession(token);
  if (!session) {
    const auto statusCode = SCODE_WEB_SRV_SESSION_TIMEOUT;
    const auto statusMsg = fmt::format(
        "Get user info failed because of not login or session timeout.");
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);
    return;
  }

  const auto identity = GET_RAND_STR();
  const auto sql = fmt::format(
      "SELECT * from `baseUserInfo` where `userId` = {};", session->userId_);
  const auto [ret, execRet] =
      WebSrv::get_mutable_instance().getDBEng()->syncExec(identity, sql);
  if (ret != 0) {
    const auto statusCode = SCODE_WEB_SRV_EXEC_DB_CMD_FAILED;
    const auto statusMsg = fmt::format("Get user info failed. {}", execRet);
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);

    return;
  }

  const auto resp = MakeCommonHttpResp(SCODE_SUCCESS, "", execRet, reqBody);
  callback(resp);

  LOG_D("Send http response. [{} - {}] {} {}", SCODE_SUCCESS,
        GetStatusMsg(SCODE_SUCCESS), execRet, reqBody)
}
