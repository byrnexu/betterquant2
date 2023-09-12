/*!
 * \file TBLTrdSymbolList.hpp
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

namespace bq::db::trdSymbolList {

struct FieldGroupOfKey {
  std::string step;
  std::string condition;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string trdListType;
  JSER(FieldGroupOfKey, step, condition, marketCode, symbolType, symbolCode,
       trdListType)
};

struct FieldGroupOfVal {
  std::string name;
  JSER(FieldGroupOfVal, name)
};

struct FieldGroupOfAll {
  std::string step;
  std::string condition;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string trdListType;
  std::string name;
  std::string updateTime;
  JSER(FieldGroupOfAll, step, condition, marketCode, symbolType, symbolCode,
       trdListType, name, updateTime)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`riskTrdSymbolList`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::trdSymbolList

using TBLTrdSymbolList = bq::db::trdSymbolList::TableSchema;
