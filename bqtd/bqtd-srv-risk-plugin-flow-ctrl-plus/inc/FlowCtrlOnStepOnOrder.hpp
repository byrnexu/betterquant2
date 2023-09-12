/*!
 * \file FlowCtrlOnStepOnOrder.hpp
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

class FlowCtrlOnStepOnOrder : public FlowCtrlOnStepBase {
 public:
  FlowCtrlOnStepOnOrder(const FlowCtrlOnStepOnOrder&) = delete;
  FlowCtrlOnStepOnOrder& operator=(const FlowCtrlOnStepOnOrder&) = delete;
  FlowCtrlOnStepOnOrder(const FlowCtrlOnStepOnOrder&&) = delete;
  FlowCtrlOnStepOnOrder& operator=(const FlowCtrlOnStepOnOrder&&) = delete;

  using FlowCtrlOnStepBase::FlowCtrlOnStepBase;

 private:
  std::tuple<int, std::string> doHandleFlowCtrl(const OrderInfoSPtr& orderInfo,
                                                std::uint32_t combNo,
                                                std::uint32_t threadNo) final;
};

using FlowCtrlOnStepOnOrderSPtr = std::shared_ptr<FlowCtrlOnStepOnOrder>;

}  // namespace bq::td::srv
