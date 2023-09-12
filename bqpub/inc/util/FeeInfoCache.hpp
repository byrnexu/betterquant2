/*!
 * \file FeeInfoCache.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "db/DBEngDef.hpp"
#include "db/TBLFeeInfo.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

struct FeeInfo {
  TrdAcctId trdAcctId_;
  std::string side_;
  std::string posDirection_;
  std::string marketCode_;
  std::string symbolType_;
  std::string symbolCode_;

  FeeType feeType_;
  Decimal commission_;
  Decimal minCommission_;
  Decimal stampDuty_;
  Decimal minStampDuty_;
  Decimal transFee_;
  Decimal minTransFee_;
  std::uint64_t updateTime_;
};
using FeeInfoSPtr = std::shared_ptr<FeeInfo>;

using Identity2FeeInfo = absl::node_hash_map<std::uint64_t, FeeInfoSPtr>;
using Identity2FeeInfoSPtr = std::shared_ptr<Identity2FeeInfo>;

class FeeInfoCache;
using FeeInfoCacheSPtr = std::shared_ptr<FeeInfoCache>;

class FeeInfoCache {
 public:
  FeeInfoCache(const FeeInfoCache&) = delete;
  FeeInfoCache& operator=(const FeeInfoCache&) = delete;
  FeeInfoCache(const FeeInfoCache&&) = delete;
  FeeInfoCache& operator=(const FeeInfoCache&&) = delete;

  explicit FeeInfoCache(const db::DBEngSPtr& dbEng);

 public:
  int load(AcctId acctId = 0);
  int reload(AcctId acctId = 0) { return load(acctId); }

  FeeInfoSPtr getFeeInfo(const OrderInfo* orderInfo,
                         PosDirection posDirection) const;
  FeeInfoSPtr getFeeInfo(TrdAcctId trdAcctId, const std::string& side,
                         const std::string& posDirection,
                         const std::string& marketCode,
                         const std::string& symbolType,
                         const std::string& symbolCode) const;

  Decimal calcFee(const OrderInfo* orderInfo, PosDirection posDirection,
                  Decimal dealAmt) const;

 private:
  db::DBEngSPtr dbEng_;

  Identity2FeeInfoSPtr identity2FeeInfo_{nullptr};
  mutable std::ext::spin_mutex mtxIdentity2FeeInfo_;

  std::uint64_t updateTime_{0};
};

}  // namespace bq
