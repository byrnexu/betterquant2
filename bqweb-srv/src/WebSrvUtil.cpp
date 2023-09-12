/*!
 * \file WebSrvUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#include "WebSrvUtil.hpp"

#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq {

std::string MakeCommonHttpBody(int statusCode, const std::string& statusMsg,
                               std::string rspBody, std::string reqBody,
                               WriteLog writeLog) {
  const auto escapedMsg = EscapeStr(statusMsg);
  auto ret = R"({"statusCode":)" + std::to_string(statusCode) +
             R"(,"statusMsg":")" + escapedMsg + R"("})";

  if (rspBody.empty()) {
    rspBody = "{}";
  }

  ret[ret.size() - 1] = ',';
  ret.append(R"("rsp":)");
  ret.append(rspBody);
  ret.append("}");

  if (!reqBody.empty()) {
    ret[ret.size() - 1] = ',';
    ret.append(R"("req":)");
    ret.append(reqBody);
    ret.append("}");
  }

  if (writeLog == WriteLog::False) {
    if (ret.size() <= 0x400) {
      LOG_D("Make common http response body. {}", ret);
    } else {
      LOG_D("Make common http response body. {} bytes", ret.size());
    }
  } else {
    if (ret.size() <= 0x400) {
      LOG_I("Make common http response body. {}", ret);
    } else {
      LOG_I("Make common http response body. {} bytes", ret.size());
    }
  }

  return ret;
}

HttpResponsePtr MakeCommonHttpResp(int statusCode, const std::string& statusMsg,
                                   const std::string& rspBody,
                                   const std::string& reqBody,
                                   WriteLog writeLog) {
  if (statusCode != 0) {
    LOG_W("{} {}", statusMsg, reqBody);
  }
  auto resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k200OK);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  const auto rsp =
      MakeCommonHttpBody(statusCode, statusMsg, rspBody, reqBody, writeLog);
  resp->setBody(rsp);
  return resp;
}

std::string MakeRestfulReqInfo(const HttpRequestPtr& req) {
  const auto reqInfo = fmt::format("{}?{}", req->getPath(), req->getQuery());
  return fmt::format(R"({{"restful":"{}"}})", reqInfo);
}

}  // namespace bq
