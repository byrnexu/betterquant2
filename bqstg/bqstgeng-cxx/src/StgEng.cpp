/*!
 * \file StgEng.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgEng.hpp"

#include "PosMgrOfStgInst.hpp"
#include "StgEngDef.hpp"
#include "StgEngImpl.hpp"
#include "StgInstTaskHandlerBase.hpp"
#include "StgInstTaskHandlerImpl.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/StgInstInfo.hpp"
#include "util/PosSnapshot.hpp"

namespace bq::stg {

StgEng::StgEng(const std::string& configFilename)
    : stgEngImpl_(std::make_shared<StgEngImpl>(configFilename)) {}

int StgEng::init(const StgInstTaskHandlerBaseSPtr& taskHandler) {
  const auto ret = stgEngImpl_->init();
  if (ret != 0) {
    return ret;
  }
  installStgInstTaskHandler(taskHandler);
  return ret;
}

StgInstInfoSPtr StgEng::getDftStgInstInfo() const {
  return stgEngImpl_->getDftStgInstInfo();
}

int StgEng::run() { return stgEngImpl_->run(); }

void StgEng::installStgInstTaskHandler(
    const StgInstTaskHandlerBaseSPtr& taskHandler) {
  StgInstTaskHandlerBundle stgInstTaskHandlerBundle = {
      [&](const auto& stgInstInfo, const auto* commonIPCData) {
        taskHandler->onStgManualIntervention(stgInstInfo, commonIPCData);
      },

      [&](const auto& stgInstInfo, const auto& topicContent) {
        taskHandler->onPushTopic(stgInstInfo, topicContent);
      },

      [&](const auto& stgInstInfo, const auto* orderInfo) {
        taskHandler->onOrderRet(stgInstInfo, orderInfo);
      },
      [&](const auto& stgInstInfo, const auto* orderInfo) {
        taskHandler->onCancelOrderRet(stgInstInfo, orderInfo);
      },

      [&](const auto& stgInstInfo, const auto* commonIPCData) {
        taskHandler->onAlgoOrder(stgInstInfo, commonIPCData);
      },

      [&](const auto& stgInstInfo, const auto* trades) {
        taskHandler->onTrades(stgInstInfo, trades);
      },
      [&](const auto& stgInstInfo, const auto* orders) {
        taskHandler->onOrders(stgInstInfo, orders);
      },
      [&](const auto& stgInstInfo, const auto* books) {
        taskHandler->onBooks(stgInstInfo, books);
      },
      [&](const auto& stgInstInfo, const auto* candle) {
        taskHandler->onCandle(stgInstInfo, candle);
      },
      [&](const auto& stgInstInfo, const auto* tickers) {
        taskHandler->onTickers(stgInstInfo, tickers);
      },
      [&](const auto& stgInstInfo, const auto* bid1Ask1) {
        taskHandler->onBid1Ask1(stgInstInfo, bid1Ask1);
      },
      [&](const auto& stgInstInfo, const auto* lastPrice) {
        taskHandler->onLastPrice(stgInstInfo, lastPrice);
      },
      [&](const auto& stgInstInfo, const auto* candle) {
        taskHandler->onDynCandle(stgInstInfo, candle);
      },

      [&]() { taskHandler->onStgStart(); },
      [&](const auto& stgInstInfo) {
        taskHandler->onStgInstStart(stgInstInfo);
      },

      [&]() { taskHandler->onStgStop(); },
      [&](const auto& stgInstInfo) { taskHandler->onStgInstStop(stgInstInfo); },

      [&](const auto& stgInstInfo) { taskHandler->onStgInstAdd(stgInstInfo); },
      [&](const auto& stgInstInfo) { taskHandler->onStgInstDel(stgInstInfo); },
      [&](const auto& stgInstInfo) { taskHandler->onStgInstChg(stgInstInfo); },
      [&](const auto& stgInstInfo, const auto& timerName) {
        taskHandler->onStgInstTimer(stgInstInfo, timerName);
      },

      [&](const auto& stgInstInfo, const auto& posUpdateOfAcctId) {
        taskHandler->onPosUpdateOfAcctId(stgInstInfo, posUpdateOfAcctId);
      },
      [&](const auto& stgInstInfo, const auto& posSnapshotOfAcctId) {
        taskHandler->onPosSnapshotOfAcctId(stgInstInfo, posSnapshotOfAcctId);
      },

      [&](const auto& stgInstInfo, const auto& posUpdateOfStgId) {
        taskHandler->onPosUpdateOfStgId(stgInstInfo, posUpdateOfStgId);
      },
      [&](const auto& stgInstInfo, const auto& posSnapshotOfStgId) {
        taskHandler->onPosSnapshotOfStgId(stgInstInfo, posSnapshotOfStgId);
      },

      [&](const auto& stgInstInfo, const auto& posUpdateOfStgInstId) {
        taskHandler->onPosUpdateOfStgInstId(stgInstInfo, posUpdateOfStgInstId);
      },
      [&](const auto& stgInstInfo, const auto& posSnapshotOfStgInstId) {
        taskHandler->onPosSnapshotOfStgInstId(stgInstInfo,
                                              posSnapshotOfStgInstId);
      },

      [&](const auto& stgInstInfo, const auto& assetsUpdate) {
        taskHandler->onAssetsUpdate(stgInstInfo, assetsUpdate);
      },
      [&](const auto& stgInstInfo, const auto& assetsSnapshot) {
        taskHandler->onAssetsSnapshot(stgInstInfo, assetsSnapshot);
      },

      [&](const auto& stgInstInfo, const auto& asyncTask) {
        taskHandler->onOtherStgInstTask(stgInstInfo, asyncTask);
      }};

  //!
  //! 消息到达触发StgEngImpl回调，然后触发StgInstTaskHandlerBundle中的回调，最后
  //! 触发StgInstTaskHandlerBase中的回调函数。
  //!
  const auto stgInstTaskHandlerImpl = std::make_shared<StgInstTaskHandlerImpl>(
      stgEngImpl_.get(), stgInstTaskHandlerBundle);
  stgEngImpl_->installStgInstTaskHandler(stgInstTaskHandlerImpl);
}

std::tuple<int, OrderId> StgEng::order(const StgInstInfoSPtr& stgInstInfo,
                                       MarketCode marketCode,
                                       const std::string& symbolCode, Side side,
                                       PosDirection posDirection,
                                       Decimal orderPrice, Decimal orderSize,
                                       TrdAcctId trdAcctId,
                                       CloseTDayStg closeTDayStg, AlgoId algoId,
                                       const SimedTDInfoSPtr& simedTDInfo) {
  return stgEngImpl_->order(stgInstInfo, marketCode, symbolCode, side,
                            posDirection, orderPrice, orderSize, trdAcctId,
                            closeTDayStg, algoId, simedTDInfo);
}

std::tuple<int, OrderId> StgEng::order(const StgInstInfoSPtr& stgInstInfo,
                                       MarketCode marketCode,
                                       const std::string& symbolCode, Side side,
                                       Decimal orderPrice, Decimal orderSize,
                                       TrdAcctId trdAcctId,
                                       CloseTDayStg closeTDayStg, AlgoId algoId,
                                       const SimedTDInfoSPtr& simedTDInfo) {
  return stgEngImpl_->order(stgInstInfo, marketCode, symbolCode, side,
                            orderPrice, orderSize, trdAcctId, closeTDayStg,
                            algoId, simedTDInfo);
}

std::tuple<int, OrderId> StgEng::order(OrderInfoSPtr& orderInfo) {
  return stgEngImpl_->order(orderInfo);
}

int StgEng::cancelOrder(OrderId orderId) {
  return stgEngImpl_->cancelOrder(orderId);
}

std::vector<OrderId> StgEng::cancelAllOrderOfStg() {
  return stgEngImpl_->cancelAllOrderOfStg();
}

std::vector<OrderId> StgEng::cancelAllOrderOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) {
  return stgEngImpl_->cancelAllOrderOfStgInst(stgInstInfo);
}

std::vector<OrderId> StgEng::cancelAllOrderOfStgInst(StgInstId stgInstId) {
  return stgEngImpl_->cancelAllOrderOfStgInst(stgInstId);
}

std::vector<OrderId> StgEng::cancelAllOrderOfAlgo(AlgoId algoId) {
  return stgEngImpl_->cancelAllOrderOfAlgo(algoId);
}

std::tuple<int, AlgoId> StgEng::algoOrder(
    const StgInstInfoSPtr& stgInstInfo, const std::string& algoType,
    const std::string& algoName, std::uint32_t lifetime,
    const std::string& algoParamsInJsonFmt) {
  return stgEngImpl_->algoOrder(stgInstInfo, algoType, algoName, lifetime,
                                algoParamsInJsonFmt);
}

int StgEng::cancelAlgoOrder(AlgoId algoId) {
  return stgEngImpl_->cancelAlgoOrder(algoId);
}

std::string StgEng::getProgressOfAlgoOrder(AlgoId algoId) {
  return getProgressOfAlgoOrder(algoId);
}

MarketDataCacheSPtr StgEng::getMarketDataCache() const {
  return stgEngImpl_->getMarketDataCache();
}

std::tuple<int, OrderInfoSPtr> StgEng::getOrderInfo(OrderId orderId) const {
  return stgEngImpl_->getOrderInfo(orderId);
}

int StgEng::sub(StgInstId subscriber, const std::string& topic) {
  return stgEngImpl_->sub(subscriber, topic);
}

int StgEng::unSub(StgInstId subscriber, const std::string& topic) {
  return stgEngImpl_->unSub(subscriber, topic);
}

std::tuple<int, std::string> StgEng::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t tsBegin, std::uint64_t tsEnd) {
  return stgEngImpl_->queryHisMDBetween2Ts(stgInstInfo, topic, tsBegin, tsEnd);
}

std::tuple<int, std::string> StgEng::queryHisMDBetween2Ts(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext) {
  return stgEngImpl_->queryHisMDBetween2Ts(stgInstInfo, marketCode, symbolType,
                                           symbolCode, mdType, tsBegin, tsEnd,
                                           ext);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  return stgEngImpl_->querySpecificNumOfHisMDBeforeTs(stgInstInfo, topic, ts,
                                                      num);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDBeforeTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  return stgEngImpl_->querySpecificNumOfHisMDBeforeTs(
      stgInstInfo, marketCode, symbolType, symbolCode, mdType, ts, num, ext);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
    std::uint64_t ts, int num) {
  return stgEngImpl_->querySpecificNumOfHisMDAfterTs(stgInstInfo, topic, ts,
                                                     num);
}

std::tuple<int, std::string> StgEng::querySpecificNumOfHisMDAfterTs(
    const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
    SymbolType symbolType, const std::string& symbolCode, MDType mdType,
    std::uint64_t ts, int num, const std::string& ext) {
  return stgEngImpl_->querySpecificNumOfHisMDAfterTs(
      stgInstInfo, marketCode, symbolType, symbolCode, mdType, ts, num, ext);
}

void StgEng::installStgInstTimer(StgInstId stgInstId,
                                 const std::string& timerName,
                                 const std::string& execTime,
                                 std::uint32_t timeZone) {
  stgEngImpl_->installStgInstTimer(stgInstId, timerName, execTime, timeZone);
}

void StgEng::installStgInstTimer(StgInstId stgInstId,
                                 const std::string& timerName,
                                 ExecAtStartup execAtStartUp,
                                 std::uint32_t milliSecInterval,
                                 std::uint64_t maxExecTimes) {
  stgEngImpl_->installStgInstTimer(stgInstId, timerName, execAtStartUp,
                                   milliSecInterval, maxExecTimes);
}

void StgEng::uninstallStgInstTimer(StgInstId stgInstId,
                                   const std::string& timerName) {
  stgEngImpl_->uninstallStgInstTimer(stgInstId, timerName);
}

std::vector<OrderInfoSPtr> StgEng::getUnclosedOrderInfoGroup(
    const StgInstInfoSPtr& stgInstInfo) const {
  return stgEngImpl_->getUnclosedOrderInfoGroup(stgInstInfo);
}

PosOfStgInstGroupSPtr StgEng::getPosOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) const {
  return stgEngImpl_->getPosOfStgInst(stgInstInfo);
}

bool StgEng::saveStgPrivateData(StgInstId stgInstId,
                                const std::string& jsonStr) {
  return stgEngImpl_->saveStgPrivateData(stgInstId, jsonStr);
}

std::string StgEng::loadStgPrivateData(StgInstId stgInstId) {
  return stgEngImpl_->loadStgPrivateData(stgInstId);
}

std::tuple<int, std::string> StgEng::syncExecSql(const std::string& sql) {
  return stgEngImpl_->syncExecSql(sql);
}

void StgEng::saveToDB(const PnlSPtr& pnl) {
  if (!pnl) return;
  stgEngImpl_->saveToDB(pnl);
}

void StgEng::logTrace(const std::string& fmt,
                      const std::vector<std::string>& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logTrace(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logDebug(const std::string& fmt,
                      const std::vector<std::string>& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logDebug(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logInfo(const std::string& fmt,
                     const std::vector<std::string>& args,
                     const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logInfo(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logWarn(const std::string& fmt,
                     const std::vector<std::string>& args,
                     const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logWarn(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logError(const std::string& fmt,
                      const std::vector<std::string>& args,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logError(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logCritical(const std::string& fmt,
                         const std::vector<std::string>& args,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logCritical(fmt, args, stgInstInfo, notifyToTerminal);
}

void StgEng::logTrace(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logTrace(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logDebug(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logDebug(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logInfo(const std::string& fmt, const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logInfo(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logWarn(const std::string& fmt, const StgInstInfoSPtr& stgInstInfo,
                     NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logWarn(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logError(const std::string& fmt,
                      const StgInstInfoSPtr& stgInstInfo,
                      NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logError(fmt, stgInstInfo, notifyToTerminal);
}

void StgEng::logCritical(const std::string& fmt,
                         const StgInstInfoSPtr& stgInstInfo,
                         NotifyToTerminal notifyToTerminal) {
  stgEngImpl_->logCritical(fmt, stgInstInfo, notifyToTerminal);
}

}  // namespace bq::stg
