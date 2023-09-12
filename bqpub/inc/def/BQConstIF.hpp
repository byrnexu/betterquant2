/*!
 * \file BQConstIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/PchBase.hpp"
#include "util/StdExt.hpp"

namespace bq {

enum class NotifyToTerminal { True, False };

enum class MarketCode : std::uint16_t {
  Okex = 1,
  Binance = 2,
  Coinbase = 3,
  Kraken = 4,

  SSE = 101,
  SZSE,
  SHFE,
  CZCE,
  DCE,
  CFFEX,
  INE,
  GFEX,

  Others = UINT16_MAX - 1
};
std::string GetMarketName(MarketCode marketCode);
MarketCode GetMarketCode(const std::string& marketCodeName);

constexpr bool IsCNMarketOfFutures(MarketCode marketCode) {
  return std::ext::is_any_value_of<MarketCode,         //
                                   MarketCode::SHFE,   //
                                   MarketCode::CZCE,   //
                                   MarketCode::DCE,    //
                                   MarketCode::CFFEX,  //
                                   MarketCode::INE     //
                                   >(marketCode);
}

constexpr bool IsCNMarketOfSpots(MarketCode marketCode) {
  return std::ext::is_any_value_of<MarketCode,
                                   MarketCode::SSE,  //
                                   MarketCode::SZSE  //
                                   >(marketCode);
}

constexpr bool IsCNMarket(MarketCode marketCode) {
  return IsCNMarketOfSpots(marketCode) || IsCNMarketOfFutures(marketCode);
}

enum class SymbolType : std::uint8_t {
  Spot = 1,
  Futures = 2,
  CFutures = 3,
  Perp = 4,
  CPerp = 5,
  Option = 6,
  index = 7,

  //! 主板股票
  CN_MainBoard = 21,
  //! 中小板股票
  CN_SecondBoard = 22,
  //! 创业板股票
  CN_StartupBoard = 23,
  //! 指数
  CN_Index = 24,
  //! 科创板股票(上海)
  CN_TechBoard = 25,
  //! 国债
  CN_StateBond = 26,
  //! 企业债
  CN_EnterpriseBond = 27,
  //! 公司债
  CN_CompanyBond = 28,
  //! 转换债券
  CN_ConvertableBond = 29,
  //! 国债逆回购
  CN_NationalBondReverseRepo = 30,
  //! 本市场股票 ETF
  CN_ETF_SingleMarketStock = 31,
  //! 跨市场股票 ETF
  CN_ETF_InterMarketStock = 32,
  //! 跨境股票 ETF
  CN_ETF_CrossBorderStock = 33,
  //! 本市场实物债券 ETF
  CN_ETF_SingleMarketBond = 34,
  //! 黄金 ETF
  CN_ETF_Gold = 35,
  //! 分级基金子基金
  CN_StructuredFundChild = 36,
  //! 深交所仅申赎基金
  CN_SZSE_RecreationFund = 37,
  //! 个股期权
  CN_StockOption = 38,
  //! ETF期权
  CN_ETF_Option = 39,

  //! 配股
  CN_Allotment = 40,

  //! 上交所申赎型货币基金
  CN_MoneyMonetaryFundSHCR = 41,
  //! 上交所交易型货币基金
  CN_MonetaryFundSHTR = 42,
  //! 深交所货币基金
  CN_MonetaryFundSZ = 43,

  //! 期货
  CN_Futures = 80,
  //! 期货期权
  CN_FuturesOptions = 81,
  //! 组合
  CN_FuturesComb = 82,
  //! 即期
  CN_SpotFutures = 83,
  //! 期转现
  CN_FuturesEFP = 84,
  //! 现货期权
  CN_SpotOptions = 85,
  //! TAS合约
  CN_FuturesTAS = 86,
  //! 金属指数
  CN_FuturesMI = 87,

  Others = UINT8_MAX - 1
};

enum class BusinessType : std::uint8_t { Normal = 1, Others = UINT8_MAX - 1 };

enum class Side : std::uint8_t { Bid = 1, Ask = 2, Others = UINT8_MAX - 1 };

enum class PosDirection : std::uint8_t {
  Open = 1,
  Close = 2,
  CloseTDay = 3,
  CloseYDay = 4,
  Both = 5,
  Others = UINT8_MAX - 1
};

enum class PosSide : std::uint8_t {
  Long = 1,
  Short = 2,
  Both = 3,
  Others = UINT8_MAX - 1
};

enum class LiquidityDirection : std::uint8_t {
  Taker = 1,
  Maker = 2,
  Others = UINT8_MAX - 1
};

enum class OrderType : std::uint8_t { Limit = 1, Others = UINT8_MAX - 1 };

enum class OrderTypeExtra : std::uint8_t {
  Normal = 1,
  MakeOnly = 2,
  Ioc = 3,
  Fok = 4,
  Others = UINT8_MAX - 1
};

enum class CloseTDayStg : std::uint8_t {
  AllowCloseTDay = 1,
  RejectCloseTDay = 2,
  RejectUntilOpenTDay = 3,
  Others = UINT8_MAX - 1
};

enum class OpenedTDay { True = 1, False = 2 };

enum class FeeType : std::uint8_t {
  ByAmt = 1,
  ByVol = 2,
  Others = UINT8_MAX - 1
};

enum class MDType : std::uint8_t {
  Trades = 1,
  Orders = 2,
  Books = 3,
  Tickers = 4,
  Candle = 5,
  Bid1Ask1 = 6,
  LastPrice = 7,
  DynCandle = 8,

  Others = UINT8_MAX - 1
};

enum class OrderStatus {
  Created = 1,
  ConfirmedInLocal = 3,
  Pending = 5,
  ConfirmedByExch = 10,
  PartialFilled = 20,
  Filled = 100,
  Canceled = 101,
  PartialFilledCanceled = 105,
  Failed = 110,
  Others = 127
};

enum SymbolState : std::uint8_t {
  Preopen = 1,
  Online = 2,
  Suspend = 3,
  Settlement = 4,

  //! 风险警示板
  CN_StatusST = 21,
  //! 首日上市
  CN_StatusNIPO = 22,
  //! 普通
  CN_StatusCommon = 23,
  //! 恢复上市
  CN_StatusResume = 24,
  //! 退市整理期
  CN_StatusDelisting = 25,

  Others = UINT8_MAX - 1
};

enum class PnlType : std::uint8_t { Profit = 1, Loss = 2 };

const static std::string CN_OFFICIAL_CURRENCY = "CNY";

constexpr static std::uint32_t MAX_DEPTH_LEVEL = 400;
//! cn具体的处理类中用到，比如RawMDHandlerOfXTP生成内部格式的Books的时候用到
constexpr static std::uint32_t MAX_DEPTH_LEVEL_OF_CN = 10;
constexpr static std::uint32_t MAX_DEPTH_LEVEL_IN_TICKER = 10;

const static std::string SEP_OF_DEPTH_FIELDS = ",";
const static std::string SEP_OF_DEPTH_REC = ";";

//! orderInfo中用到的常量
constexpr static std::uint16_t MAX_EXCH_ORDER_ID_LEN = 24;
constexpr static std::uint16_t MAX_SYMBOL_CODE_LEN = 32;
constexpr static std::uint32_t MAX_SIMED_TD_INFO = 16 * 1024;
constexpr static std::uint16_t MAX_CURRENCY_LEN = 16;
constexpr static std::uint16_t MAX_TRADE_ID_LEN = 32;

//! 行情和资产信息中用到的常量
constexpr static std::uint16_t MAX_TRADE_NO_LEN = 32;
constexpr static std::uint16_t MAX_ORDER_NO_LEN = 32;
constexpr static std::uint16_t MAX_ORDER_ID_LEN = 16;
constexpr static std::uint16_t MAX_TRADING_DAY_LEN = 9;
constexpr static std::uint16_t MAX_ASSETS_NAME_LEN = 32;

const static std::string TOPIC_PREFIX_OF_MARKET_DATA = "MD";
const static std::string TOPIC_PREFIX_OF_TRADE_DATA = "TD";
const static std::string SEP_OF_TOPIC = "@";

const static std::string SEP_OF_SYMBOL_SPOT = "-";
const static std::string SEP_OF_SYMBOL_FUTURES = "-";
const static std::string SEP_OF_SYMBOL_PERP = "-";

const static std::string SEP_OF_REC_IDENTITY = "/";
const static std::string SEP_OF_COND_AND = "&";

const static std::uint64_t UNDEFINED_FIELD_MIN_TS = 946684800000000;
const static std::uint64_t UNDEFINED_FIELD_MAX_TS = 1893456000000000;
const static std::string UNDEFINED_FIELD_MIN_DATETIME =
    "2000-01-01 00:00:00.000000";
const static std::string UNDEFINED_FIELD_MAX_DATETIME =
    "2030-01-01 00:00:00.000000";

//! 扩展信息消息类型
const static std::string EXT_MSG_ID_SIMED_TD = "simedTD";

}  // namespace bq
