/*!
 * \file BQCTPUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#pragma once

#include "API.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc::ctp {

SymbolType GetSymbolType(TThostFtdcAPIProductClassType exchSymbolType);

}  // namespace bq::md::svc::ctp
