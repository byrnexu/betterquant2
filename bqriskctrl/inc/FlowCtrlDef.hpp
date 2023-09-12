/*!
 * \file FlowCtrlDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "FlowCtrlConst.hpp"
#include "db/TBLRiskCtrlTriggerInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"

namespace bq::db::flowCtrlRule {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::flowCtrlRule

namespace bq {

using TSQue = std::queue<std::uint64_t, boost::circular_buffer<std::uint64_t>>;

//! LimitValue用于表示单位时间内流控的时候充当ConditionValue2LimitValueGroup的值
//! 的模板
struct LimitValue {
 public:
  LimitValue() = default;
  LimitValue(Decimal value) : value_(value) {}

  Decimal value_{0};
  std::uint32_t msInterval_;
  TSQue tsQue_;

  std::string toStr() const {
    const auto ret = fmt::format("value: {}; msInterval: {}; tsQueSize: {}",
                                 value_, msInterval_, tsQue_.size());
    return ret;
  }

 public:
  void reset(std::uint32_t queueSize) {
    for (std::uint32_t i = 0; i < queueSize; ++i) {
      tsQue_.push(UNDEFINED_FIELD_MIN_TS / 1000);
    }
  }
};

using TSQueInSHM =
    std::queue<std::uint64_t,
               boost::circular_buffer<std::uint64_t, Uint64Alloc>>;
using TSQueInSHMDelType = bip::deleter<TSQueInSHM, SegmentMgr>;
using TSQueInSHMSPtr =
    bip::shared_ptr<TSQueInSHM, VoidAlloc, TSQueInSHMDelType>;

struct LimitValueInSHM {
 public:
  LimitValueInSHM() = default;
  LimitValueInSHM(Decimal value) : value_(value) {}

  Decimal value_{0};
  std::uint32_t msInterval_;
  TSQueInSHMSPtr tsQue_;

 public:
  void reset(std::uint32_t queueSize) {
    for (std::uint32_t i = 0; i < queueSize; ++i) {
      tsQue_->push(UNDEFINED_FIELD_MIN_TS / 1000);
    }
  }
};

//! map of {{"acctId=10000&marketCode=SSE",  LimitValueInSHM*}}
using ConditionValue2LimitValueGroup = std::map<std::string, LimitValueInSHM*>;

struct FlowCtrlRule {
  std::uint32_t no_;
  std::string name_;

  std::string step_;
  FlowCtrlTarget target_;

  //! acctId=1&stgId=10000
  std::string condition_;

  //! ["acctId", "stgId"]
  ConditionFieldGroup conditionFieldGroup_;

  //! {{"acctId": "*"}, {"stgId": "10000"}}
  ConditionTemplate conditionTemplate_;

  //! std::map<std::string, LimitValue>; 各种acctId和stgId组合的流控状态缓存
  ConditionValue2LimitValueGroup conditionValue2LimitValueGroup_;

  //! 每次、总数或者单位时间内的限制
  FlowCtrlLimitType limitType_;

  LimitValue limitValueInConf_;

  std::string action_;
  std::vector<FlowCtrlAction> actionGroup_;

  bool contains(FlowCtrlAction action) {
    const auto iter =
        std::find(std::begin(actionGroup_), std::end(actionGroup_), action);
    if (iter != std::end(actionGroup_)) return true;
    return false;
  }

  std::string prefixOfLimitValueKey() const {
    std::string ret = fmt::format("{}-{}-{}-{}-{}-", no_, name_, step_,
                                  magic_enum::enum_name(target_), condition_);
    return ret;
  }

  std::string toStr() {
    if (!str_.empty()) return str_;
    const auto name = name_.empty() ? R"("")" : name_;
    str_ =
        fmt::format("no: {}; name: {}; step: {}; target: {}; condition: {}; {}",
                    no_, name, step_, magic_enum::enum_name(target_),
                    condition_, limitValueInConf_.toStr());
    return str_;
  }

  // clang-format off
  std::string getSqlOfInsert(const std::string& conditionValue) const {
    const auto sql = fmt::format(
        "INSERT INTO {} ("
        "`no`,"
        "`name`,"
        "`details`"
        ")"
        "VALUES"
        "("
        " {} ,"  // no
        "'{}',"  // name
        "'{}'"   // details
        "); ",
      TBLRiskCtrlTriggerInfo::TableName,
      no_,
      name_,
      toJson()
    );

    return sql;
  }
  // clang-format on

  std::string toJson() const;

 private:
  std::string str_;
  std::string jsonStr_;
};
using FLowCtrlRuleSPtr = std::shared_ptr<FlowCtrlRule>;

using Target2FlowCtrlRuleGroup =
    std::multimap<FlowCtrlTarget, FLowCtrlRuleSPtr>;
using Target2FlowCtrlRuleGroupSPtr = std::shared_ptr<Target2FlowCtrlRuleGroup>;

using No2FlowCtrlRuleGroup = std::map<std::uint32_t, FLowCtrlRuleSPtr>;
using No2FlowCtrlRuleGroupSPtr = std::shared_ptr<No2FlowCtrlRuleGroup>;

using RecFlowCtrlRuleGroup = std::vector<bq::db::flowCtrlRule::RecordSPtr>;

//! 决定是否开启相关流控
struct FlowCtrlTargetState {
  //! 单笔下单数量
  bool orderSizeEachTime_{false};
  //! 累计下单数量
  bool orderSizeTotal_{false};

  //! 单笔下单金额
  bool orderAmtEachTime_{false};
  //! 累计下单金额
  bool orderAmtTotal_{false};

  //! 累计下单笔数
  bool orderTimesTotal_{false};
  //! 单位时间下单笔数
  bool orderTimesWithinTime_{false};

  //! 累计撤单笔数
  bool cancelOrderTimesTotal_{false};
  //! 单位时间撤单笔数
  bool cancelOrderTimesWithinTime_{false};

  //! 累计被拒单笔数
  bool rejectOrderTimesTotal_{false};
  //! 单位时间被拒单笔数
  bool rejectOrderTimesWithinTime_{false};

  //! 累计成交数量
  bool holdVolTotal_{false};
  //! 累计成交金额
  bool holdAmtTotal_{false};

  //! 当日累计开仓
  bool openTDayTotal_{false};
};
using FlowCtrlTargetStateSPtr = std::shared_ptr<FlowCtrlTargetState>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

}  // namespace bq
