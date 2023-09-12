/*!
 * \file FLowCtrlRuleTable.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/11
 *
 * \brief
 */

#pragma once

#include "FlowCtrlConst.hpp"
#include "FlowCtrlDef.hpp"
#include "FlowCtrlUtil.hpp"
#include "RiskCtrlDataMgr.hpp"
#include "util/Pch.hpp"

// clang-format off
//! -------------------------------------------------------------------------------  
//! Target2FlowCtrlRuleGroup [std::multimap<target, FlowCtrlRule>]
//! -------------------------------------------------------------------------------  
//! |-1st rec
//! |-|-target = OrderSizeEachTime
//! | |-no     = 
//! | |-name   =       
//! | |-step   = acctId
//! | |-condition           = "acctId=*&marketCode=SSE"
//! | |-conditionFieldGroup = ["acctId", "marketCode"]
//! | |-conditionTemplate   = {{"acctId":""}, {"marketCode":"SSE"}} [std::map<std::string, std::string>] 
//! | |-conditionValue2LimitValueGroup [std::map<std::string, LimitValue>]
//! | | |- acctId=10000&marketCode=SSE -> struct LimitValue {value, msInterval, circular_buffer<std::uint64> tsGroup}
//! | | |- acctId=10001&marketCode=SSE -> struct LimitValue {value, msInterval, circular_buffer<std::uint64> tsGroup}
//! | | |- ...
//! | |-limitValue type = struct LimitValue {value, msInterval, circular_buffer<std::uint64> tsGroup}
//! | |-action = RejectOrder/PubTopic
//! | |
//! |-2nd rec
//! |-|-target = OrderSizeEachTime
//!   |- ...
//! -------------------------------------------------------------------------------
// clang-format on

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq::db::flowCtrlRule {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::flowCtrlRule

namespace bq {

class FlowCtrlRuleMgr : public RiskCtrlDataMgr {
 public:
  FlowCtrlRuleMgr(const FlowCtrlRuleMgr&) = delete;
  FlowCtrlRuleMgr& operator=(const FlowCtrlRuleMgr&) = delete;
  FlowCtrlRuleMgr(const FlowCtrlRuleMgr&&) = delete;
  FlowCtrlRuleMgr& operator=(const FlowCtrlRuleMgr&&) = delete;

  explicit FlowCtrlRuleMgr(const std::string& step, std::uint32_t threadNo,
                           const std::string& fieldGroupUsedToGenHash);

  std::tuple<int, std::string> init(
      const RecFlowCtrlRuleGroup& recFlowCtrlRuleGroup);

 private:
  std::tuple<int, std::string> handleCurFlowCtrlRuleRec(
      const db::flowCtrlRule::RecordSPtr& rec,
      const ConditionValue2LimitValueGroup& conditionValue2LimitValueGroup =
          ConditionValue2LimitValueGroup());

 public:
  const Target2FlowCtrlRuleGroupSPtr& getTarget2FlowCtrlRuleGroup() const {
    return target2FlowCtrlRuleGroup_;
  }

  const No2FlowCtrlRuleGroupSPtr& getNo2FlowCtrlRuleGroup() const {
    return no2FlowCtrlRuleGroup_;
  }

 private:
  std::tuple<int, std::string> initTarget2FlowCtrlRuleGroup(
      const RecFlowCtrlRuleGroup& recFlowCtrlRuleGroup);

 private:
  std::tuple<int, std::string> handleTableChgTypeOfAdd(const Doc& doc) final;
  std::tuple<int, std::string> handleTableChgTypeOfDel(const Doc& doc) final;
  std::tuple<int, std::string> handleTableChgTypeOfChg(const Doc& doc) final;

  db::flowCtrlRule::RecordSPtr makeRec(const Doc& doc);

 private:
  Target2FlowCtrlRuleGroupSPtr target2FlowCtrlRuleGroup_{nullptr};
  No2FlowCtrlRuleGroupSPtr no2FlowCtrlRuleGroup_{nullptr};
};

using FlowCtrlRuleMgrSPtr = std::shared_ptr<FlowCtrlRuleMgr>;

}  // namespace bq
