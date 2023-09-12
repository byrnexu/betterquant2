/*!
 * \file MDPlayback.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/26
 *
 * \brief
 */

#include "MDPlayback.hpp"

#include "Config.hpp"
#include "MDCache.hpp"
#include "MDSvcOfCN.hpp"
#include "RawMDHandler.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "def/Def.hpp"
#include "def/RawMD.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"

namespace bq::md::svc {

void MDPlayback::start() {
  keepRunning_.store(true);
  threadPlayback_ = std::make_unique<std::thread>([this]() { playback(); });
}

void MDPlayback::stop() {
  keepRunning_.store(false);
  if (threadPlayback_->joinable()) {
    threadPlayback_->join();
  }
}

void MDPlayback::playback() {
  const auto intervalBetweenCacheCheck =
      CONFIG["simedMode"]["milliSecIntervalBetweenCacheCheck"]
          .as<std::uint32_t>(1);

  while (keepRunning_.load()) {
    const auto ts2MarketDataOfSimGroup = mdSvc_->getMDCache()->pop();
    if (ts2MarketDataOfSimGroup == nullptr) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(intervalBetweenCacheCheck));
    } else {
      playback(ts2MarketDataOfSimGroup);
    }
  }
}

void MDPlayback::playback(
    const Ts2MarketDataOfSimGroupSPtr& ts2MarketDataOfSimGroup) {
  if (ts2MarketDataOfSimGroup->empty()) return;

  auto playbackSpeed = CONFIG["simedMode"]["playbackSpeed"].as<double>(1);
  if (playbackSpeed == 0) playbackSpeed = UINT32_MAX;

  LOG_I("Begin to playback {} num of market data between {} - {}",
        ts2MarketDataOfSimGroup->size(),
        ConvertTsToPtime(std::begin(*ts2MarketDataOfSimGroup)->first),
        ConvertTsToPtime(std::rbegin(*ts2MarketDataOfSimGroup)->first));

  for (const auto& rec : *ts2MarketDataOfSimGroup) {
    if (keepRunning_.load() == false) break;
    const auto& marketDataOfSim = rec.second;
    switch (marketDataOfSim->mdType_) {
      case MDType::Trades: {
        auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
            MsgType::Trades, marketDataOfSim->data_.c_str(),
            marketDataOfSim->data_.size(), true);
        mdSvc_->getRawMDHandler()->dispatch(rawMD);
      } break;

      case MDType::Orders: {
        auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
            MsgType::Orders, marketDataOfSim->data_.c_str(),
            marketDataOfSim->data_.size(), true);
        mdSvc_->getRawMDHandler()->dispatch(rawMD);
      } break;

      case MDType::Books: {
        auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
            MsgType::Books, marketDataOfSim->data_.c_str(),
            marketDataOfSim->data_.size(), true);
        mdSvc_->getRawMDHandler()->dispatch(rawMD);
      } break;

      case MDType::Tickers: {
        auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
            MsgType::Tickers, marketDataOfSim->data_.c_str(),
            marketDataOfSim->data_.size(), true);
        mdSvc_->getRawMDHandler()->dispatch(rawMD);
      } break;

      case MDType::Candle: {
        auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
            MsgType::Candle, marketDataOfSim->data_.c_str(),
            marketDataOfSim->data_.size(), true);
        mdSvc_->getRawMDHandler()->dispatch(rawMD);
      } break;

      default:
        assert(1 == 2 && "Entered an impossible code segment");
        break;
    }
  }
}

}  // namespace bq::md::svc
