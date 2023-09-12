/*!
 * \file AlgoTWAP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 *
 * doOnTimer不停的检查正在处理的状态机curOrderStateMachine_的NextActionOfAlgo，然
 * 后根据NextActionOfAlgo来决定继续调用当前状态机的onTimer或者切换到下一个状态机，
 * 当状态机报单结束之后，如果有零股，将零股放入算上重新生成新的报单状态机队列。另
 * 外此模块还负责向curOrderStateMachine_投递行情和回报。
 *
 */

#include "AlgoTWAP.hpp"

#include "AlgoMgr.hpp"
#include "AlgoTWAPDef.hpp"
#include "AlgoTWAPFSM.hpp"
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

TWAP::TWAP(AlgoMgr* algoMgr, const StgInstInfoSPtr& stgInstInfo,
           const std::string& algoType, const std::string& algoName,
           std::uint32_t lifetime)
    : AlgoOrder(algoMgr, stgInstInfo, algoType, algoName, lifetime) {}

int TWAP::afterInit(const std::string& algoParamsInJsonFmt) {
  if (const auto statusCode = initAlgoParams(algoParamsInJsonFmt);
      statusCode != 0) {
    return statusCode;
  }

  if (const auto statusCode = initTrdSymParams(); statusCode != 0) {
    return statusCode;
  }

  if (const auto statusCode = initOrderStateMachineGroup(); statusCode != 0) {
    return statusCode;
  }

  return 0;
}

// clang-format off
/*
{
  "trdAcctId": 100000,                         # 交易账号
  "marketCode": "SZSE",                        # 市场
  "symbolCode": "000002",                      # 代码
  "side": "Bid",                               # 买卖
  "posDirection": "Open",                      # 开平
  "totalSize": 10000,                          # 报单数量
  "splitNum": 30,                              # 子单数量
  "msIntervalOfSubOrder": 0,                   # 子单间隔
  "minMSIntervalOfOrderAndCancelOrder": 2000,  # 当挂单不在 bid/ask 之间会自动撤单，下单和撤单的最短间隔，避免频繁的报单撤单
  "initialUrgency": "PriceOfMaker",            # 每个子单的执行紧急度
  "tickerOffset": 0,                           # 
  "cancelTimesOfUpgradeUrgency": 2             # 撤单次数超过当前值升级Urgency
}
 */
// clang-format on
//
int TWAP::initAlgoParams(const std::string& algoParamsInJsonFmt) {
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

  if (!doc.HasMember("totalSize")) {
    getStgEng()->logInfo("[ALGO] Invalid algo params totalSize. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc["totalSize"].IsDouble() && !doc["totalSize"].IsUint64()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params totalSize. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("splitNum") || !doc["splitNum"].IsUint()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params splitNum. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("msIntervalOfSubOrder") ||
      !doc["msIntervalOfSubOrder"].IsUint()) {
    getStgEng()->logInfo("[ALGO] Invalid algo params msIntervalOfSubOrder. {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("minMSIntervalOfOrderAndCancelOrder") ||
      !doc["minMSIntervalOfOrderAndCancelOrder"].IsUint()) {
    getStgEng()->logInfo(
        "[ALGO] Invalid algo params minMSIntervalOfOrderAndCancelOrder. {}",
        {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (!doc.HasMember("cancelTimesOfUpgradeUrgency") ||
      !doc["cancelTimesOfUpgradeUrgency"].IsUint()) {
    getStgEng()->logInfo(
        "[ALGO] Invalid algo params cancelTimesOfUpgradeUrgency. {}",
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

  if (doc.HasMember("priceRange") && doc["priceRange"].IsArray()) {
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

  twapParams_ = std::make_shared<TWAPParams>();
  twapParams_->trdAcctId_ = doc["trdAcctId"].GetUint();
  twapParams_->marketCode_ = optMarketCode.value();
  twapParams_->symbolCode_ = doc["symbolCode"].GetString();
  twapParams_->side_ = optSide.value();
  if (optPosDirection != std::nullopt) {
    twapParams_->posDirection_ = optPosDirection.value();
  }
  twapParams_->totalSize_ = doc["totalSize"].GetDouble();
  twapParams_->splitNum_ = doc["splitNum"].GetUint();
  twapParams_->msIntervalOfSubOrder_ = doc["msIntervalOfSubOrder"].GetUint();
  twapParams_->minMSIntervalOfOrderAndCancelOrder_ =
      doc["minMSIntervalOfOrderAndCancelOrder"].GetUint();
  twapParams_->cancelTimesOfUpgradeUrgency_ =
      doc["cancelTimesOfUpgradeUrgency"].GetUint();
  twapParams_->initialUrgency_ = optInitialUrgency.value();
  twapParams_->tickerOffset_ = doc["tickerOffset"].GetUint();
  twapParams_->closeTDayStg_ = optCloseTDayStg.value();
  if (doc.HasMember("priceRange") && doc["priceRange"].IsArray()) {
    const auto priceRange = std::make_tuple(doc["priceRange"][0].GetDouble(),
                                            doc["priceRange"][1].GetDouble());
    twapParams_->priceRange_ =
        std::make_optional<decltype(priceRange)>(priceRange);
  }

  if (DEC::ZERO(twapParams_->totalSize_)) {
    getStgEng()->logInfo("[ALGO] Invalid algo params totalSize. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (twapParams_->splitNum_ == 0) {
    getStgEng()->logInfo("[ALGO] Invalid algo params splitNum. [{}] {}",
                         {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  if (twapParams_->cancelTimesOfUpgradeUrgency_ == 0) {
    getStgEng()->logInfo(
        "[ALGO] Invalid algo params cancelTimesOfUpgradeUrgency. [{}] {}",
        {toStr(), (algoParamsInJsonFmt)}, getStgInstInfo());
    return SCODE_ALGO_INVALID_ALGO_PARAM;
  }

  return 0;
}

int TWAP::initTrdSymParams() {
  const auto [sCodeOfGetSym, recSymbolInfo] =
      getAlgoMgr()
          ->getStgEng()
          ->getTBLMonitorOfSymbolInfo()
          ->getRecSymbolInfoBySymbolCode(
              GetMarketName(twapParams_->marketCode_),
              twapParams_->symbolCode_);
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

  if (twapParams_->side_ == Side::Bid) {
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

int TWAP::initOrderStateMachineGroup() {
  int statusCode = 0;
  std::vector<Decimal> orderSizeGroup;
  if (IsCNMarketOfSpots(twapParams_->marketCode_)) {
    std::tie(statusCode, orderSizeGroup) =
        splitTotalSize(twapParams_->totalSize_, twapParams_->splitNum_, 100.0);
  } else {
    std::tie(statusCode, orderSizeGroup) =
        splitTotalSize(twapParams_->totalSize_, twapParams_->splitNum_,
                       trdSymParams_->precOfOrderVol_);
  }
  if (statusCode != 0) {
    return statusCode;
  }

  for (std::size_t no = 0; no < orderSizeGroup.size(); ++no) {
    const auto orderStateMachine =
        std::make_shared<OrderStateMachine>(this, no, orderSizeGroup[no]);
    orderStateMachineGroup_.emplace_back(orderStateMachine);
  }
  getStgEng()->logInfo(
      "[ALGO] Init order fsm group success. [size = {}] [{}]",
      {std::to_string(orderStateMachineGroup_.size()), toStr()},
      getStgInstInfo());

  return 0;
}

std::tuple<int, TopicGroup> TWAP::doGetTopicGroupNeedSub() {
  TopicGroup topicGroupNeedSub;

  if (IsCNMarketOfSpots(twapParams_->marketCode_)) {
    topicGroupNeedSub.emplace(fmt::format(
        "shm://MD.{}.Spot/{}/Bid1Ask1", GetMarketName(twapParams_->marketCode_),
        twapParams_->symbolCode_));
    if (getTWAPParams()->priceRange_.has_value()) {
      topicGroupNeedSub.emplace(fmt::format(
          "shm://MD.{}.Spot/{}/LastPrice",
          GetMarketName(twapParams_->marketCode_), twapParams_->symbolCode_));
    }

  } else if (IsCNMarketOfFutures(twapParams_->marketCode_)) {
    topicGroupNeedSub.emplace(fmt::format(
        "shm://MD.{}.Futures/{}/Bid1Ask1",
        GetMarketName(twapParams_->marketCode_), twapParams_->symbolCode_));
    if (getTWAPParams()->priceRange_.has_value()) {
      topicGroupNeedSub.emplace(fmt::format(
          "shm://MD.{}.Futures/{}/LastPrice",
          GetMarketName(twapParams_->marketCode_), twapParams_->symbolCode_));
    }

  } else {
    getStgEng()->logInfo("[ALGO] Invalid market code {}. [{}]",
                         {GetMarketName(twapParams_->marketCode_), toStr()},
                         getStgInstInfo());
  }

  return {0, topicGroupNeedSub};
}

bool TWAP::algoOrderIsFinished() { return orderStateMachineGroup_.empty(); }

void TWAP::doOnTimer() {
  if (bid1ask1_ == nullptr) return;

  //! 第一次进入
  if (!curOrderStateMachine_) {
    handleFirstOrder();
  } else {
    switch (curOrderStateMachine_->nextActionOfAlgo_) {
      case NextActionOfAlgo::HandleNextOrder:
        handleNextOrder();
        break;

      case NextActionOfAlgo::TerminateAlgo:
        terminateAlgo();
        break;

      default:
        curOrderStateMachine_->onTimer(*curOrderStateMachine_);
        break;
    }
  }
}

void TWAP::handleFirstOrder() {
  if (!orderStateMachineGroup_.empty()) {
    //! 待处理订单队列不空
    curOrderStateMachine_ = orderStateMachineGroup_.front();
    getStgEng()->logInfo("[ALGO] Begin to handle first order. [{}] {}",
                         {toStr(), curOrderStateMachine_->toStr()},
                         getStgInstInfo());
    curOrderStateMachine_->start();
  } else {
    //! 待处理订单队列为空
    getStgEng()->logInfo("[ALGO] Pending order queue is empty. [{}] {}",
                         {toStr(), curOrderStateMachine_->toStr()},
                         getStgInstInfo());
    return;
  }
}

void TWAP::handleNextOrder() {
  //! 处理下一个订单前，检查和上一笔订单的间隔是否小于msIntervalOfSubOrder
  const auto now = GetTotalMSSince1970();
  if (now - curOrderStateMachine_->startTime_ <
      getTWAPParams()->msIntervalOfSubOrder_) {
    return;
  }

  //! 保存已完结订单
  orderStateMachineGroupFilled_.emplace_back(curOrderStateMachine_);

  if (!orderStateMachineGroup_.empty()) {
    //! 移除已处理订单
    orderStateMachineGroup_.pop_front();
    getStgEng()->logInfo("[ALGO] Pop order from order fsm group. [{}] {}",
                         {toStr(), curOrderStateMachine_->toStr()},
                         getStgInstInfo());
    //! 通知算法单进度
    const auto data = notifyProgressOfAlgoOrder();
    syncToDB(data);

  } else {
    getStgEng()->logInfo(
        "[ALGO] Exec algo order failed "
        "because of code enters impossible logic. [{}] {}",
        {toStr(), curOrderStateMachine_->toStr()}, getStgInstInfo());
    setAlgoStatus(AlgoStatus::ExecFailed);
    return;
  }

  //! 处理刚完结的订单中未成交的数量
  handleUnfilled(curOrderStateMachine_);

  if (!orderStateMachineGroup_.empty()) {
    //! 获取下一个订单
    curOrderStateMachine_ = orderStateMachineGroup_.front();
    getStgEng()->logInfo(
        "[ALGO] Switch to next order. [progress = {}/{}] [{}] {}",
        {std::to_string(orderStateMachineGroupFilled_.size()),
         std::to_string(orderStateMachineGroup_.size() +
                        orderStateMachineGroupFilled_.size()),
         toStr(), curOrderStateMachine_->toStr()},
        getStgInstInfo());
    curOrderStateMachine_->start();
  } else {
    //! 没有后续的订单，算法结束，父类AlgoOrder中会处理
  }
}

void TWAP::handleUnfilled(const OrderStateMachineSPtr& orderStateMachine) {
  const auto unfilledQuantity =
      orderStateMachine->orderSize_ - orderStateMachine->dealSize_;
  if (DEC::ZERO(unfilledQuantity)) {
    return;
  }

  //! 获取新的splitNum和totalSize
  std::uint32_t splitNum = 0;
  if (!orderStateMachineGroup_.empty()) {
    splitNum = orderStateMachineGroup_.size();
    getStgEng()->logInfo(
        "[ALGO] It is found that the "
        "order in the order queue has an unfilled quantity {} . [{}]",
        {std::to_string(unfilledQuantity), toStr()}, getStgInstInfo());
  } else {
    //! 这个逻辑是处理最后一笔订单的未成交
    splitNum = 1;
    getStgEng()->logInfo(
        "[ALGO] It is found that the "
        "last order in the order queue has an unfilled quantity {} . [{}]",
        {std::to_string(unfilledQuantity), toStr()}, getStgInstInfo());
  }

  auto totalSize = unfilledQuantity;
  std::for_each(std::begin(orderStateMachineGroup_),
                std::end(orderStateMachineGroup_),
                [&totalSize](const auto& stateMachine) {
                  totalSize += stateMachine->orderSize_;
                });

  int statusCode = 0;
  std::vector<Decimal> orderSizeGroup;
  if (IsCNMarketOfSpots(twapParams_->marketCode_)) {
    std::tie(statusCode, orderSizeGroup) =
        splitTotalSize(totalSize, splitNum, 100.0);
  } else {
    std::tie(statusCode, orderSizeGroup) =
        splitTotalSize(totalSize, splitNum, trdSymParams_->precOfOrderVol_);
  }

  //! 恢复no
  std::size_t no = 0;
  if (!orderStateMachineGroup_.empty()) {
    no = orderStateMachineGroup_.front()->no_;
  } else {
    no = LastUnfilledOrderNo;
  }

  //! 重新生成TWAP报单队列
  orderStateMachineGroup_.clear();
  for (std::size_t i = 0; i < orderSizeGroup.size(); ++i, ++no) {
    const auto orderStateMachine =
        std::make_shared<OrderStateMachine>(this, no, orderSizeGroup[i]);
    orderStateMachineGroup_.emplace_back(orderStateMachine);
  }

  getStgEng()->logInfo(
      "[ALGO] Regenerate order fsm group success."
      "[totalSize: {}; splitNum: {}] [size = {}] [{}]",
      {std::to_string(totalSize), std::to_string(splitNum),
       std::to_string(orderStateMachineGroup_.size()), toStr()},
      getStgInstInfo());
}

void TWAP::terminateAlgo() {
  //! 告诉AlgoMgr目前算法但状态，AlgoMgr会移除相关资源
  setAlgoStatus(AlgoStatus::ExecFailed);
  setStatusCode(curOrderStateMachine_->statusCode_);
  getStgEng()->logInfo("[ALGO] Exec algo order failed. [{}] {} [{} - {}]",
                       {toStr(), curOrderStateMachine_->toStr(),
                        std::to_string(curOrderStateMachine_->statusCode_),
                        GetStatusMsg(curOrderStateMachine_->statusCode_)},
                       getStgInstInfo());
  const auto data = notifyProgressOfAlgoOrder();
  syncToDB(data);
}

std::string TWAP::notifyProgressOfAlgoOrder() {
  Decimal totalDealSize = 0;
  Decimal totalDealAmt = 0;
  Decimal avgDealPrice = 0;
  for (const auto& orderStateMachine : orderStateMachineGroupFilled_) {
    totalDealSize += orderStateMachine->dealSize_;
    totalDealAmt +=
        (orderStateMachine->dealSize_ * orderStateMachine->avgDealPrice_);
  }
  if (!DEC::ZERO(totalDealSize)) {
    avgDealPrice = totalDealAmt / totalDealSize;
  }

  const auto numOfTotalOrder =
      orderStateMachineGroupFilled_.size() + orderStateMachineGroup_.size();
  const auto numOfFinishedOrder = orderStateMachineGroupFilled_.size();

  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("stgInstId");
  writer.Uint(getStgInstInfo()->stgInstId_);
  writer.Key("algoId");
  writer.Uint64(getAlgoId());
  writer.Key("numOfFinishedOrder");
  writer.Uint(numOfFinishedOrder);
  writer.Key("numOfTotalOrder");
  writer.Uint(numOfTotalOrder);
  writer.Key("totalDealSize");
  writer.Double(totalDealSize);
  writer.Key("avgDealPrice");
  writer.Double(avgDealPrice);
  writer.Key("statusCode");
  writer.Int(getStatusCode());
  writer.Key("statusMsg");
  writer.String(GetStatusMsg(getStatusCode()).c_str());
  writer.EndObject();

  std::string progress = strBuf.GetString();
  std::string algoParams =
      RemoveWhitespaceAndNewlines(getAlgoParamsInJsonFmt());

  const auto data = fmt::format(  //
      R"({{"algoPrams":{},"progress":{}}})", algoParams, progress);
  getStgEng()->logInfo(
      "[ALGO] Try to notify progress of algo order. {} [{}] {}",
      {data, toStr(), curOrderStateMachine_->toStr()}, getStgInstInfo());

  const auto commonIPCData = MakeCommonIPCData(data, MSG_ID_ON_ALGO_ORDER);
  auto asyncTask = std::make_shared<SHMIPCAsyncTask>(
      std::make_shared<SHMIPCTask>(
          commonIPCData, sizeof(CommonIPCData) + commonIPCData->dataLen_ + 1,
          CopyIPCData::False),
      getStgInstInfo()->stgInstId_);
  getAlgoMgr()->getStgEng()->getStgInstTaskDispatcher()->dispatch(asyncTask);

  return data;
}

void TWAP::doOnOrderRet(const OrderInfo* orderInfo) {
  if (curOrderStateMachine_) {
    curOrderStateMachine_->onOrderRet(*curOrderStateMachine_, orderInfo);
  } else {
    getStgEng()->logInfo(
        "[ALGO] Exec algo order failed "
        "because of code enters impossible logic. [{}] {}",
        {toStr(), curOrderStateMachine_->toStr()}, getStgInstInfo());
  }
}

void TWAP::doOnCancelOrderRet(const OrderInfo* orderInfo) {
  if (curOrderStateMachine_) {
    curOrderStateMachine_->onCancelOrderRet(*curOrderStateMachine_, orderInfo);
  } else {
    getStgEng()->logInfo(
        "[ALGO] Exec algo order failed "
        "because of code enters impossible logic. [{}] {}",
        {toStr(), curOrderStateMachine_->toStr()}, getStgInstInfo());
  }
}

void TWAP::doOnBid1Ask1(const Bid1Ask1* bid1ask1) {
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

  const auto noPriceRangLimit = !getTWAPParams()->priceRange_.has_value();
  if (noPriceRangLimit) {
    //! 如果算法单无价格区间限制
    bid1ask1_ = std::make_shared<Bid1Ask1>(*bid1ask1);
    getStgEng()->logInfo(
        "[ALGO] {} {} {}",
        {std::to_string(bid1ask1->bidPrice_),
         std::to_string(bid1ask1->askPrice_), bid1ask1->toStr()},
        getStgInstInfo());

  } else {
    //! 如果算法单有价格区间限制
    Decimal priceOfLowerBound = 0;
    Decimal priceOfUpperBound = 0;

    //! 获得算法单的价格区间限制
    std::tie(priceOfLowerBound, priceOfUpperBound) =
        getTWAPParams()->priceRange_.value();

    if (lastPrice_) {
      if (DEC::GT(lastPrice_->lastPrice_, priceOfUpperBound)) {
        //! 如果有最新价且价格不在区间里，那么清空bid1ask1使得onTimer不再触发
        if (bid1ask1_) bid1ask1_.reset();

      } else {
        //! 如果有最新价且价格不在区间里，那么清空bid1ask1使得onTimer不再触发
        if (DEC::GE(priceOfLowerBound, lastPrice_->lastPrice_)) {
          if (bid1ask1_) bid1ask1_.reset();
        } else {
          //! 有最新价且价格在区间里
          bid1ask1_ = std::make_shared<Bid1Ask1>(*bid1ask1);
#ifndef _NDEBUG
          getStgEng()->logInfo(
              "[ALGO] {} {} {}",
              {std::to_string(bid1ask1->bidPrice_),
               std::to_string(bid1ask1->askPrice_), bid1ask1->toStr()},
              getStgInstInfo(), NotifyToTerminal::False);
#endif
        }
      }

    } else {
      //! 因为lastPrice_从来没有reset，所以理论上不可能进入这里
      if (bid1ask1_) bid1ask1_.reset();
    }
  }
}

void TWAP::doOnLastPrice(const LastPrice* lastPrice) {
  if (DEC::ZERO(lastPrice->lastPrice_)) {
    getStgEng()->logInfo(
        "[ALGO] Recv invalid lastprice because of price zero. [{}] {}",
        {toStr(), lastPrice->toStr()}, getStgInstInfo());
    return;
  }

  lastPrice_ = std::make_shared<LastPrice>(*lastPrice);

#ifndef _NDEBUG
  const auto curMinute = GetTotalSecSince1970() / 60;
  static std::decay_t<decltype(curMinute)> minuteAlreadyPrint = 0;
  if (curMinute != minuteAlreadyPrint) {
    getStgEng()->logInfo("[ALGO] {} ", {lastPrice->toStr()}, getStgInstInfo(),
                         NotifyToTerminal::False);
    minuteAlreadyPrint = curMinute;
  }
#endif
}

}  // namespace bq::algo
