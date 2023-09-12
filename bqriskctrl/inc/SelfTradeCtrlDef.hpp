/*!
 * \file SelfTradeCtrlDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::selfTradeCtrlRange {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::selfTradeCtrlRange

namespace bq {

using Hash2ConditionGroup = std::map<std::uint64_t, std::string>;

struct SelfTradeCtrlRange {
  //! acctId&trdAcctId
  std::string conditionGroupInStrFmt_;

  //! [acctId, trdAcctId]
  ConditionFieldGroup conditionFieldGroup_;

  //! {{8324779239794279, "acctId=10000&trdAcctId=100000"}, ...}
  Hash2ConditionGroup hash2ConditionGroup_;
};
using SelfTradeCtrlRangeSPtr = std::shared_ptr<SelfTradeCtrlRange>;

}  // namespace bq
