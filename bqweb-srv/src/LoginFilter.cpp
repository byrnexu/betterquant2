/*!
 * \file LoginFilter.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#include "LoginFilter.hpp"

#include "SessionTable.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "def/StatusCode.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Logger.hpp"

namespace bq {

void LoginFilter::doFilter(const HttpRequestPtr &req, FilterCallback &&fcb,
                           FilterChainCallback &&fccb) {
  LOG_D("Enter login filter");
  const auto tokenOfCurReq = req->getHeader("token");
  const auto &sessionTable = WebSrv::get_mutable_instance().getSessionTable();
  const auto sessionExists =
      sessionTable->checkAndUpdateLastActiveTimeOfSession(tokenOfCurReq);
  if (sessionExists) {
    fccb();
    return;
  }

  const auto reqInfo = MakeRestfulReqInfo(req);
  LOG_W("User from {} not login in or expired. [{} {}] token: {}",
        req->peerAddr().toIpPort(), reqInfo, std::string(req->body()),
        tokenOfCurReq);

  //! 为了使用MakeCommonHttpResp函数，造一个假的请求
  const auto statusCode = SCODE_WEB_SRV_SESSION_TIMEOUT;
  const auto reqBody =
      fmt::format(R"({{"{}":"{}"}})", MSG_NAME, MSG_SESSION_CHECK);
  const auto resp = MakeCommonHttpResp(statusCode, "", "", reqBody);

  resp->addHeader("Access-Control-Allow-Origin", "*");
  resp->addHeader("Access-Control-Allow-Methods", "*");
  resp->addHeader("Access-Control-Allow-Headers", "*");
  resp->addHeader("Access-Control-Max-Age", "86400");

  fcb(resp);
}

}  // namespace bq
