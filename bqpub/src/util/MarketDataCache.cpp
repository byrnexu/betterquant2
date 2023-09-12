/*!
 * \file MarketDataCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/MarketDataCache.hpp"

#include "SHMIPC.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/StatusCode.hpp"
#include "def/SymbolCodeIF.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"

namespace bq {

MarketDataCache::MarketDataCache() {}

void MarketDataCache::cache(const TickersSPtr& tickers) {
#ifndef NDEBUG
  std::uint64_t loggerThreshold = 100000;
#else
  std::uint64_t loggerThreshold = 1000000;
#endif

  //! xtp有收到一些价格为0的tickers，但是有昨收或者昨结这样的数据，这里不过滤
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopic2LastTickersGroup_);
    topic2LastTickersGroup_[tickers->shmHeader_.topicHash_] = tickers;
    if (++timesOfRecvTickers_ % loggerThreshold == 0) {
      LOG_D("===== TICKERS CACHE ===== {} \n{}", timesOfRecvTickers_,
            tickers2Str());
    }
  }
}

//! topic = MD@Binance@Spot@BTC-USDT@Tickers
//! topic = MD@SSE@Spot@600600@Tickers
TickersSPtr MarketDataCache::getLastTickers(const std::string& topic) {
  TickersSPtr ret;
  const auto topicHash = XXH3_64bits(topic.data(), topic.size());
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopic2LastTickersGroup_);
    const auto iter = topic2LastTickersGroup_.find(topicHash);
    if (iter != std::end(topic2LastTickersGroup_)) {
      ret = iter->second;
    }
  }
  return ret;
}

//! topic = MD@Binance@Spot@BTC-USDT@Tickers
//! topic = MD@SSE@Spot@600600@Tickers
TickersSPtr MarketDataCache::getLastTickers(MarketCode marketCode,
                                            SymbolType symbolType,
                                            const std::string& symbolCode) {
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                  SEP_OF_TOPIC, GetMarketName(marketCode), SEP_OF_TOPIC,
                  magic_enum::enum_name(symbolType), SEP_OF_TOPIC, symbolCode,
                  SEP_OF_TOPIC, magic_enum::enum_name(MDType::Tickers));
  return getLastTickers(topic);
}

std::string MarketDataCache::tickers2Str() const {
  std::string ret;
  for (const auto& rec : topic2LastTickersGroup_) {
    ret = ret + rec.second->toStr() + "\n";
  }
  return ret;
}

std::tuple<int, std::vector<SymbolCodeSPtr>, std::uint64_t, Decimal> CalcPrice(
    const MarketDataCacheSPtr& marketDataCache, MarketCode marketCode,
    const std::string& baseCurrency, const std::string& quoteCurrency,
    const std::string& quoteCurrencyForCalc,
    const std::string& quoteCurrencyForConv) {
  //!
  //! 考虑最复杂的情形，假设仓位是DOT-USD，计价币种(quoteCurrencyForCalc)BTC，
  //! 转换币种(quoteCurrencyForConv)USDT，我们现在需要获取的是BTC-USD的价格。
  //!
  //! 从下面的getLastTickers方法调用来看，我们需要获得价格的币对是：
  //!
  //! $(quoteCurrencyForCalc)-$(quoteCurrency)        BTC-USD
  //! $(quoteCurrency)-$(quoteCurrencyForCalc)        USD-BTC
  //! $(quoteCurrencyForCalc)-$(quoteCurrencyForConv) BTC-USDT
  //! $(quoteCurrency)-$(quoteCurrencyForConv)        USD-USDT
  //!

  const auto makeSymbolGroupForCalc = [&]() {
    std::vector<SymbolCodeSPtr> ret;
    // quoteCurrencyForCalc - quoteCurrency
    ret.emplace_back(std::make_shared<SymbolCode>(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrencyForCalc, SEP_OF_SYMBOL_SPOT,
                    quoteCurrency)));
    // quoteCurrency - quoteCurrencyForCalc
    ret.emplace_back(std::make_shared<SymbolCode>(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrency, SEP_OF_SYMBOL_SPOT,
                    quoteCurrencyForCalc)));
    // quoteCurrencyForCalc - quoteCurrencyForConv
    ret.emplace_back(std::make_shared<SymbolCode>(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrencyForCalc, SEP_OF_SYMBOL_SPOT,
                    quoteCurrencyForConv)));
    // quoteCurrency - quoteCurrencyForConv
    ret.emplace_back(std::make_shared<SymbolCode>(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrency, SEP_OF_SYMBOL_SPOT,
                    quoteCurrencyForConv)));

    return ret;
  };

  std::vector<SymbolCodeSPtr> symbolGroupForCalc;

  if (!marketDataCache) {
    return {SCODE_SUCCESS, symbolGroupForCalc, GetTotalUSSince1970(), 1.0};
  }

  if (quoteCurrencyForCalc == quoteCurrency) {
    return {SCODE_SUCCESS, symbolGroupForCalc, GetTotalUSSince1970(), 1.0};

  } else {
    //! 尝试获取BTC-USD的价格
    const auto lastTickers = marketDataCache->getLastTickers(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrencyForCalc, SEP_OF_SYMBOL_SPOT,
                    quoteCurrency));
    if (lastTickers &&  //
        !DEC::ZERO(lastTickers->lastPrice_) &&
        !DEC::EQ(lastTickers->lastPrice_, DBL_MAX)) {
      //! 如果找到BTC-USD的价格，那么直接返回
      return {SCODE_SUCCESS, symbolGroupForCalc, lastTickers->mdHeader_.exchTs_,
              lastTickers->lastPrice_};

    } else {
      //! 如果没有找到BTC-USD的价格，那么尝试找USD-BTC的价格
      const auto lastTickers = marketDataCache->getLastTickers(
          marketCode, SymbolType::Spot,
          fmt::format("{}{}{}", quoteCurrency, SEP_OF_SYMBOL_SPOT,
                      quoteCurrencyForCalc));
      if (lastTickers &&  //
          !DEC::ZERO(lastTickers->lastPrice_) &&
          !DEC::EQ(lastTickers->lastPrice_, DBL_MAX)) {
        return {SCODE_SUCCESS, symbolGroupForCalc,
                lastTickers->mdHeader_.exchTs_, 1.0 / lastTickers->lastPrice_};
      }
    }
  }

  //!
  //! 上述过程查找BTC-USD和USD-BTC的价格都失败，那么跟据转换币种来计算BTC-USD
  //! 的价格，尝试获取BTC-USDT和USD-USDT的价格。
  //!
  //! 尝试获取BTC-USDT的lastTickers
  TickersSPtr lastTickersOfCalc = nullptr;
  if (quoteCurrencyForCalc == quoteCurrencyForConv) {
    lastTickersOfCalc = std::make_shared<Tickers>();
    lastTickersOfCalc->mdHeader_.exchTs_ = GetTotalUSSince1970();
    lastTickersOfCalc->lastPrice_ = 1.0;
  } else {
    lastTickersOfCalc = marketDataCache->getLastTickers(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrencyForCalc, SEP_OF_SYMBOL_SPOT,
                    quoteCurrencyForConv));
  }

  //! 尝试获取USD-USDT的lastTickers
  TickersSPtr lastTickers = nullptr;
  if (quoteCurrency == quoteCurrencyForConv) {
    lastTickers = std::make_shared<Tickers>();
    lastTickers->mdHeader_.exchTs_ = GetTotalUSSince1970();
    lastTickers->lastPrice_ = 1.0;
  } else {
    lastTickers = marketDataCache->getLastTickers(
        marketCode, SymbolType::Spot,
        fmt::format("{}{}{}", quoteCurrency, SEP_OF_SYMBOL_SPOT,
                    quoteCurrencyForConv));
  }

  if (lastTickers && lastTickersOfCalc && !DEC::ZERO(lastTickers->lastPrice_) &&
      !DEC::ZERO(lastTickersOfCalc->lastPrice_) &&
      !DEC::EQ(lastTickers->lastPrice_, DBL_MAX) &&
      !DEC::EQ(lastTickersOfCalc->lastPrice_, DBL_MAX)) {
    const auto lastPrice =
        lastTickersOfCalc->lastPrice_ / lastTickers->lastPrice_;
    const auto updateTime =
        lastTickers->mdHeader_.exchTs_ < lastTickersOfCalc->mdHeader_.exchTs_
            ? lastTickers->mdHeader_.exchTs_
            : lastTickersOfCalc->mdHeader_.exchTs_;
    return {SCODE_SUCCESS, symbolGroupForCalc, updateTime, lastPrice};
  }

  symbolGroupForCalc = makeSymbolGroupForCalc();
  return {SCODE_BQPUB_CALC_PRICE_FAILED, symbolGroupForCalc,
          GetTotalUSSince1970(), 1};
}

}  // namespace bq
