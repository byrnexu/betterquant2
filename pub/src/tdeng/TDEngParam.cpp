/*!
 * \file TDEngParam.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "tdeng/TDEngParam.hpp"

#include "def/Def.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq::tdeng {

TDEngParam::TDEngParam(const std::string& host, int port, const std::string& db,
                       const std::string& username, const std::string& password,
                       int connPoolSize)
    : host_(host),
      port_(port),
      db_(db),
      username_(username),
      password_(password),
      connPoolSize_(connPoolSize) {}

std::tuple<int, TDEngParamSPtr> MakeTDEngParam(
    const std::string& tdEngParamInStrFmt) {
  auto [retOfStr2Map, tdEngParamTable] = Str2Map(
      tdEngParamInStrFmt, SEP_OF_REC_IN_TDE_PARAM, SEP_OF_FIELD_IN_TDE_PARAM);
  if (retOfStr2Map != 0) {
    LOG_E("Make td engine param failed. {}", tdEngParamInStrFmt);
    return {-1, TDEngParamSPtr()};
  }

  std::string fieldName;
  std::string fieldValue;
  auto ret = std::make_shared<TDEngParam>();
  try {
    fieldName = "host";
    fieldValue = tdEngParamTable[fieldName];
    ret->host_ = fieldValue;

    fieldName = "port";
    fieldValue = tdEngParamTable[fieldName];
    ret->port_ = CONV(int, fieldValue);

    fieldName = "username";
    fieldValue = tdEngParamTable[fieldName];
    ret->username_ = fieldValue;

    fieldName = "db";
    fieldValue = tdEngParamTable[fieldName];
    ret->db_ = fieldValue;

    fieldName = "password";
    fieldValue = tdEngParamTable[fieldName];
    ret->password_ = fieldValue;

    fieldName = "connpoolsize";
    fieldValue = tdEngParamTable[fieldName];
    ret->connPoolSize_ = CONV(int, fieldValue);

  } catch (const std::exception& e) {
    LOG_E("Make td engine param failed because of invalid field info of {}. {}",
          fieldName, e.what());
    return {-1, TDEngParamSPtr()};
  }

  return {0, ret};
}

}  // namespace bq::tdeng
