/*!
 * \file TBLStgInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "util/Pch.hpp"

namespace bq::db::stgInfo {

struct FieldGroupOfKey {
  StgId stgId;
  JSER(FieldGroupOfKey, stgId)
};

struct FieldGroupOfVal {
  StgGrpId stgGrpId;
  std::string stgName;
  std::string stgDesc;
  std::string updateTime;
  JSER(FieldGroupOfVal, stgGrpId, stgName, stgDesc, updateTime)
};

struct FieldGroupOfAll {
  StgGrpId stgGrpId;
  StgId stgId;
  std::string stgName;
  std::string stgDesc;
  std::string updateTime;
  JSER(FieldGroupOfAll, stgGrpId, stgId, stgName, stgDesc, updateTime)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseStgInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::stgInfo

using TBLStgInfo = bq::db::stgInfo::TableSchema;
