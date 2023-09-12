/*!
 * \file TBLAcctGrpOfSharedPos.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::acctGrpOfSharedPos {

struct FieldGroupOfKey {
  AcctId acctId;
  JSER(FieldGroupOfKey, acctId)
};

struct FieldGroupOfVal {
  AcctGrpId acctGrpId;
  JSER(FieldGroupOfVal, acctGrpId)
};

struct FieldGroupOfAll {
  AcctId acctId;
  AcctGrpId acctGrpId;
  JSER(FieldGroupOfAll, acctId, acctGrpId)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`baseAcctGrpOfSharedPos`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::acctGrpOfSharedPos

using TBLAcctGrpOfSharedPos = bq::db::acctGrpOfSharedPos::TableSchema;
