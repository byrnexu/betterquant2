/*!
 * \file FlowCtrlOnStepOnOrderRet.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/12
 *
 * \brief
 */

#pragma once

#include "FlowCtrlOnStepBase.hpp"

namespace std::ext {
template <typename key_t, typename value_t>
class lru_cache;
template <typename key_t, typename value_t>
using lru_cache_sptr = std::shared_ptr<lru_cache<key_t, value_t>>;
}  // namespace std::ext

namespace bq::td::srv {

class FlowCtrlOnStepOnOrderRet : public FlowCtrlOnStepBase {
 public:
  FlowCtrlOnStepOnOrderRet(const FlowCtrlOnStepOnOrderRet&) = delete;
  FlowCtrlOnStepOnOrderRet& operator=(const FlowCtrlOnStepOnOrderRet&) = delete;
  FlowCtrlOnStepOnOrderRet(const FlowCtrlOnStepOnOrderRet&&) = delete;
  FlowCtrlOnStepOnOrderRet& operator=(const FlowCtrlOnStepOnOrderRet&&) =
      delete;

  FlowCtrlOnStepOnOrderRet(TDSrvRiskPluginFlowCtrlPlus* plugin);

 private:
  bool checkIfProcRepeated(const OrderInfoSPtr& orderInfo,
                           FlowCtrlTarget target);

 private:
  std::tuple<int, std::string> doHandleFlowCtrl(const OrderInfoSPtr& orderInfo,
                                                std::uint32_t combNo,
                                                std::uint32_t threadNo) final;

  std::ext::lru_cache_sptr<std::string, OrderInfoSPtr> orderInfoCache_{nullptr};
};

using FlowCtrlOnStepOnOrderRetSPtr = std::shared_ptr<FlowCtrlOnStepOnOrderRet>;

}  // namespace bq::td::srv
