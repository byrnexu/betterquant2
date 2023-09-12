/*!
 * \file ExternalStatusCodeCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/ExternalStatusCodeCache.hpp"

#include "db/DBE.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"

namespace bq::td {

ExternalStatusCodeCache::ExternalStatusCodeCache(const db::DBEngSPtr& dbEng)
    : dbEng_(dbEng),
      identity2ExternalStatusCode_(
          std::make_shared<Identity2ExternalStatusCode>()) {}

int ExternalStatusCodeCache::load(const std::string& apiName, UserId userId) {
  auto sql = fmt::format("SELECT * FROM {} WHERE 1=1 ",
                         TBLExternalStatusCode::TableName);

  if (!apiName.empty()) {
    sql = sql + fmt::format("AND `apiName` = '{}' ", apiName);
  }
  if (userId != 0) {
    sql = sql + fmt::format("AND `userId` = '{}' ", userId);
  }

  auto [retOfExec, tblRecSet] =
      db::TBLRecSetMaker<TBLExternalStatusCode>::ExecSql(dbEng_, sql);
  if (retOfExec != 0) {
    LOG_W("Query external statuscode failed.");
    return retOfExec;
  }

  auto identity2ExternalStatusCode =
      std::make_shared<Identity2ExternalStatusCode>();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    const auto identity = fmt::format("{}{}{}{}{}",                       //
                                      rec->apiName, SEP_OF_REC_IDENTITY,  //
                                      rec->userId, SEP_OF_REC_IDENTITY,   //
                                      rec->externalStatusCode);
    const auto hash = XXH3_64bits(identity.data(), identity.size());
    identity2ExternalStatusCode->emplace(hash, rec);
  }
  LOG_D("Load external statuscode. [size = {}]",
        identity2ExternalStatusCode->size());

  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxIdentity2ExternalStatusCode_);
    identity2ExternalStatusCode_ = identity2ExternalStatusCode;
  }

  return 0;
}

int ExternalStatusCodeCache::getAndSetStatusCodeIfNotExists(
    const std::string& apiName, UserId userId,
    const std::string& externalStatusCode, const std::string& externalStatusMsg,
    int defaultValue) {
  const auto identity = fmt::format("{}{}{}{}{}",                  //
                                    apiName, SEP_OF_REC_IDENTITY,  //
                                    userId, SEP_OF_REC_IDENTITY,   //
                                    externalStatusCode);
  const auto hash = XXH3_64bits(identity.data(), identity.size());
  std::shared_ptr<db::externalStatusCode::Record> rec;
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxIdentity2ExternalStatusCode_);
    const auto iter = identity2ExternalStatusCode_->find(hash);
    if (iter != std::end(*identity2ExternalStatusCode_)) {
      return iter->second->statusCode;
    } else {
      rec = std::make_shared<db::externalStatusCode::Record>();
      rec->apiName = apiName;
      rec->userId = userId;
      rec->externalStatusCode = externalStatusCode;
      rec->externalStatusMsg = externalStatusMsg;
      //! 预设为默认为0，如果-1的话，下次同类型状态码的订单就被完结了
      rec->statusCode = defaultValue;
      identity2ExternalStatusCode_->emplace(hash, rec);
    }
  }

  //!
  //! 如果返回-1，那么回报都会变成废单，进入完结状态，即使给新的externalStatusCode
  //! 设置了对应的statusCode此订单也无法继续处理，所以ExecRecInsert失败不返回-1。
  //!
  const auto tblRec = std::make_shared<db::TBLRec<TBLExternalStatusCode>>(rec);
  const auto ret = ExecRecInsert(dbEng_, tblRec);
  if (ret == 0) {
    LOG_I("Sync external statuscode to db success. {}", identity);
  } else {
    LOG_W("Sync external statuscode to db failed. {}", identity);
  }
  return defaultValue;
}

}  // namespace bq::td
