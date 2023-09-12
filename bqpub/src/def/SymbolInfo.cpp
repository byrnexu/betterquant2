/*!
 * \file SymbolInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/02
 *
 * \brief
 */

#include "def/SymbolInfo.hpp"

namespace bq {

std::string SymbolInfo::toStr() const {
  const auto ret = fmt::format(
      "marketCode: {}; symbolType: {}; symbolCode: {}; exchSymbolCode: {}");
  return ret;
}

}  // namespace bq
