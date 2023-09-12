/*!
 * \file TBLTrdAcctInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::db::trdAcctInfo {

struct FieldGroupOfKey {
  std::uint32_t trdAcctId;
  JSER(FieldGroupOfKey, trdAcctId)
};

struct FieldGroupOfVal {
  std::uint32_t acctId;
  std::string trdAcctName;
  std::string updateTime;
  JSER(FieldGroupOfVal, acctId, trdAcctName, updateTime)
};

struct FieldGroupOfAll {
  std::uint32_t acctId;
  std::uint32_t trdAcctId;
  std::string trdAcctName;
  std::string updateTime;
  JSER(FieldGroupOfAll, acctId, trdAcctId, trdAcctName, trdAcctName, updateTime)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseTrdAcctInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::trdAcctInfo

using TBLTrdAcctInfo = bq::db::trdAcctInfo::TableSchema;
