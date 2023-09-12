/*!
 * \file TBLStgLoggerInfo.hpp
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

namespace bq::db::stgLoggerInfo {

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`stgLoggerInfo`";
};

}  // namespace bq::db::stgLoggerInfo

using TBLStgLoggerInfo = bq::db::stgLoggerInfo::TableSchema;
