/*!
 * \file TBLHisPnl.hpp
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

namespace bq::db::hisPnl {

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`hisPnl`";
};

}  // namespace bq::db::hisPnl

using TBLHisPnl = bq::db::hisPnl::TableSchema;
