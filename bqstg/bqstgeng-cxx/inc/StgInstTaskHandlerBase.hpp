/*!
 * \file StgInstTaskHandlerBase.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "BQPub.hpp"
#include "Pub.hpp"
#include "SHMIPCPub.hpp"

namespace bq {
struct CommonIPCData;
using CommonIPCDataSPtr = std::shared_ptr<CommonIPCData>;
}  // namespace bq

namespace bq::stg {

class StgEng;

class StgInstTaskHandlerBase {
  friend class StgEng;

 public:
  StgInstTaskHandlerBase(const StgInstTaskHandlerBase&) = delete;
  StgInstTaskHandlerBase& operator=(const StgInstTaskHandlerBase&) = delete;
  StgInstTaskHandlerBase(const StgInstTaskHandlerBase&&) = delete;
  StgInstTaskHandlerBase& operator=(const StgInstTaskHandlerBase&&) = delete;

  /**
   * @Synopsis
   *
   * @Param stgEng 测量引擎对象
   */
  explicit StgInstTaskHandlerBase(StgEng* stgEng);

 public:
  /**
   * @Synopsis
   *
   * @Returns 策略引擎对象
   */
  StgEng* getStgEng() const { return stgEng_; }

 private:
  /**
   * @Synopsis
   *
   * @Param stgInstInfo   子策略id
   * @Param commonIPCData 人工干预信息
   */
  virtual void onStgManualIntervention(const StgInstInfoSPtr& stgInstInfo,
                                       const CommonIPCData* commonIPCData) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo  子策略id
   * @Param commonIPCData 订阅的topic内容
   */
  virtual void onPushTopic(const StgInstInfoSPtr& stgInstInfo,
                           const CommonIPCData* commonIPCData) {}

  /**
   * @Synopsis 下单应答或者回报
   *
   * @Param stgInstInfo 子策略id
   * @Param orderInfo   订单信息
   */
  virtual void onOrderRet(const StgInstInfoSPtr& stgInstInfo,
                          const OrderInfo* orderInfo) {}

  /**
   * @Synopsis 撤单应答
   *
   * @Param stgInstInfo 子策略id
   * @Param orderInfo   订单信息
   */
  virtual void onCancelOrderRet(const StgInstInfoSPtr& stgInstInfo,
                                const OrderInfo* orderInfo) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo
   * @Param commonIPCData
   */
  virtual void onAlgoOrder(const StgInstInfoSPtr& stgInstInfo,
                           const CommonIPCData* commonIPCData) {}

 private:
  /**
   * @Synopsis 逐笔成交行情
   *
   * @Param stgInstInfo 子策略id
   * @Param trades      逐笔成交
   */
  virtual void onTrades(const StgInstInfoSPtr& stgInstInfo,
                        const Trades* trades) {}

  /**
   * @Synopsis 逐笔委托行情
   *
   * @Param stgInstInfo 子策略id
   * @Param orders      逐笔委托
   */
  virtual void onOrders(const StgInstInfoSPtr& stgInstInfo,
                        const Orders* orders) {}

  /**
   * @Synopsis 订单簿行情
   *
   * @Param stgInstInfo 子策略id
   * @Param books       订单簿
   */
  virtual void onBooks(const StgInstInfoSPtr& stgInstInfo, const Books* books) {
  }

  /**
   * @Synopsis k线行情
   *
   * @Param stgInstInfo 子策略id
   * @Param candle      k线
   */
  virtual void onCandle(const StgInstInfoSPtr& stgInstInfo,
                        const Candle* candle) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo 子策略id
   * @Param tickers     tickers
   */
  virtual void onTickers(const StgInstInfoSPtr& stgInstInfo,
                         const Tickers* tickers) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo
   * @Param bid1Ask1
   */
  virtual void onBid1Ask1(const StgInstInfoSPtr& stgInstInfo,
                          const Bid1Ask1* bid1Ask1) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo
   * @Param lastPrice
   */
  virtual void onLastPrice(const StgInstInfoSPtr& stgInstInfo,
                           const LastPrice* lastPrice) {}

  /**
   * @Synopsis
   *
   * @Param stgInstInfo
   * @Param candle
   */
  virtual void onDynCandle(const StgInstInfoSPtr& stgInstInfo,
                           const Candle* candle) {}

 private:
  /**
   * @Synopsis 策略引擎启动回调
   */
  virtual void onStgStart() {}

  /**
   * @Synopsis 子策略启动回调
   *
   * @Param stgInstInfo 策略实例id
   */
  virtual void onStgInstStart(const StgInstInfoSPtr& stgInstInfo) {}

 private:
  /**
   * @Synopsis 策略引擎停止回调
   */
  virtual void onStgStop() {}

  /**
   * @Synopsis 子策略停止回调
   *
   * @Param stgInstInfo 策略实例id
   */
  virtual void onStgInstStop(const StgInstInfoSPtr& stgInstInfo) {}

 private:
  /**
   * @Synopsis 子策略增加回调
   *
   * @Param stgInstInfo
   */
  virtual void onStgInstAdd(const StgInstInfoSPtr& stgInstInfo) {}

  /**
   * @Synopsis 子策略移除回调
   *
   * @Param stgInstInfo
   */
  virtual void onStgInstDel(const StgInstInfoSPtr& stgInstInfo) {}

  /**
   * @Synopsis 子策略参数变化回调
   *
   * @Param stgInstInfo
   */
  virtual void onStgInstChg(const StgInstInfoSPtr& stgInstInfo) {}

  /**
   * @Synopsis 子策略定时器触发事件
   *
   * @Param stgInstInfo
   * @Param timerName
   */
  virtual void onStgInstTimer(const StgInstInfoSPtr& stgInstInfo,
                              const std::string& timerName) {}

 private:
  /**
   * @Synopsis 账户层面仓位变动信息，仓位有变动就会收到一个账户层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosUpdateOfAcctId(const StgInstInfoSPtr& stgInstInfo,
                                   const PosSnapshotSPtr& posSnapshot) {}

  /**
   * @Synopsis
   * 账户层面仓位快照信息，不管仓位是否变化都会定时收到账户层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosSnapshotOfAcctId(const StgInstInfoSPtr& stgInstInfo,
                                     const PosSnapshotSPtr& posSnapshot) {}

  /**
   * @Synopsis 策略层面仓位变动信息，仓位有变动就会收到一个策略层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosUpdateOfStgId(const StgInstInfoSPtr& stgInstInfo,
                                  const PosSnapshotSPtr& posSnapshot) {}

  /**
   * @Synopsis
   * 策略层面仓位快照信息，不管仓位是否变化都会定时收到策略层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosSnapshotOfStgId(const StgInstInfoSPtr& stgInstInfo,
                                    const PosSnapshotSPtr& posSnapshot) {}

  /**
   * @Synopsis 子策略层面仓位变动信息，仓位有变动就会收到一个子策略层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosUpdateOfStgInstId(const StgInstInfoSPtr& stgInstInfo,
                                      const PosSnapshotSPtr& posSnapshot) {}

  /**
   * @Synopsis
   * 策略层面仓位快照信息，不管仓位是否变化都会定时收到子策略层面仓位全量
   *
   * @Param stgInstInfo 子策略实例id
   * @Param posSnapshot
   */
  virtual void onPosSnapshotOfStgInstId(const StgInstInfoSPtr& stgInstInfo,
                                        const PosSnapshotSPtr& posSnapshot) {}

  virtual void onAssetsUpdate(const StgInstInfoSPtr& stgInstInfo,
                              const AssetsUpdateSPtr& assetsUpdate) {}

  virtual void onAssetsSnapshot(const StgInstInfoSPtr& stgInstInfo,
                                const AssetsSnapshotSPtr& assetsSnapshot) {}

 private:
  virtual void onOtherStgInstTask(const StgInstInfoSPtr& stgInstInfo,
                                  const SHMIPCAsyncTaskSPtr& asyncTask) {}

 private:
  StgEng* stgEng_;
};

}  // namespace bq::stg
