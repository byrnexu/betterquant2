/*!
 * \file SysParams.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq {

void UpdateSysParams(const db::DBEngSPtr& dbEng, const std::string& paramName,
                     const std::string& paramType,
                     const std::string& paramValue);

}
