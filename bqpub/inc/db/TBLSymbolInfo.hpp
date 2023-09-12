/*!
 * \file TBLSymbolInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::symbolInfo {

struct FieldGroupOfKey {
  std::string marketCode;
  std::string symbolCode;
  JSER(FieldGroupOfKey, marketCode, symbolCode)
};

struct FieldGroupOfVal {
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string symbolName;
  std::string exchSymbolCode;
  std::string alias;

  std::string underlyingIndex;

  std::string baseCurrency;
  std::string quoteCurrency;
  std::string settlementCurrency;

  std::string upperLimitPrice;
  std::string lowerLimitPrice;

  std::string preClosePrice;
  std::string preSettlementPrice;

  std::string precOfBidOrderPrice;
  std::string precOfBidOrderVol;
  std::string precOfAskOrderPrice;
  std::string precOfAskOrderVol;

  std::string minBidOrderVol;
  std::string maxBidOrderVol;
  std::string minAskOrderVol;
  std::string maxAskOrderVol;

  std::string minBidOrderAmt;
  std::string maxBidOrderAmt;
  std::string minAskOrderAmt;
  std::string maxAskOrderAmt;

  std::uint32_t parValue;
  std::string contractMult;
  std::string maxLeverage;

  std::string symbolState;

  std::string launchTime;
  std::string deliveryTime;

  int isDel;

  JSER(FieldGroupOfVal, marketCode, symbolType, symbolCode, symbolName,
       exchSymbolCode, alias, underlyingIndex, baseCurrency, quoteCurrency,
       settlementCurrency, upperLimitPrice, lowerLimitPrice, preClosePrice,
       preSettlementPrice, precOfBidOrderPrice, precOfBidOrderVol,
       precOfAskOrderPrice, precOfAskOrderVol, minBidOrderVol, maxBidOrderVol,
       minAskOrderVol, maxAskOrderVol, minBidOrderAmt, maxBidOrderAmt,
       minAskOrderAmt, maxAskOrderAmt, parValue, contractMult, maxLeverage,
       symbolState, launchTime, deliveryTime, isDel)
};

struct FieldGroupOfAll {
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string symbolName;
  std::string exchSymbolCode;
  std::string alias;

  std::string underlyingIndex;

  std::string baseCurrency;
  std::string quoteCurrency;
  std::string settlementCurrency;

  std::string upperLimitPrice;
  std::string lowerLimitPrice;

  std::string preClosePrice;
  std::string preSettlementPrice;

  std::string precOfBidOrderPrice;
  std::string precOfBidOrderVol;
  std::string precOfAskOrderPrice;
  std::string precOfAskOrderVol;

  std::string minBidOrderVol;
  std::string maxBidOrderVol;
  std::string minAskOrderVol;
  std::string maxAskOrderVol;

  std::string minBidOrderAmt;
  std::string maxBidOrderAmt;
  std::string minAskOrderAmt;
  std::string maxAskOrderAmt;

  std::uint32_t parValue;
  std::string contractMult;
  std::string maxLeverage;

  std::string symbolState;

  std::string launchTime;
  std::string deliveryTime;

  int isDel;

  std::uint64_t hashOfMktSym_;
  std::uint64_t hashOfMktSymExchSym_;

  void initHashInfo() {
    const auto hashDataOfMktSym = fmt::format("{}{}", marketCode, symbolCode);
    const auto hashDataOfMktSymExchSym =
        fmt::format("{}{}{}", marketCode, symbolType, exchSymbolCode);
    hashOfMktSym_ =
        XXH3_64bits(hashDataOfMktSym.data(), hashDataOfMktSym.size());
    hashOfMktSymExchSym_ = XXH3_64bits(hashDataOfMktSymExchSym.data(),
                                       hashDataOfMktSymExchSym.size());
  }

  JSER(FieldGroupOfAll, marketCode, symbolType, symbolCode, symbolName,
       exchSymbolCode, alias, underlyingIndex, baseCurrency, quoteCurrency,
       settlementCurrency, upperLimitPrice, lowerLimitPrice, preClosePrice,
       preSettlementPrice, precOfBidOrderPrice, precOfBidOrderVol,
       precOfAskOrderPrice, precOfAskOrderVol, minBidOrderVol, maxBidOrderVol,
       minAskOrderVol, maxAskOrderVol, minBidOrderAmt, maxBidOrderAmt,
       minAskOrderAmt, maxAskOrderAmt, parValue, contractMult, maxLeverage,
       symbolState, launchTime, deliveryTime, isDel)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseSymbolInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::symbolInfo

using TBLSymbolInfo = bq::db::symbolInfo::TableSchema;
