/*!
 * \file FlowCtrlOnStepOnOrder.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#include "FlowCtrlOnStepOnOrder.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlUtil.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "def/DataStruOfTD.hpp"

namespace bq::td::srv {

std::tuple<int, std::string> FlowCtrlOnStepOnOrder::doHandleFlowCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  const auto orderAmt = orderInfo->orderSize_ * orderInfo->orderPrice_;

  //! 单笔下单数量
  if (plugin_->getFlowCtrlTargetState()->orderSizeEachTime_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderSizeEachTime,
        UpdateStgOfLimitValue::CompareAndUpdate, orderInfo->orderSize_);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计下单数量
  if (plugin_->getFlowCtrlTargetState()->orderSizeTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderSizeTotal,
        UpdateStgOfLimitValue::CompareAndUpdate, orderInfo->orderSize_);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 单笔下单金额
  if (plugin_->getFlowCtrlTargetState()->orderAmtEachTime_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderAmtEachTime,
        UpdateStgOfLimitValue::CompareAndUpdate, orderAmt);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计下单金额
  if (plugin_->getFlowCtrlTargetState()->orderAmtTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderAmtTotal,
        UpdateStgOfLimitValue::CompareAndUpdate, orderAmt);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计下单笔数
  if (plugin_->getFlowCtrlTargetState()->orderTimesTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderTimesTotal,
        UpdateStgOfLimitValue::CompareAndUpdate, 1);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 单位时间下单笔数
  if (plugin_->getFlowCtrlTargetState()->orderTimesWithinTime_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OrderTimesWithinTime,
        UpdateStgOfLimitValue::CompareAndUpdate, 1);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计被拒单笔数
  if (plugin_->getFlowCtrlTargetState()->rejectOrderTimesTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::RejectOrderTimesTotal,
        UpdateStgOfLimitValue::Compare);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 单位时间被拒单笔数
  if (plugin_->getFlowCtrlTargetState()->rejectOrderTimesWithinTime_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::RejectOrderTimesWithinTime,
        UpdateStgOfLimitValue::Compare);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计成交数量
  if (plugin_->getFlowCtrlTargetState()->holdVolTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::HoldVolTotal,
        UpdateStgOfLimitValue::CompareAndUpdate, orderInfo->orderSize_);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计成交金额
  if (plugin_->getFlowCtrlTargetState()->holdAmtTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::HoldAmtTotal,
        UpdateStgOfLimitValue::CompareAndUpdate,
        orderInfo->orderSize_ * orderInfo->orderPrice_);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  //! 累计成交金额
  if (plugin_->getFlowCtrlTargetState()->openTDayTotal_) {
    const auto [statusCode, details] = handleFlowCtrlForTarget(
        orderInfo, combNo, threadNo, FlowCtrlTarget::OpenTDayTotal,
        UpdateStgOfLimitValue::CompareAndUpdate,
        orderInfo->orderSize_ * orderInfo->orderPrice_);
    if (statusCode != 0) {
      return {statusCode, details};
    }
  }

  return {0, ""};
}

}  // namespace bq::td::srv
