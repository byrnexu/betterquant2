/*!
 * \file TBLAcctInfo.hpp
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

namespace bq::db::acctInfo {

struct FieldGroupOfKey {
  std::string marketCode;
  std::string symbolType;
  std::uint32_t acctId;
  JSER(FieldGroupOfKey, marketCode, symbolType, acctId)
};

struct FieldGroupOfVal {
  AcctGrpId acctGrpId;
  std::string acctData;
  std::string updateTime;
  JSER(FieldGroupOfVal, acctGrpId, acctData, updateTime)
};

struct FieldGroupOfAll {
  AcctGrpId acctGrpId;
  std::string marketCode;
  std::string symbolType;
  std::uint32_t acctId;
  std::string acctName;
  std::string acctData;
  std::string updateTime;
  int isDel;
  JSER(FieldGroupOfAll, acctGrpId, marketCode, symbolType, acctId, acctName,
       acctData, updateTime, isDel)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseAcctInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::acctInfo

using TBLAcctInfo = bq::db::acctInfo::TableSchema;
