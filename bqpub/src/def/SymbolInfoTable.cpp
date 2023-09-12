/*!
 * \file SymbolInfoTable.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/02
 *
 * \brief
 */

#include "def/SymbolInfoTable.hpp"

#include "def/StatusCode.hpp"
#include "util/Logger.hpp"

namespace bq {

void SymbolInfoTable::add(const SymbolInfoSPtr& symbolInfo) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMIDXSymbolInfo_);
    const auto ret = midxSymbolInfo_.emplace(symbolInfo);
    if (ret.second == false) {
      LOG_W("Add symbol info to symbol table failed. [{}]",
            symbolInfo->toStr());
    }
  }
}

std::tuple<int, SymbolInfoSPtr> SymbolInfoTable::getSymbolInfoBySym(
    const std::string& symbolCode) const {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMIDXSymbolInfo_);
    auto& idx = midxSymbolInfo_.get<TagSymbolCode>();
    const auto iter = idx.find(symbolCode);
    if (iter != std::end(idx)) {
      return {0, *iter};
    }
  }
  LOG_W("Can not find symbol info of symbol {}.", symbolCode);
  return {SCODE_DB_CAN_NOT_FIND_SYM_CODE, nullptr};
}

std::tuple<int, SymbolInfoSPtr> SymbolInfoTable::getSymbolInfoByExchSym(
    const std::string& exchSymbolCode) const {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMIDXSymbolInfo_);
    auto& idx = midxSymbolInfo_.get<TagExchSymbolCode>();
    const auto iter = idx.find(exchSymbolCode);
    if (iter != std::end(idx)) {
      return {0, *iter};
    }
  }
  LOG_W("Can not find symbol info of exch symbol {}.", exchSymbolCode);
  return {SCODE_DB_CAN_NOT_FIND_EXCH_SYM_CODE, nullptr};
}

std::vector<std::string> SymbolInfoTable::getExchSymbolCodeGroup() const {
  std::vector<std::string> ret;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMIDXSymbolInfo_);
    for (auto iter = std::begin(midxSymbolInfo_);
         iter != std::end(midxSymbolInfo_); ++iter) {
      ret.emplace_back((*iter)->exchSymbolCode_);
    }
  }
  return ret;
}

std::size_t SymbolInfoTable::size() const {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMIDXSymbolInfo_);
    return midxSymbolInfo_.size();
  }
}

}  // namespace bq
