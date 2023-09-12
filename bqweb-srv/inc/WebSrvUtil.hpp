/*!
 * \file WebSrvUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include <drogon/HttpController.h>

#include "def/BQDef.hpp"
#include "def/ConstIF.hpp"
#include "util/Pch.hpp"

using namespace drogon;

namespace bq {

std::string MakeCommonHttpBody(int statusCode, const std::string& statusMsg,
                               std::string rspBody, std::string reqBody,
                               WriteLog writeLog = WriteLog::True);

HttpResponsePtr MakeCommonHttpResp(int statusCode, const std::string& statusMsg,
                                   const std::string& rspBody,
                                   const std::string& reqBody,
                                   WriteLog writeLog = WriteLog::True);

std::string MakeRestfulReqInfo(const HttpRequestPtr& req);

}  // namespace bq
