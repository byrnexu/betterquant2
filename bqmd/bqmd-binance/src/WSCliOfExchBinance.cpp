/*!
 * \file WSCliOfExchBinance.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "WSCliOfExchBinance.hpp"

#include "BooksCache.hpp"
#include "Config.hpp"
#include "MDSvc.hpp"
#include "MDSvcDef.hpp"
#include "MDSvcOfBinanceConst.hpp"
#include "MDSvcOfBinanceUtil.hpp"
#include "MDSvcUtil.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "TopicGroupMustSubMaint.hpp"
#include "WSTask.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/MDWSCliAsyncTaskArg.hpp"
#include "util/BQUtil.hpp"
#include "util/Json.hpp"
#include "util/String.hpp"
#include "util/Util.hpp"

namespace bq::md::svc::binance {

WSCliOfExchBinance::WSCliOfExchBinance(MDSvc* mdSvc)
    : WSCliOfExch(mdSvc), booksCache_(std::make_shared<BooksCache>(mdSvc)) {}

void WSCliOfExchBinance::onBeforeOpen(
    web::WSCli* wsCli, const web::ConnMetadataSPtr& connMetadata) {
  //! 清除订单簿快照缓存和订单簿增量缓存
  booksCache_->reset();
}

WSCliAsyncTaskArgSPtr WSCliOfExchBinance::MakeWSCliAsyncTaskArg(
    const web::TaskFromSrvSPtr& task) const {
  auto ret = std::make_shared<WSCliAsyncTaskArg>();

  ret->doc_ = yyjson_read(task->msg_->get_payload().data(),
                          task->msg_->get_payload().size(), 0);
  ret->root_ = yyjson_doc_get_root(ret->doc_);

  const auto e = yyjson_obj_get(ret->root_, "e");
  if (yyjson_equals_str(e, EXCH_MD_TYPE_IN_QUOTE_OF_TRADES)) {
    ret->wsMsgType_ = MsgType::Trades;
  } else if (yyjson_equals_str(e, EXCH_MD_TYPE_IN_QUOTE_OF_BOOKS)) {
    ret->wsMsgType_ = MsgType::Books;
  } else if (yyjson_equals_str(e, EXCH_MD_TYPE_IN_QUOTE_OF_TICKERS)) {
    ret->wsMsgType_ = MsgType::Tickers;
  } else if (yyjson_equals_str(e, EXCH_MD_TYPE_IN_QUOTE_OF_CANDLE)) {
    ret->wsMsgType_ = MsgType::Candle;
  } else {
    ret->wsMsgType_ = MsgType::Others;
  }
  return ret;
}

/* trades
 *
 * {
 * "e": "aggTrade",
 * "E": 123456789,
 * "s": "BNBBTC",
 * "a": 12345,
 * "p": "0.001",
 * "q": "100",
 * "f": 100,
 * "l": 105,
 * "T": 123456785,
 * "m": true,
 * "M": true
 * }
 *
 */
std::string WSCliOfExchBinance::handleMDTrades(WSCliAsyncTaskSPtr& asyncTask) {
  auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);
  const auto& marketCode = mdSvc_->getMarketCode();
  const auto& symbolType = mdSvc_->getSymbolType();

  yyjson_val* vals = nullptr;
  yyjson_val* valExchTs = nullptr;
  yyjson_val* vala = nullptr;
  yyjson_val* valPrice = nullptr;
  yyjson_val* valSize = nullptr;
  yyjson_val* valf = nullptr;
  yyjson_val* vall = nullptr;
  yyjson_val* valTradeTime = nullptr;
  yyjson_val* valExchSide = nullptr;

  yyjson_val *valFieldName, *valFieldValue;
  yyjson_obj_iter iter;
  yyjson_obj_iter_init(arg->root_, &iter);
  while ((valFieldName = yyjson_obj_iter_next(&iter))) {
    if (yyjson_equals_str(valFieldName, "s")) {
      vals = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "E")) {
      valExchTs = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "a")) {
      vala = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "p")) {
      valPrice = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "q")) {
      valSize = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "f")) {
      valf = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "l")) {
      vall = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "T")) {
      valTradeTime = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "m")) {
      valExchSide = yyjson_obj_iter_get_val(valFieldName);
    }
  }

  std::string exchSymbolCode = yyjson_get_str(vals);
  boost::to_lower(exchSymbolCode);
  const auto [ret, symbolCode] =
      mdSvc_->getTBLMonitorOfSymbolInfo()->getSymbolCode(marketCode, symbolType,
                                                         exchSymbolCode);
  if (ret != 0) {
    const auto statusMsg = fmt::format(
        "Handle market data of trades failed because of "
        "get symbol code failed. [marketCode = {}, exchSymbolCode = {}]",
        marketCode, exchSymbolCode);
    LOG_W(statusMsg);
    return "";
  }

  const auto exchSide = yyjson_get_bool(valExchSide);
  const auto a = yyjson_get_uint(vala);
  const auto f = yyjson_get_uint(valf);
  const auto l = yyjson_get_uint(vall);

  const auto exchTs = yyjson_get_uint(valExchTs) * 1000;
  const auto tradeTime = yyjson_get_uint(valTradeTime);
  const auto price = yyjson_get_str(valPrice);
  const auto size = yyjson_get_str(valSize);

  const auto [topic, topicHash] =
      MakeTopicInfo(marketCode, symbolType, symbolCode, MDType::Trades);

  mdSvc_->getSHMSrv()->pushMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto trades = static_cast<Trades*>(shmBuf);
        trades->shmHeader_.topicHash_ = topicHash;
        trades->mdHeader_.exchTs_ = exchTs;
        trades->mdHeader_.localTs_ = asyncTask->task_->localTs_;
        trades->mdHeader_.marketCode_ = mdSvc_->getMarketCodeEnum();
        trades->mdHeader_.symbolType_ = mdSvc_->getSymbolTypeEnum();
        strncpy(trades->mdHeader_.symbolCode_, symbolCode,
                sizeof(trades->mdHeader_.symbolCode_) - 1);
        trades->mdHeader_.mdType_ = MDType::Trades;
        trades->tradeTime_ = tradeTime * 1000;
        snprintf(trades->tradeNo_, sizeof(trades->tradeNo_) - 1,
                 "%" PRIu64 "-%" PRIu64 "-%" PRIu64 "", a, f, l);
        trades->price_ = CONV(Decimal, price);
        trades->size_ = CONV(Decimal, size);
        trades->side_ = GetSide(exchSide);
        if (mdSvc_->saveMarketData()) {
          arg->marketDataOfUnifiedFmt_ = trades->dataOfUnifiedFmt();
          arg->exchTs_ = exchTs;
          arg->topic_ = topic;
        }
      },
      PUB_CHANNEL, MSG_ID_ON_MD_TRADES, sizeof(Trades));

  //! Total num: 10000; avg: 15; med: 9; min: 3; max: 804
#ifdef PERF_TEST
  EXEC_PERF_TEST("Trades", asyncTask->task_->localTs_, 10000, 100);
#endif
  return topic;
}

/* tickers
{
  "e": "24hrMiniTicker",
  "E": 123456789,
  "s": "BNBBTC",
  "c": "0.0025",
  "o": "0.0010",
  "h": "0.0025",
  "l": "0.0010",
  "v": "10000",
  "q": "18"
}
 */
std::string WSCliOfExchBinance::handleMDTickers(WSCliAsyncTaskSPtr& asyncTask) {
  auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);
  const auto& marketCode = mdSvc_->getMarketCode();
  const auto& symbolType = mdSvc_->getSymbolType();

  yyjson_val* vals = nullptr;
  yyjson_val* valExchTs = nullptr;
  yyjson_val* valLastPrice = nullptr;
  yyjson_val* valOpen = nullptr;
  yyjson_val* valHigh = nullptr;
  yyjson_val* valLow = nullptr;
  yyjson_val* valVol = nullptr;
  yyjson_val* valAmt = nullptr;

  yyjson_val *valFieldName, *valFieldValue;
  yyjson_obj_iter iter;
  yyjson_obj_iter_init(arg->root_, &iter);
  while ((valFieldName = yyjson_obj_iter_next(&iter))) {
    if (yyjson_equals_str(valFieldName, "s")) {
      vals = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "E")) {
      valExchTs = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "c")) {
      valLastPrice = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "o")) {
      valOpen = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "h")) {
      valHigh = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "l")) {
      valLow = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "v")) {
      valVol = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "q")) {
      valAmt = yyjson_obj_iter_get_val(valFieldName);
    }
  }

  std::string exchSymbolCode = yyjson_get_str(vals);
  boost::to_lower(exchSymbolCode);
  const auto [ret, symbolCode] =
      mdSvc_->getTBLMonitorOfSymbolInfo()->getSymbolCode(marketCode, symbolType,
                                                         exchSymbolCode);
  if (ret != 0) {
    const auto statusMsg = fmt::format(
        "Handle market data of tickers failed because of "
        "get symbol code failed. [marketCode = {}, exchSymbolCode = {}]",
        marketCode, exchSymbolCode);
    LOG_W(statusMsg);
    return "";
  }

  const auto exchTs = yyjson_get_uint(valExchTs) * 1000;
  const auto lastPrice = yyjson_get_str(valLastPrice);
  const auto open = yyjson_get_str(valOpen);
  const auto high = yyjson_get_str(valHigh);
  const auto low = yyjson_get_str(valLow);
  const auto vol = yyjson_get_str(valVol);
  const auto amt = yyjson_get_str(valAmt);

  const auto [topic, topicHash] =
      MakeTopicInfo(marketCode, symbolType, symbolCode, MDType::Tickers);

  mdSvc_->getSHMSrv()->pushMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto tickers = static_cast<Tickers*>(shmBuf);
        tickers->shmHeader_.topicHash_ = topicHash;
        tickers->mdHeader_.exchTs_ = exchTs;
        tickers->mdHeader_.localTs_ = asyncTask->task_->localTs_;
        tickers->mdHeader_.marketCode_ = mdSvc_->getMarketCodeEnum();
        tickers->mdHeader_.symbolType_ = mdSvc_->getSymbolTypeEnum();
        strncpy(tickers->mdHeader_.symbolCode_, symbolCode,
                sizeof(tickers->mdHeader_.symbolCode_) - 1);
        tickers->mdHeader_.mdType_ = MDType::Tickers;
        tickers->lastPrice_ = CONV(Decimal, lastPrice);
        tickers->open_ = CONV(Decimal, open);
        tickers->high_ = CONV(Decimal, high);
        tickers->low_ = CONV(Decimal, low);
        tickers->vol_ = CONV(Decimal, vol);
        tickers->amt_ = CONV(Decimal, amt);
        if (mdSvc_->saveMarketData()) {
          arg->marketDataOfUnifiedFmt_ = tickers->dataOfUnifiedFmt();
          arg->exchTs_ = exchTs;
          arg->topic_ = topic;
        }
      },
      PUB_CHANNEL, MSG_ID_ON_MD_TICKERS, sizeof(Tickers));

  //! Total num: 1043; avg: 15; med: 14; min: 6; max: 165
#ifdef PERF_TEST
  EXEC_PERF_TEST("Tickers", asyncTask->task_->localTs_, 10000, 100);
#endif
  return topic;
}

/*
 * candle
 *
 * {
 *  "e": "kline",
 *  "E": 123456789,
 *  "s": "BNBBTC",
 *  "k":
 *  {
 *  "t": 123400000,
 *  "T": 123460000,
 *  "s": "BNBBTC",
 *  "i": "1m",
 *  "f": 100,
 *  "L": 200,
 *  "o": "0.0010",
 *  "c": "0.0020",
 *  "h": "0.0025",
 *  "l": "0.0015",
 *  "v": "1000",
 *  "n": 100,
 *  "x": false,
 *  "q": "1.0000",
 *  "V": "500",
 *  "Q": "0.500",
 *  "B": "123456"
 *  }
 * }
 *
 */
std::string WSCliOfExchBinance::handleMDCandle(WSCliAsyncTaskSPtr& asyncTask) {
  auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);
  const auto& marketCode = mdSvc_->getMarketCode();
  const auto& symbolType = mdSvc_->getSymbolType();

  yyjson_val* valExchTs = yyjson_obj_get(arg->root_, "E");
  const auto exchTs = yyjson_get_uint(valExchTs) * 1000;

  yyjson_val* vals = nullptr;
  yyjson_val* valOpen = nullptr;
  yyjson_val* valClose = nullptr;
  yyjson_val* valHigh = nullptr;
  yyjson_val* valLow = nullptr;
  yyjson_val* valAmt = nullptr;
  yyjson_val* valVol = nullptr;

  const auto valk = yyjson_obj_get(arg->root_, "k");

  yyjson_val *valFieldName, *valFieldValue;
  yyjson_obj_iter iter;
  yyjson_obj_iter_init(valk, &iter);
  while ((valFieldName = yyjson_obj_iter_next(&iter))) {
    if (yyjson_equals_str(valFieldName, "s")) {
      vals = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "o")) {
      valOpen = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "c")) {
      valClose = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "h")) {
      valHigh = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "l")) {
      valLow = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "q")) {
      valAmt = yyjson_obj_iter_get_val(valFieldName);
    } else if (yyjson_equals_str(valFieldName, "v")) {
      valVol = yyjson_obj_iter_get_val(valFieldName);
    }
  }

  std::string exchSymbolCode = yyjson_get_str(vals);
  boost::to_lower(exchSymbolCode);
  const auto [ret, symbolCode] =
      mdSvc_->getTBLMonitorOfSymbolInfo()->getSymbolCode(marketCode, symbolType,
                                                         exchSymbolCode);
  if (ret) {
    const auto statusMsg = fmt::format(
        "Handle market data of candle failed because of "
        "get symbol code failed. [marketCode = {}, exchSymbolCode = {}]",
        marketCode, exchSymbolCode);
    LOG_W(statusMsg);
    return "";
  }

  const auto [topic, topicHash] =
      MakeTopicInfo(marketCode, symbolType, symbolCode, MDType::Candle,
                    SUFFIX_OF_CANDLE_DETAIL);

  mdSvc_->getSHMSrv()->pushMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto candle = static_cast<Candle*>(shmBuf);
        candle->shmHeader_.topicHash_ = topicHash;
        candle->mdHeader_.exchTs_ = exchTs;
        candle->mdHeader_.localTs_ = asyncTask->task_->localTs_;
        candle->mdHeader_.marketCode_ = mdSvc_->getMarketCodeEnum();
        candle->mdHeader_.symbolType_ = mdSvc_->getSymbolTypeEnum();
        strncpy(candle->mdHeader_.symbolCode_, symbolCode,
                sizeof(candle->mdHeader_.symbolCode_) - 1);
        candle->mdHeader_.mdType_ = MDType::Candle;
        candle->open_ = CONV(Decimal, yyjson_get_str(valOpen));
        candle->high_ = CONV(Decimal, yyjson_get_str(valHigh));
        candle->low_ = CONV(Decimal, yyjson_get_str(valLow));
        candle->close_ = CONV(Decimal, yyjson_get_str(valClose));
        candle->vol_ = CONV(Decimal, yyjson_get_str(valVol));
        candle->amt_ = CONV(Decimal, yyjson_get_str(valAmt));
        if (mdSvc_->saveMarketData()) {
          arg->marketDataOfUnifiedFmt_ = candle->dataOfUnifiedFmt();
          arg->exchTs_ = exchTs;
          arg->topic_ = topic;
        }
      },
      PUB_CHANNEL, MSG_ID_ON_MD_CANDLE, sizeof(Candle));

  //! Total num: 611; avg: 16; med: 16; min: 6; max: 157
#ifdef PERF_TEST
  EXEC_PERF_TEST("Candle", asyncTask->task_->localTs_, 10000, 100);
#endif
  return topic;
}

/*
{
  "e": "depthUpdate",
  "E": 123456789,
  "s": "BNBBTC",
  "U": 157,
  "u": 160,
  "b": [
    [
      "0.0024",
      "10"
    ]
  ],
  "a": [
    [
      "0.0026",
      "100"
    ]
  ]
}
*/
std::string WSCliOfExchBinance::handleMDBooks(WSCliAsyncTaskSPtr& asyncTask) {
  auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);
  const auto& marketCode = mdSvc_->getMarketCode();
  const auto& symbolType = mdSvc_->getSymbolType();

  const auto s = yyjson_obj_get(arg->root_, "s");
  std::string exchSymbolCode = yyjson_get_str(s);
  boost::to_lower(exchSymbolCode);
  const auto [ret, symbolCode] =
      mdSvc_->getTBLMonitorOfSymbolInfo()->getSymbolCode(marketCode, symbolType,
                                                         exchSymbolCode);
  if (ret != 0) {
    const auto statusMsg = fmt::format(
        "Handle market data of books failed because of "
        "get symbol code failed. [marketCode = {}, exchSymbolCode = {}]",
        marketCode, exchSymbolCode);
    LOG_W(statusMsg);
    return "";
  }

  auto [retOfHandle, snapshot] =
      booksCache_->handle(symbolCode, exchSymbolCode, arg->root_);
  if (retOfHandle != 0) {
    LOG_D("Handle market data of books snapshot for {} failed.", symbolCode);
    return "";
  }
  const auto valExchTs = yyjson_obj_get(arg->root_, "E");
  const auto exchTs = yyjson_get_uint(valExchTs) * 1000;

  const auto [topic, topicHash] =
      MakeTopicInfo(marketCode, symbolType, symbolCode, MDType::Books,
                    Int2StrInCompileTime<MAX_DEPTH_LEVEL>::type::value);

  mdSvc_->getSHMSrv()->pushMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto books = static_cast<Books*>(shmBuf);
        books->shmHeader_.topicHash_ = topicHash;
        books->mdHeader_.exchTs_ = exchTs;
        books->mdHeader_.localTs_ = asyncTask->task_->localTs_;
        books->mdHeader_.marketCode_ = mdSvc_->getMarketCodeEnum();
        books->mdHeader_.symbolType_ = mdSvc_->getSymbolTypeEnum();
        strncpy(books->mdHeader_.symbolCode_, symbolCode,
                sizeof(books->mdHeader_.symbolCode_) - 1);
        books->mdHeader_.mdType_ = MDType::Books;

        std::uint32_t asksLvl = 0;
        for (auto iter = std::begin(*snapshot->asks_);
             iter != std::end(*snapshot->asks_); ++iter, ++asksLvl) {
          if (asksLvl >= MAX_DEPTH_LEVEL) break;
          const auto& depthData = iter->second;
          books->asks_[asksLvl].price_ = depthData->price_;
          books->asks_[asksLvl].size_ = depthData->size_;
        }

        std::uint32_t bidsLvl = 0;
        for (auto iter = std::begin(*snapshot->bids_);
             iter != std::end(*snapshot->bids_); ++iter, ++bidsLvl) {
          if (bidsLvl >= MAX_DEPTH_LEVEL) break;
          const auto& depthData = iter->second;
          books->bids_[bidsLvl].price_ = depthData->price_;
          books->bids_[bidsLvl].size_ = depthData->size_;
        }
        if (mdSvc_->saveMarketData()) {
          arg->marketDataOfUnifiedFmt_ =
              books->dataOfUnifiedFmt(mdSvc_->getBooksDepthLevelOfSave());
          arg->exchTs_ = exchTs;
          arg->topic_ = topic;
        }
      },
      PUB_CHANNEL, MSG_ID_ON_MD_BOOKS, sizeof(Books));

  //! Total num: 10000; avg: 222; med: 35; min: 11; max: 289810
#ifdef PERF_TEST
  EXEC_PERF_TEST("Books", asyncTask->task_->localTs_, 10000, 100);
#endif
  return topic;
}

/*
 * {
 *   "result": null,
 *   "id": 1
 * }
 *
 * {"code": 1, "msg": "Invalid value type: expected Boolean", "id": '%s'}
 *
 */
//! 如果是订阅或者取消订阅的应答那么会调用RspParser::GetTopicGroupForSubOrUnSubAgain
bool WSCliOfExchBinance::isSubOrUnSubRet(WSCliAsyncTaskSPtr& asyncTask) {
  const auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);
  const auto result = yyjson_obj_get(arg->root_, "result");
  const auto id = yyjson_obj_get(arg->root_, "id");
  if (result != nullptr && id != nullptr) {
    return true;
  }

  const auto code = yyjson_obj_get(arg->root_, "code");
  const auto msg = yyjson_obj_get(arg->root_, "msg");
  if (code != nullptr && msg != nullptr) {
    return true;
  }

  return false;
}

}  // namespace bq::md::svc::binance
