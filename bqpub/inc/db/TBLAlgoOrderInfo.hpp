/*!
 * \file TBLAlgoOrderInfo.hpp
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

namespace bq::db::algoOrderInfo {

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`trdAlgoOrderInfo`";
};

}  // namespace bq::db::algoOrderInfo

using TBLTrdAlgoOrderInfo = bq::db::algoOrderInfo::TableSchema;
