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

  const auto now = static_cast<std::uint64_t>(time(nullptr)) * 1000000;

  int statusCode = 0;
  std::string data;

  std::tie(statusCode, data) = getStgEng()->queryHisMDBetween2Ts(
      nullptr, "MD@Binance@Spot@BTC-USDT@Candle", now - 60 * 1000000, now);
  std::tie(statusCode, data) = getStgEng()->queryHisMDBetween2Ts(
      nullptr, MarketCode::Binance, SymbolType::Spot, "BTC-USDT",
      MDType::Candle, now - 60 * 1000000, now);

  std::tie(statusCode, data) = getStgEng()->querySpecificNumOfHisMDBeforeTs(
      nullptr, "MD@Binance@Spot@BTC-USDT@Candle", now, 2);
  std::tie(statusCode, data) = getStgEng()->querySpecificNumOfHisMDBeforeTs(
      nullptr, MarketCode::Binance, SymbolType::Spot, "BTC-USDT",
      MDType::Candle, now, 2);

  std::tie(statusCode, data) = getStgEng()->querySpecificNumOfHisMDAfterTs(
      nullptr, "MD@Binance@Spot@BTC-USDT@Candle", now - 60 * 1000000, 1);
  std::tie(statusCode, data) = getStgEng()->querySpecificNumOfHisMDAfterTs(
      nullptr, MarketCode::Binance, SymbolType::Spot, "BTC-USDT",
      MDType::Candle, now - 60 * 1000000, 1);
}

void StgInstTaskHandlerOfSpotTest::onStgInstStart(
    const StgInstInfoSPtr& stgInstInfo) {
  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://RISK.PubChannel.Trade/AssetInfo/AcctId/10005");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000");
    getStgEng()->sub(
        stgInstInfo->stgInstId_,
        "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000/StgInstId/2");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ADA-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ADA-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ADA-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ADA-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/DOGE-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/DOGE-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/DOGE-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/DOGE-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Candle");
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 2) {
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://RISK.PubChannel.Trade/PosInfo/AcctId/10001");
    getStgEng()->sub(
        stgInstInfo->stgInstId_,
        "shm://RISK.PubChannel.Trade/PosInfo/StgId/10000/StgInstId/1");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/ETH-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/SOL-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/SOL-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/SOL-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/SOL-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BNB-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BNB-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BNB-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BNB-USDT/Candle");

    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Trades");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Tickers");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Books/400");
    getStgEng()->sub(stgInstInfo->stgInstId_,
                     "shm://MD.Binance.Spot/BTC-USDT/Candle");
  }
}

void StgInstTaskHandlerOfSpotTest::onStgInstAdd(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfSpotTest::onStgInstDel(
    const StgInstInfoSPtr& stgInstInfo) {}
void StgInstTaskHandlerOfSpotTest::onStgInstChg(
    const StgInstInfoSPtr& stgInstInfo) {}

void StgInstTaskHandlerOfSpotTest::onStgInstTimer(
    const StgInstInfoSPtr& stgInstInfo, const std::string& timerName) {
  static bool alreadyOrder = false;
  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1 && alreadyOrder == false) {
    const auto symbolCode = "ADA-USDT";
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
    auto side = Side::Ask;  //
    if (side == Side::Bid) {
      price *= 1.01;
    } else {
      price *= 0.99;
    }
    price = static_cast<int>(price * 10000) / 10000.0;
    getStgEng()->order(stgInstInfo, MarketCode::Binance, symbolCode, side,
                       PosDirection::Both, price, 30.0, 100001);
    alreadyOrder = true;
  }
}

void StgInstTaskHandlerOfSpotTest::onTrades(const StgInstInfoSPtr& stgInstInfo,
                                            const Trades* trades) {
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

void StgInstTaskHandlerOfSpotTest::onBooks(const StgInstInfoSPtr& stgInstInfo,
                                           const Books* books) {
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

void StgInstTaskHandlerOfSpotTest::onCandle(const StgInstInfoSPtr& stgInstInfo,
                                            const Candle* candle) {
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

void StgInstTaskHandlerOfSpotTest::onTickers(const StgInstInfoSPtr& stgInstInfo,
                                             const Tickers* tickers) {
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

void StgInstTaskHandlerOfSpotTest::onStgManualIntervention(
    const StgInstInfoSPtr& stgInstInfo, const CommonIPCData* commonIPCData) {
  LOG_I("On stg manual intervention. {}", commonIPCData->data_);
}

void StgInstTaskHandlerOfSpotTest::onOrderRet(
    const StgInstInfoSPtr& stgInstInfo, const OrderInfo* orderInfo) {
  if (orderInfo->orderStatus_ == OrderStatus::ConfirmedByExch) {
    getStgEng()->cancelOrder(orderInfo->orderId_);
  }
}

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

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond =
        "stgId=10000&stgInstId=1&marketCode=Binance&"
        "symbolType=Spot&symbolCode=ADA-USDT";
    const auto [statusCode, posInfoGroup] =
        posUpdate->queryPosInfoGroup(queryCond);
    if (statusCode == 0 && !posInfoGroup->empty()) {
      LOG_I("Query posInfoGroup of {}. [size = {}] [{}]", queryCond,
            posInfoGroup->size(), (*posInfoGroup)[0]->toStr());
    } else {
      LOG_W("Query posInfoGroup of {} failed. [{}]", queryCond);
    }
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond = "stgId=10000";
    const auto [statusCode, pnl] =
        posUpdate->queryPnl(queryCond, QuoteCurrencyForCalc("USDT"));
    if (statusCode == 0 && pnl) {
      getStgEng()->saveToDB(pnl);
      LOG_I("Query pnl of [{}]", pnl->toStr());
    } else {
      LOG_W("Query pnl of {} failed. [{} - {}]", queryCond, statusCode,
            GetStatusMsg(statusCode));
    }
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond = "stgId=10000&stgInstId=1";
    const auto [statusCode, pnl] =
        posUpdate->queryPnl(queryCond, QuoteCurrencyForCalc("USDT"));
    if (statusCode == 0 && pnl) {
      getStgEng()->saveToDB(pnl);
      LOG_I("Query pnl of [{}]", pnl->toStr());
    } else {
      LOG_W("Query pnl of {} failed. [{} - {}]", queryCond, statusCode,
            GetStatusMsg(statusCode));
    }
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond = "stgId=10000&stgInstId=2";
    const auto [statusCode, pnl] =
        posUpdate->queryPnl(queryCond, QuoteCurrencyForCalc("USDT"));
    if (statusCode == 0 && pnl) {
      getStgEng()->saveToDB(pnl);
      LOG_I("Query pnl of [{}]", pnl->toStr());
    } else {
      LOG_W("Query pnl of {} failed. [{} - {}]", queryCond, statusCode,
            GetStatusMsg(statusCode));
    }
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond = "stgId=10000&stgInstId=2&symbolCode=DOT-USDT-Perp";
    const auto [statusCode, pnl] =
        posUpdate->queryPnl(queryCond, QuoteCurrencyForCalc("USDT"));
    if (statusCode == 0 && pnl) {
      getStgEng()->saveToDB(pnl);
      LOG_I("Query pnl of [{}]", pnl->toStr());
    } else {
      LOG_W("Query pnl of {} failed. [{} - {}]", queryCond, statusCode,
            GetStatusMsg(statusCode));
    }
  }

  if (StgInstIdOfTriggerSignal(stgInstInfo) == 1) {
    const auto queryCond = "stgId=10000&stgInstId=2&symbolCode=ETH-USDT-Perp";
    const auto [statusCode, pnl] =
        posUpdate->queryPnl(queryCond, QuoteCurrencyForCalc("USDT"));
    if (statusCode == 0 && pnl) {
      getStgEng()->saveToDB(pnl);
      LOG_I("Query pnl of [{}]", pnl->toStr());
    } else {
      LOG_W("Query pnl of {} failed. [{} - {}]", queryCond, statusCode,
            GetStatusMsg(statusCode));
    }
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
