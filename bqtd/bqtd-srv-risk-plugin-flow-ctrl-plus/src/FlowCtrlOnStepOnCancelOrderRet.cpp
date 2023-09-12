/*!
 * \file FlowCtrlOnStepOnCancelOrderRet.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#include "FlowCtrlOnStepOnCancelOrderRet.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlUtil.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "def/DataStruOfTD.hpp"

namespace bq::td::srv {

std::tuple<int, std::string> FlowCtrlOnStepOnCancelOrderRet::doHandleFlowCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  return {0, ""};
}

}  // namespace bq::td::srv
