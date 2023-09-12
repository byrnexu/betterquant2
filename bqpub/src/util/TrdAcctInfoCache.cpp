/*!
 * \file TrdAcctInfoCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/TrdAcctInfoCache.hpp"

#include "db/DBE.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLTrdAcctInfo.hpp"
#include "def/BQConst.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"

namespace bq {

TrdAcctInfoCache::TrdAcctInfoCache(const db::DBEngSPtr& dbEng)
    : dbEng_(dbEng),
      trdAcctId2TrdAcctInfo_(std::make_shared<TrdAcctId2TrdAcctInfo>()) {}

int TrdAcctInfoCache::load() {
  auto sql = fmt::format("SELECT * FROM {}; ", TBLTrdAcctInfo::TableName);

  auto [retOfExec, tblRecSet] =
      db::TBLRecSetMaker<TBLTrdAcctInfo>::ExecSql(dbEng_, sql);
  if (retOfExec != 0) {
    LOG_W("Query trd acct info failed.");
    return retOfExec;
  }

  std::uint64_t lastUpdateTimeInDB = 0;
  auto trdAcctId2TrdAcctInfo = std::make_shared<TrdAcctId2TrdAcctInfo>();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    auto updateTime = ConvertDBTimeToTS(rec->updateTime);
    if (updateTime > lastUpdateTimeInDB) {
      lastUpdateTimeInDB = updateTime;
    }
    trdAcctId2TrdAcctInfo->emplace(rec->trdAcctId, rec);
  }
  LOG_D("Load trd acct info. [size = {}]", trdAcctId2TrdAcctInfo_->size());

  if (lastUpdateTimeInDB > lastUpdateTime_) {
    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxTrdAcctId2TrdAcctInfo_);
      trdAcctId2TrdAcctInfo_ = trdAcctId2TrdAcctInfo;
    }
    lastUpdateTime_ = lastUpdateTimeInDB;
  }

  return 0;
}

std::tuple<int, AcctId> TrdAcctInfoCache::getAcctId(TrdAcctId trdAcctId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTrdAcctId2TrdAcctInfo_);
    const auto iter = trdAcctId2TrdAcctInfo_->find(trdAcctId);
    if (iter != std::end(*trdAcctId2TrdAcctInfo_)) {
      return {0, iter->second->acctId};
    }
  }
  return {SCODE_DB_CAN_NOT_FIND_ACCT_ID, 0};
}

}  // namespace bq
