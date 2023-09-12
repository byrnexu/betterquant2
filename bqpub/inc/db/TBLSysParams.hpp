/*!
 * \file TBLSysParams.hpp
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

namespace bq::db::sysParams {

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseSysParams`";
};

}  // namespace bq::db::sysParams

using TBLSysParams = bq::db::sysParams::TableSchema;
