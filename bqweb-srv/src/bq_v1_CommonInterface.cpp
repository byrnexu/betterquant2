/*!
 * \file bq_v1_CommonInterface.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_CommonInterface.hpp"

#include "CacheOfDBRet.hpp"
#include "CommonIPCData.hpp"
#include "Config.hpp"
#include "ReqBody2CallbackGroup.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMSrv.hpp"
#include "StgMgr.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "db/DBE.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "def/StgConst.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"

using namespace bq::v1;

void CommonInterface::commonInterface(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback) {
  LOG_T("Recv common interface request.");

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

  const auto writeLog = checkIfWriteLog(reqBody);
  if (writeLog == WriteLog::True) {
    LOG_I("Recv http request of common interface. {}", reqBody);
  } else {
    LOG_D("Recv http request of common interface. {}", reqBody);
  }

  if (!doc.HasMember(MSG_NAME) || !doc[MSG_NAME].IsString()) {
    const auto statusCode = SCODE_WEB_SRV_INVALID_BODY_IN_REQ;
    const auto statusMsg = fmt::format(
        "Parse request body failed because of no field of {}.", MSG_NAME);
    const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
    callback(resp);
    return;
  }

  const std::string msgName = doc[MSG_NAME].GetString();
  if (msgName == MSG_EXEC_SQL) {
    handleMsgExecSql(req, std::move(callback), doc, reqBody, writeLog);
  } else if (msgName == MSG_ORDER) {
    handleMsgOrder(req, std::move(callback), doc, reqBody);
  } else if (msgName == MSG_CANCEL_ORDER) {
    handleMsgCancelOrder(req, std::move(callback), doc, reqBody);
  } else if (msgName == MSG_START_STG) {
    handleMsgStartStg(req, std::move(callback), doc, reqBody);
  } else if (msgName == MSG_STOP_STG) {
    handleMsgStopStg(req, std::move(callback), doc, reqBody);
  } else if (msgName == MSG_GET_STG_STATUS) {
    handleMsgGetStgStatus(req, std::move(callback), doc, reqBody);
  } else if (msgName == MSG_QUERY_PNL) {
    handleMsgQueryPnl(req, std::move(callback), doc, reqBody);
  } else {
    LOG_W("Unhandled msg {}.", msgName);
  }
}

void CommonInterface::handleMsgExecSql(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody, WriteLog writeLog) {
  //
  bool needCacheDBRet = false;
  bool getDBRetFromCache = false;
  std::string dbRet = "";

  //! 先从缓存获取是否有数据库执行结果
  const auto sql = doc["sql"].GetString();
  std::tie(needCacheDBRet, getDBRetFromCache, dbRet) =
      WebSrv::get_mutable_instance().getCacheOfDBRet()->get(sql);

  //! 如果从缓存获取数据失败那么从数据库查询
  if (getDBRetFromCache == false) {
    const auto identity = GET_RAND_STR();
    int ret = 0;
    std::tie(ret, dbRet) =
        WebSrv::get_mutable_instance().getDBEng()->syncExec(identity, sql);
    if (ret != 0) {
      const auto statusCode = SCODE_WEB_SRV_EXEC_DB_CMD_FAILED;
      const auto statusMsg =
          fmt::format("Exec common interface failed. {}", dbRet);
      const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
      callback(resp);
      return;
    }
  }

  if (needCacheDBRet) {
    WebSrv::get_mutable_instance().getCacheOfDBRet()->cache(sql, dbRet);
  }

  const auto resp =
      MakeCommonHttpResp(SCODE_SUCCESS, "", dbRet, reqBody, writeLog);
  callback(resp);

  if (dbRet.size() < 0x400) {
    LOG_D("Send http response. {}", dbRet)
  } else {
    LOG_D("Send http response. [size = {}]", dbRet.size())
  }
}

//! 手工下单
void CommonInterface::handleMsgOrder(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  WebSrv::get_mutable_instance().getReqBody2CallbackGroup()->cacheCallback(
      reqBody, std::move(callback));
  forwardReqToStg(req, doc, reqBody, STG_OF_MANUAL, MSG_ID_ON_STG_MANUAL_ORDER);
}

//! 手工撤单
void CommonInterface::handleMsgCancelOrder(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  WebSrv::get_mutable_instance().getReqBody2CallbackGroup()->cacheCallback(
      reqBody, std::move(callback));
  forwardReqToStg(req, doc, reqBody, STG_OF_MANUAL,
                  MSG_ID_ON_STG_MANUAL_CANCEL_ORDER);
}

void CommonInterface::handleMsgStartStg(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  const auto userId = doc["userId"].GetUint();
  const auto stgId = doc["stgId"].GetUint();
  const auto startCmd = doc["startCmd"].GetString();
  const auto [statusCode, statusMsg] =
      WebSrv::get_mutable_instance().getStgMgr()->startStg(userId, stgId,
                                                           startCmd);
  const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
  callback(resp);
}

void CommonInterface::handleMsgStopStg(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  const auto stgId = doc["stgId"].GetUint();
  const auto [statusCode, statusMsg] =
      WebSrv::get_mutable_instance().getStgMgr()->stopStg(stgId);
  const auto resp = MakeCommonHttpResp(statusCode, statusMsg, "", reqBody);
  callback(resp);
}

void CommonInterface::handleMsgGetStgStatus(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  const auto rspBody = WebSrv::get_mutable_instance().getStgMgr()->toJson();
  const auto resp =
      MakeCommonHttpResp(SCODE_SUCCESS, "", rspBody, reqBody, WriteLog::False);
  callback(resp);
}

void CommonInterface::handleMsgQueryPnl(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback, Doc &doc,
    const std::string &reqBody) const {
  //
  auto makeStgInstId2PnlGroup = [](const auto key2PnlGroup,
                                   const std::string &prefix) {
    std::map<StgInstId, PnlSPtr> stgInstId2PnlGroup;
    for (const auto &rec : *key2PnlGroup) {
      const auto &key = rec.first;
      if (!boost::starts_with(key, prefix)) {
        continue;
      }

      const auto &pnl = rec.second;
      const auto [statusCode, statusMsg, conditionTemplate] =
          MakeConditionTemplate(pnl->queryCond_);
      if (statusCode != 0) {
        LOG_W("[PNL] Make condition template failed. [{} - {}]", statusCode,
              statusMsg);
        return std::make_tuple(statusCode, stgInstId2PnlGroup);
      }
      const auto iter = conditionTemplate.find("stgInstId");
      if (iter == std::end(conditionTemplate)) {
        LOG_W("[PNL] No field of stgInstId in query condition of pnl {}",
              pnl->queryCond_);
        return std::make_tuple(-1, stgInstId2PnlGroup);
      }

      const auto stgInstId = CONV(StgInstId, iter->second);
      stgInstId2PnlGroup.emplace(stgInstId, pnl);
      LOG_D("[PNL] Get pnl of stg inst id {}. {}", stgInstId, pnl->toStr());
    }
    return std::make_tuple(0, stgInstId2PnlGroup);
  };

  auto makeRspBody = [](StgId stgId, const auto &stgInstId2PnlGroup) {
    rapidjson::StringBuffer strBuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
    writer.StartObject();
    writer.Key("pnlGroup");
    writer.StartArray();
    for (const auto &rec : stgInstId2PnlGroup) {
      const auto stgInstId = rec.first;
      const auto &pnl = rec.second;
      writer.StartObject();
      writer.Key("stgId");
      writer.Uint(stgId);
      writer.Key("stgInstId");
      writer.Uint(stgInstId);
      writer.Key("pnlUnReal");
      writer.Double(pnl->pnlUnReal_);
      writer.Key("pnlReal");
      writer.Double(pnl->pnlReal_);
      writer.Key("fee");
      writer.Double(pnl->fee_);
      writer.Key("updateTime");
      writer.String(ConvertTsToDBTime(pnl->updateTime_).c_str());
      writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
    return std::string(strBuf.GetString());
  };

  const auto groupCond = fmt::format("stgId{}stgInstId", SEP_OF_COND_AND);
  const auto [sCodeOfQuery, key2PnlGroup] =
      WebSrv::get_mutable_instance().queryPnlGroupBy(groupCond);
  if (sCodeOfQuery != 0) {
    LOG_W("[PNL] Query pnl group by {} failed. [{} - {}]", groupCond,
          sCodeOfQuery, GetStatusMsg(sCodeOfQuery));
    return;
  }
  if (!key2PnlGroup) {
    LOG_W("[PNL] Query pnl group by {} failed because of result is nullptr. ",
          groupCond);
    return;
  }

  const auto stgId = doc["stgId"].GetUint();
  const auto prefix = fmt::format("stgId={}{}", stgId, SEP_OF_COND_AND);

  const auto [sCodeOfMakeS2P, stgInstId2PnlGroup] =
      makeStgInstId2PnlGroup(key2PnlGroup, prefix);
  if (sCodeOfMakeS2P != 0) {
    return;
  }

  const auto rspBody = makeRspBody(stgId, stgInstId2PnlGroup);
  LOG_D("[PNL] Get pnl. {}", rspBody);

  const auto resp =
      MakeCommonHttpResp(SCODE_SUCCESS, "", rspBody, reqBody, WriteLog::False);
  callback(resp);
}

void CommonInterface::forwardReqToStg(const HttpRequestPtr &req, Doc &doc,
                                      const std::string &reqBody, StgId stgId,
                                      MsgId msgId) const {
  WebSrv::get_mutable_instance().getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
      [&](void *shmBuf) {
        auto commonIPCData = static_cast<CommonIPCData *>(shmBuf);
        memcpy(commonIPCData->data_, reqBody.c_str(), reqBody.size());
      },
      stgId, msgId, sizeof(CommonIPCData) + reqBody.size() + 1);

  LOG_I("Forward http request. {}", reqBody)
}

bq::WriteLog CommonInterface::checkIfWriteLog(
    const std::string &reqBody) const {
  WriteLog writeLog = WriteLog::False;
  if (boost::contains(reqBody, "uspTrdPosOfUserTrdAcctGet") ||
      boost::contains(reqBody, "uspOrderInfoGetUnClosed") ||
      boost::contains(reqBody, "uspOrderInfoGetClosed") ||
      boost::contains(reqBody, "uspStgLoggerInfoOnePage") ||
      boost::contains(reqBody, "uspUnClosedOrderOfStgInstOnePage") ||
      boost::contains(reqBody, "uspClosedOrderOfStgInstOnePage") ||
      boost::contains(reqBody, "uspPosInfoStgInstOnePage") ||
      boost::contains(reqBody, "uspUnClosedOrderOfStgOnePage") ||
      boost::contains(reqBody, "uspClosedOrderOfStgOnePage") ||
      boost::contains(reqBody, "uspPosInfoStgOnePage") ||
      boost::contains(reqBody, "uspStgGetOnePage") ||
      boost::contains(reqBody, "uspStgInstGetOnePage") ||
      boost::contains(reqBody, "getStgStatus") ||
      boost::contains(reqBody, "queryPnl")) {
    writeLog = WriteLog::False;
  } else {
    writeLog = WriteLog::True;
  }
  return writeLog;
}
