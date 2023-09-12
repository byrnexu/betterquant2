/*!
 * \file PosSnapshotImpl.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/Def.hpp"
#include "def/Pnl.hpp"
#include "def/PosInfo.hpp"

namespace bq {

struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;

class PosSnapshotImpl;
using PosSnapshotImplSPtr = std::shared_ptr<PosSnapshotImpl>;

class PosSnapshotImpl {
  // add stgGrpId
  inline static std::map<std::string, int> FieldName2NoInPosInfo{
      {"productGrpId", 0}, {"productId", 1},   {"userId", 2},
      {"acctGrpId", 3},    {"acctId", 4},      {"trdAcctId", 5},
      {"stgGrpId", 6},     {"stgId", 7},       {"stgInstId", 8},
      {"algoId", 9},       {"marketCode", 10}, {"symbolType", 11},
      {"symbolCode", 11},  {"side", 13},       {"posSide", 14},
      {"parValue", 15},    {"feeCurrency", 16}};

 public:
  PosSnapshotImpl(const PosSnapshotImpl&) = delete;
  PosSnapshotImpl& operator=(const PosSnapshotImpl&) = delete;
  PosSnapshotImpl(const PosSnapshotImpl&&) = delete;
  PosSnapshotImpl& operator=(const PosSnapshotImpl&&) = delete;

  explicit PosSnapshotImpl(
      const std::map<std::string, PosInfoSPtr>& posInfoDetail,
      const MarketDataCacheSPtr& marketDataCache);

 public:
  const std::map<std::string, PosInfoSPtr>& getPosInfoDetail() const;

  std::tuple<int, PnlSPtr> queryPnl(
      const std::string& queryCond, const std::string& quoteCurrencyForCalc,
      const std::string& quoteCurrencyForConv,
      const std::string& origQuoteCurrencyOfUBasedContract = "USDT");

  std::tuple<int, Key2PnlGroupSPtr> queryPnlGroupBy(
      const std::string& groupCond, const std::string& quoteCurrencyForCalc,
      const std::string& quoteCurrencyForConv,
      const std::string& origQuoteCurrencyOfUBasedContract = "USDT");

 public:
  std::tuple<int, PosInfoGroupSPtr> queryPosInfoGroup(
      const std::string& queryCond);

  std::tuple<int, Key2PosInfoBundleSPtr> queryPosInfoGroupBy(
      const std::string& groupCond);

 private:
  std::map<std::string, PosInfoSPtr> posInfoDetail_;
  MarketDataCacheSPtr marketDataCache_{nullptr};

  std::map<std::string, Key2PnlGroupSPtr> cond2Key2PnlGroup_;
  std::map<std::string, Key2PosInfoBundleSPtr> cond2Key2PosInfoBundle_;
};

}  // namespace bq
