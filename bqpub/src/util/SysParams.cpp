/*!
 * \file SysParams.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */

#include "util/SysParams.hpp"

#include "db/DBE.hpp"
#include "db/TBLSysParams.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"

namespace bq {

void UpdateSysParams(const db::DBEngSPtr& dbEng, const std::string& paramName,
                     const std::string& paramType,
                     const std::string& paramValue) {
  if (paramName.empty()) {
    LOG_W("Update sys param failed because of empty param name.");
    return;
  }

  if (paramType.empty()) {
    LOG_W("Update sys param failed because of empty param type.");
    return;
  }

  if (paramValue.empty()) {
    LOG_W("Update sys param failed because of empty param value.");
    return;
  }

  const auto identity = GET_RAND_STR();

  // clang-format off
  const auto sql = fmt::format(
   "REPLACE INTO {} ("
     "`paramName`,"
     "`paramType`,"
     "`paramValue`"
   ")"
   "VALUES"
   "("
     "'{}',"  // paramName
     "'{}',"  // paramType
     "'{}' "  // paramValue
  "); ",
  TBLSysParams::TableName,
  paramName,
  paramType,
  paramValue
  );
  // clang-format on

  auto [statusCode, execRet] = dbEng->syncExec(identity, sql);
  if (statusCode != 0) {
    LOG_W(
        "Update sys params failed. "
        "[paramName: {}; paramType: {}; paramValue: {}]",
        paramName, paramType, paramValue);
  }
}

}  // namespace bq
