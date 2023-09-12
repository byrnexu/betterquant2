/*!
 * \file MarketDataCache.hpp
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
#include "def/DefIF.hpp"
#include "util/PchBase.hpp"
#include "util/StdExt.hpp"

namespace bq {
struct Tickers;
using TickersSPtr = std::shared_ptr<Tickers>;
}  // namespace bq

namespace bq {

class RiskMgr;

class MarketDataCache {
 public:
  MarketDataCache(const MarketDataCache&) = delete;
  MarketDataCache& operator=(const MarketDataCache&) = delete;
  MarketDataCache(const MarketDataCache&&) = delete;
  MarketDataCache& operator=(const MarketDataCache&&) = delete;

  explicit MarketDataCache();

 public:
  void cache(const TickersSPtr& tickers);
  TickersSPtr getLastTickers(const std::string& topic);
  TickersSPtr getLastTickers(MarketCode marketCode, SymbolType symbolType,
                             const std::string& symbolCode);

 private:
  std::string tickers2Str() const;

 private:
  std::map<TopicHash, TickersSPtr> topic2LastTickersGroup_;
  std::ext::spin_mutex mtxTopic2LastTickersGroup_;

  std::uint64_t timesOfRecvTickers_{0};
};

using MarketDataCacheSPtr = std::shared_ptr<MarketDataCache>;

struct SymbolCode;
using SymbolCodeSPtr = std::shared_ptr<SymbolCode>;

std::tuple<int, std::vector<SymbolCodeSPtr>, std::uint64_t, Decimal> CalcPrice(
    const MarketDataCacheSPtr& marketDataCache, MarketCode marketCode,
    const std::string& baseCurrency, const std::string& quoteCurrency,
    const std::string& quoteCurrencyForCalc,
    const std::string& quoteCurrencyForConv);

}  // namespace bq
