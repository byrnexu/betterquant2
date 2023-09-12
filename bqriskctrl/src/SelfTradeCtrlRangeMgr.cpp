/*!
 * \file SelfTradeCtrlRangeMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#include "SelfTradeCtrlRangeMgr.hpp"

#include "db/TBLSelfTradeCtrlRange.hpp"
#include "def/ConditionUtil.hpp"
#include "def/DataStruOfTD.hpp"
#include "util/Logger.hpp"

namespace bq {

std::tuple<int, std::string> SelfTradeCtrlRangeMgr::init(
    const std::vector<db::selfTradeCtrlRange::RecordSPtr>& recSet) {
  for (const auto& rec : recSet) {
    const auto [statusCode, statusMsg] = handleCurSelfTradeCtrlRec(rec);
    if (statusCode != 0) {
      return {statusCode, statusMsg};
    }
  }
  return {0, ""};
}

std::tuple<int, std::string> SelfTradeCtrlRangeMgr::handleCurSelfTradeCtrlRec(
    const db::selfTradeCtrlRange::RecordSPtr& rec) {
  //! conditionFieldGroup ["acctId", "trdAcctId"]
  auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  const auto iter = key2SelfTradeCtrlRangeGroup_.find(conditionGroupInStrFmt);
  if (iter == std::end(key2SelfTradeCtrlRangeGroup_)) {
    auto selfTradeCtrlRange = std::make_shared<SelfTradeCtrlRange>();

    //! conditionGroupInStrFmt = acctId&trdAcctId
    selfTradeCtrlRange->conditionGroupInStrFmt_ = conditionGroupInStrFmt;
    //! conditionFieldGroup ["acctId", "trdAcctId"]
    selfTradeCtrlRange->conditionFieldGroup_ = conditionFieldGroup;

    const auto hash = XXH3_64bits(rec->condition.data(), rec->condition.size());
    selfTradeCtrlRange->hash2ConditionGroup_.emplace(hash, rec->condition);

    key2SelfTradeCtrlRangeGroup_.emplace(conditionGroupInStrFmt,
                                         selfTradeCtrlRange);
    LOG_I("Add self trade ctrl {} for {}-{}.", rec->condition, step_,
          threadNo_);

  } else {
    auto& selfTradeCtrlRange = iter->second;

    const auto hash = XXH3_64bits(rec->condition.data(), rec->condition.size());
    selfTradeCtrlRange->hash2ConditionGroup_.emplace(hash, rec->condition);
    LOG_I("Add self trade ctrl {} for {}-{}.", rec->condition, step_,
          threadNo_);
  }

  return {0, ""};
}

std::tuple<bool, std::string> SelfTradeCtrlRangeMgr::inTheSelfTradeCtrlList(
    const OrderInfoSPtr& orderInfo) {
  for (const auto& rec : key2SelfTradeCtrlRangeGroup_) {
    const auto& selfTradeCtrlRange = rec.second;

    //! orderInfo 结合条件字段 [acctId, trdAcctId] 得到 condition =
    //! acctId=10000&trdAcctId=100000
    const auto condition = MakeConditioFieldInfoInStrFmt(
        orderInfo.get(), selfTradeCtrlRange->conditionFieldGroup_);

    const auto hash = XXH3_64bits(condition.data(), condition.size());
    const auto iter = selfTradeCtrlRange->hash2ConditionGroup_.find(hash);
    if (iter != std::end(selfTradeCtrlRange->hash2ConditionGroup_)) {
      return {true, std::move(condition)};
    }
  }

  return {false, ""};
}

/*
{
        "pluginName": "risk-plugin-self-trade-ctrl",
        "tableChgType": "Add",
        "data": {
                "step": "acctId-trdAcctId",
                "condition": "acctId=10000&trdAcctId=100000",
                "name": "",
                "updateTime": "2023-03-18 20:55:32.991380"
        }
}
*/
std::tuple<int, std::string> SelfTradeCtrlRangeMgr::handleTableChgTypeOfAdd(
    const Doc& doc) {
  const auto rec = makeRec(doc);
  return handleCurSelfTradeCtrlRec(rec);
}

std::tuple<int, std::string> SelfTradeCtrlRangeMgr::handleTableChgTypeOfDel(
    const Doc& doc) {
  auto rec = makeRec(doc);
  //! conditionFieldGroup ["acctId", "trdAcctId"]
  auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  const auto iter = key2SelfTradeCtrlRangeGroup_.find(conditionGroupInStrFmt);
  if (iter == std::end(key2SelfTradeCtrlRangeGroup_)) {
    const auto statusMsg =
        fmt::format("Can't find rec when handle table chg type of del. {}",
                    conditionGroupInStrFmt);
    return {-1, statusMsg};
  }

  auto& selfTradeCtrlRange = iter->second;
  //! rec->condition == acctId=10000&trdAcctId=100000
  const auto hash = XXH3_64bits(rec->condition.data(), rec->condition.size());
  const auto iterRange = selfTradeCtrlRange->hash2ConditionGroup_.find(hash);
  if (iterRange == std::end(selfTradeCtrlRange->hash2ConditionGroup_)) {
    const auto statusMsg = fmt::format(
        "Can't find rec when handle table chg type of del. {}", rec->condition);
    return {-1, statusMsg};
  }

  selfTradeCtrlRange->hash2ConditionGroup_.erase(iterRange);
  LOG_I("Del self trade ctrl {} for {}-{}.", rec->condition, step_, threadNo_);

  return {0, ""};
}

std::tuple<int, std::string> SelfTradeCtrlRangeMgr::handleTableChgTypeOfChg(
    const Doc& doc) {
  int statusCode = 0;
  std::string statusMsg;

  std::tie(statusCode, statusMsg) = handleTableChgTypeOfDel(doc);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  return handleTableChgTypeOfAdd(doc);
}

db::selfTradeCtrlRange::RecordSPtr SelfTradeCtrlRangeMgr::makeRec(
    const Doc& doc) {
  auto rec = std::make_shared<db::selfTradeCtrlRange::Record>();
  rec->step = doc["data"]["step"].GetString();
  rec->condition = doc["data"]["condition"].GetString();
  rec->name = doc["data"]["name"].GetString();
  rec->updateTime = doc["data"]["updateTime"].GetString();
  return rec;
}

}  // namespace bq
