/*!
 * \file StgInfoCache.hpp
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

namespace bq::db::stgInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::stgInfo

namespace bq {

using StgId2StgInfo = absl::node_hash_map<StgId, bq::db::stgInfo::RecordSPtr>;
using StgId2StgInfoSPtr = std::shared_ptr<StgId2StgInfo>;

class StgInfoCache;
using StgInfoCacheSPtr = std::shared_ptr<StgInfoCache>;

class StgInfoCache {
 public:
  StgInfoCache(const StgInfoCache&) = delete;
  StgInfoCache& operator=(const StgInfoCache&) = delete;
  StgInfoCache(const StgInfoCache&&) = delete;
  StgInfoCache& operator=(const StgInfoCache&&) = delete;

  explicit StgInfoCache(const db::DBEngSPtr& dbEng);

  StgGrpId getStgGrpId(StgId stgId);

 public:
  int load();
  int reload() { return load(); }

 private:
  db::DBEngSPtr dbEng_;

  StgId2StgInfoSPtr stgId2StgInfo_{nullptr};
  mutable std::ext::spin_mutex mtxStgId2StgInfo_;

  std::uint64_t lastUpdateTime_{0};
};

}  // namespace bq
