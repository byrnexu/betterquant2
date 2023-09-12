/*!
 * \file TBLPnlMonitorRange.hpp
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

namespace bq::db::pnlMonitorRange {

struct FieldGroupOfKey {
  std::string step;
  std::string condition;
  std::string pnlType;
  JSER(FieldGroupOfKey, step, condition, pnlType)
};

struct FieldGroupOfVal {
  std::string limitValue;
  JSER(FieldGroupOfVal, limitValue)
};

struct FieldGroupOfAll {
  std::string step;
  std::string condition;
  std::string pnlType;
  std::string limitValue;
  std::string name;
  std::string updateTime;
  JSER(FieldGroupOfAll, step, condition, pnlType, limitValue, name, updateTime)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`riskPnlMonitorRange`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::pnlMonitorRange

using TBLPnlMonitorRange = bq::db::pnlMonitorRange::TableSchema;
