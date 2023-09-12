/*!
 * \file TDEngParam.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::tdeng {

constexpr static std::string_view MODULE_TDE_PARAM = {"TDENG_PARAM"};

const static std::string SEP_OF_REC_IN_TDE_PARAM = ";";
const static std::string SEP_OF_FIELD_IN_TDE_PARAM = "=";

struct TDEngParam {
  TDEngParam() = default;
  TDEngParam(const std::string& host, int port, const std::string& db,
             const std::string& username, const std::string& password,
             int connPoolSize);
  std::string host_;
  int port_;
  std::string db_;
  std::string username_;
  std::string password_;
  int connPoolSize_{1};
};
using TDEngParamSPtr = std::shared_ptr<TDEngParam>;

std::tuple<int, TDEngParamSPtr> MakeTDEngParam(
    const std::string& dbEngParamInStrFmt);

}  // namespace bq::tdeng
