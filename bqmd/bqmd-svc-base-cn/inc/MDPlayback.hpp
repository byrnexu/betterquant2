/*!
 * \file MDPlayback.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/26
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc {

struct MarketDataOfSim;
using MarketDataOfSimSPtr = std::shared_ptr<MarketDataOfSim>;

using Ts2MarketDataOfSimGroup = std::map<std::uint64_t, MarketDataOfSimSPtr>;
using Ts2MarketDataOfSimGroupSPtr = std::shared_ptr<Ts2MarketDataOfSimGroup>;

class MDSvcOfCN;

class MDPlayback;
using MDPlaybackSPtr = std::shared_ptr<MDPlayback>;

class MDPlayback {
 public:
  MDPlayback(const MDPlayback&) = delete;
  MDPlayback& operator=(const MDPlayback&) = delete;
  MDPlayback(const MDPlayback&&) = delete;
  MDPlayback& operator=(const MDPlayback&&) = delete;

  explicit MDPlayback(MDSvcOfCN* mdSvc) : mdSvc_(mdSvc) {}

 public:
  void start();
  void stop();

 private:
  void playback();
  void notifySubscribersSimedMDWillBeSend();
  void playback(const Ts2MarketDataOfSimGroupSPtr& ts2MarketDataOfSimGroup);

 private:
  MDSvcOfCN* mdSvc_{nullptr};

  std::atomic_bool keepRunning_{true};
  std::unique_ptr<std::thread> threadPlayback_{nullptr};
  std::uint32_t playbackSpeed_{1};
};

}  // namespace bq::md::svc
