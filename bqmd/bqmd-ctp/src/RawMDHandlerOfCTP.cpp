/*!
 * \file RawMDHandlerOfCTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#include "RawMDHandlerOfCTP.hpp"

#include "Config.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvcOfCTP.hpp"
#include "MDSvcOfCTPUtil.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/Def.hpp"
#include "def/RawMD.hpp"
#include "def/RawMDAsyncTaskArg.hpp"
#include "def/SymbolInfoTable.hpp"
#include "util/BQMDUtil.hpp"
#include "util/BQUtilOfCTP.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/File.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::md::svc::ctp {

std::tuple<int, RawMDAsyncTaskSPtr> RawMDHandlerOfCTP::makeAsyncTask(
    const RawMDSPtr& task) {
  switch (task->msgType_) {
    case MsgType::NewSymbol: {
      const auto md = static_cast<CThostFtdcInstrumentField*>(task->data_);
      task->marketCode_ = GetMarketCode(md->ExchangeID);
      task->symbolType_ = SymbolType::Futures;
      //! 因为topic是根据这里的symbolCode生成，所以这里先转换为本地代码
      task->symbolCode_ =
          bq::md::GetSymbolCode(task->marketCode_, md->InstrumentID);
      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    case MsgType::Tickers: {
      const auto md = static_cast<CThostFtdcDepthMarketDataField*>(task->data_);
      const auto [statusCode, symbolInfo] =
          mdSvc_->getSymbolInfoTable()->getSymbolInfoByExchSym(
              md->InstrumentID);
      task->marketCode_ =
          (statusCode == 0) ? symbolInfo->marketCode_ : MarketCode::Others;
      task->symbolType_ = SymbolType::Futures;
      //! 因为topic是根据这里的symbolCode生成，所以这里先转换为本地代码
      task->symbolCode_ =
          bq::md::GetSymbolCode(task->marketCode_, md->InstrumentID);

      return {0, std::make_shared<RawMDAsyncTask>(task)};
    };

    default:
      return {-1, nullptr};
  }

  return {-1, nullptr};
}

void RawMDHandlerOfCTP::handleNewSymbol(RawMDAsyncTaskSPtr& asyncTask) {
  //! 代码入库
  auto recSymbolInfo = makeRecSymbolInfo(asyncTask);
  const auto tblRec =
      std::make_shared<db::TBLRec<TBLSymbolInfo>>(recSymbolInfo);
  ExecRecReplace(mdSvc_->getDBEng(), tblRec);

  //! symbolInfoTable结构维护，symbolInfoTable用于订阅时获取交易所代码和全订阅
  //! 时候的交易所代码列表
  const auto marketCode = GetMarketCode(recSymbolInfo->marketCode);
  if (marketCode == MarketCode::Others) {
    LOG_W("Get invalid market code. [marketCode: {}; symbolCode: {}]",
          recSymbolInfo->marketCode, recSymbolInfo->symbolCode);
  }
  const auto symbolType =
      magic_enum::enum_cast<SymbolType>(recSymbolInfo->symbolType).value();
  mdSvc_->getSymbolInfoTable()->add(std::make_shared<SymbolInfo>(
      marketCode, symbolType, recSymbolInfo->symbolCode,
      recSymbolInfo->exchSymbolCode));

  if (asyncTask->task_->isLast_) {
    LOG_I("Query symbol table success. [size = {}]",
          mdSvc_->getSymbolInfoTable()->size());
    const auto mdSvc = dynamic_cast<MDSvcOfCTP*>(mdSvc_);
    mdSvc->getBarrierOfGetSymbolTable()->set_value();
  }
}

db::symbolInfo::RecordSPtr RawMDHandlerOfCTP::makeRecSymbolInfo(
    RawMDAsyncTaskSPtr& asyncTask) {
  const auto exchSymbolInfo =
      static_cast<CThostFtdcInstrumentField*>(asyncTask->task_->data_);

  auto symbolInfo = std::make_shared<db::symbolInfo::Record>();

  symbolInfo->marketCode = exchSymbolInfo->ExchangeID;
  symbolInfo->symbolType =
      ENUM_TO_STR(GetSymbolType(exchSymbolInfo->ProductClass));
  symbolInfo->symbolCode = bq::md::GetSymbolCode(asyncTask->task_->marketCode_,
                                                 exchSymbolInfo->InstrumentID);
  symbolInfo->symbolName =
      CodeConvert(exchSymbolInfo->InstrumentName, "gb2312", "utf8");
  symbolInfo->exchSymbolCode = exchSymbolInfo->InstrumentID;
  symbolInfo->alias;

  symbolInfo->underlyingIndex;

  symbolInfo->baseCurrency;
  symbolInfo->quoteCurrency = CN_OFFICIAL_CURRENCY;
  symbolInfo->settlementCurrency = CN_OFFICIAL_CURRENCY;

  symbolInfo->upperLimitPrice = "0";
  symbolInfo->lowerLimitPrice = "0";

  symbolInfo->preClosePrice = "0";
  symbolInfo->preSettlementPrice = "0";

  symbolInfo->precOfBidOrderPrice =
      fmt::format("{}", exchSymbolInfo->PriceTick);
  symbolInfo->precOfBidOrderVol = "1";
  symbolInfo->precOfAskOrderPrice =
      fmt::format("{}", exchSymbolInfo->PriceTick);
  symbolInfo->precOfAskOrderVol = "1";

  symbolInfo->minBidOrderVol =
      fmt::format("{}", exchSymbolInfo->MinLimitOrderVolume);
  symbolInfo->maxBidOrderVol =
      fmt::format("{}", exchSymbolInfo->MaxLimitOrderVolume);
  symbolInfo->minAskOrderVol =
      fmt::format("{}", exchSymbolInfo->MinLimitOrderVolume);
  symbolInfo->maxAskOrderVol =
      fmt::format("{}", exchSymbolInfo->MaxLimitOrderVolume);

  symbolInfo->minBidOrderAmt = "0";
  symbolInfo->maxBidOrderAmt = "0";
  symbolInfo->minAskOrderAmt = "0";
  symbolInfo->maxAskOrderAmt = "0";

  symbolInfo->parValue = exchSymbolInfo->VolumeMultiple;
  symbolInfo->contractMult = std::to_string(exchSymbolInfo->VolumeMultiple);
  symbolInfo->maxLeverage = "0";

  symbolInfo->symbolState = magic_enum::enum_name(SymbolState::Online);

  symbolInfo->launchTime = UNDEFINED_FIELD_MIN_DATETIME;
  symbolInfo->deliveryTime = UNDEFINED_FIELD_MAX_DATETIME;

  LOG_T("Get symbol {}.{} {}", symbolInfo->symbolCode, symbolInfo->marketCode,
        symbolInfo->symbolName);

  return symbolInfo;
}

bool RawMDHandlerOfCTP::handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) {
  const auto md =
      static_cast<CThostFtdcDepthMarketDataField*>(asyncTask->task_->data_);
  const auto exchTs =
      CalcTs(mdSvc_->getTradingDay(), md->UpdateTime, md->UpdateMillisec) -
      CHINA_TIME_ZONE_OFFSET_IN_US;
  if (!checkIfMDIsLegal(asyncTask, exchTs)) {
    return false;
  }

  if (md->OpenPrice == 0 || md->OpenPrice == DBL_MAX) return false;
  if (md->HighestPrice == 0 || md->HighestPrice == DBL_MAX) return false;
  if (md->LowestPrice == 0 || md->LowestPrice == DBL_MAX) return false;
  if (md->LastPrice == 0 || md->LastPrice == DBL_MAX) return false;

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

  // fields in body
  tickers->open_ = md->OpenPrice;
  tickers->high_ = md->HighestPrice;
  tickers->low_ = md->LowestPrice;

  tickers->lastPrice_ = md->LastPrice;
  tickers->lastSize_ = 0;

  tickers->upperLimitPrice_ = md->UpperLimitPrice;
  tickers->lowerLimitPrice_ = md->LowerLimitPrice;

  tickers->preClosePrice_ = md->PreClosePrice;
  tickers->preSettlementPrice_ = md->PreSettlementPrice;

  tickers->closePrice_ = md->ClosePrice;
  tickers->settlementPrice_ = md->SettlementPrice;

  tickers->preOpenInterest_ = md->PreOpenInterest;
  tickers->openInterest_ = md->OpenInterest;

  tickers->vol_ = md->Volume;
  tickers->amt_ = md->Turnover;

  tickers->bidPrice_ = md->BidPrice1;
  tickers->bidSize_ = md->BidVolume1;
  tickers->bids_[0].price_ = md->BidPrice1;
  tickers->bids_[0].size_ = md->BidVolume1;
  tickers->bids_[1].price_ = md->BidPrice2;
  tickers->bids_[1].size_ = md->BidVolume2;
  tickers->bids_[2].price_ = md->BidPrice3;
  tickers->bids_[2].size_ = md->BidVolume3;
  tickers->bids_[3].price_ = md->BidPrice4;
  tickers->bids_[3].size_ = md->BidVolume4;
  tickers->bids_[4].price_ = md->BidPrice5;
  tickers->bids_[4].size_ = md->BidVolume5;

  tickers->askPrice_ = md->AskPrice1;
  tickers->askSize_ = md->AskVolume1;
  tickers->asks_[0].price_ = md->AskPrice1;
  tickers->asks_[0].size_ = md->AskVolume1;
  tickers->asks_[1].price_ = md->AskPrice2;
  tickers->asks_[1].size_ = md->AskVolume2;
  tickers->asks_[2].price_ = md->AskPrice3;
  tickers->asks_[2].size_ = md->AskVolume3;
  tickers->asks_[3].price_ = md->AskPrice4;
  tickers->asks_[3].size_ = md->AskVolume4;
  tickers->asks_[4].price_ = md->AskPrice5;
  tickers->asks_[4].size_ = md->AskVolume5;

  strncpy(tickers->tradingDay_, md->TradingDay,
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

}  // namespace bq::md::svc::ctp
