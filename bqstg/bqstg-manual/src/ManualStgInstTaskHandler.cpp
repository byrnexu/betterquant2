/*!
 * \file ManualStgInstTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "ManualStgInstTaskHandler.hpp"

#include "CommonIPCData.hpp"
#include "StgEng.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"

namespace bq::stg {

void ManualStgInstTaskHandler::onStgStart() {}

void ManualStgInstTaskHandler::onStgInstStart(
    const StgInstInfoSPtr& stgInstInfo) {}

void ManualStgInstTaskHandler::onStgInstAdd(
    const StgInstInfoSPtr& stgInstInfo) {}
void ManualStgInstTaskHandler::onStgInstDel(
    const StgInstInfoSPtr& stgInstInfo) {}
void ManualStgInstTaskHandler::onStgInstChg(
    const StgInstInfoSPtr& stgInstInfo) {}

void ManualStgInstTaskHandler::onStgInstTimer(
    const StgInstInfoSPtr& stgInstInfo, const std::string& timerName) {}

void ManualStgInstTaskHandler::onTrades(const StgInstInfoSPtr& stgInstInfo,
                                        const Trades* trades) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, trades->toStr());
}

void ManualStgInstTaskHandler::onOrders(const StgInstInfoSPtr& stgInstInfo,
                                        const Orders* orders) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, orders->toStr());
}

void ManualStgInstTaskHandler::onBooks(const StgInstInfoSPtr& stgInstInfo,
                                       const Books* books) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, books->toStr());
}

void ManualStgInstTaskHandler::onCandle(const StgInstInfoSPtr& stgInstInfo,
                                        const Candle* candle) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, candle->toStr());
}

void ManualStgInstTaskHandler::onTickers(const StgInstInfoSPtr& stgInstInfo,
                                         const Tickers* tickers) {
  LOG_I("{}: {}", stgInstInfo->stgInstId_, tickers->toStr());
}

void ManualStgInstTaskHandler::onStgManualIntervention(
    const StgInstInfoSPtr& stgInstInfo, const CommonIPCData* commonIPCData) {
  LOG_I("On stg manual intervention. {}", commonIPCData->data_);
}

void ManualStgInstTaskHandler::onOrderRet(const StgInstInfoSPtr& stgInstInfo,
                                          const OrderInfo* orderInfo) {}

void ManualStgInstTaskHandler::onCancelOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {}

void ManualStgInstTaskHandler::onPosSnapshotOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onPosUpdateOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onPosSnapshotOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onPosUpdateOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onPosSnapshotOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onPosUpdateOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onAssetsSnapshot(
    const StgInstInfoSPtr& stgInstInfo,
    const AssetsSnapshotSPtr& assetsSnapshot) {
  for (const auto& rec : *assetsSnapshot) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onAssetsUpdate(
    const StgInstInfoSPtr& stgInstInfo, const AssetsUpdateSPtr& assetsUpdate) {
  for (const auto& rec : *assetsUpdate) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void ManualStgInstTaskHandler::onOtherStgInstTask(
    const StgInstInfoSPtr& stgInstInfo, const SHMIPCAsyncTaskSPtr& asyncTask) {}

}  // namespace bq::stg
