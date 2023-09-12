/*!
 * \file StgLoggerInfo.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/04
 *
 * \brief
 */

#include "util/StgLoggerInfo.hpp"

#include "db/DBE.hpp"
#include "db/TBLStgLoggerInfo.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Logger.hpp"
#include "util/LoggerUtil.hpp"
#include "util/String.hpp"

namespace bq {

void StgLoggerInfoInsertImpl(db::DBEngSPtr& dbEng, LogLevel logLevel,
                             const StgInstInfoSPtr& stgInstInfo,
                             const std::string& msg) {
  if (!IsValidJsonStrVal(msg)) {
    LOG_W(
        "Insert log to db failed "
        "because of invalid str value in logdata. [{}]",
        msg);
    return;
  }

  // clang-format off
  const auto sql = 
      fmt::format("call uspStgLoggerInfoCreate"
        "("
        " {} ,"  // userId
        " {} ,"  // stgId
        " {} ,"  // stgInstId
        "'{}',"  // level
        "'{}',"  // data
        "'{}' "  // createTime
        "); ",
        stgInstInfo->userId_,
        stgInstInfo->stgId_,
        stgInstInfo->stgInstId_,
        magic_enum::enum_name(logLevel),
        msg,
        ConvertTsToDBTime(GetTotalUSSince1970())
  );
  // clang-format on

  const auto identity = GET_RAND_STR();
  LOG_D("Try to exec sql {}.", sql);
  const auto [ret, execRet] = dbEng->syncExec(identity, sql);
  if (ret != 0) {
    LOG_W("Exec sql failed. [{}] [{}]", sql, execRet);
    return;
  }
}

void StgLoggerInfoInsert(db::DBEngSPtr& dbEng, LogLevel logLevel,
                         const StgInstInfoSPtr& stgInstInfo,
                         const std::string& logData) {
  if (!boost::contains(logData, "\n")) {
    StgLoggerInfoInsertImpl(dbEng, logLevel, stgInstInfo, logData);
  } else {
    std::vector<std::string> msgGroup;
    boost::split(msgGroup, logData, boost::is_any_of("\n"),
                 boost::token_compress_on);
    for (const auto& msg : msgGroup) {
      StgLoggerInfoInsertImpl(dbEng, logLevel, stgInstInfo, msg);
    }
  }
}

}  // namespace bq
