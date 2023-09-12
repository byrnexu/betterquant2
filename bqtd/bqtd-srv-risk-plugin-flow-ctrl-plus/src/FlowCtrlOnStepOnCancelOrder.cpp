/*!
 * \file FlowCtrlOnStepOnCancelOrder.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#include "FlowCtrlOnStepOnCancelOrder.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlUtil.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "def/DataStruOfTD.hpp"

namespace bq::td::srv {

std::tuple<int, std::string> FlowCtrlOnStepOnCancelOrder::doHandleFlowCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  //! 累计撤单次数
  if (plugin_->getFlowCtrlTargetState()->cancelOrderTimesTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::CancelOrderTimesTotal,
        UpdateStgOfLimitValue::CompareAndUpdate, 1);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 单位时间撤单次数
  if (plugin_->getFlowCtrlTargetState()->cancelOrderTimesWithinTime_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::CancelOrderTimesWithinTime,
        UpdateStgOfLimitValue::CompareAndUpdate, 1);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  return {0, ""};
}

}  // namespace bq::td::srv
