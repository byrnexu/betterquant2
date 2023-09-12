/*!
 * \file FlowCtrlOnStepOnCancelOrder.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#pragma once

#include "FlowCtrlOnStepBase.hpp"

namespace bq::td::srv {

class FlowCtrlOnStepOnCancelOrder : public FlowCtrlOnStepBase {
 public:
  FlowCtrlOnStepOnCancelOrder(const FlowCtrlOnStepOnCancelOrder&) = delete;
  FlowCtrlOnStepOnCancelOrder& operator=(const FlowCtrlOnStepOnCancelOrder&) =
      delete;
  FlowCtrlOnStepOnCancelOrder(const FlowCtrlOnStepOnCancelOrder&&) = delete;
  FlowCtrlOnStepOnCancelOrder& operator=(const FlowCtrlOnStepOnCancelOrder&&) =
      delete;

  using FlowCtrlOnStepBase::FlowCtrlOnStepBase;

 private:
  std::tuple<int, std::string> doHandleFlowCtrl(const OrderInfoSPtr& orderInfo,
                                                std::uint32_t combNo,
                                                std::uint32_t threadNo) final;
};

using FlowCtrlOnStepOnCancelOrderSPtr =
    std::shared_ptr<FlowCtrlOnStepOnCancelOrder>;

}  // namespace bq::td::srv
