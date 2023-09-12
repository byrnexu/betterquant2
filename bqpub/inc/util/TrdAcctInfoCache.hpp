/*!
 * \file TrdAcctInfoCache.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "db/DBEngDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::trdAcctInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::trdAcctInfo

namespace bq {

using TrdAcctId2TrdAcctInfo =
    absl::node_hash_map<AcctId, bq::db::trdAcctInfo::RecordSPtr>;
using TrdAcctId2TrdAcctInfoSPtr = std::shared_ptr<TrdAcctId2TrdAcctInfo>;

class TrdAcctInfoCache;
using TrdAcctInfoCacheSPtr = std::shared_ptr<TrdAcctInfoCache>;

class TrdAcctInfoCache {
 public:
  TrdAcctInfoCache(const TrdAcctInfoCache&) = delete;
  TrdAcctInfoCache& operator=(const TrdAcctInfoCache&) = delete;
  TrdAcctInfoCache(const TrdAcctInfoCache&&) = delete;
  TrdAcctInfoCache& operator=(const TrdAcctInfoCache&&) = delete;

  explicit TrdAcctInfoCache(const db::DBEngSPtr& dbEng);

  std::tuple<int, AcctId> getAcctId(TrdAcctId trdAcctId);

 public:
  int load();
  int reload() { return load(); }

 private:
  db::DBEngSPtr dbEng_;

  TrdAcctId2TrdAcctInfoSPtr trdAcctId2TrdAcctInfo_{nullptr};
  mutable std::ext::spin_mutex mtxTrdAcctId2TrdAcctInfo_;

  std::uint64_t lastUpdateTime_{0};
};

}  // namespace bq
