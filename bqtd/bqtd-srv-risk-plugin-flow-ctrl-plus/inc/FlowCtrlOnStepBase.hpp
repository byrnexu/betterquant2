/*!
 * \file FlowCtrlOnStepBase.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/ConditionDef.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

enum class FlowCtrlTarget : uint8_t;

struct FlowCtrlRule;
using FlowCtrlRuleSPtr = std::shared_ptr<FlowCtrlRule>;

struct LimitValueInSHM;
}  // namespace bq

namespace bq::td::srv {

class TDSrvRiskPluginFlowCtrlPlus;

enum class UpdateStgOfLimitValue { CompareAndUpdate = 1, Compare, Update };

class FlowCtrlOnStepBase {
 public:
  FlowCtrlOnStepBase(const FlowCtrlOnStepBase&) = delete;
  FlowCtrlOnStepBase& operator=(const FlowCtrlOnStepBase&) = delete;
  FlowCtrlOnStepBase(const FlowCtrlOnStepBase&&) = delete;
  FlowCtrlOnStepBase& operator=(const FlowCtrlOnStepBase&&) = delete;

  FlowCtrlOnStepBase(TDSrvRiskPluginFlowCtrlPlus* plugin) : plugin_(plugin) {}

  std::tuple<int, std::string> handleFlowCtrl(const OrderInfoSPtr& orderInfo,
                                              std::uint32_t combNo,
                                              std::uint32_t threadNo) {
    return doHandleFlowCtrl(orderInfo, combNo, threadNo);
  }

 private:
  virtual std::tuple<int, std::string> doHandleFlowCtrl(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo) = 0;

 protected:
  std::tuple<int, std::string> handleFlowCtrlForTarget(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, FlowCtrlTarget target,
      UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value = 0);

 private:
  std::tuple<bool, std::string> checkIfTriggerFlowCtrl(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt,
      UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value);

 private:
  std::tuple<bool, std::string> checkIfTriggerFlowCtrlOfHoldInfo(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt,
      const ConditionFieldGroup& conditionFieldGroup);

  std::tuple<bool, std::string> checkIfTriggerFlowCtrlOfOpenTDay(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt,
      const ConditionFieldGroup& conditionFieldGroup);

 private:
  std::tuple<bool, std::string> checkIfTriggerFlowCtrlForNumLimitEachTime(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt, Decimal value);

 private:
  std::tuple<bool, std::string> checkIfTriggerFlowCtrlForNumLimitTotal(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt,
      UpdateStgOfLimitValue updateStgOfLimitValue, Decimal value);

  std::tuple<bool, std::string> numLimitTotalCompareAndUpdate(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt, Decimal value);

  std::tuple<bool, std::string> numLimitTotalCompare(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt);

  void numLimitTotalUpdate(const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
                           std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
                           const std::string& conditionValueInStrFmt,
                           Decimal value);

 private:
  std::tuple<bool, std::string> checkIfTriggerFlowCtrlForNumLimitWithInTime(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt,
      UpdateStgOfLimitValue updateStgOfLimitValue);

  std::tuple<bool, std::string> numLimitWithInTimeCompareAndUpdate(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt);

  std::tuple<bool, std::string> numLimitWithInTimeCompare(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
      const std::string& conditionValueInStrFmt);

  void numLimitWithInTimeUpdate(const OrderInfoSPtr& orderInfo,
                                std::uint32_t combNo, std::uint32_t threadNo,
                                const FlowCtrlRuleSPtr& rule,
                                const std::string& conditionValueInStrFmt);

 protected:
  TDSrvRiskPluginFlowCtrlPlus* plugin_{nullptr};
};

using FlowCtrlOnStepBaseSPtr = std::shared_ptr<FlowCtrlOnStepBase>;

}  // namespace bq::td::srv
