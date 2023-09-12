/*!
 * \file SymbolInfoIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/02
 *
 * \brief
 */

#pragma once

#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"
#include "def/ConstIF.hpp"
#include "def/DefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct SymbolInfo;
using SymbolInfoSPtr = std::shared_ptr<SymbolInfo>;

struct SymbolInfo {
  SymbolInfo(MarketCode marketCode, SymbolType symbolType,
             const std::string& symbolCode, const std::string& exchSymbolCode)
      : marketCode_(marketCode),
        symbolType_(symbolType),
        symbolCode_(symbolCode),
        exchSymbolCode_(exchSymbolCode) {}
  MarketCode marketCode_;
  SymbolType symbolType_;
  std::string symbolCode_;
  std::string exchSymbolCode_;

  std::string toStr() const;
};

}  // namespace bq
