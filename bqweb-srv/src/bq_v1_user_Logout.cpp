/*!
 * \file bq_v1_Logout.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_user_Logout.hpp"

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

void Logout::logout(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) const {
  LOG_I("Recv logout request.");

  const auto resp = MakeCommonHttpResp(SCODE_SUCCESS, "", "", "");
  callback(resp);

  LOG_D("Send http response. [{} - {}]", SCODE_SUCCESS,
        GetStatusMsg(SCODE_SUCCESS))
}
