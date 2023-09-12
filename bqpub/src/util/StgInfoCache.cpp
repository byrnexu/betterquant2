/*!
 * \file StgInfoCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/StgInfoCache.hpp"

#include "db/DBE.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLStgInfo.hpp"
#include "def/BQConst.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"

namespace bq {

StgInfoCache::StgInfoCache(const db::DBEngSPtr& dbEng)
    : dbEng_(dbEng), stgId2StgInfo_(std::make_shared<StgId2StgInfo>()) {}

int StgInfoCache::load() {
  auto sql = fmt::format("SELECT * FROM {}; ", TBLStgInfo::TableName);

  auto [retOfExec, tblRecSet] =
      db::TBLRecSetMaker<TBLStgInfo>::ExecSql(dbEng_, sql);
  if (retOfExec != 0) {
    LOG_W("Query stg info failed.");
    return retOfExec;
  }

  std::uint64_t lastUpdateTimeInDB = 0;
  auto stgId2StgInfo = std::make_shared<StgId2StgInfo>();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    auto updateTime = ConvertDBTimeToTS(rec->updateTime);
    if (updateTime > lastUpdateTimeInDB) {
      lastUpdateTimeInDB = updateTime;
    }
    stgId2StgInfo->emplace(rec->stgId, rec);
  }
  LOG_D("Load stg info. [size = {}]", stgId2StgInfo_->size());

  if (lastUpdateTimeInDB > lastUpdateTime_) {
    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2StgInfo_);
      stgId2StgInfo_ = stgId2StgInfo;
    }
    lastUpdateTime_ = lastUpdateTimeInDB;
  }

  return 0;
}

StgGrpId StgInfoCache::getStgGrpId(StgId stgId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2StgInfo_);
    const auto iter = stgId2StgInfo_->find(stgId);
    if (iter != std::end(*stgId2StgInfo_)) {
      return iter->second->stgGrpId;
    }
  }
  return 0;
}

}  // namespace bq
