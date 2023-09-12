/*!
 * \file SelfTradeCtrlRangeMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "RiskCtrlDataMgr.hpp"
#include "SelfTradeCtrlDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class SelfTradeCtrlRangeMgr : public RiskCtrlDataMgr {
 public:
  SelfTradeCtrlRangeMgr(const SelfTradeCtrlRangeMgr&) = delete;
  SelfTradeCtrlRangeMgr& operator=(const SelfTradeCtrlRangeMgr&) = delete;
  SelfTradeCtrlRangeMgr(const SelfTradeCtrlRangeMgr&&) = delete;
  SelfTradeCtrlRangeMgr& operator=(const SelfTradeCtrlRangeMgr&&) = delete;

  using RiskCtrlDataMgr::RiskCtrlDataMgr;

 public:
  std::tuple<int, std::string> init(
      const std::vector<db::selfTradeCtrlRange::RecordSPtr>& recSet);

 public:
  std::tuple<bool, std::string> inTheSelfTradeCtrlList(
      const OrderInfoSPtr& orderInfo);

 private:
  std::tuple<int, std::string> handleTableChgTypeOfAdd(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfDel(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfChg(const Doc& doc);

  db::selfTradeCtrlRange::RecordSPtr makeRec(const Doc& doc);

 private:
  std::tuple<int, std::string> handleCurSelfTradeCtrlRec(
      const db::selfTradeCtrlRange::RecordSPtr& rec);

 private:
  // clang-format off
  //!
  //! key 是 "acctId&trdAcctId" key 就是创建的时候用到，用来归集同一类风控范围
  //! val 是 SelfTradeCtrlRangeSPtr 
  //!
  //! SelfTradeCtrlRangeSPtr
  //! {
  //!   conditionGroupInStrFmt_ : "acctId&trdAcctId" ,
  //!   conditionFieldGroup_ : ["acctId", "trdAcctId"]
  //!   hash2ConditionGroup_ : {{8324779239794279, "acctId=10000&trdAcctId=100000"}, ...}
  //! }
  //!
  // clang-format on
  std::map<std::string, SelfTradeCtrlRangeSPtr> key2SelfTradeCtrlRangeGroup_;
};

}  // namespace bq
