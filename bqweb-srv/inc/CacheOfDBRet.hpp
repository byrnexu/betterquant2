/*!
 * \file CacheOfDBRet.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/28
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

struct DBRetInfo {
  DBRetInfo(const std::string& sql, std::uint64_t msOfValidTime)
      : sql_(sql), msOfValidTime_(msOfValidTime) {}
  const std::string sql_;
  const std::uint64_t msOfValidTime_;
  std::uint64_t cacheTime_{0};
  std::string dbRet_;
};
using DBRetInfoSPtr = std::shared_ptr<DBRetInfo>;
using Sql2DBRetInfo = std::map<std::string, DBRetInfoSPtr>;

class CacheOfDBRet {
 public:
  CacheOfDBRet(const CacheOfDBRet&) = delete;
  CacheOfDBRet& operator=(const CacheOfDBRet&) = delete;
  CacheOfDBRet(const CacheOfDBRet&&) = delete;
  CacheOfDBRet& operator=(const CacheOfDBRet&&) = delete;

  CacheOfDBRet() = default;

  int init();

  std::tuple<bool, bool, std::string> get(const std::string& sql);
  void cache(const std::string& sql, const std::string& dbRet);

 private:
  Sql2DBRetInfo sql2DBRetInfo_;
  mutable std::mutex mtxSql2DBRetInfo_;
};

}  // namespace bq
