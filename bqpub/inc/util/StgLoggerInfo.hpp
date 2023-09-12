/*!
 * \file StgLoggerInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/04
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/Util.hpp"

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq {

enum class LogLevel;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

void StgLoggerInfoInsert(db::DBEngSPtr& dbEng, LogLevel logLevel,
                         const StgInstInfoSPtr& stgInstInfo,
                         const std::string& logData);

}  // namespace bq
