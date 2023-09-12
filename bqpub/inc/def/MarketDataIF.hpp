/*!
 * \file MarketDataIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMHeader.hpp"
#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"

namespace bq {

struct Trades {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  std::uint64_t tradeTime_;
  char tradeNo_[MAX_TRADE_NO_LEN];

  Decimal price_;
  Decimal size_;

  Side side_;

  char bidOrderId_[MAX_ORDER_ID_LEN];
  char askOrderId_[MAX_ORDER_ID_LEN];

  char tradingDay_[MAX_TRADING_DAY_LEN];

  std::uint16_t extDataLen_{0};
  char extData_[0];

  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using TradesSPtr = std::shared_ptr<Trades>;
using TradesUPtr = std::unique_ptr<Trades>;

struct Orders {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  std::uint64_t orderTime_;
  char orderNo_[MAX_ORDER_NO_LEN];

  Decimal price_;
  Decimal size_;

  Side side_;

  char tradingDay_[MAX_TRADING_DAY_LEN];

  std::uint16_t extDataLen_{0};
  char extData_[0];

  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using OrdersSPtr = std::shared_ptr<Orders>;
using OrdersUPtr = std::unique_ptr<Orders>;

struct Depth {
  Decimal price_{0};
  Decimal size_{0};
  std::uint32_t orderNum_{0};
};

struct Books {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  Decimal lastPrice_;
  Decimal totalVol_;
  Decimal totalAmt_;
  std::uint64_t tradesCount_;

  char tradingDay_[MAX_TRADING_DAY_LEN];

  Depth asks_[MAX_DEPTH_LEVEL];
  Depth bids_[MAX_DEPTH_LEVEL];

  std::uint16_t extDataLen_{0};
  char extData_[0];

  std::string toStr() const;
  std::string toJson(std::uint32_t level = MAX_DEPTH_LEVEL) const;
  std::string data(std::uint32_t level = MAX_DEPTH_LEVEL) const;
  std::string dataOfUnifiedFmt(std::uint32_t level = MAX_DEPTH_LEVEL) const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using BooksSPtr = std::shared_ptr<Books>;
using BooksUPtr = std::unique_ptr<Books>;

struct Bid1Ask1 {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  Decimal askPrice_;
  Decimal askSize_;
  Decimal bidPrice_;
  Decimal bidSize_;

  char tradingDay_[MAX_TRADING_DAY_LEN];

  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using Bid1Ask1SPtr = std::shared_ptr<Bid1Ask1>;
using Bid1Ask1UPtr = std::unique_ptr<Bid1Ask1>;

struct LastPrice {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  Decimal lastPrice_;
  Decimal lastSize_;

  char tradingDay_[MAX_TRADING_DAY_LEN];

  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using LastPriceSPtr = std::shared_ptr<LastPrice>;
using LastPriceUPtr = std::unique_ptr<LastPrice>;

struct Tickers {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;

  Decimal open_;
  Decimal high_;
  Decimal low_;

  Decimal lastPrice_;
  Decimal lastSize_;

  Decimal upperLimitPrice_;
  Decimal lowerLimitPrice_;

  Decimal preClosePrice_;
  Decimal preSettlementPrice_;

  Decimal closePrice_;
  Decimal settlementPrice_;

  Decimal preOpenInterest_;
  Decimal openInterest_;

  Decimal vol_;
  Decimal amt_;

  Decimal askPrice_;
  Decimal askSize_;
  Decimal bidPrice_;
  Decimal bidSize_;

  char tradingDay_[MAX_TRADING_DAY_LEN];

  Depth asks_[MAX_DEPTH_LEVEL_IN_TICKER];
  Depth bids_[MAX_DEPTH_LEVEL_IN_TICKER];

  std::uint16_t extDataLen_{0};
  char extData_[0];

  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
  std::string getTDEngSqlRawPrefix() const;
  std::string getTDEngSqlRawTagsPart(const std::string& apiName) const;
  std::string getTDEngSqlRawValuesPart(void* data, std::size_t dataLen) const;
};
using TickersSPtr = std::shared_ptr<Tickers>;
using TickersUPtr = std::unique_ptr<Tickers>;

struct Candle {
  SHMHeader shmHeader_;
  MDHeader mdHeader_;
  std::uint64_t startTs_{0};
  std::uint32_t interval_{60};
  std::uint64_t startTsOfCandle_{0};
  Decimal open_;
  Decimal high_;
  Decimal low_;
  Decimal close_;
  Decimal vol_;
  Decimal amt_;
  std::uint16_t extDataLen_{0};
  char extData_[0];
  std::string toStr() const;
  std::string toJson() const;
  std::string data() const;
  std::string dataOfUnifiedFmt() const;

  std::string getTDEngSqlPrefix() const;
  std::string getTDEngSqlTagsPart() const;
  std::string getTDEngSqlValuesPart() const;
};
using CandleSPtr = std::shared_ptr<Candle>;
using CandleUPtr = std::unique_ptr<Candle>;

std::string MakeMarketData(const SHMHeader& shmHeader, const MDHeader& mdHeader,
                           const std::string& data);

std::tuple<int, Trades> MakeTrades(const std::string& jsonStr);
std::tuple<int, Books> MakeBooks(const std::string& jsonStr);
std::tuple<int, Candle> MakeCandle(const std::string& jsonStr);
std::tuple<int, Tickers> MakeTickers(const std::string& jsonStr);

MsgId GetMsgIdByMDType(MDType mdType);

}  // namespace bq
