/*!
 * \file StgInstTaskHandlerImpl.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/PchBase.hpp"

namespace bq {
struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
template <typename Task>
struct AsyncTask;

using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;

struct PosInfo;
using PosInfoSPtr = std::shared_ptr<PosInfo>;

class PosSnapshot;
using PosSnapshotSPtr = std::shared_ptr<PosSnapshot>;

struct AssetInfo;
using AssetInfoSPtr = std::shared_ptr<AssetInfo>;

using AssetsUpdate = std::map<std::string, AssetInfoSPtr>;
using AssetsUpdateSPtr = std::shared_ptr<AssetsUpdate>;

using AssetsSnapshot = std::map<std::string, AssetInfoSPtr>;
using AssetsSnapshotSPtr = std::shared_ptr<AssetsSnapshot>;
}  // namespace bq

namespace bq {
struct Trades;
struct Orders;
struct Books;
struct Tickers;
struct Candle;
struct OrderInfo;
struct Bid1Ask1;
struct LastPrice;
using TradesSPtr = std::shared_ptr<Trades>;
using OrdersSPtr = std::shared_ptr<Orders>;
using BooksSPtr = std::shared_ptr<Books>;
using TickersSPtr = std::shared_ptr<Tickers>;
using CandleSPtr = std::shared_ptr<Candle>;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
using Bid1Ask1SPtr = std::shared_ptr<Bid1Ask1>;
using LastPriceSPtr = std::shared_ptr<LastPrice>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct CommonIPCData;
using CommonIPCDataSPtr = std::shared_ptr<CommonIPCData>;
}  // namespace bq

namespace bq::stg {

struct StgInstTaskHandlerBundle {
  std::function<void(const StgInstInfoSPtr&, const CommonIPCData*)>
      onStgManualIntervention_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const CommonIPCData*)>
      onPushTopic_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const OrderInfo*)> onOrderRet_{
      nullptr};
  std::function<void(const StgInstInfoSPtr&, const OrderInfo*)>
      onCancelOrderRet_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const CommonIPCData*)>
      onAlgoOrder_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const Trades*)> onTrades_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const Orders*)> onOrders_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const Books*)> onBooks_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const Candle*)> onCandle_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const Tickers*)> onTickers_{
      nullptr};
  std::function<void(const StgInstInfoSPtr&, const Bid1Ask1*)> onBid1Ask1_{
      nullptr};
  std::function<void(const StgInstInfoSPtr&, const LastPrice*)> onLastPrice_{
      nullptr};
  std::function<void(const StgInstInfoSPtr&, const Candle*)> onDynCandle_{
      nullptr};

  std::function<void()> onStgStart_{nullptr};
  std::function<void(const StgInstInfoSPtr&)> onStgInstStart_{nullptr};
  std::function<void()> onStgStop_{nullptr};
  std::function<void(const StgInstInfoSPtr&)> onStgInstStop_{nullptr};

  std::function<void(const StgInstInfoSPtr&)> onStgInstAdd_{nullptr};
  std::function<void(const StgInstInfoSPtr&)> onStgInstDel_{nullptr};
  std::function<void(const StgInstInfoSPtr&)> onStgInstChg_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const std::string&)>
      onStgInstTimer_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosUpdateOfAcctId_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosSnapshotOfAcctId_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosUpdateOfStgId_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosSnapshotOfStgId_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosUpdateOfStgInstId_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const PosSnapshotSPtr&)>
      onPosSnapshotOfStgInstId_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const AssetsUpdateSPtr&)>
      onAssetsUpdate_{nullptr};
  std::function<void(const StgInstInfoSPtr&, const AssetsSnapshotSPtr&)>
      onAssetsSnapshot_{nullptr};

  std::function<void(const StgInstInfoSPtr&, const SHMIPCAsyncTaskSPtr&)>
      onOtherStgInstTask_{nullptr};
};

class StgEngImpl;

class StgInstTaskHandlerImpl {
 public:
  StgInstTaskHandlerImpl(const StgInstTaskHandlerImpl&) = delete;
  StgInstTaskHandlerImpl& operator=(const StgInstTaskHandlerImpl&) = delete;
  StgInstTaskHandlerImpl(const StgInstTaskHandlerImpl&&) = delete;
  StgInstTaskHandlerImpl& operator=(const StgInstTaskHandlerImpl&&) = delete;

  explicit StgInstTaskHandlerImpl(
      StgEngImpl* stgEng,
      const StgInstTaskHandlerBundle& stgInstTaskHandlerBundle);

 public:
  StgEngImpl* getStgEngImpl() const { return stgEng_; }
  StgInstTaskHandlerBundle& getStgInstTaskHandlerBundle() {
    return stgInstTaskHandlerBundle_;
  }

 public:
  void handleAsyncTask(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void handleAsyncTaskImpl(const StgInstInfoSPtr& stgInstInfo,
                           const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  bool beforeOnStgManualIntervention(const StgInstInfoSPtr& stgInstInfo,
                                     const std::string& instruction);

 private:
  void beforeOnOrderRet(const StgInstInfoSPtr& stgInstInfo,
                        const OrderInfo* orderInfo);

  void beforeOnCancelOrderRet(const StgInstInfoSPtr& stgInstInfo,
                              const OrderInfo* orderInfo);

 private:
  StgEngImpl* stgEng_;
  StgInstTaskHandlerBundle stgInstTaskHandlerBundle_;
};

}  // namespace bq::stg
