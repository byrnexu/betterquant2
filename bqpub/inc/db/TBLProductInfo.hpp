/*!
 * \file TBLProductInfo.hpp
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

namespace bq::db::productInfo {

struct FieldGroupOfKey {
  ProductId productId;
  JSER(FieldGroupOfKey, productId)
};

struct FieldGroupOfVal {
  ProductGrpId productGrpId;
  std::string productName;
  std::string productDesc;
  std::string updateTime;
  JSER(FieldGroupOfVal, productGrpId, productName, productDesc, updateTime)
};

struct FieldGroupOfAll {
  ProductGrpId productGrpId;
  ProductId productId;
  std::string productName;
  std::string productDesc;
  std::string updateTime;
  JSER(FieldGroupOfAll, productGrpId, productId, productName, productDesc,
       updateTime)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseProductInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::productInfo

using TBLProductInfo = bq::db::productInfo::TableSchema;
