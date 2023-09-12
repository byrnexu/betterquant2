/*!
 * \file BQMDUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "util/Pch.hpp"

namespace bq::md {

std::string RemoveDepthInTopicOfBooks(const std::string& topic);

std::string GetSymbolCode(MarketCode marketCode,
                          const std::string& exchSymbolCode);
}  // namespace bq::md
