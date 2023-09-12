/*!
 * \file RawMDHandlerOfXTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#include "RawMDHandlerOfXTP.hpp"

#include "Config.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvcOfXTP.hpp"
#include "MDSvcOfXTPUtil.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/RawMD.hpp"
#include "def/RawMDAsyncTaskArg.hpp"
#include "util/BQMDUtil.hpp"
#include "util/Datetime.hpp"
#include "util/File.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::md::svc::xtp {

std::tuple<int, RawMDAsyncTaskSPtr> RawMDHandlerOfXTP::makeAsyncTask(
    const RawMDSPtr& task) {
  switch (task->msgType_) {
    case MsgType::NewSymbol: {
      const auto md = static_cast<XTPQFI*>(task->data_);
      task->marketCode_ = GetMarketCode(md->exchange_id);
      task->symbolType_ = SymbolType::Spot;
      task->symbolCode_ = md->ticker;
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    case MsgType::Tickers: {
      const auto md = static_cast<XTPMD*>(task->data_);
      task->marketCode_ = GetMarketCode(md->exchange_id);
      task->symbolType_ = SymbolType::Spot;
      task->symbolCode_ = md->ticker;
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    case MsgType::Trades: {
      const auto md = static_cast<XTPTBT*>(task->data_);
      task->marketCode_ = GetMarketCode(md->exchange_id);
      task->symbolType_ = SymbolType::Spot;
      task->symbolCode_ = md->ticker;
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    case MsgType::Orders: {
      const auto md = static_cast<XTPTBT*>(task->data_);
      task->marketCode_ = GetMarketCode(md->exchange_id);
      task->symbolType_ = SymbolType::Spot;
      task->symbolCode_ = md->ticker;
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    case MsgType::Books: {
      const auto md = static_cast<XTPOB*>(task->data_);
      task->marketCode_ = GetMarketCode(md->exchange_id);
      task->symbolType_ = SymbolType::Spot;
      task->symbolCode_ = md->ticker;
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    default:
      return {-1, nullptr};
  }

  return {-1, nullptr};
}

void RawMDHandlerOfXTP::handleNewSymbol(RawMDAsyncTaskSPtr& asyncTask) {
  const auto exchSymbolInfo = static_cast<XTPQFI*>(asyncTask->task_->data_);

  auto symbolInfo = std::make_shared<db::symbolInfo::Record>();

  symbolInfo->marketCode =
      GetMarketName(GetMarketCode(exchSymbolInfo->exchange_id));
  const SymbolType symbolType = GetSymbolType(exchSymbolInfo->security_type);
  symbolInfo->symbolType = magic_enum::enum_name(symbolType);
  symbolInfo->symbolCode = exchSymbolInfo->ticker;
  symbolInfo->symbolName = exchSymbolInfo->ticker_name;
  symbolInfo->exchSymbolCode = exchSymbolInfo->ticker;
  symbolInfo->alias;

  symbolInfo->underlyingIndex;

  symbolInfo->baseCurrency;
  symbolInfo->quoteCurrency = CN_OFFICIAL_CURRENCY;
  symbolInfo->settlementCurrency = CN_OFFICIAL_CURRENCY;

  symbolInfo->upperLimitPrice =
      fmt::format("{}", exchSymbolInfo->upper_limit_price);
  symbolInfo->lowerLimitPrice =
      fmt::format("{}", exchSymbolInfo->lower_limit_price);

  symbolInfo->preClosePrice =
      fmt::format("{}", exchSymbolInfo->pre_close_price);
  symbolInfo->preSettlementPrice = fmt::format("{}", 0);

  symbolInfo->precOfBidOrderPrice =
      fmt::format("{}", exchSymbolInfo->price_tick);
  symbolInfo->precOfBidOrderVol =
      fmt::format("{}", exchSymbolInfo->bid_qty_unit);
  symbolInfo->precOfAskOrderPrice =
      fmt::format("{}", exchSymbolInfo->price_tick);
  symbolInfo->precOfAskOrderVol =
      fmt::format("{}", exchSymbolInfo->ask_qty_unit);

  symbolInfo->minBidOrderVol =
      fmt::format("{}", exchSymbolInfo->bid_qty_lower_limit);
  symbolInfo->maxBidOrderVol =
      fmt::format("{}", exchSymbolInfo->bid_qty_upper_limit);
  symbolInfo->minAskOrderVol =
      fmt::format("{}", exchSymbolInfo->ask_qty_lower_limit);
  symbolInfo->maxAskOrderVol =
      fmt::format("{}", exchSymbolInfo->ask_qty_upper_limit);

  symbolInfo->minBidOrderAmt = "0";
  symbolInfo->maxBidOrderAmt = "0";
  symbolInfo->minAskOrderAmt = "0";
  symbolInfo->maxAskOrderAmt = "0";

  symbolInfo->parValue;
  symbolInfo->contractMult = "0";
  symbolInfo->maxLeverage = "0";

  symbolInfo->symbolState =
      magic_enum::enum_name(GetSymbolState(exchSymbolInfo->security_status));

  symbolInfo->launchTime = UNDEFINED_FIELD_MIN_DATETIME;
  symbolInfo->deliveryTime = UNDEFINED_FIELD_MAX_DATETIME;

  LOG_T("Get symbol {}.{} {} {}", symbolInfo->symbolCode,
        symbolInfo->marketCode, symbolInfo->symbolName,
        exchSymbolInfo->ticker_name);
  const auto tblRec = std::make_shared<db::TBLRec<TBLSymbolInfo>>(symbolInfo);
  ExecRecReplace(mdSvc_->getDBEng(), tblRec);
}

bool RawMDHandlerOfXTP::handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) {
  const auto md = static_cast<XTPMD*>(asyncTask->task_->data_);
  const auto exchTs =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  if (!checkIfMDIsLegal(asyncTask, exchTs)) {
    return false;
  }

  if (md->open_price == 0 || md->open_price == DBL_MAX) return false;
  if (md->high_price == 0 || md->high_price == DBL_MAX) return false;
  if (md->low_price == 0 || md->low_price == DBL_MAX) return false;
  if (md->last_price == 0 || md->last_price == DBL_MAX) return false;

  // buf is released when asyncTask->task_ is destructed
  auto buf = calloc(1, sizeof(Tickers));
  asyncTask->task_->dataAfterConv_ = buf;
  asyncTask->task_->dataAfterConvLen_ = sizeof(Tickers);

  // init tickers
  auto tickers = static_cast<Tickers*>(buf);
  tickers->shmHeader_.topicHash_ = asyncTask->task_->topicHash_;

  tickers->mdHeader_.localTs_ = asyncTask->task_->localTs_;
  tickers->mdHeader_.marketCode_ = asyncTask->task_->marketCode_;
  tickers->mdHeader_.symbolType_ = asyncTask->task_->symbolType_;
  strncpy(tickers->mdHeader_.symbolCode_, asyncTask->task_->symbolCode_.c_str(),
          sizeof(tickers->mdHeader_.symbolCode_) - 1);
  tickers->mdHeader_.mdType_ = asyncTask->task_->mdType_;
  tickers->mdHeader_.exchTs_ = exchTs;

  tickers->open_ = md->open_price;
  tickers->high_ = md->high_price;
  tickers->low_ = md->low_price;

  tickers->lastPrice_ = md->last_price;
  tickers->lastSize_ = 0;

  tickers->upperLimitPrice_ = md->upper_limit_price;
  tickers->lowerLimitPrice_ = md->lower_limit_price;

  tickers->preClosePrice_ = md->pre_close_price;
  tickers->preSettlementPrice_ = md->pre_settl_price;

  tickers->closePrice_ = md->close_price;
  tickers->settlementPrice_ = md->settl_price;

  tickers->preOpenInterest_ = md->pre_total_long_positon;
  tickers->openInterest_ = md->total_long_positon;

  tickers->vol_ = md->qty;
  tickers->amt_ = md->turnover;

  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL_IN_TICKER; ++i) {
    if (i == sizeof(md->bid_qty)) break;
    tickers->bids_[i].price_ = md->bid[i];
    tickers->bids_[i].size_ = md->bid_qty[i];
    if (i == 0) {
      tickers->bidPrice_ = md->bid[i];
      tickers->bidSize_ = md->bid_qty[i];
    }
  }

  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL_IN_TICKER; ++i) {
    if (i == sizeof(md->ask_qty)) break;
    tickers->asks_[i].price_ = md->ask[i];
    tickers->asks_[i].size_ = md->ask_qty[i];
    if (i == 0) {
      tickers->askPrice_ = md->ask[i];
      tickers->askSize_ = md->ask_qty[i];
    }
  }

  strncpy(tickers->tradingDay_, mdSvc_->getTradingDay().c_str(),
          sizeof(tickers->tradingDay_) - 1);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(tickers->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.",
          GetMarketName(tickers->mdHeader_.marketCode_));
    return true;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_TICKERS, tickers, sizeof(Tickers));
  return true;
}

bool RawMDHandlerOfXTP::handleMDTrades(RawMDAsyncTaskSPtr& asyncTask) {
  const auto md = static_cast<XTPTBT*>(asyncTask->task_->data_);
  const auto exchTs =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  if (!checkIfMDIsLegal(asyncTask, exchTs)) {
    return false;
  }

  if (md->trade.price == 0 || md->trade.price == DBL_MAX) return false;
  if (md->trade.qty == 0 || md->trade.qty == DBL_MAX) return false;

  // buf is released when asyncTask->task_ is destructed
  auto buf = calloc(1, sizeof(Trades));
  asyncTask->task_->dataAfterConv_ = buf;
  asyncTask->task_->dataAfterConvLen_ = sizeof(Trades);

  // init trades
  auto trades = static_cast<Trades*>(buf);

  trades->shmHeader_.topicHash_ = asyncTask->task_->topicHash_;

  trades->mdHeader_.localTs_ = asyncTask->task_->localTs_;
  trades->mdHeader_.marketCode_ = asyncTask->task_->marketCode_;
  trades->mdHeader_.symbolType_ = asyncTask->task_->symbolType_;
  strncpy(trades->mdHeader_.symbolCode_, asyncTask->task_->symbolCode_.c_str(),
          sizeof(trades->mdHeader_.symbolCode_) - 1);
  trades->mdHeader_.mdType_ = asyncTask->task_->mdType_;
  trades->mdHeader_.exchTs_ = exchTs;

  trades->tradeTime_ =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  snprintf(trades->tradeNo_, sizeof(trades->tradeNo_) - 1, "%d-%" PRIu64 "",
           md->trade.channel_no, md->trade.seq);
  trades->side_ =
      GetSideFromTrades(asyncTask->task_->marketCode_, md->trade.trade_flag);
  trades->price_ = md->trade.price;
  trades->size_ = md->trade.qty;
  snprintf(trades->bidOrderId_, sizeof(trades->bidOrderId_) - 1, "%" PRIu64 "",
           md->trade.bid_no);
  snprintf(trades->askOrderId_, sizeof(trades->askOrderId_) - 1, "%" PRIu64 "",
           md->trade.ask_no);

  strncpy(trades->tradingDay_, mdSvc_->getTradingDay().c_str(),
          sizeof(trades->tradingDay_) - 1);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(trades->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.",
          GetMarketName(trades->mdHeader_.marketCode_));
    return true;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_TRADES, trades, sizeof(Trades));
  return true;
}

bool RawMDHandlerOfXTP::handleMDOrders(RawMDAsyncTaskSPtr& asyncTask) {
  const auto md = static_cast<XTPTBT*>(asyncTask->task_->data_);
  const auto exchTs =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  if (!checkIfMDIsLegal(asyncTask, exchTs)) {
    return false;
  }

  if (md->entrust.price == 0 || md->entrust.price == DBL_MAX) return false;
  if (md->entrust.qty == 0 || md->entrust.qty == DBL_MAX) return false;

  // buf is released when asyncTask->task_ is destructed
  auto buf = calloc(1, sizeof(Orders));
  asyncTask->task_->dataAfterConv_ = buf;
  asyncTask->task_->dataAfterConvLen_ = sizeof(Orders);

  // init orders
  auto orders = static_cast<Orders*>(buf);

  orders->shmHeader_.topicHash_ = asyncTask->task_->topicHash_;

  orders->mdHeader_.localTs_ = asyncTask->task_->localTs_;
  orders->mdHeader_.marketCode_ = asyncTask->task_->marketCode_;
  orders->mdHeader_.symbolType_ = asyncTask->task_->symbolType_;
  strncpy(orders->mdHeader_.symbolCode_, asyncTask->task_->symbolCode_.c_str(),
          sizeof(orders->mdHeader_.symbolCode_) - 1);
  orders->mdHeader_.mdType_ = asyncTask->task_->mdType_;
  orders->mdHeader_.exchTs_ = exchTs;

  orders->orderTime_ =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  snprintf(orders->orderNo_, sizeof(orders->orderNo_) - 1, "%d-%" PRIu64 "",
           md->entrust.channel_no, md->entrust.seq);
  orders->side_ =
      GetSideFromOrders(asyncTask->task_->marketCode_, md->entrust.side);
  orders->price_ = md->entrust.price;
  orders->size_ = md->entrust.qty;

  strncpy(orders->tradingDay_, mdSvc_->getTradingDay().c_str(),
          sizeof(orders->tradingDay_) - 1);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(orders->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.",
          GetMarketName(orders->mdHeader_.marketCode_));
    return true;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_ORDERS, orders, sizeof(Orders));
  return true;
}

bool RawMDHandlerOfXTP::handleMDBooks(RawMDAsyncTaskSPtr& asyncTask) {
  const auto md = static_cast<XTPOB*>(asyncTask->task_->data_);
  const auto exchTs =
      ConvertDatetimeToTs(md->data_time) - CHINA_TIME_ZONE_OFFSET_IN_US;
  if (!checkIfMDIsLegal(asyncTask, exchTs)) {
    return false;
  }

  // buf is released when asyncTask->task_ is destructed
  auto buf = calloc(1, sizeof(Books));
  asyncTask->task_->dataAfterConv_ = buf;
  asyncTask->task_->dataAfterConvLen_ = sizeof(Books);

  // init books
  auto books = static_cast<Books*>(buf);
  books->shmHeader_.topicHash_ = asyncTask->task_->topicHash_;

  books->mdHeader_.localTs_ = asyncTask->task_->localTs_;
  books->mdHeader_.marketCode_ = asyncTask->task_->marketCode_;
  books->mdHeader_.symbolType_ = asyncTask->task_->symbolType_;
  strncpy(books->mdHeader_.symbolCode_, asyncTask->task_->symbolCode_.c_str(),
          sizeof(books->mdHeader_.symbolCode_) - 1);
  books->mdHeader_.mdType_ = asyncTask->task_->mdType_;
  books->mdHeader_.exchTs_ = exchTs;

  if (md->last_price == 0 || md->last_price == DBL_MAX) return false;

  books->lastPrice_ = md->last_price;
  books->totalVol_ = md->qty;
  books->totalAmt_ = md->turnover;
  books->tradesCount_ = md->trades_count;

  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL_OF_CN; ++i) {
    if (i == sizeof(md->bid_qty)) break;
    books->bids_[i].price_ = md->bid[i];
    books->bids_[i].size_ = md->bid_qty[i];
  }

  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL_OF_CN; ++i) {
    if (i == sizeof(md->ask_qty)) break;
    books->asks_[i].price_ = md->ask[i];
    books->asks_[i].size_ = md->ask_qty[i];
  }

  strncpy(books->tradingDay_, mdSvc_->getTradingDay().c_str(),
          sizeof(books->tradingDay_) - 1);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(books->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.",
          GetMarketName(books->mdHeader_.marketCode_));
    return true;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_BOOKS, books, sizeof(Books));
  return true;
}

}  // namespace bq::md::svc::xtp
