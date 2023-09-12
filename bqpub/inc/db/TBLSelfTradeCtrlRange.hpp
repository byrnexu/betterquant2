/*!
 * \file TBLSelfTradeCtrlRange.hpp
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

namespace bq::db::selfTradeCtrlRange {

struct FieldGroupOfKey {
  std::string step;
  std::string condition;
  JSER(FieldGroupOfKey, step, condition)
};

struct FieldGroupOfVal {
  std::string name;
  JSER(FieldGroupOfVal, name)
};

struct FieldGroupOfAll {
  std::string step;
  std::string condition;
  std::string name;
  std::string updateTime;
  JSER(FieldGroupOfAll, step, condition, name, updateTime)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`riskSelfTradeCtrlRange`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::selfTradeCtrlRange

using TBLSelfTradeCtrlRange = bq::db::selfTradeCtrlRange::TableSchema;
