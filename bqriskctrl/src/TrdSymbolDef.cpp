#include "TrdSymbolDef.hpp"

/*!
 * \file TrdSymbolDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#include "TrdSymbolDef.hpp"
#include "db/TBLTrdSymbolList.hpp"

namespace bq {

TrdSymbol::TrdSymbol(const bq::db::trdSymbolList::RecordSPtr rec) {
  marketCode_ = rec->marketCode;
  symbolType_ = rec->symbolType;
  symbolCode_ = rec->symbolCode;
}

std::uint64_t TrdSymbol::getHash() const {
  const auto str = toStr();
  const auto hash = XXH3_64bits(str.data(), str.size());
  return hash;
}

std::string TrdSymbol::toStr() const {
  const auto ret =
      fmt::format("{}-{}-{}", marketCode_, symbolType_, symbolCode_);
  return ret;
}

}  // namespace bq
