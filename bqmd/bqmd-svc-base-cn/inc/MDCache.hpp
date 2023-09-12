/*!
 * \file MDCache.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/27
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "util/Pch.hpp"

namespace bq {
struct MarketDataCond;
using MarketDataCondSPtr = std::shared_ptr<MarketDataCond>;
}  // namespace bq

namespace bq::md::svc {

struct MarketDataOfSim {
  std::uint64_t localTs_;
  MDType mdType_;
  std::string data_;
  std::uint64_t delay_;
};
using MarketDataOfSimSPtr = std::shared_ptr<MarketDataOfSim>;

using Ts2MarketDataOfSimGroup = std::map<std::uint64_t, MarketDataOfSimSPtr>;
using Ts2MarketDataOfSimGroupSPtr = std::shared_ptr<Ts2MarketDataOfSimGroup>;

class MDSvcOfCN;

class MDCache;
using MDCacheSPtr = std::shared_ptr<MDCache>;

class MDCache {
 public:
  MDCache(const MDCache&) = delete;
  MDCache& operator=(const MDCache&) = delete;
  MDCache(const MDCache&&) = delete;
  MDCache& operator=(const MDCache&&) = delete;

  explicit MDCache(MDSvcOfCN* mdSvc) : mdSvc_(mdSvc) {}

 public:
  int start();
  void stop();

 public:
  Ts2MarketDataOfSimGroupSPtr pop();

 private:
  void cacheHisMD();
  void cacheHisMDOf1Batch();

  Ts2MarketDataOfSimGroupSPtr makeMDCacheOfCurBatch(std::uint64_t startLocalTs,
                                                    std::uint64_t endLocalTs);

  void calcDelayBetweenAdjacentMD(
      Ts2MarketDataOfSimGroupSPtr& mdCacheOfCurBatch);

 private:
  MDSvcOfCN* mdSvc_{nullptr};

  std::atomic_bool keepRunning_{false};
  std::unique_ptr<std::thread> threadCacheMDHis_{nullptr};

  std::deque<Ts2MarketDataOfSimGroupSPtr> mdCache_;
  mutable std::mutex mtxMDCache_;

  std::uint32_t secOfCacheMD_;

  std::uint64_t exchTsStart_;
  std::uint64_t exchTsEnd_;

  std::uint64_t tsStart_;
  std::uint64_t tsEnd_;

  std::uint64_t tsStartOfCurCache_;
};

}  // namespace bq::md::svc
