/*!
 * \file StgInstTaskHandlerOfFuturesTest.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgInstTaskHandlerOfFuturesTest.hpp"

#include "StgEng.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"

namespace bq::stg {

void StgInstTaskHandlerOfFuturesTest::onStgStart() {
  getStgEng()->installStgInstTimer(2, "stgInst1Timer", ExecAtStartup::True,
                                   MilliSecInterval(1000), ExecTimes(100));
}

void StgInstTaskHandlerOfFuturesTest::onStgInstStart(
    const StgInstInfoSPtr& stgInstInfo) {
  getStgEng()->sub(stgInstInfo->stgInstId_,
                   "MD@Binance@Futures@DOT-USDT-Futures@Trades");
  getStgEng()->sub(stgInstInfo->stgInstId_,
                   "MD@Binance@Futures@ETH-USDT-Futures@Trades");
  getStgEng()->sub(stgInstInfo->stgInstId_,
                   "MD@Binance@Futures@DOT-USDT-Futures@Tickers");
  getStgEng()->sub(stgInstInfo->stgInstId_,
                   "MD@Binance@Futures@DOT-USDT-Futures@Books@400");
  getStgEng()->sub(stgInstInfo->stgInstId_,
                   "MD@Binance@Futures@DOT-USDT-Futures@Candle");

  getStgEng()->sub(
      stgInstInfo->stgInstId_,
      "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000/StgInstId/2");
}

void StgInstTaskHandlerOfFuturesTest::onStgInstAdd(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfFuturesTest::onStgInstDel(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfFuturesTest::onStgInstChg(
    const StgInstInfoSPtr& stgInstInfo) {}

void StgInstTaskHandlerOfFuturesTest::onStgInstTimer(
    const StgInstInfoSPtr& stgInstInfo, const std::string& timerName) {
  static bool alreadyOrder = false;
  if (StgInstIdOfTriggerSignal(stgInstInfo) == 2 && alreadyOrder == false) {
    const auto symbolCode = "ETH-USDT-Futures";
    Decimal price = 0.0;
    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxSymbol2Price_);
      const auto iter = symbol2Price_.find(symbolCode);
      if (iter == std::end(symbol2Price_)) {
        return;
      } else {
        price = iter->second;
      }
    }
    auto side = Side::Bid;  //
    if (side == Side::Bid) {
      price *= 1.01;
    } else {
      price *= 0.99;
    }
    price = static_cast<int>(price * 100) / 100.0;
    getStgEng()->order(stgInstInfo, MarketCode::Binance, symbolCode, side,
                       PosDirection::Open, price, 0.1, 100004);
    alreadyOrder = true;
  }
}

void StgInstTaskHandlerOfFuturesTest::onTrades(
    const StgInstInfoSPtr& stgInstInfo, const Trades* trades) {
  //!
  //! Intel(R)cpu Xeon(R) Platinum 8369B CPU @ 2.70GHz
  //!
  //! Trades Total num: 10001; avg: 21.75662; med: 18; min: 10; max: 80
  //! Trades Total num: 10001; avg: 22.79422; med: 20; min: 8 ; max: 133
  //! Trades Total num: 10001; avg: 20.62384; med: 18; min: 8 ; max: 115
#ifdef PERF_TEST
  EXEC_PERF_TEST("Trades", trades->mdHeader_.localTs_, 10000, 100);
  return;
#endif
  LOG_D("{}: {}", stgInstInfo->stgInstId_, trades->toStr());
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSymbol2Price_);
    symbol2Price_[trades->mdHeader_.symbolCode_] = trades->price_;
  }
}

void StgInstTaskHandlerOfFuturesTest::onOrders(
    const StgInstInfoSPtr& stgInstInfo, const Orders* orders) {
#ifdef PERF_TEST
  EXEC_PERF_TEST("Orders", orders->mdHeader_.localTs_, 10000, 100);
  return;
#endif
  LOG_D("{}: {}", stgInstInfo->stgInstId_, orders->toStr());
}

void StgInstTaskHandlerOfFuturesTest::onBooks(
    const StgInstInfoSPtr& stgInstInfo, const Books* books) {
  //!
  //! Intel(R)cpu Xeon(R) Platinum 8369B CPU @ 2.70GHz
  //!
  //! Books Total num: 10001; avg: 107.24368; med: 97 ; min: 40; max: 558
  //! Books Total num: 10001; avg: 116.52285; med: 112; min: 37; max: 460
  //! Books Total num: 10001; avg: 109.27547; med: 106; min: 40; max: 436
#ifdef PERF_TEST
  EXEC_PERF_TEST("Books", books->mdHeader_.localTs_, 10000, 100);
  return;
#endif
  LOG_D("{}: {}", stgInstInfo->stgInstId_, books->toStr());
}

void StgInstTaskHandlerOfFuturesTest::onCandle(
    const StgInstInfoSPtr& stgInstInfo, const Candle* candle) {
  //!
  //! Intel(R)cpu Xeon(R) Platinum 8369B CPU @ 2.70GHz
  //!
  //! Candle Total num: 1001; avg: 22.98701; med: 20; min: 12; max: 73
  //! Candle Total num: 1001; avg: 22.41758; med: 20; min: 12; max: 74
  //! Candle Total num: 1001; avg: 22.25275; med: 20; min: 13; max: 81
#ifdef PERF_TEST
  EXEC_PERF_TEST("Candle", candle->mdHeader_.localTs_, 1000, 10);
  return;
#endif
  LOG_D("{}: {}", stgInstInfo->stgInstId_, candle->toStr());
}

void StgInstTaskHandlerOfFuturesTest::onTickers(
    const StgInstInfoSPtr& stgInstInfo, const Tickers* tickers) {
  //!
  //! Intel(R)cpu Xeon(R) Platinum 8369B CPU @ 2.70GHz
  //!
  //! Tickers Total num: 1001; avg: 30.52547; med: 27; min: 15; max: 72
  //! Tickers Total num: 1001; avg: 27.56344; med: 26; min: 15; max: 72
  //! Tickers Total num: 1001; avg: 27.59940; med: 25; min: 14; max: 112
#ifdef PERF_TEST
  EXEC_PERF_TEST("Tickers", tickers->mdHeader_.localTs_, 1000, 10);
  return;
#endif
  LOG_D("{}: {}", stgInstInfo->stgInstId_, tickers->toStr());
}

void StgInstTaskHandlerOfFuturesTest::onOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {
  if (orderInfo->orderStatus_ == OrderStatus::ConfirmedByExch) {
    getStgEng()->cancelOrder(orderInfo->orderId_);
  }
}

void StgInstTaskHandlerOfFuturesTest::onCancelOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {}

void StgInstTaskHandlerOfFuturesTest::onPosSnapshotOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onPosUpdateOfAcctId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_I("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onPosSnapshotOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onPosUpdateOfStgId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_I("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onPosSnapshotOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posSnapshot) {
  for (const auto& rec : posSnapshot->getPosInfoDetail()) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onPosUpdateOfStgInstId(
    const StgInstInfoSPtr& stgInstInfo, const PosSnapshotSPtr& posUpdate) {
  for (const auto& rec : posUpdate->getPosInfoDetail()) {
    LOG_I("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onAssetsSnapshot(
    const StgInstInfoSPtr& stgInstInfo,
    const AssetsSnapshotSPtr& assetsSnapshot) {
  for (const auto& rec : *assetsSnapshot) {
    LOG_D("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onAssetsUpdate(
    const StgInstInfoSPtr& stgInstInfo, const AssetsUpdateSPtr& assetsUpdate) {
  for (const auto& rec : *assetsUpdate) {
    LOG_I("StgInstId: {} {} {}", stgInstInfo->stgInstId_, __func__,
          rec.second->toStr());
  }
}

void StgInstTaskHandlerOfFuturesTest::onOtherStgInstTask(
    const StgInstInfoSPtr& stgInstInfo, const SHMIPCAsyncTaskSPtr& asyncTask) {}

}  // namespace bq::stg
