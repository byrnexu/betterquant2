/*!
 * \file PnlIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct SymbolCode;
using SymbolCodeSPtr = std::shared_ptr<SymbolCode>;

struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;

struct Pnl {
  std::string queryCond_;

  Decimal pnlUnReal_{0};
  Decimal pnlReal_{0};

  Decimal fee_{0};

  std::uint64_t updateTime_{0};
  std::string quoteCurrencyForCalc_;

  int statusCode_{0};
  std::vector<SymbolCodeSPtr> symbolGroupForCalc_;

  std::string toStr() const;

  Decimal getTotalPnl() const;
  bool isValid(std::uint32_t secDelayOfPrice) const;

  std::uint64_t delay() const;

  std::string getSqlOfInsert() const;

  std::string toJson() const;
  void fromJson(const std::string& jsonStr);
};

using Key2PnlGroup = std::map<std::string, PnlSPtr>;
using Key2PnlGroupSPtr = std::shared_ptr<Key2PnlGroup>;

}  // namespace bq
