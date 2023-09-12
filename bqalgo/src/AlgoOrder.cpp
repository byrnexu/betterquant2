/*!
 * \file AlgoOrder.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 */

#include "AlgoOrder.hpp"

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "AlgoMgr.hpp"
#include "AlgoUtil.hpp"
#include "StgEngImpl.hpp"
#include "db/TBLAlgoOrderInfo.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/String.hpp"

namespace bq::algo {

AlgoOrder::AlgoOrder(AlgoMgr* algoMgr, const StgInstInfoSPtr& stgInstInfo,
                     const std::string& algoType, const std::string& algoName,
                     std::uint32_t lifetime)
    : algoMgr_(algoMgr),
      algoStatus_{AlgoStatus::NotStarted},
      stgInstInfo_(stgInstInfo),
      algoId_(GET_RAND_INT()),
      algoType_(algoType),
      algoName_(algoName),
      startTime_(GetTotalSecSince1970()),
      lifetime_(lifetime) {}

int AlgoOrder::doInit(const std::string& algoParamsInJsonFmt) {
  const auto statusCode = initStatusCodeGroupOfRetryOrder(algoParamsInJsonFmt);
  if (statusCode != 0) {
    return statusCode;
  }
  return 0;
}

int AlgoOrder::initStatusCodeGroupOfRetryOrder(
    const std::string& algoParamsInJsonFmt) {
  Doc doc;
  if (doc.Parse(algoParamsInJsonFmt.data()).HasParseError()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params in json format. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("sys") || !doc["sys"].IsObject()) {
    return 0;
  }

  if (!doc["sys"].HasMember("statusCodeGroupOfRetryOrder") ||
      !doc["sys"]["statusCodeGroupOfRetryOrder"].IsArray()) {
    return 0;
  }

  for (std::size_t i = 0; i < doc["sys"]["statusCodeGroupOfRetryOrder"].Size();
       ++i) {
    if (!doc["sys"]["statusCodeGroupOfRetryOrder"][i].IsInt()) {
      getStgEng()->logInfo(
          "[ALGO] Invalid algo params of "
          "statusCodeGroupOfRetry Orderin json format. [{}] {}",
          {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
      return SCODE_ALGO_INVALID_ALGO_PARAM;
    }
    const auto statusCodeOfRetryOrder =
        doc["sys"]["statusCodeGroupOfRetryOrder"][i].GetInt();
    statusCodeGroupOfRetryOrder_.emplace(statusCodeOfRetryOrder);
  }

  if (!doc["sys"].HasMember("msIntervalOfRetryOrder") ||
      !doc["sys"]["msIntervalOfRetryOrder"].IsUint()) {
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  msIntervalOfRetryOrder_ = doc["sys"]["msIntervalOfRetryOrder"].GetUint();
  return 0;
}

std::tuple<int, TopicGroup> AlgoOrder::sub(
    const std::string& algoParamsInJsonFmt) {
  const auto [sCodeGetPreSub, topicGroupPreSub] =
      getPreSubTopicGroupFromParams(algoParamsInJsonFmt);
  if (sCodeGetPreSub != 0) {
    return {sCodeGetPreSub, TopicGroup()};
  }

  TopicGroup topicGroupAlreadySub;

  for (const auto& topic : topicGroupPreSub) {
    const auto statusCode =
        getAlgoMgr()->getStgEng()->sub(getStgInstInfo()->stgInstId_, topic);
    if (statusCode == 0) {
      getStgEng()->logInfo("[ALGO] Sub topic {} success. [{}] ",
                           {topic, toStr()}, getStgInstInfo());
      topicGroupAlreadySub.emplace(topic);
    } else if (statusCode == SCODE_BQPUB_TOPIC_ALREADY_SUB) {
      //! 重复订阅不算错误，不然一个实例启动两个算法单，第二个算法单有相同订阅会启动失败
      topicGroupAlreadySub.emplace(topic);
    } else {
      getStgEng()->logInfo("[ALGO] Sub topic {} failed. [{} - {}] [{}]",
                           {topic, std::to_string(statusCode),
                            GetStatusMsg(statusCode), toStr()},
                           getStgInstInfo());
      //! 其他的错误只是日志警告，算法单还是正常启动
      topicGroupAlreadySub.emplace(topic);
    }
  }

  const auto [sCodeOfGetNeedSub, topicGroupNeedSub] = getTopicGroupNeedSub();
  if (sCodeOfGetNeedSub != 0) {
    getStgEng()->logInfo(
        "[ALGO] Get topic group need sub failed. [{} - {}] [{}]",
        {std::to_string(sCodeOfGetNeedSub), GetStatusMsg(sCodeOfGetNeedSub),
         toStr()},
        getStgInstInfo());
    return {sCodeOfGetNeedSub, TopicGroup()};
  }

  for (const auto& topic : topicGroupNeedSub) {
    const auto statusCode =
        getAlgoMgr()->getStgEng()->sub(getStgInstInfo()->stgInstId_, topic);
    if (statusCode == 0) {
      getStgEng()->logInfo("[ALGO] Sub topic {} success. [{}] ",
                           {topic, toStr()}, getStgInstInfo());
      topicGroupAlreadySub.emplace(topic);
    } else if (statusCode == SCODE_BQPUB_TOPIC_ALREADY_SUB) {
      //! 重复订阅不算错误，不然一个实例启动两个算法单，第二个算法单有相同订阅会启动失败
      topicGroupAlreadySub.emplace(topic);
    } else {
      getStgEng()->logInfo("[ALGO] Sub topic {} failed. [{} - {}] [{}]",
                           {topic, std::to_string(statusCode),
                            GetStatusMsg(statusCode), toStr()},
                           getStgInstInfo());
      //! 其他的错误只是日志警告，算法单还是正常启动
      topicGroupAlreadySub.emplace(topic);
    }
  }

  return {0, topicGroupAlreadySub};
}

std::tuple<int, TopicGroup> AlgoOrder::unSub(
    const std::string& algoParamsInJsonFmt) {
  const auto [sCodeGetPreSub, topicGroupPreSub] =
      getPreSubTopicGroupFromParams(algoParamsInJsonFmt);
  if (sCodeGetPreSub != 0) {
    return {sCodeGetPreSub, TopicGroup()};
  }

  TopicGroup topicGroupAlreadyUnSub;
  for (const auto& topic : topicGroupPreSub) {
    const auto statusCode =
        getAlgoMgr()->getStgEng()->unSub(getStgInstInfo()->stgInstId_, topic);
    if (statusCode == 0) {
      getStgEng()->logInfo("[ALGO] Unsub topic {} success. [{}] ",
                           {topic, toStr()}, getStgInstInfo());
      topicGroupAlreadyUnSub.emplace(topic);
    } else if (statusCode == SCODE_BQPUB_TOPIC_NOT_SUB) {
      topicGroupAlreadyUnSub.emplace(topic);
    } else {
      getStgEng()->logInfo("[ALGO] Unsub topic {} failed. [{} - {}] [{}]",
                           {topic, std::to_string(statusCode),
                            GetStatusMsg(statusCode), toStr()},
                           getStgInstInfo());
      //! 其他的错误只是日志警告，不影响后续流程
      topicGroupAlreadyUnSub.emplace(topic);
    }
  }

  const auto [sCodeOfGetNeedSub, topicGroupNeedSub] = getTopicGroupNeedSub();
  if (sCodeOfGetNeedSub != 0) {
    getStgEng()->logInfo(
        "[ALGO] Get topic group need sub failed. [{} - {}] [{}]",
        {std::to_string(sCodeOfGetNeedSub), GetStatusMsg(sCodeOfGetNeedSub),
         toStr()},
        getStgInstInfo());
    return {sCodeOfGetNeedSub, TopicGroup()};
  }

  for (const auto& topic : topicGroupNeedSub) {
    const auto statusCode =
        getAlgoMgr()->getStgEng()->unSub(getStgInstInfo()->stgInstId_, topic);
    if (statusCode == 0) {
      getStgEng()->logInfo("[ALGO] Unsub topic {} success. [{}] ",
                           {topic, toStr()}, getStgInstInfo());
      topicGroupAlreadyUnSub.emplace(topic);
    } else if (statusCode == SCODE_BQPUB_TOPIC_NOT_SUB) {
      topicGroupAlreadyUnSub.emplace(topic);
    } else {
      getStgEng()->logInfo("[ALGO] Unsub topic {} failed. [{} - {}] [{}]",
                           {topic, std::to_string(statusCode),
                            GetStatusMsg(statusCode), toStr()},
                           getStgInstInfo());
      //! 其他的错误只是日志警告，不影响后续流程
      topicGroupAlreadyUnSub.emplace(topic);
    }
  }

  return {0, topicGroupAlreadyUnSub};
}

std::tuple<int, TopicGroup> AlgoOrder::getPreSubTopicGroupFromParams(
    const std::string& algoParamsInJsonFmt) {
  Doc doc;
  if (doc.Parse(algoParamsInJsonFmt.data()).HasParseError()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params in json format. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return {SCODE_ALGO_INVALID_ALGO_PARAM, TopicGroup()};
  }

  if (!doc.HasMember("sys") || !doc["sys"].IsObject()) {
    return {0, TopicGroup()};
  }

  if (!doc["sys"].HasMember("preSubTopicGroup") ||
      !doc["sys"]["preSubTopicGroup"].IsArray()) {
    return {0, TopicGroup()};
  }

  TopicGroup topicGroup;
  for (std::size_t i = 0; i < doc["sys"]["preSubTopicGroup"].Size(); ++i) {
    if (!doc["sys"]["preSubTopicGroup"][i].IsString()) {
      getStgEng()->logInfo(
          "[ALGO] Invalid algo params "
          "of preSubTopicGroup in json format. [{}] {}",
          {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
      return {SCODE_ALGO_INVALID_ALGO_PARAM, TopicGroup()};
    }

    const auto topic = doc["sys"]["preSubTopicGroup"][i].GetString();
    topicGroup.emplace(topic);
  }

  return {0, topicGroup};
}

void AlgoOrder::onTimer() {
  doOnTimer();

  if (algoOrderIsOutOfTime()) {
    //! AlgoMgr会定时检测算法单状态，设为OutOfTime后AlgoMgr会将该算法单相关资源清空
    setAlgoStatus(AlgoStatus::OutOfTime);
  }

  if (algoOrderIsFinished()) {
    setAlgoStatus(AlgoStatus::Finished);
    getStgEng()->logInfo("[ALGO] Algo order is finished. [{}]", {toStr()},
                         getStgInstInfo());
  }
}

bool AlgoOrder::algoOrderIsOutOfTime() {
  const auto now = GetTotalSecSince1970();
  const auto algoOrderExecTimeDur = now - startTime_;
  if (algoOrderExecTimeDur > lifetime_) {
    getStgEng()->logInfo(
        "[ALGO] Algo order exec time duration {} is "
        "greater than lifetime {}, so it is out of time. [{}]",
        {std::to_string(algoOrderExecTimeDur), std::to_string(lifetime_),
         toStr()},
        getStgInstInfo());
    return true;
  }
  return false;
}

void AlgoOrder::cancelAllOrders() {
  getAlgoMgr()->getStgEng()->cancelAllOrderOfAlgo(getAlgoId());
}

void AlgoOrder::syncToDB(const std::string& data) {
  if (data.empty()) return;

  const auto identity = GET_RAND_STR();

  // clang-format off
  const auto sql = fmt::format(
    "REPLACE INTO {} ("
    "`stgId`,"
    "`stgInstId`,"
    "`algoId`,"
    "`algoType`,"
    "`algoName`,"
    "`startTime`,"
    "`lifeTime`,"
    "`data`"
    ")"
  "VALUES"
  "("
    " {}, "  // stgId
    " {}, "  // stgInstId
    " {}, "  // algoId
    "'{}',"  // algoType
    "'{}',"  // algoName
    "'{}',"  // startTime
    " {} ,"  // lifeTime
    "'{}' "  // data
  "); ",
    TBLTrdAlgoOrderInfo::TableName,
    stgInstInfo_->stgId_,
    stgInstInfo_->stgInstId_,
    algoId_,
    algoType_,
    algoName_,
    ConvertTsToDBTime(startTime_ * 1e6),
    lifetime_,  
    data
  );
  // clang-format on

  const auto [ret, execRet] =
      getAlgoMgr()->getStgEng()->getDBEng()->asyncExec(identity, sql);
  if (ret != 0) {
    getStgEng()->logInfo("Sync algo order info to db failed. [{}]", {sql},
                         getStgInstInfo());
  }
}

stg::StgEngImpl* AlgoOrder::getStgEng() { return algoMgr_->getStgEng(); }

}  // namespace bq::algo
