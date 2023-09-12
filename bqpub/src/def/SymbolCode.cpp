/*!
 * \file SymbolCode.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "def/SymbolCode.hpp"

#include "def/BQConst.hpp"
#include "util/Logger.hpp"

namespace bq {

std::string SymbolCode::toStr() const {
  std::string ret;
  ret = fmt::format("{} {} {}", GetMarketName(marketCode_),
                    magic_enum::enum_name(symbolType_), symbolCode_);
  return ret;
}

std::string SymbolCodeGroup2Str(
    const std::vector<SymbolCodeSPtr> symbolInfoGroup) {
  std::string ret;
  for (const auto symbolInfo : symbolInfoGroup) {
    ret = ret + symbolInfo->toStr() + "; ";
  }
  if (ret.size() > 1) {
    ret = ret.substr(0, ret.length() - 2);
  }
  return ret;
}

}  // namespace bq
