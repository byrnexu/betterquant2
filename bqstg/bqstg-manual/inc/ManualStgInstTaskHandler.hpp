/*!
 * \file ManualStgInstTaskHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "StgInstTaskHandlerBase.hpp"
#include "util/StdExt.hpp"

namespace bq::stg {

class ManualStgInstTaskHandler : public StgInstTaskHandlerBase {
 public:
  ManualStgInstTaskHandler(const ManualStgInstTaskHandler&) = delete;
  ManualStgInstTaskHandler& operator=(const ManualStgInstTaskHandler&) = delete;
  ManualStgInstTaskHandler(const ManualStgInstTaskHandler&&) = delete;
  ManualStgInstTaskHandler& operator=(const ManualStgInstTaskHandler&&) =
      delete;

  using StgInstTaskHandlerBase::StgInstTaskHandlerBase;

 private:
  void onStgStart() final;
  void onStgInstStart(const StgInstInfoSPtr& stgInstInfo) final;

 private:
  void onStgInstAdd(const StgInstInfoSPtr& stgInstInfo) final;
  void onStgInstDel(const StgInstInfoSPtr& stgInstInfo) final;
  void onStgInstChg(const StgInstInfoSPtr& stgInstInfo) final;
  void onStgInstTimer(const StgInstInfoSPtr& stgInstInfo,
                      const std::string& timerName) final;

 private:
  void onTrades(const StgInstInfoSPtr& stgInstInfo, const Trades* trades) final;

  void onOrders(const StgInstInfoSPtr& stgInstInfo, const Orders* orders) final;

  void onBooks(const StgInstInfoSPtr& stgInstInfo, const Books* books) final;

  void onCandle(const StgInstInfoSPtr& stgInstInfo, const Candle* candle) final;

  void onTickers(const StgInstInfoSPtr& stgInstInfo,
                 const Tickers* tickers) final;

 private:
  void onStgManualIntervention(const StgInstInfoSPtr& stgInstInfo,
                               const CommonIPCData* commonIPCData) final;

  void onOrderRet(const StgInstInfoSPtr& stgInstInfo,
                  const OrderInfo* orderInfo) final;

  void onCancelOrderRet(const StgInstInfoSPtr& stgInstInfo,
                        const OrderInfo* orderInfo) final;

 private:
  void onPosSnapshotOfAcctId(const StgInstInfoSPtr& stgInstInfo,
                             const PosSnapshotSPtr& posSnapshotOfAcctId) final;

  void onPosUpdateOfAcctId(const StgInstInfoSPtr& stgInstInfo,
                           const PosSnapshotSPtr& posUpdateOfAcctId) final;

  void onPosSnapshotOfStgId(const StgInstInfoSPtr& stgInstInfo,
                            const PosSnapshotSPtr& posSnapshotOfStgId) final;

  void onPosUpdateOfStgId(const StgInstInfoSPtr& stgInstInfo,
                          const PosSnapshotSPtr& posUpdateOfStgId) final;

  void onPosSnapshotOfStgInstId(
      const StgInstInfoSPtr& stgInstInfo,
      const PosSnapshotSPtr& posSnapshotOfStgInstId) final;

  void onPosUpdateOfStgInstId(
      const StgInstInfoSPtr& stgInstInfo,
      const PosSnapshotSPtr& posUpdateOfStgInstId) final;

  void onAssetsSnapshot(const StgInstInfoSPtr& stgInstInfo,
                        const AssetsSnapshotSPtr& assetsSnapshot) final;

  void onAssetsUpdate(const StgInstInfoSPtr& stgInstInfo,
                      const AssetsUpdateSPtr& assetsUpdate) final;

 private:
  void onOtherStgInstTask(const StgInstInfoSPtr& stgInstInfo,
                          const SHMIPCAsyncTaskSPtr& asyncTask) final;
};

}  // namespace bq::stg
