/*!
 * \file TDSrvRiskPluginFlowCtrlPlus.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "TDSrvRiskPlugin.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

enum class FlowCtrlTarget : uint8_t;

struct FlowCtrlRule;
using FlowCtrlRuleSPtr = std::shared_ptr<FlowCtrlRule>;

struct FlowCtrlTargetState;
using FlowCtrlTargetStateSPtr = std::shared_ptr<FlowCtrlTargetState>;

struct LimitValueInSHM;
}  // namespace bq

namespace bq::td::srv {

class FlowCtrlOnStepOnCancelOrder;
using FlowCtrlOnStepOnCancelOrderSPtr =
    std::shared_ptr<FlowCtrlOnStepOnCancelOrder>;
class FlowCtrlOnStepOnCancelOrderRet;
using FlowCtrlOnStepOnCancelOrderRetSPtr =
    std::shared_ptr<FlowCtrlOnStepOnCancelOrderRet>;
class FlowCtrlOnStepOnOrder;
using FlowCtrlOnStepOnOrderSPtr = std::shared_ptr<FlowCtrlOnStepOnOrder>;
class FlowCtrlOnStepOnOrderRet;
using FlowCtrlOnStepOnOrderRetSPtr = std::shared_ptr<FlowCtrlOnStepOnOrderRet>;

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPluginFlowCtrlPlus
    : public TDSrvRiskPlugin {
 public:
  TDSrvRiskPluginFlowCtrlPlus(const TDSrvRiskPluginFlowCtrlPlus&) = delete;
  TDSrvRiskPluginFlowCtrlPlus& operator=(const TDSrvRiskPluginFlowCtrlPlus&) =
      delete;
  TDSrvRiskPluginFlowCtrlPlus(const TDSrvRiskPluginFlowCtrlPlus&&) = delete;
  TDSrvRiskPluginFlowCtrlPlus& operator=(const TDSrvRiskPluginFlowCtrlPlus&&) =
      delete;

  explicit TDSrvRiskPluginFlowCtrlPlus(TDSrv* tdSrv);

 public:
  FlowCtrlTargetStateSPtr getFlowCtrlTargetState() const {
    return flowCtrlTargetState_;
  }

 private:
  int doLoad() final;
  void initFlowCtrlTargetState();

 private:
  void doUnload() final;

 private:
  boost::dll::fs::path getLocation() const final;

 private:
  int doOnRiskCtrlConfChg(const std::string& step, const Doc& doc,
                          const std::string& conf, std::uint32_t combNo,
                          std::uint32_t threadNo) final;

  void updateTsQueOfLimitValueInSHM(const std::string& step, const Doc& doc,
                                    const std::string& conf,
                                    std::uint32_t combNo,
                                    std::uint32_t threadNo);

 public:
  LimitValueInSHM* openOrCtorLimitValue(
      std::uint32_t combNo, std::uint32_t threadNo,
      const FlowCtrlRuleSPtr& rule, const std::string& conditionValueInStrFmt,
      bool createTSQue = false);

 private:
  std::tuple<int, std::string> doOnOrder(const OrderInfoSPtr& orderInfo,
                                         std::uint32_t combNo,
                                         std::uint32_t threadNo) final;

  std::tuple<int, std::string> doOnCancelOrder(const OrderInfoSPtr& orderInfo,
                                               std::uint32_t combNo,
                                               std::uint32_t threadNo) final;

 private:
  std::tuple<int, std::string> doOnOrderRet(const OrderInfoSPtr& orderInfo,
                                            std::uint32_t combNo,
                                            std::uint32_t threadNo) final;

  std::tuple<int, std::string> doOnCancelOrderRet(
      const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
      std::uint32_t threadNo) final;

 private:
  FlowCtrlTargetStateSPtr flowCtrlTargetState_{nullptr};

  FlowCtrlOnStepOnOrderSPtr flowCtrlOnStepOnOrder_{nullptr};
  FlowCtrlOnStepOnCancelOrderSPtr flowCtrlOnStepOnCancelOrder_{nullptr};
  FlowCtrlOnStepOnOrderRetSPtr flowCtrlOnStepOnOrderRet_{nullptr};
  FlowCtrlOnStepOnCancelOrderRetSPtr flowCtrlOnStepOnCancelOrderRet_{nullptr};
};

}  // namespace bq::td::srv

inline bq::td::srv::TDSrvRiskPluginFlowCtrlPlus* Create(
    bq::td::srv::TDSrv* tdSrv) {
  return new bq::td::srv::TDSrvRiskPluginFlowCtrlPlus(tdSrv);
}

BOOST_DLL_ALIAS_SECTIONED(Create, CreatePlugin, PlugIn)
