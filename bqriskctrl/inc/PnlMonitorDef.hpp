/*!
 * \file PnlMonitorDef.hpp
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

namespace bq::db::pnlMonitorRange {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::pnlMonitorRange

namespace bq {

enum class PnlType : std::uint8_t;

struct PnlThreshold {
  //! acctId=10000&trdAcctId=100000
  PnlThreshold(const std::string& condition, PnlType pnlType,
               Decimal limitValue)
      : condition_(condition), pnlType_(pnlType), limitValue_(limitValue) {}
  std::string condition_;
  PnlType pnlType_;
  Decimal limitValue_;
};
using PnlThresholdSPtr = std::shared_ptr<PnlThreshold>;
using Hash2PnlThresholdGroup = std::map<std::uint64_t, PnlThresholdSPtr>;

struct PnlMonitorRange {
  //! acctId&trdAcctId
  std::string conditionGroupInStrFmt_;

  //! [acctId, trdAcctId]
  ConditionFieldGroup conditionFieldGroup_;

  //! {{8324779239794279, PnlThreshold }, ...}
  Hash2PnlThresholdGroup hash2PnlThresholdGroup_;
};
using PnlMonitorRangeSPtr = std::shared_ptr<PnlMonitorRange>;

}  // namespace bq
