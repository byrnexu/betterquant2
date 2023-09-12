/*!
 * \file SymbolCodeIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct SymbolCode;
using SymbolCodeSPtr = std::shared_ptr<SymbolCode>;

struct SymbolCode {
  SymbolCode(MarketCode marketCode, SymbolType symbolType,
             const std::string& symbolCode)
      : marketCode_(marketCode),
        symbolType_(symbolType),
        symbolCode_(symbolCode) {}
  MarketCode marketCode_;
  SymbolType symbolType_;
  std::string symbolCode_;

  std::string toStr() const;
};

}  // namespace bq
