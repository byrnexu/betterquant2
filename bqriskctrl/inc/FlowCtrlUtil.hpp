/*!
 * \file FlowCtrlUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "FlowCtrlConst.hpp"
#include "FlowCtrlDef.hpp"
#include "util/Pch.hpp"

namespace bq {

bool LimitTargetsAreStateful(FlowCtrlTarget target);

std::tuple<int, std::string, FlowCtrlLimitType> GetLimitType(
    FlowCtrlTarget target);

//! 10000 or 100/1000ms -> LimitValue
std::tuple<int, std::string, LimitValue> MakeLimitValue(
    FlowCtrlLimitType limitType, const std::string& LimitValueInStrFmt);

std::string MakeRuleInfo(int statusCode);

}  // namespace bq
