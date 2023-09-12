/*!
 * \file bq_v1_CommonInterface.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#pragma once

#include <drogon/HttpController.h>

#include "SHMIPCMsgId.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

using namespace drogon;

namespace bq {
namespace v1 {

/*
 * curl -i -k -H "Content-type: application/json" -X POST -d
 * '{"sql":"SELECT * FROM acctInfo"}' "http://localhost/v1/commonInterface"
 */
class CommonInterface : public drogon::HttpController<CommonInterface> {
 public:
  METHOD_LIST_BEGIN
  // http://localhost/v1/commonInterface
  ADD_METHOD_TO(CommonInterface::commonInterface, "/v1/commonInterface", Post,
                Options, "bq::LoginFilter");
  METHOD_LIST_END

  void commonInterface(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

 private:
  void handleMsgExecSql(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        Doc &doc, const std::string &reqBody,
                        WriteLog writeLog = WriteLog::True);

 private:
  void handleMsgOrder(const HttpRequestPtr &req,
                      std::function<void(const HttpResponsePtr &)> &&callback,
                      Doc &doc, const std::string &reqBody) const;

  void handleMsgCancelOrder(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
      const std::string &reqBody) const;

  void handleMsgStartStg(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
      const std::string &reqBody) const;

  void handleMsgStopStg(const HttpRequestPtr &req,
                        std::function<void(const HttpResponsePtr &)> &&callback,
                        Doc &doc, const std::string &reqBody) const;

  void handleMsgGetStgStatus(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
      const std::string &reqBody) const;

  void handleMsgQueryPnl(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
      const std::string &reqBody) const;

 private:
  void forwardReqToStg(const HttpRequestPtr &req, Doc &doc,
                       const std::string &reqBody, StgId stgId,
                       MsgId msgId) const;

 private:
  WriteLog checkIfWriteLog(const std::string &reqBody) const;
};

}  // namespace v1
}  // namespace bq
