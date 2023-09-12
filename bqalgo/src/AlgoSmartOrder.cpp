/*!
 * \file AlgoSmartOrder.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 *
 */

#include "AlgoSmartOrder.hpp"

#include "AlgoMgr.hpp"
#include "AlgoSmartOrderDef.hpp"
#include "AlgoUtil.hpp"
#include "CommonIPCData.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCTask.hpp"
#include "StgEngImpl.hpp"
#include "TrdSymParams.hpp"
#include "db/DBE.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "util/BQUtil.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::algo {

SmartOrder::SmartOrder(AlgoMgr* algoMgr, const StgInstInfoSPtr& stgInstInfo,
                       const std::string& algoType, const std::string& algoName,
                       std::uint32_t lifetime)
    : AlgoOrder(algoMgr, stgInstInfo, algoType, algoName, lifetime) {}

int SmartOrder::afterInit(const std::string& algoParamsInJsonFmt) {
  if (const auto statusCode = initAlgoParams(algoParamsInJsonFmt);
      statusCode != 0) {
    return statusCode;
  }

  if (const auto statusCode = initTrdSymParams(); statusCode != 0) {
    return statusCode;
  }

  return 0;
}

int SmartOrder::initAlgoParams(const std::string& algoParamsInJsonFmt) {
  Doc doc;
  if (doc.Parse(algoParamsInJsonFmt.data()).HasParseError()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params in json format. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("trdAcctId") || !doc["trdAcctId"].IsUint()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params trdAcctId. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("marketCode") || !doc["marketCode"].IsString()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params marketCode. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("symbolCode") || !doc["symbolCode"].IsString()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params symbolCode. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("side") || !doc["side"].IsString()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params side. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("orderSize")) {
    getStgEng()->logInfo("[ALGO] Invalid algo params orderSize. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc["orderSize"].IsUint() && !doc["orderSize"].IsDouble()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params orderSize. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("initialUrgency") || !doc["initialUrgency"].IsString()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params initialUrgency. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("tickerOffset") || !doc["tickerOffset"].IsUint()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params tickerOffset. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("closeTDayStg") || !doc["closeTDayStg"].IsString()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params closeTDayStg. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("priceRange") || !doc["priceRange"].IsArray()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params priceRange. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (doc["priceRange"].Size() != 2) {
    getStgEng()->logInfo(
        "[ALGO] Invalid size of algo params priceRange. [{}] {}",
        {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (doc["priceRange"][0].IsDouble() || doc["priceRange"][0].IsUint()) {
  } else {
    getStgEng()->logInfo(
        "[ALGO] Invalid element type of algo params priceRange. [{}] {}",
        {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (doc["priceRange"][1].IsDouble() || doc["priceRange"][1].IsUint()) {
  } else {
    getStgEng()->logInfo(
        "[ALGO] Invalid element type of algo params priceRange. [{}] {}",
        {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  const auto marketCodeInStrFmt = doc["marketCode"].GetString();
  const auto optMarketCode =
      magic_enum::enum_cast<MarketCode>(marketCodeInStrFmt);
  if (optMarketCode == std::nullopt) {
    getStgEng()->logInfo("[ALGO] Invalid algo params marketCode. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  const auto sideInStrFmt = doc["side"].GetString();
  const auto optSide = magic_enum::enum_cast<Side>(sideInStrFmt);
  if (optSide == std::nullopt) {
    getStgEng()->logInfo("[ALGO] Invalid algo params side. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  std::optional<PosDirection> optPosDirection;
  if (doc.HasMember("posDirection") && doc["posDirection"].IsString()) {
    const auto posDirectionInStrFmt = doc["posDirection"].GetString();
    optPosDirection = magic_enum::enum_cast<PosDirection>(posDirectionInStrFmt);
    if (optPosDirection == std::nullopt) {
      getStgEng()->logInfo("[ALGO] Invalid algo params posDirection. [{}] {}",
                           {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
      return SCODE_ALGO_INVALID_ALGO_PARAM;
    }
  }

  const auto initialUrgencyInStrFmt = doc["initialUrgency"].GetString();
  const auto optInitialUrgency =
      magic_enum::enum_cast<Urgency>(initialUrgencyInStrFmt);
  if (optInitialUrgency == std::nullopt) {
    getStgEng()->logInfo("[ALGO] Invalid algo params initialUrgency. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  const auto closeTDayStgInStrFmt = doc["closeTDayStg"].GetString();
  const auto optCloseTDayStg =
      magic_enum::enum_cast<CloseTDayStg>(closeTDayStgInStrFmt);
  if (optCloseTDayStg == std::nullopt) {
    getStgEng()->logInfo("[ALGO] Invalid algo params closeTDayStg. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  smartOrderParams_ = std::make_shared<SmartOrderParams>();
  smartOrderParams_->trdAcctId_ = doc["trdAcctId"].GetUint();
  smartOrderParams_->marketCode_ = optMarketCode.value();
  smartOrderParams_->symbolCode_ = doc["symbolCode"].GetString();
  smartOrderParams_->side_ = optSide.value();
  if (optPosDirection != std::nullopt) {
    smartOrderParams_->posDirection_ = optPosDirection.value();
  }
  smartOrderParams_->orderSize_ = doc["orderSize"].GetUint64();
  smartOrderParams_->initialUrgency_ = optInitialUrgency.value();
  smartOrderParams_->tickerOffset_ = doc["tickerOffset"].GetUint();
  smartOrderParams_->closeTDayStg_ = optCloseTDayStg.value();
  smartOrderParams_->priceRange_ = std::make_tuple(
      doc["priceRange"][0].GetDouble(), doc["priceRange"][1].GetDouble());

  if (DEC::ZERO(smartOrderParams_->orderSize_)) {
    getStgEng()->logInfo("[ALGO] Invalid algo params orderSize. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  //! 获得算法单的价格区间限制
  std::tie(priceOfLowerBound_, priceOfUpperBound_) =
      getSmartOrderParams()->priceRange_;

  return 0;
}

int SmartOrder::initTrdSymParams() {
  const auto [sCodeOfGetSym, recSymbolInfo] =
      getAlgoMgr()
          ->getStgEng()
          ->getTBLMonitorOfSymbolInfo()
          ->getRecSymbolInfoBySymbolCode(
              GetMarketName(smartOrderParams_->marketCode_),
              smartOrderParams_->symbolCode_);
  if (sCodeOfGetSym != 0) {
    return sCodeOfGetSym;
  }

  // clang-format off
  trdSymParams_ = std::make_shared<TrdSymParams>();
  trdSymParams_->upperLimitPrice_ = CONV(Decimal, recSymbolInfo->upperLimitPrice);
  trdSymParams_->lowerLimitPrice_ = CONV(Decimal, recSymbolInfo->lowerLimitPrice);
  trdSymParams_->preClosePrice_ = CONV(Decimal, recSymbolInfo->preClosePrice);
  trdSymParams_->preSettlementPrice_ = CONV(Decimal, recSymbolInfo->preSettlementPrice);
  trdSymParams_->precOfBidOrderPrice_ = CONV(Decimal, recSymbolInfo->precOfBidOrderPrice);
  trdSymParams_->precOfBidOrderVol_ = CONV(Decimal, recSymbolInfo->precOfBidOrderVol);
  trdSymParams_->precOfAskOrderPrice_ = CONV(Decimal, recSymbolInfo->precOfAskOrderPrice);
  trdSymParams_->precOfAskOrderVol_ = CONV(Decimal, recSymbolInfo->precOfAskOrderVol);
  trdSymParams_->minBidOrderVol_ = CONV(Decimal, recSymbolInfo->minBidOrderVol);
  trdSymParams_->maxBidOrderVol_ = CONV(Decimal, recSymbolInfo->maxBidOrderVol);
  trdSymParams_->minAskOrderVol_ = CONV(Decimal, recSymbolInfo->minAskOrderVol);
  trdSymParams_->maxAskOrderVol_ = CONV(Decimal, recSymbolInfo->maxAskOrderVol);
  trdSymParams_->minBidOrderAmt_ = CONV(Decimal, recSymbolInfo->minBidOrderAmt);
  trdSymParams_->maxBidOrderAmt_ = CONV(Decimal, recSymbolInfo->maxBidOrderAmt);
  trdSymParams_->minAskOrderAmt_ = CONV(Decimal, recSymbolInfo->minAskOrderAmt);
  trdSymParams_->maxAskOrderAmt_ = CONV(Decimal, recSymbolInfo->maxAskOrderAmt);
  trdSymParams_->parValue_ = recSymbolInfo->parValue;
  // clang-format on

  if (smartOrderParams_->side_ == Side::Bid) {
    trdSymParams_->precOfOrderPrice_ = trdSymParams_->precOfBidOrderPrice_;
    trdSymParams_->precOfOrderVol_ = trdSymParams_->precOfBidOrderVol_;
    trdSymParams_->minOrderVol_ = trdSymParams_->minBidOrderVol_;
    trdSymParams_->maxOrderVol_ = trdSymParams_->maxBidOrderVol_;
    trdSymParams_->minOrderAmt_ = trdSymParams_->minBidOrderAmt_;
    trdSymParams_->maxOrderAmt_ = trdSymParams_->maxBidOrderAmt_;
  } else {
    trdSymParams_->precOfOrderPrice_ = trdSymParams_->precOfAskOrderPrice_;
    trdSymParams_->precOfOrderVol_ = trdSymParams_->precOfAskOrderVol_;
    trdSymParams_->minOrderVol_ = trdSymParams_->minAskOrderVol_;
    trdSymParams_->maxOrderVol_ = trdSymParams_->maxAskOrderVol_;
    trdSymParams_->minOrderAmt_ = trdSymParams_->minAskOrderAmt_;
    trdSymParams_->maxOrderAmt_ = trdSymParams_->maxAskOrderAmt_;
  }

  if (DEC::ZERO(trdSymParams_->precOfOrderVol_)) {
    getStgEng()->logInfo("[ALGO] Invalid trd params precOfOrderVol. [{}]",
                         {toStr()}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (DEC::ZERO(trdSymParams_->precOfOrderPrice_)) {
    getStgEng()->logInfo("[ALGO] Invalid trd params precOfOrderPrice. [{}]",
                         {toStr()}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  return 0;
}

std::tuple<int, TopicGroup> SmartOrder::doGetTopicGroupNeedSub() {
  TopicGroup topicGroupNeedSub;

  if (IsCNMarketOfSpots(smartOrderParams_->marketCode_)) {
    topicGroupNeedSub.emplace(
        fmt::format("shm://MD.{}.Spot/{}/Bid1Ask1",
                    GetMarketName(smartOrderParams_->marketCode_),
                    smartOrderParams_->symbolCode_));
    topicGroupNeedSub.emplace(
        fmt::format("shm://MD.{}.Spot/{}/LastPrice",
                    GetMarketName(smartOrderParams_->marketCode_),
                    smartOrderParams_->symbolCode_));

  } else if (IsCNMarketOfFutures(smartOrderParams_->marketCode_)) {
    topicGroupNeedSub.emplace(
        fmt::format("shm://MD.{}.Futures/{}/Bid1Ask1",
                    GetMarketName(smartOrderParams_->marketCode_),
                    smartOrderParams_->symbolCode_));
    topicGroupNeedSub.emplace(
        fmt::format("shm://MD.{}.Futures/{}/LastPrice",
                    GetMarketName(smartOrderParams_->marketCode_),
                    smartOrderParams_->symbolCode_));

  } else {
    getStgEng()->logInfo(
        "[ALGO] Invalid market code {}. [{}]",
        {GetMarketName(smartOrderParams_->marketCode_), toStr()},
        getStgInstInfo());
  }

  return {0, topicGroupNeedSub};
}

bool SmartOrder::algoOrderIsFinished() {
  return orderStatus_ >= OrderStatus::Filled;
}

void SmartOrder::doOnTimer() {
  if (lastPrice_ == nullptr) return;
  if (bid1ask1_ == nullptr) return;

  if (alreadySendOrder_ == false) {
    alreadySendOrder_ = tryToSendOrder();
  }
}

bool SmartOrder::tryToSendOrder() {
  if (DEC::LT(lastPrice_->lastPrice_, priceOfLowerBound_) ||
      DEC::GT(lastPrice_->lastPrice_, priceOfUpperBound_)) {
#ifndef _NDEBUG
    const auto curTs = GetTotalSecSince1970() / 10;
    static std::decay_t<decltype(curTs)> tsAlreadyPrint = 0;
    if (curTs != tsAlreadyPrint) {
      getStgEng()->logInfo("[ALGO] lastPrice: {}; priceRange: [{}, {}] ",
                           {std::to_string(lastPrice_->lastPrice_),
                            std::to_string(priceOfLowerBound_),
                            std::to_string(priceOfUpperBound_)},
                           getStgInstInfo());
      tsAlreadyPrint = curTs;
    }
#endif
    return false;
  }

  //! 根据Urgency计算下单价格
  const auto orderPrice =
      GetPriceOfUrgency(bid1ask1_, getSmartOrderParams()->side_,
                        getSmartOrderParams()->initialUrgency_,
                        getSmartOrderParams()->tickerOffset_,
                        getTrdSymParams()->precOfOrderPrice_);

  //! 开始报单
  std::tie(statusCode_, orderId_) =
      getAlgoMgr()->getStgEng()->order(getStgInstInfo(),                      //
                                       getSmartOrderParams()->marketCode_,    //
                                       getSmartOrderParams()->symbolCode_,    //
                                       getSmartOrderParams()->side_,          //
                                       getSmartOrderParams()->posDirection_,  //
                                       orderPrice,                            //
                                       getSmartOrderParams()->orderSize_,     //
                                       getSmartOrderParams()->trdAcctId_,     //
                                       getSmartOrderParams()->closeTDayStg_,  //
                                       getAlgoId());                          //
  if (statusCode_ != 0) {
    orderStatus_ = OrderStatus::Failed;
    getStgEng()->logInfo(
        "[ALGO] Send order failed with price {}. [{}] [{} - {}]",
        {std::to_string(orderPrice), toStr(), std::to_string(statusCode_),
         GetStatusMsg(statusCode_)},
        getStgInstInfo());
  } else {
    getStgEng()->logInfo(
        "[ALGO] Send order {} with price {} "
        "because of lastprice {} in range [{}, {}]. [{}]",
        {std::to_string(orderId_), std::to_string(orderPrice),
         std::to_string(lastPrice_->lastPrice_),
         std::to_string(priceOfLowerBound_), std::to_string(priceOfUpperBound_),
         toStr()},
        getStgInstInfo());
  }

  return true;
}

std::string SmartOrder::notifyProgressOfAlgoOrder() {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("stgInstId");
  writer.Uint(getStgInstInfo()->stgInstId_);
  writer.Key("algoId");
  writer.Uint64(getAlgoId());
  writer.Key("orderId");
  writer.Uint64(orderId_);
  writer.Key("dealSize");
  writer.Double(dealSize_);
  writer.Key("avgDealPrice");
  writer.Double(avgDealPrice_);
  writer.Key("orderStatus");
  writer.String(ENUM_TO_CSTR(orderStatus_));
  writer.Key("statusCode");
  writer.Int(getStatusCode());
  writer.Key("statusMsg");
  writer.String(GetStatusMsg(getStatusCode()).c_str());
  writer.EndObject();

  std::string progress = strBuf.GetString();
  std::string algoParams =
      RemoveWhitespaceAndNewlines(getAlgoParamsInJsonFmt());

  const auto data = fmt::format(  //
      R"({{"algoPrams":{},"progress":{}}}, getStgInstInfo())", algoParams,
      progress);
  getStgEng()->logInfo("[ALGO] Try to notify progress of algo order. {} [{}]",
                       {data, toStr()}, getStgInstInfo());

  const auto commonIPCData = MakeCommonIPCData(data, MSG_ID_ON_ALGO_ORDER);
  auto asyncTask = std::make_shared<SHMIPCAsyncTask>(
      std::make_shared<SHMIPCTask>(
          commonIPCData, sizeof(CommonIPCData) + commonIPCData->dataLen_ + 1,
          CopyIPCData::False),
      getStgInstInfo()->stgInstId_);
  getAlgoMgr()->getStgEng()->getStgInstTaskDispatcher()->dispatch(asyncTask);

  return data;
}

void SmartOrder::doOnOrderRet(const OrderInfo* orderInfo) {
  bool needNotify = false;

  if (orderInfo->orderStatus_ > orderStatus_) {
    orderStatus_ = orderInfo->orderStatus_;
    needNotify = true;
  }

  if (DEC::GT(orderInfo->dealSize_, dealSize_)) {
    dealSize_ = orderInfo->dealSize_;
    avgDealPrice_ = orderInfo->avgDealPrice_;
    needNotify = true;
  }

  if (needNotify) {
    const auto data = notifyProgressOfAlgoOrder();
    syncToDB(data);
  }
}

void SmartOrder::doOnCancelOrderRet(const OrderInfo* orderInfo) {}

void SmartOrder::doOnBid1Ask1(const Bid1Ask1* bid1ask1) {
  if (DEC::ZERO(bid1ask1->bidPrice_) || DEC::ZERO(bid1ask1->askPrice_) ||
      DEC::ZERO(bid1ask1->bidSize_) || DEC::ZERO(bid1ask1->askSize_)) {
    getStgEng()->logInfo(
        "[ALGO] Recv invalid bid1ask1 "
        "because of price or size is zero. [{}] {}",
        {toStr(), bid1ask1->toStr()}, getStgInstInfo());
    return;
  }

  if (DEC::GT(bid1ask1->bidPrice_, bid1ask1->askPrice_)) {
    getStgEng()->logInfo(
        "[ALGO] Recv invalid bid1ask1 "
        "because bidprice is greater than askprice. [{}] {}",
        {toStr(), bid1ask1->toStr()}, getStgInstInfo());
    return;
  }

  bid1ask1_ = std::make_shared<Bid1Ask1>(*bid1ask1);

#ifndef _NDEBUG
  getStgEng()->logInfo("[ALGO] {} {} {}",
                       {std::to_string(bid1ask1->bidPrice_),
                        std::to_string(bid1ask1->askPrice_), bid1ask1->toStr()},
                       getStgInstInfo(), NotifyToTerminal::False);
#endif
}

void SmartOrder::doOnLastPrice(const LastPrice* lastPrice) {
  if (DEC::ZERO(lastPrice->lastPrice_)) {
    getStgEng()->logInfo(
        "[ALGO] Recv invalid lastprice because of price zero. [{}] {}",
        {toStr(), lastPrice->toStr()}, getStgInstInfo());
    return;
  }

  lastPrice_ = std::make_shared<LastPrice>(*lastPrice);

#ifndef _NDEBUG
  const auto curTs = GetTotalSecSince1970() / 10;
  static std::decay_t<decltype(curTs)> tsAlreadyPrint = 0;
  if (curTs != tsAlreadyPrint) {
    getStgEng()->logInfo("[ALGO] {} ", {lastPrice->toStr()}, getStgInstInfo(),
                         NotifyToTerminal::False);
    tsAlreadyPrint = curTs;
  }
#endif
}

}  // namespace bq::algo
