/*!
 * \file StgInstTaskHandlerOfSpotTest.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgInstTaskHandlerOfSpotTest.hpp"

#include "CommonIPCData.hpp"
#include "StgEng.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"

namespace bq::stg {

void StgInstTaskHandlerOfSpotTest::onStgStart() {
  LOG_I("Private data: {}", getStgEng()->loadStgPrivateData(1));
  getStgEng()->saveStgPrivateData(1, "Hello world!");

  getStgEng()->installStgInstTimer(1, "stgInst1Timer", ExecAtStartup::True,
                                   MilliSecInterval(1000), ExecTimes(100));
}

void StgInstTaskHandlerOfSpotTest::onStgInstStart(
    const StgInstInfoSPtr& stgInstInfo) {
  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Orders");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Tickers");
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 2) {
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SZSE.Spot/002077/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Orders");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SZSE.Spot/002077/Orders");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SSE.Spot/603123/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.SZSE.Spot/002077/Tickers");
  }
}

void StgInstTaskHandlerOfSpotTest::onStgInstAdd(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfSpotTest::onStgInstDel(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfSpotTest::onStgInstChg(
    const StgInstInfoSPtr& stgInstInfo) {}

void StgInstTaskHandlerOfSpotTest::onStgInstTimer(
    const StgInstInfoSPtr& stgInstInfo, const std::string& timerName) {}

void StgInstTaskHandlerOfSpotTest::onTrades(const StgInstInfoSPtr& stgInstInfo,
                                            const Trades* trades) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, trades->toStr());
}

void StgInstTaskHandlerOfSpotTest::onOrders(const StgInstInfoSPtr& stgInstInfo,
                                            const Orders* orders) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, orders->toStr());
}

void StgInstTaskHandlerOfSpotTest::onBooks(const StgInstInfoSPtr& stgInstInfo,
                                           const Books* books) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, books->toStr());
}

void StgInstTaskHandlerOfSpotTest::onCandle(const StgInstInfoSPtr& stgInstInfo,
                                            const Candle* candle) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, candle->toStr());
}

void StgInstTaskHandlerOfSpotTest::onTickers(const StgInstInfoSPtr& stgInstInfo,
                                             const Tickers* tickers) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, tickers->toStr());
}

void StgInstTaskHandlerOfSpotTest::onStgManualIntervention(
    const StgInstInfoSPtr& stgInstInfo, const CommonIPCData* commonIPCData) {
  LOG_I("On stg manual intervention. {}", commonIPCData->data_);
}

void StgInstTaskHandlerOfSpotTest::onOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {}

void StgInstTaskHandlerOfSpotTest::onCancelOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {}

void StgInstTaskHandlerOfSpotTest::onPosSnapshotOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onPosUpdateOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onPosSnapshotOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onPosUpdateOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onPosSnapshotOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onPosUpdateOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onAssetsSnapshot(
    const StgInstInfoSPtr& stgInstInfo,
    const AssetsSnapshotSPtr& assetsSnapshot) {
  for (const auto& rec : *assetsSnapshot) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onAssetsUpdate(
    const StgInstInfoSPtr& stgInstInfo, const AssetsUpdateSPtr& assetsUpdate) {
  for (const auto& rec : *assetsUpdate) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfSpotTest::onOtherStgInstTask(
    const StgInstInfoSPtr& stgInstInfo, const SHMIPCAsyncTaskSPtr& asyncTask) {}

}  // namespace bq::stg
