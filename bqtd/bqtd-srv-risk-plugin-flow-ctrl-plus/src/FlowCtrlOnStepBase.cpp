/*!
 * \file FlowCtrlOnStepBase.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 *
 * 功能：根据 orderInfo 和 conditionFieldGroup 生成 conditionValueInStrFmt
 * "acctId=10000&marketCode=SSE" ，然后再根据数据库里的 condition
 * "acctId=10001&marketCode=*" 字段判断 是否触发风风控，如果触发风控，那么找出
 * rule 的 c2l 中的 limitValue 也就是 c2l[conditionValueInStrFmt]
 * 判断是不是超流控
 *
 * 注意：MatchConditionTemplate 仅仅用于判断当前 orderInfo 是否符合触发风控的
 * condition，checkIfTriggerFlowCtrl 中 limitValue 的统计单位还是
 * acctId=10000&marketCode=SSE 这样精确的 key 并不是 acctId=10000&marketCode=*
 * 这样的 key，但是在 checkIfTriggerFlowCtrlOfHoldInfo 和
 * checkIfTriggerFlowCtrlOfOpenTDay 中像 acctId=10000&marketCode=*
 * 这样的风控条件 limitValue 是累加的，所以累计持仓数量、金额和当日开仓不能通过
 * acctId=10000&symbolCode= 为每个代码设定约束，而是要精确指定，当然也可以通过
 * symbolCode=IF*这样的通配符只指定同一类合约的累加限制
 *
 */

#include "FlowCtrlOnStepBase.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlRuleMgr.hpp"
#include "FlowCtrlUtil.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "SHMIPCUtil.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "TDSrvRiskPluginUtil.hpp"
#include "db/DBE.hpp"
#include "def/ConditionUtil.hpp"
#include "def/DataStruOfTD.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/SHMUtil.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

std::tuple<int, std::string> FlowCtrlOnStepBase::handleFlowCtrlForTarget(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, FlowCtrlTarget target,
    UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value) {
  const auto& target2FlowCtrlRuleGroup =
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getFlowCtrlRuleMgrGroup()[threadNo]
          ->getTarget2FlowCtrlRuleGroup();

  //! 根据 target 从 target2FlowCtrlRuleGroup 查找符合条件的记录
  const auto range = target2FlowCtrlRuleGroup->equal_range(target);

  //! 遍历 target2FlowCtrlRuleGroup 符合条件的记录
  for (auto iterRange = range.first; iterRange != range.second; ++iterRange) {
    //! target like FlowCtrlTarget::OrderSizeTotal
    const auto target = iterRange->first;
    const auto& rule = iterRange->second;

    /*
     *
     * 根据 orderInfo 和 conditionFieldGroup ["acctId", "marketCode"] 获取：
     *
     * conditionValueInStrFmt = "acctId=10000&marketCode=SSE"
     * conditionValue = {{"acctId":"10000"}, {"marketCode":"SSE"}}
     *
     * conditionValueInStrFmt 是根据 conditionFieldGroup 的顺序也就是数据库中
     * condition 的顺序生成的，因此会出现 condition 字符串中字段的顺序不一样但
     * 实际的条件一样的情况，不过这样也不影响流控的处理，有一种可能的结果就是
     * ConditionValue2LimitValueGroup 中会出现两条记录，但他们 conditionValue
     * 是一样的，只是字段顺序不同而已。
     *
     */

    const auto& conditionFieldGroup = rule->conditionFieldGroup_;
    const auto [sCodeOfMake, sMsgOfMake, conditionValueInStrFmt,
                conditionValue] =
        MakeConditioValue(orderInfo, conditionFieldGroup);
    if (sCodeOfMake != 0) {
      L_W(plugin_->logger(), "[{}] {}", plugin_->name(), sMsgOfMake);
      //! 检查下一条风控规则
      continue;
    }

    //! 检查 conditionValue 是否符合风控条件模板 conditionTemplate
    const auto [sCodeOfMatch, sMsgOfMatch, matchConditionTemplte] =
        MatchConditionTemplate(conditionValue, rule->conditionTemplate_);
    if (sCodeOfMatch != 0) {
      L_W(plugin_->logger(), "[{}] {}", plugin_->name(), sMsgOfMatch);
      //! 检查下一条风控规则
      continue;
    }

    //! 如果当前订单不符合风控条件，那么检查下一条风控规则
    if (matchConditionTemplte == false) continue;

    //! 如果当前订单符合风控条件，那么检查是否触发风控
    bool triggerRiskCtrl = false;
    std::string details;
    if (rule->target_ == FlowCtrlTarget::HoldVolTotal ||
        rule->target_ == FlowCtrlTarget::HoldAmtTotal) {
      std::tie(triggerRiskCtrl, details) = checkIfTriggerFlowCtrlOfHoldInfo(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt,
          conditionFieldGroup);

    } else if (rule->target_ == FlowCtrlTarget::OpenTDayTotal) {
      std::tie(triggerRiskCtrl, details) = checkIfTriggerFlowCtrlOfOpenTDay(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt,
          conditionFieldGroup);

    } else {
      std::tie(triggerRiskCtrl, details) = checkIfTriggerFlowCtrl(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt,
          updateStgOfLimitValue, value);
    }

    //! 如果没有触发风控，那么检查下一条风控规则
    if (triggerRiskCtrl == false) continue;

    //! 返回风控编号
    return {rule->no_, details};
  }

  return {0, ""};
}

std::tuple<bool, std::string> FlowCtrlOnStepBase::checkIfTriggerFlowCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt,
    UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value) {
  switch (rule->limitType_) {
    //! 检查每笔是否超限的流控
    case FlowCtrlLimitType::NumLimitEachTime:
      return checkIfTriggerFlowCtrlForNumLimitEachTime(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt, value);

    //! 检查总数是否超限的流控
    case FlowCtrlLimitType::NumLimitTotal:
      return checkIfTriggerFlowCtrlForNumLimitTotal(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt,
          updateStgOfLimitValue, value);

    //! 检查单位时间内数量是否超限的流控
    case FlowCtrlLimitType::NumLimitWithinTime:
      return checkIfTriggerFlowCtrlForNumLimitWithInTime(
          orderInfo, combNo, threadNo, rule, conditionValueInStrFmt,
          updateStgOfLimitValue);

    default:
      //! limitType_不合法，FlowCtrlRuleMgr::init失败，所以理论上不会进入这里
      return {false, ""};
  }
}

//! 每次的数值是否超限
std::tuple<bool, std::string>
FlowCtrlOnStepBase::checkIfTriggerFlowCtrlForNumLimitEachTime(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt, Decimal value) {
  bool triggerRiskCtrl = false;
  std::string details;
  if (DEC::GT(value, rule->limitValueInConf_.value_)) {
    triggerRiskCtrl = true;

    const auto riskCtrlMsg =
        fmt::format("Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(),
                    value, rule->limitValueInConf_.value_, orderInfo->orderId_);

    details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_), rule->no_,
                                         riskCtrlMsg, orderInfo);

    plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                 riskCtrlMsg, details);

    L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
  }

  return {triggerRiskCtrl, details};
}

//! 累计数量是否超限
std::tuple<bool, std::string>
FlowCtrlOnStepBase::checkIfTriggerFlowCtrlForNumLimitTotal(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt,
    UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value) {
  switch (updateStgOfLimitValue) {
    //! 判断累计数量是否超限并更新
    case UpdateStgOfLimitValue::CompareAndUpdate:
      return numLimitTotalCompareAndUpdate(orderInfo, combNo, threadNo, rule,
                                           conditionValueInStrFmt, value);

    //! 仅判断累计数量是否超限
    case UpdateStgOfLimitValue::Compare:
      return numLimitTotalCompare(orderInfo, combNo, threadNo, rule,
                                  conditionValueInStrFmt);

    //! 仅更新累计数量
    case UpdateStgOfLimitValue::Update:
      numLimitTotalUpdate(orderInfo, combNo, threadNo, rule,
                          conditionValueInStrFmt, value);
  }
  return {false, ""};
}

//! 判断累计数量是否超限并更新
std::tuple<bool, std::string> FlowCtrlOnStepBase::numLimitTotalCompareAndUpdate(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt, Decimal value) {
  bool triggerRiskCtrl = false;
  std::string details;

  auto& c2l = rule->conditionValue2LimitValueGroup_;
  //! 根据 "acctId=10000&marketCode=SSE" 找到 limitValue
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    //! 如果 "acctId=10000&marketCode=SSE" 对应的 limitValue 存在
    auto limitValueInSHM = iter->second;
    //! 先计算出当前 value 累加后的 newValue
    const auto newValue = limitValueInSHM->value_ + value;
    if (!DEC::GT(newValue, rule->limitValueInConf_.value_)) {
      //! 如果当前 value 累加后的 newValue 不触发风控，那么用 newValue 更新
      //! limitValueInSHM
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash([limitValueInSHM, newValue]() {
            limitValueInSHM->value_ = newValue;
          });

    } else {
      //! 如果当前 value 累加后的 newValue 触发风控
      triggerRiskCtrl = true;
      const auto riskCtrlMsg = fmt::format(
          "Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(), newValue,
          rule->limitValueInConf_.value_, orderInfo->orderId_);
      details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_),
                                           rule->no_, riskCtrlMsg, orderInfo);
      plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                   riskCtrlMsg, details);
      L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
    }
  } else {
    //! 如果该 conditionValue 是第一次进入风控检查
    if (!DEC::GT(value, rule->limitValueInConf_.value_)) {
      //! 构建或者打开 limitValueInSHM (重启服务的时候limitValueInSHM所以是打开)
      auto limitValueInSHM = plugin_->openOrCtorLimitValue(
          combNo, threadNo, rule, conditionValueInStrFmt);

      //! 设置 c2l[conditionValueInStrFmt]
      c2l[conditionValueInStrFmt] = limitValueInSHM;

      //! 重启后还会进入这个逻辑，因此下面是累加 value，而不是将 value 赋给
      //! newValue
      const auto newValue = limitValueInSHM->value_ + value;
      if (!DEC::GT(newValue, rule->limitValueInConf_.value_)) {
        //! 如果当前 value 累加后的 newValue 不触发风控，那么用 newValue 更新
        //! limitValueInSHM
        plugin_->getTDSrv()
            ->getRiskCtrlModuleComb()[combNo]
            ->getRiskCtrlStatusUpdatersGroup()[threadNo]
            ->stash([limitValueInSHM, newValue]() {
              limitValueInSHM->value_ = newValue;
            });

      } else {
        //! 如果当前 value 累加后的 newValue 触发风控
        triggerRiskCtrl = true;
        const auto riskCtrlMsg = fmt::format(
            "Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(), newValue,
            rule->limitValueInConf_.value_, orderInfo->orderId_);
        details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_),
                                             rule->no_, riskCtrlMsg, orderInfo);
        plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                     riskCtrlMsg, details);
        L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
      }

      if (c2l.size() % 100 == 0) {
        L_W(plugin_->logger(), "Size of risk ctrl list is {}. {}", c2l.size(),
            rule->toStr());
      }

    } else {
      //! 如果 value 很大直接触发风控
      triggerRiskCtrl = true;
      const auto riskCtrlMsg = fmt::format(
          "Trigger risk ctrl, cur value is "
          "greater than the accumulated value. [{}]. [{} > {}] {}",
          rule->toStr(), value, rule->limitValueInConf_.value_,
          orderInfo->orderId_);
      details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_),
                                           rule->no_, riskCtrlMsg, orderInfo);
      plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                   riskCtrlMsg, details);
      L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
    }
  }

  return {triggerRiskCtrl, details};
}

//! 仅判断累计数量是否超限
std::tuple<bool, std::string> FlowCtrlOnStepBase::numLimitTotalCompare(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt) {
  bool triggerRiskCtrl = false;
  std::string details;

  LimitValueInSHM* limitValueInSHM = nullptr;

  auto& c2l = rule->conditionValue2LimitValueGroup_;
  //! 根据 "acctId=10000&marketCode=SSE" 找到 limitValue
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    limitValueInSHM = iter->second;
  } else {
    /*
     * 如果没有下面的代码，那么假设已经到达触发风控的阈值，系统关闭重启，由于c2l
     * 中没有limitValueInSHM，因此只有再此触发一次风控c2l中产生limitValueInSHM之
     * 后才会继续触发风控。
     */
    limitValueInSHM = plugin_->openOrCtorLimitValue(combNo, threadNo, rule,
                                                    conditionValueInStrFmt);
    c2l[conditionValueInStrFmt] = limitValueInSHM;
  }

  if (DEC::GT(limitValueInSHM->value_, rule->limitValueInConf_.value_)) {
    triggerRiskCtrl = true;
    const auto riskCtrlMsg =
        fmt::format("Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(),
                    limitValueInSHM->value_, rule->limitValueInConf_.value_,
                    orderInfo->orderId_);
    details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_), rule->no_,
                                         riskCtrlMsg, orderInfo);
    plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                 riskCtrlMsg, details);
    L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
  }

  return {triggerRiskCtrl, details};
}

//! 仅更新累计数量，适用于事中风控
void FlowCtrlOnStepBase::numLimitTotalUpdate(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt, Decimal value) {
  auto& c2l = rule->conditionValue2LimitValueGroup_;

  //! 根据 "acctId=10000&marketCode=SSE" 找到 limitValue
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    //! 如果 "acctId=10000&marketCode=SSE" 对应的 limitValue 存在
    auto limitValueInSHM = iter->second;

    //! 那么更新 limitValueInSHM->value_
    plugin_->getTDSrv()
        ->getRiskCtrlModuleComb()[combNo]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->stash(
            [limitValueInSHM, value]() { limitValueInSHM->value_ += value; });

  } else {
    //! 如果该 conditionValue 是第一次进入风控检查
    auto limitValueInSHM = plugin_->openOrCtorLimitValue(
        combNo, threadNo, rule, conditionValueInStrFmt);

    //! 设置 c2l[conditionValueInStrFmt]
    c2l[conditionValueInStrFmt] = limitValueInSHM;

    //! 重启后还会进入这个逻辑，因此即使是第一次进入，下面也是累加 value
    //! 而不是赋值
    plugin_->getTDSrv()
        ->getRiskCtrlModuleComb()[combNo]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->stash(
            [limitValueInSHM, value]() { limitValueInSHM->value_ += value; });

    L_T(plugin_->logger(),
        "Add rec {} to c2l, Only update cur value to {} + {}, not compare. {}",
        conditionValueInStrFmt, limitValueInSHM->value_, value, rule->toStr());

    if (c2l.size() % 100 == 0) {
      L_W(plugin_->logger(), "Size of risk ctrl list is {}. {}", c2l.size(),
          rule->toStr());
    }
  }
}

//! 判断是否超流控
std::tuple<bool, std::string>
FlowCtrlOnStepBase::checkIfTriggerFlowCtrlForNumLimitWithInTime(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt,
    UpdateStgOfLimitValue updateStgOfLimitValue) {
  switch (updateStgOfLimitValue) {
    //! 判断是否超流控并更新流控时间队列
    case UpdateStgOfLimitValue::CompareAndUpdate:
      return numLimitWithInTimeCompareAndUpdate(orderInfo, combNo, threadNo,
                                                rule, conditionValueInStrFmt);

    //! 判断是否超流控
    case UpdateStgOfLimitValue::Compare:
      return numLimitWithInTimeCompare(orderInfo, combNo, threadNo, rule,
                                       conditionValueInStrFmt);

    //! 更新流控时间队列
    case UpdateStgOfLimitValue::Update:
      numLimitWithInTimeUpdate(orderInfo, combNo, threadNo, rule,
                               conditionValueInStrFmt);
  }
  return {false, ""};
}

//! 判断是否超流控并更新流控时间队列
std::tuple<bool, std::string>
FlowCtrlOnStepBase::numLimitWithInTimeCompareAndUpdate(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt) {
  bool triggerRiskCtrl = false;
  std::string details;

  const auto now = GetTotalMSSince1970();

  auto& c2l = rule->conditionValue2LimitValueGroup_;
  //! 根据 "acctId=10000&marketCode=SSE" 找到 limitValue
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    //! 如果 "acctId=10000&marketCode=SSE" 对应的 limitValue 存在
    auto limitValueInSHM = iter->second;
    //! 先计算出假设当前订单报出的 curMSInterval
    const auto curMSInterval = now - limitValueInSHM->tsQue_->front();
    if (curMSInterval < rule->limitValueInConf_.msInterval_) {
      //! 如果当前订单报出触发风控
      triggerRiskCtrl = true;
      const auto riskCtrlMsg = fmt::format(
          "Trigger risk ctrl [{}]. [{} < {}] {}", rule->toStr(), curMSInterval,
          rule->limitValueInConf_.msInterval_, orderInfo->orderId_);
      details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_),
                                           rule->no_, riskCtrlMsg, orderInfo);
      plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                   riskCtrlMsg, details);
      L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
    } else {
      //! 如果当前订单报出不触发风控，那么更新 tsQue
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getRiskCtrlStatusUpdatersGroup()[threadNo]
          ->stash(
              [limitValueInSHM, now]() { limitValueInSHM->tsQue_->push(now); });
      L_T(plugin_->logger(), "Stash push action. keyOfTSQue = {}, ts = {} {}",
          conditionValueInStrFmt, now, rule->toStr());
      L_T(plugin_->logger(),
          "{} {} - {} >= {} so not trigger. keyOfTSQue = {}, [tsQueSize = {}]",
          rule->no_, now, limitValueInSHM->tsQue_->front(),
          rule->limitValueInConf_.msInterval_, conditionValueInStrFmt,
          limitValueInSHM->tsQue_->size());
    }

  } else {
    //! 如果该 conditionValue 是第一次进入风控检查
    bool createTSQue = true;

    //! 构建或者打开 limitValueInSHM (重启服务的时候limitValueInSHM所以是打开)
    auto limitValueInSHM = plugin_->openOrCtorLimitValue(
        combNo, threadNo, rule, conditionValueInStrFmt, createTSQue);

    //! 设置 c2l[conditionValueInStrFmt]
    c2l[conditionValueInStrFmt] = limitValueInSHM;

    //! 更新 tsQue ，这里做了简单处理，不考虑重启以后第一次就触发风控的场景
    plugin_->getTDSrv()
        ->getRiskCtrlModuleComb()[combNo]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->stash(
            [limitValueInSHM, now]() { limitValueInSHM->tsQue_->push(now); });

    L_T(plugin_->logger(), "Stash push action. keyOfTSQue = {}, ts = {} {}",
        conditionValueInStrFmt, now, rule->toStr());

    L_T(plugin_->logger(),
        "{} {} - {} >= {} ? at first time, so not trigger. "
        "keyOfTSQue = {}, [tsQueSize = {}] ",
        rule->no_, now, limitValueInSHM->tsQue_->front(),
        rule->limitValueInConf_.msInterval_, conditionValueInStrFmt,
        limitValueInSHM->tsQue_->size());

    if (c2l.size() % 100 == 0) {
      L_W(plugin_->logger(), "Size of risk ctrl list is {}. {}", c2l.size(),
          rule->toStr());
    }
  }

  return {triggerRiskCtrl, details};
}

//! 判断是否超流控
std::tuple<bool, std::string> FlowCtrlOnStepBase::numLimitWithInTimeCompare(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt) {
  bool triggerRiskCtrl = false;
  std::string details;

  LimitValueInSHM* limitValueInSHM = nullptr;

  //! 先获取 limitValueInSHM
  const auto now = GetTotalMSSince1970();
  auto& c2l = rule->conditionValue2LimitValueGroup_;
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    limitValueInSHM = iter->second;
  } else {
    /*
     * 如果没有下面的代码，那么假设已经到达触发风控的阈值，系统关闭重启，由于c2l
     * 中没有limitValueInSHM，因此只有再此触发一次风控c2l中产生limitValueInSHM之
     * 后才会继续触发风控。
     */
    bool createTSQue = true;
    limitValueInSHM = plugin_->openOrCtorLimitValue(
        combNo, threadNo, rule, conditionValueInStrFmt, createTSQue);
    c2l[conditionValueInStrFmt] = limitValueInSHM;
  }

  //! 先计算出假设当前订单报出的 curMSInterval
  const auto curMSInterval = now - limitValueInSHM->tsQue_->front();
  if (curMSInterval < rule->limitValueInConf_.msInterval_) {
    //! 如果当前订单报出触发风控
    triggerRiskCtrl = true;
    const auto riskCtrlMsg = fmt::format(
        "Trigger risk ctrl [{}]. [{} < {}] {}", rule->toStr(), curMSInterval,
        rule->limitValueInConf_.msInterval_, orderInfo->orderId_);

    details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_), rule->no_,
                                         riskCtrlMsg, orderInfo);

    plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                 riskCtrlMsg, details);

    L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);

  } else {
    //! 如果当前订单报出不触发风控
    L_T(plugin_->logger(),
        "[{}] {} - {} >= {} keyOfTSQue = {}, [tsQueSize = {}], "
        "so not trigger. {}",
        plugin_->name(), now, limitValueInSHM->tsQue_->front(),
        rule->limitValueInConf_.msInterval_, conditionValueInStrFmt,
        limitValueInSHM->tsQue_->size(), rule->toStr());
  }

  return {triggerRiskCtrl, details};
}

//! 更新流控时间队列，适用于事中风控
void FlowCtrlOnStepBase::numLimitWithInTimeUpdate(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt) {
  const auto now = GetTotalMSSince1970();

  auto& c2l = rule->conditionValue2LimitValueGroup_;
  //! 根据 "acctId=10000&marketCode=SSE" 找到 limitValue
  const auto iter = c2l.find(conditionValueInStrFmt);
  if (iter != std::end(c2l)) {
    //! 如果 "acctId=10000&marketCode=SSE" 对应的 limitValue 存在
    auto limitValueInSHM = iter->second;
    plugin_->getTDSrv()
        ->getRiskCtrlModuleComb()[combNo]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->stash(
            [limitValueInSHM, now]() { limitValueInSHM->tsQue_->push(now); });
    L_T(plugin_->logger(), "Stash push action. keyOfTSQue = {}, ts = {} {}",
        conditionValueInStrFmt, now, rule->toStr());

  } else {
    //! 如果该 conditionValue 是第一次进入风控检查
    bool createTSQue = true;
    auto limitValueInSHM = plugin_->openOrCtorLimitValue(
        combNo, threadNo, rule, conditionValueInStrFmt, createTSQue);
    c2l[conditionValueInStrFmt] = limitValueInSHM;
    plugin_->getTDSrv()
        ->getRiskCtrlModuleComb()[combNo]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->stash(
            [limitValueInSHM, now]() { limitValueInSHM->tsQue_->push(now); });
    L_T(plugin_->logger(), "Stash push action. keyOfTSQue = {}, ts = {} {}",
        conditionValueInStrFmt, now, rule->toStr());

    if (c2l.size() % 100 == 0) {
      L_W(plugin_->logger(), "Size of risk ctrl list is {}. {}", c2l.size(),
          rule->toStr());
    }
  }
}

std::tuple<bool, std::string>
FlowCtrlOnStepBase::checkIfTriggerFlowCtrlOfHoldInfo(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt,
    const ConditionFieldGroup& conditionFieldGroup) {
  Decimal newValue = 0;

  bool triggerRiskCtrl;
  std::string details;

  //! 根据条件模糊查询获取所有仓位信息，因为是模糊查询，所以这里累加的是所有符合
  //! 条件的记录，如果明确对某一个品种做流控，需要在流控表里明确指定代码
  const auto posInfoGroup =
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getPosMgrGroup()[threadNo]
          ->getPosInfoGroup<LockFunc::False, DeepClone::False>(
              rule->conditionTemplate_, conditionFieldGroup);
  for (const auto& posInfo : posInfoGroup) {
    switch (rule->target_) {
      case FlowCtrlTarget::HoldVolTotal:
        newValue += std::fabs(posInfo->pos_);
        break;
      case FlowCtrlTarget::HoldAmtTotal:
        newValue += (std::fabs(posInfo->pos_) * posInfo->avgOpenPrice_);
        break;
      default:
        break;
    }
  }

  //! 根据条件模糊查询获取所有未完结订单，因为是模糊查询，所以这里累加的是所有符合
  //! 条件的记录，如果明确对某一个品种做流控，需要在流控表里明确指定代码
  const auto orderInfoGroup =
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getOrdMgrGroup()[threadNo]
          ->getOrderInfoGroup<LockFunc::False, DeepClone::False>(
              rule->conditionTemplate_, conditionFieldGroup);
  for (const auto& orderInfo : orderInfoGroup) {
    switch (rule->target_) {
      case FlowCtrlTarget::HoldVolTotal:
        newValue += (orderInfo->orderSize_ - std::fabs(orderInfo->dealSize_));
        break;
      case FlowCtrlTarget::HoldAmtTotal:
        newValue += ((orderInfo->orderSize_ - std::fabs(orderInfo->dealSize_)) *
                     orderInfo->orderPrice_);
        break;
      default:
        break;
    }
  }

  //! 这里不用加上当前订单的未成交数量，因为该订单已经在OrdMgr中所以上面的步骤已
  //! 经把该订单的数量或者金额累加进去了

  if (DEC::GT(newValue, rule->limitValueInConf_.value_)) {
    triggerRiskCtrl = true;
    const auto riskCtrlMsg = fmt::format(
        "Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(), newValue,
        rule->limitValueInConf_.value_, orderInfo->orderId_);
    details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_), rule->no_,
                                         riskCtrlMsg, orderInfo);
    plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                 riskCtrlMsg, details);
    L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
    return {triggerRiskCtrl, details};
  }
  L_T(plugin_->logger(), "[{}] Not trigger risk ctrl [{}]. [{} <= {}] {}",
      plugin_->name(), rule->toStr(), newValue, rule->limitValueInConf_.value_,
      orderInfo->toShortStr());

  return {false, ""};
}

std::tuple<bool, std::string>
FlowCtrlOnStepBase::checkIfTriggerFlowCtrlOfOpenTDay(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt,
    const ConditionFieldGroup& conditionFieldGroup) {
  Decimal newValue = 0;

  bool triggerRiskCtrl;
  std::string details;

  //! 根据条件模糊查询获取所有仓位信息，因为是模糊查询，所以这里累加的是所有符合
  //! 条件的记录，如果明确对某一个品种做流控，需要在流控表里明确指定代码
  const auto posInfoGroup =
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getPosMgrGroup()[threadNo]
          ->getPosInfoGroup<LockFunc::False, DeepClone::False>(
              rule->conditionTemplate_, conditionFieldGroup);
  for (const auto& posInfo : posInfoGroup) {
    newValue += std::fabs(posInfo->totalOpenSize_ - posInfo->preTotalOpenSize_);
  }

  //! 根据条件模糊查询获取所有未完结订单，因为是模糊查询，所以这里累加的是所有符合
  //! 条件的记录，如果明确对某一个品种做流控，需要在流控表里明确指定代码
  const auto orderInfoGroup =
      plugin_->getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getOrdMgrGroup()[threadNo]
          ->getOrderInfoGroup<LockFunc::False, DeepClone::False>(
              rule->conditionTemplate_, conditionFieldGroup);
  for (const auto& orderInfo : orderInfoGroup) {
    newValue += orderInfo->getUndealedSize();
  }

  //! 这里不用加上当前订单的未成交数量，因为该订单已经在OrdMgr中所以上面的步骤已
  //! 经把该订单的未成交数量累加进去了

  if (DEC::GT(newValue, rule->limitValueInConf_.value_)) {
    triggerRiskCtrl = true;
    const auto riskCtrlMsg = fmt::format(
        "Trigger risk ctrl [{}]. [{} > {}] {}", rule->toStr(), newValue,
        rule->limitValueInConf_.value_, orderInfo->toShortStr());
    details = MakeRiskCtrlTriggerDetails(ENUM_TO_STR(rule->target_), rule->no_,
                                         riskCtrlMsg, orderInfo);
    plugin_->saveTriggerInfoToDB(ENUM_TO_STR(rule->target_), rule->no_,
                                 riskCtrlMsg, details);
    L_W(plugin_->logger(), "[{}] {}", plugin_->name(), riskCtrlMsg);
    return {triggerRiskCtrl, details};
  }

  L_T(plugin_->logger(), "[{}] Not trigger risk ctrl [{}]. [{} <= {}] {}",
      plugin_->name(), rule->toStr(), newValue, rule->limitValueInConf_.value_,
      orderInfo->toShortStr());

  return {false, ""};
}

}  // namespace bq::td::srv
