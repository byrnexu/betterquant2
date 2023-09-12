/*!
 * \file TrdSymbolDef.hpp
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

namespace bq::db::trdSymbolList {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::trdSymbolList

namespace bq {

struct TrdSymbol {
  explicit TrdSymbol(const db::trdSymbolList::RecordSPtr rec);
  std::string marketCode_;
  std::string symbolType_;
  std::string symbolCode_;

  std::uint64_t getHash() const;
  std::string toStr() const;
};
using TrdSymbolSPtr = std::shared_ptr<TrdSymbol>;
using Hash2TrdSymbolGroup = std::map<std::uint64_t, TrdSymbolSPtr>;

// clang-format off
/*
 * TrdSymbolList = {
 *   conditionGroupInStrFmt_ = "acctId&trdAcctId",
 *   conditionFieldGroup_ = std::vector of length 2, capacity 2 = {"acctId", "trdAcctId"}, 
 *   whiteList_ = std::map with 0 elements, 
 *   blackList_ = std::map with 0 elements
 * }
 */
// clange-format on
//
struct TrdSymbolList {
  //! acctId&trdAcctId
  std::string conditionGroupInStrFmt_;

  //! [acctId, trdAcctId]
  ConditionFieldGroup conditionFieldGroup_;

  //! key = acctId=10000&trdAcctId=100000
  std::map<std::string, Hash2TrdSymbolGroup> whiteList_;
  //! key = acctId=10000&trdAcctId=100000
  std::map<std::string, Hash2TrdSymbolGroup> blackList_;
};
using TrdSymbolListSPtr = std::shared_ptr<TrdSymbolList>;

}  // namespace bq
