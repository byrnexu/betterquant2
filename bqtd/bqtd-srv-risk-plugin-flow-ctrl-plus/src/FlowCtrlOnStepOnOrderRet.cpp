/*!
 * \file FlowCtrlOnStepOnOrderRet.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#include "FlowCtrlOnStepOnOrderRet.hpp"

#include "FlowCtrlConst.hpp"
#include "FlowCtrlUtil.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/LRUCache.hpp"
#include "util/Logger.hpp"

namespace bq::td::srv {

FlowCtrlOnStepOnOrderRet::FlowCtrlOnStepOnOrderRet(
    TDSrvRiskPluginFlowCtrlPlus* plugin)
    : FlowCtrlOnStepBase(plugin),
      orderInfoCache_(
          std::make_shared<std::ext::lru_cache<std::string, OrderInfoSPtr>>(
              16 * 1024)) {}

std::tuple<int, std::string> FlowCtrlOnStepOnOrderRet::doHandleFlowCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  //! 累计被拒单笔数
  if (plugin_->getFlowCtrlTargetState()->rejectOrderTimesTotal_) {
    //! orderInfo->statusCode_ 是负数，所以要注意区间的判断
    if (orderInfo->statusCode_ <= SCODE_EXTERNAL_SYS_ORDER_REJECTED_MIN &&
        orderInfo->statusCode_ > SCODE_EXTERNAL_SYS_ORDER_REJECTED_MAX) {
      //! 检查是否收到同一订单的拒单信息
      if (checkIfProcRepeated(orderInfo,
                              FlowCtrlTarget::RejectOrderTimesTotal)) {
        return {0, ""};
      }
      const auto [statusCode, details] = handleFlowCtrlForTarget(
          orderInfo, combNo, threadNo, FlowCtrlTarget::RejectOrderTimesTotal,
          UpdateStgOfLimitValue::Update, 1);
      if (statusCode != 0) {
        return {statusCode, details};
      }
    }
  }

  //! 单位时间被拒单笔数
  if (plugin_->getFlowCtrlTargetState()->rejectOrderTimesWithinTime_) {
    //! orderInfo->statusCode_ 是负数，所以要注意区间的判断
    if (orderInfo->statusCode_ <= SCODE_EXTERNAL_SYS_ORDER_REJECTED_MIN &&
        orderInfo->statusCode_ > SCODE_EXTERNAL_SYS_ORDER_REJECTED_MAX) {
      //! 检查是否收到同一订单的拒单信息
      if (checkIfProcRepeated(orderInfo,
                              FlowCtrlTarget::RejectOrderTimesWithinTime)) {
        return {0, ""};
      }
      const auto [statusCode, details] =
          handleFlowCtrlForTarget(orderInfo, combNo, threadNo,
                                  FlowCtrlTarget::RejectOrderTimesWithinTime,
                                  UpdateStgOfLimitValue::Update);
      if (statusCode != 0) {
        return {statusCode, details};
      }
    }
  }

  return {0, ""};
}

//! 查询订单状态线程和回报线程同时触发，可能会导致同一订单同样委托状态重复推送，
//! 目前由于查询未完结订单状态的功能做了滞后推送处理，所以这种情况出现概率不大
bool FlowCtrlOnStepOnOrderRet::checkIfProcRepeated(
    const OrderInfoSPtr& orderInfo, FlowCtrlTarget target) {
  const auto key =
      fmt::format("{} - {}", orderInfo->orderId_, ENUM_TO_STR(target));
  const auto ret = orderInfoCache_->get(key);
  if (ret.first == false) {
    orderInfoCache_->push(key, orderInfo);
    if (orderInfoCache_->size() % 1000 == 0) {
      L_I(plugin_->logger(), "Size of order info cache is {}",
          orderInfoCache_->size());
    }
    return false;
  }

  const auto preOrderInfo = ret.second;
  L_W(plugin_->logger(),
      "[{}] It was found that {} "
      "may be processed repeatedly. \npreOrderInfo: {} \n curOrderInfo: {}",
      plugin_->name(), key, preOrderInfo->toShortStr(),
      orderInfo->toShortStr());

  return true;
}

}  // namespace bq::td::srv
