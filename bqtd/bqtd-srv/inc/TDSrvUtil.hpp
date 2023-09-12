/*!
 * \file TDSrvUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/14
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
struct FlowCtrlRule;
using FlowCtrlRuleSPtr = std::shared_ptr<FlowCtrlRule>;
}  // namespace bq

namespace bq {

std::string MakeTopicDataOfTriggerRiskCtrl(const std::string& rule,
                                           const OrderInfoSPtr& orderInfo);

}
