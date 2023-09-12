/*!
 * \file BQConstExt.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

enum class IsTheAssetInfoUpdated { True = 1, False = 2 };

enum class ApiProtocol : std::uint8_t {
  Tcp = 1,
  Udp = 2,
  Others = UINT8_MAX - 1
};

enum class MsgType : std::uint8_t {
  Order = 1,

  SyncUnclosedOrder = 11,
  SyncAssetsSnapshot = 12,
  SyncAssetsUpdate = 13,

  Trades = 21,
  Orders = 22,
  Books = 23,
  Tickers = 24,
  Candle = 25,
  Bid1Ask1 = 26,
  LastPrice = 27,
  DynCandle = 28,

  SubRet = 41,
  UnSubRet = 42,

  NewSymbol = 100,

  Others = UINT8_MAX - 1
};

enum class TrdListType { White = 1, Black = 2 };

enum class TableChgType { Add = 1, Del = 2, Chg = 3 };

const static std::size_t MAX_TD_SRV_RISK_PLUGIN_NUM = 32;

const static std::string HIS_MD_FILE_EXT = "dat";
const static std::string HIS_MD_INDEX_BY_ET_EXT = "et";
const static std::string HIS_MD_INDEX_BY_LT_EXT = "lt";

const static std::string SUFFIX_OF_CANDLE_DETAIL = "detail";

const static std::uint32_t MAX_SEC_OF_CACHE_MD_SIM = 3600;

const static std::string SEP_OF_TDENG_TABLE_NAME = "_";
const static std::string TBENG_DBNAME_OF_MD = "marketData";
const static std::string TBENG_ORIG_DATA_TABLE_NAME_SUFFIX = "origData";
const static std::string TBENG_TABLE_NAME_OF_ORIG_MD = "origData";

const static std::uint64_t DBL_TO_INT_MULTI = 1000000000000;
const static std::uint64_t DBL_PREC_FOR_ORDER = 10;
const static std::uint64_t DBL_PREC_FOR_PRINT = 10;

const static int UNDEFINED_FIELD_VALUE = -1;

const static std::uint64_t CHINA_TIME_ZONE_OFFSET_IN_US =
    8ULL * 3600ULL * 1000000ULL;

constexpr static int MAX_DATE_OFFSET_OF_QUERY_MD = 2;
const static std::string MIN_DATE_OF_HIS_MD = "20180101";

const static std::string PREFIX_OF_TIMER_NAME = "inst-";

}  // namespace bq
