/*!
 * \file SymbolInfoTable.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/02
 *
 * \brief
 */

#pragma once

#include "def/SymbolInfo.hpp"

namespace bq {

class SymbolInfoTable;
using SymbolInfoTableSPtr = std::shared_ptr<SymbolInfoTable>;

class SymbolInfoTable {
 public:
  SymbolInfoTable(const SymbolInfoTable&) = delete;
  SymbolInfoTable& operator=(const SymbolInfoTable&) = delete;
  SymbolInfoTable(const SymbolInfoTable&&) = delete;
  SymbolInfoTable& operator=(const SymbolInfoTable&&) = delete;

  SymbolInfoTable() = default;

  void add(const SymbolInfoSPtr& symbolInfo);

  std::tuple<int, SymbolInfoSPtr> getSymbolInfoBySym(
      const std::string& symbolCode) const;

  std::tuple<int, SymbolInfoSPtr> getSymbolInfoByExchSym(
      const std::string& exchSymbolCode) const;

  std::vector<std::string> getExchSymbolCodeGroup() const;

  std::size_t size() const;

 private:
  MIDXSymbolInfo midxSymbolInfo_;
  mutable std::ext::spin_mutex mtxMIDXSymbolInfo_;
};

}  // namespace bq
