/*!
 * \file TBLExternalStatusCode.hpp
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

namespace bq::db::externalStatusCode {

struct FieldGroupOfKey {
  std::string apiName;
  UserId userId;
  std::string externalStatusCode;

  JSER(FieldGroupOfKey, apiName, userId, externalStatusCode)
};

struct FieldGroupOfVal {
  std::string externalStatusMsg;
  int statusCode;

  JSER(FieldGroupOfVal, externalStatusMsg, statusCode)
};

struct FieldGroupOfAll {
  std::string apiName;
  UserId userId;
  std::string externalStatusCode;
  std::string externalStatusMsg;
  int statusCode;

  JSER(FieldGroupOfAll, apiName, userId, externalStatusCode, externalStatusMsg,
       statusCode)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`baseExternalStatusCode`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::externalStatusCode

using TBLExternalStatusCode = bq::db::externalStatusCode::TableSchema;
