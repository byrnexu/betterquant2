/*!
 * \file CacheOfDBRet.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/28
 *
 * \brief
 */

#include "CacheOfDBRet.hpp"

#include "Config.hpp"
#include "util/Datetime.hpp"
#include "util/GZip.hpp"
#include "util/Logger.hpp"

namespace bq {

int CacheOfDBRet::init() {
  for (std::size_t i = 0; i < CONFIG["cacheInfoOfDBRet"].size(); ++i) {
    const auto sql = CONFIG["cacheInfoOfDBRet"][i]["sql"].as<std::string>();
    const auto msOfValidTime =
        CONFIG["cacheInfoOfDBRet"][i]["msOfValidTime"].as<std::uint64_t>(0);
    const auto dbRetInfo = std::make_shared<DBRetInfo>(sql, msOfValidTime);
    {
      std::lock_guard<std::mutex> guard(mtxSql2DBRetInfo_);
      sql2DBRetInfo_.emplace(sql, dbRetInfo);
    }
  }
  return 0;
}

std::tuple<bool, bool, std::string> CacheOfDBRet::get(const std::string& sql) {
  bool needCacheDBRet = false;
  bool getDBRetFromCache = false;
  std::string dbRet = "";

  //! 获取当前时间
  const auto now = GetTotalMSSince1970();

  //! 获取缓存中的sql结果
  {
    std::lock_guard<std::mutex> guard(mtxSql2DBRetInfo_);
    const auto iter = sql2DBRetInfo_.find(sql);
    if (iter == std::end(sql2DBRetInfo_)) {
      needCacheDBRet = false;
      getDBRetFromCache = false;
      LOG_T("[CacheOfDBRet] Not cache db ret, query ret from db. [{}]", sql);
      return {needCacheDBRet, getDBRetFromCache, dbRet};
    }

    const auto& dbRetInfo = iter->second;
    if (now - dbRetInfo->cacheTime_ > dbRetInfo->msOfValidTime_) {
      //! 如果有缓存，但是是太老的结果，那么也返回从缓存获取结果失败
      needCacheDBRet = true;
      getDBRetFromCache = false;
      LOG_I(
          "[CacheOfDBRet] Cached db ret is timeout or not exists, "
          "query ret from db. [{}]",
          sql);
      return {needCacheDBRet, getDBRetFromCache, dbRet};
    } else {
      //! 以下是有缓存且在有效期内
      needCacheDBRet = false;
      getDBRetFromCache = true;
      dbRet = GZip::decomp(dbRetInfo->dbRet_);
      LOG_I("[CacheOfDBRet] Get db ret from cache. [{}]", sql);
      return {needCacheDBRet, getDBRetFromCache, ""};
    }
  }
}

void CacheOfDBRet::cache(const std::string& sql, const std::string& dbRet) {
  const auto getTotalDbRetLen = [](const Sql2DBRetInfo& sql2DBRetInfo) {
    double totalLen = 0;
    for (const auto& rec : sql2DBRetInfo) {
      totalLen += rec.second->dbRet_.length();
    }
    return totalLen;
  };

  {
    std::lock_guard<std::mutex> guard(mtxSql2DBRetInfo_);
    auto iter = sql2DBRetInfo_.find(sql);
    if (iter == std::end(sql2DBRetInfo_)) {
      LOG_W(
          "[CacheOfDBRet] Identify SQL results being cached "
          "that don't need to be cached. [{}]",
          sql);
      return;
    }

    auto& dbRetInfo = iter->second;
    dbRetInfo->cacheTime_ = GetTotalMSSince1970();
    dbRetInfo->dbRet_ = GZip::comp(dbRet);

    const auto totalDBRetLen = getTotalDbRetLen(sql2DBRetInfo_) / 1024 / 1024;
    LOG_I("[CacheOfDBRet] Cache db ret success. [num = {}, size = {}mb] [{}]",
          sql2DBRetInfo_.size(), totalDBRetLen, sql);
  }
}

}  // namespace bq
