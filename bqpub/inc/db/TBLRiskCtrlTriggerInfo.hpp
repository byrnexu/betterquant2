/*!
 * \file TBLRiskCtrlTriggerInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::riskCtrlTriggerInfo {

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`riskCtrlTriggerInfo`";
};

}  // namespace bq::db::riskCtrlTriggerInfo

using TBLRiskCtrlTriggerInfo = bq::db::riskCtrlTriggerInfo::TableSchema;
