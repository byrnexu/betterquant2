/*!
 * \file FLowCtrlRuleTable.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/11
 *
 * \brief
 */

#include "FlowCtrlRuleMgr.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlDef.hpp"
#include "db/DBE.hpp"
#include "db/TBLFlowCtrlRule.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"
#include "def/ConditionConst.hpp"
#include "def/ConditionUtil.hpp"
#include "util/BQUtil.hpp"
#include "util/Json.hpp"
#include "util/Random.hpp"

namespace bq {

FlowCtrlRuleMgr::FlowCtrlRuleMgr(const std::string& step,
                                 std::uint32_t threadNo,
                                 const std::string& fieldGroupUsedToGenHash)
    : RiskCtrlDataMgr(step, threadNo, fieldGroupUsedToGenHash),
      target2FlowCtrlRuleGroup_(std::make_shared<Target2FlowCtrlRuleGroup>()),
      no2FlowCtrlRuleGroup_(std::make_shared<No2FlowCtrlRuleGroup>()) {}

std::tuple<int, std::string> FlowCtrlRuleMgr::init(
    const RecFlowCtrlRuleGroup& recFlowCtrlRuleGroup) {
  //! 初始化 Target2FlowCtrlRuleGroup
  if (const auto [statusCode, statusMsg] =
          initTarget2FlowCtrlRuleGroup(recFlowCtrlRuleGroup);
      statusCode != 0) {
    return {statusCode, statusMsg};
  }
  return {0, ""};
}

std::tuple<int, std::string> FlowCtrlRuleMgr::initTarget2FlowCtrlRuleGroup(
    const RecFlowCtrlRuleGroup& recFlowCtrlRuleGroup) {
  for (const auto& rec : recFlowCtrlRuleGroup) {
    const auto [statusCode, statusMsg] = handleCurFlowCtrlRuleRec(rec);
    if (statusCode != 0) {
      return {statusCode, statusMsg};
    }
  }

  return {0, ""};
}

std::tuple<int, std::string> FlowCtrlRuleMgr::handleCurFlowCtrlRuleRec(
    const db::flowCtrlRule::RecordSPtr& rec,
    const ConditionValue2LimitValueGroup& conditionValue2LimitValueGroup) {
  auto rule = std::make_shared<FlowCtrlRule>();

  rule->no_ = rec->no;
  rule->name_ = rec->name;

  //! 获取 rule->target_
  const auto target = magic_enum::enum_cast<FlowCtrlTarget>(rec->target);
  if (target.has_value()) {
    rule->target_ = target.value();
  } else {
    const auto statusMsg = fmt::format(
        "Init flow ctrl rule mgr failed "
        "because of invalid target {} in db.",
        rec->target);
    return {-1, statusMsg};
  }

  //! 获取 rule->step_
  rule->step_ = rec->step;

  //! 获取 rule->actionGroup_ 此字段已经废弃
  std::vector<std::string> actionInStrFmtGroup;
  rule->action_ = rec->action;
  boost::split(actionInStrFmtGroup, rule->action_,
               boost::is_any_of(SEP_OF_COND_AND));
  if (actionInStrFmtGroup.empty()) {
    const auto statusMsg = fmt::format(
        "Init flow ctrl rule mgr failed because of empty action in db.");
    return {-1, statusMsg};
  }
  for (const auto& actionInStrFmt : actionInStrFmtGroup) {
    const auto action = magic_enum::enum_cast<FlowCtrlAction>(actionInStrFmt);
    if (action.has_value()) {
      rule->actionGroup_.emplace_back(action.value());
    } else {
      const auto statusMsg = fmt::format(
          "Init flow ctrl rule mgr failed "
          "because of invalid action {} in db.",
          rule->action_);
      return {-1, statusMsg};
    }
  }

  //! 获取 rule->condition_，这个字段用于入库
  rule->condition_ = rec->condition;

  int statusCode = 0;
  std::string statusMsg;

  //! 获取 rule->conditionFieldGroup_ ["marketCode", "symbolCode", "acctId"]
  std::tie(statusCode, statusMsg, rule->conditionFieldGroup_) =
      MakeConditionFieldGroup(rule->condition_);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! 获取 rule->conditionTemplate_ {{"acctId":"1"},{"marketCode":"SZSE"}}
  std::tie(statusCode, statusMsg, rule->conditionTemplate_) =
      MakeConditionTemplate(rule->condition_);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! 获取 rule->limitType_
  std::tie(statusCode, statusMsg, rule->limitType_) =
      GetLimitType(rule->target_);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! 获取 rule->limitValueInConf_
  std::tie(statusCode, statusMsg, rule->limitValueInConf_) =
      MakeLimitValue(rule->limitType_, rec->limitValue);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! 恢复 conditionValue2LimitValueGroup
  if (!conditionValue2LimitValueGroup.empty()) {
    rule->conditionValue2LimitValueGroup_ = conditionValue2LimitValueGroup;
    LOG_I("Restore conditionValue to limitValueGroup for rule {}. [size = {}]",
          rule->toStr(), conditionValue2LimitValueGroup.size());
  }

  target2FlowCtrlRuleGroup_->emplace(rule->target_, rule);
  no2FlowCtrlRuleGroup_->emplace(rule->no_, rule);

  LOG_D("Add flow ctrl rule of {}-{}. {}", step_, threadNo_, rule->toStr());
  return {0, ""};
}

/*
{
        "pluginName": "risk-plugin-flow-ctrl-plus",
        "tableChgType": "Add",
        "data": {
                "step": "acctId",
                "target": "OrderSizeEachTime",
                "condition": "acctId=10000",
                "limitValue": "1",
                "action": "RejectOrder&PubTopic",
                "no": 10001,
                "name": "OrderSizeEachTime"
        }
}
*/
std::tuple<int, std::string> FlowCtrlRuleMgr::handleTableChgTypeOfAdd(
    const Doc& doc) {
  const auto rec = makeRec(doc);
  return handleCurFlowCtrlRuleRec(rec);
}

std::tuple<int, std::string> FlowCtrlRuleMgr::handleTableChgTypeOfDel(
    const Doc& doc) {
  const auto rec = makeRec(doc);
  for (auto iter = std::begin(*target2FlowCtrlRuleGroup_);
       iter != std::end(*target2FlowCtrlRuleGroup_); ++iter) {
    auto& rule = iter->second;
    if (rec->step == rule->step_ &&  //
        rec->condition == rule->condition_ &&
        rec->target == ENUM_TO_STR(rule->target_)) {
      LOG_I("Del flow ctrl rule of {}-{}. {}", step_, threadNo_, rule->toStr());
      no2FlowCtrlRuleGroup_->erase(rule->no_);
      target2FlowCtrlRuleGroup_->erase(iter);
      return {0, ""};
    }
  }
  LOG_W("Del flow ctrl rule of {}-{} failed. {}", step_, threadNo_,
        ConvertDocToJsonStr(doc));
  return {-1, ""};
}

std::tuple<int, std::string> FlowCtrlRuleMgr::handleTableChgTypeOfChg(
    const Doc& doc) {
  int statusCode = 0;
  std::string statusMsg;

  //! ConditionValue2LimitValueGroup 保存下来用于恢复
  const auto rec = makeRec(doc);
  ConditionValue2LimitValueGroup conditionValue2LimitValueGroup;
  for (auto iter = std::begin(*target2FlowCtrlRuleGroup_);
       iter != std::end(*target2FlowCtrlRuleGroup_); ++iter) {
    auto& rule = iter->second;
    if (rec->step == rule->step_ &&  //
        rec->condition == rule->condition_ &&
        rec->target == ENUM_TO_STR(rule->target_)) {
      conditionValue2LimitValueGroup = rule->conditionValue2LimitValueGroup_;
      LOG_I("Del flow ctrl rule of {}-{}. {}", step_, threadNo_, rule->toStr());
      no2FlowCtrlRuleGroup_->erase(rule->no_);
      target2FlowCtrlRuleGroup_->erase(iter);
      break;
    }
  }

  std::tie(statusCode, statusMsg) =
      handleCurFlowCtrlRuleRec(rec, conditionValue2LimitValueGroup);

  return {statusCode, statusMsg};
}

db::flowCtrlRule::RecordSPtr FlowCtrlRuleMgr::makeRec(const Doc& doc) {
  auto rec = std::make_shared<db::flowCtrlRule::Record>();
  rec->step = doc["data"]["step"].GetString();
  rec->target = doc["data"]["target"].GetString();
  rec->condition = doc["data"]["condition"].GetString();
  rec->limitValue = doc["data"]["limitValue"].GetString();
  rec->action = doc["data"]["action"].GetString();
  rec->no = doc["data"]["no"].GetInt();
  rec->name = doc["data"]["name"].GetString();
  return rec;
}

}  // namespace bq
