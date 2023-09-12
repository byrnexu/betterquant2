/*!
 * \file FlowCtrlDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionConst.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

//! vector of ["acctId", "marketCode"]
using ConditionFieldGroup = std::vector<std::string>;

//! map of {{"acctId":""}, {"marketCode":"SSE"}}
using ConditionTemplate = std::map<std::string, std::string>;

//! map of {{"acctId":"10000"}, {"marketCode":"SSE"}}
using ConditionValue = std::map<std::string, std::string>;

}  // namespace bq
