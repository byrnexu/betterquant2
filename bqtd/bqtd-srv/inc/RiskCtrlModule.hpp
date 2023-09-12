/*!
 * \file RiskCtrlModule.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/17
 *
 * \brief
 */

#pragma ocne

#include "TDSrvDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"

namespace bq {

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

class RiskCtrlStatusUpdaters;
using RiskCtrlStatusUpdatersSPtr = std::shared_ptr<RiskCtrlStatusUpdaters>;

class PnlMonitorRangeMgr;
using PnlMonitorRangeMgrSPtr = std::shared_ptr<PnlMonitorRangeMgr>;

class SelfTradeCtrlRangeMgr;
using SelfTradeCtrlRangeMgrSPtr = std::shared_ptr<SelfTradeCtrlRangeMgr>;

class TrdSymbolListMgr;
using TrdSymbolListMgrSPtr = std::shared_ptr<TrdSymbolListMgr>;

class FlowCtrlRuleMgr;
using FlowCtrlRuleMgrSPtr = std::shared_ptr<FlowCtrlRuleMgr>;

//! vector of ["acctId", "marketCode"]
using ConditionFieldGroup = std::vector<std::string>;
}  // namespace bq

namespace bq::td::srv {

//! step_用于创建的内存块的名称以及流控或者其他插件中筛选当前step使用的记录
struct RiskCtrlModuleConf {
  std::string step_;
  std::string fieldGroupUsedToGenHash_;
  std::string tdSrvTaskDispatcherParam_;
  std::string riskCtrlpluginPath_;
  std::uint32_t tdSrvRiskSegmentSize_;
};
using RiskCtrlModuleConfSPtr = std::shared_ptr<RiskCtrlModuleConf>;

class TDSrv;

class TDSrvRiskPluginMgr;
using TDSrvRiskPluginMgrSPtr = std::shared_ptr<TDSrvRiskPluginMgr>;

class TDGWTaskHandler;
using TDGWTaskHandlerSPtr = std::shared_ptr<TDGWTaskHandler>;

class StgEngTaskHandler;
using StgEngTaskHandlerSPtr = std::shared_ptr<StgEngTaskHandler>;

class RiskCtrlModule {
 public:
  RiskCtrlModule(const RiskCtrlModule&) = delete;
  RiskCtrlModule& operator=(const RiskCtrlModule&) = delete;
  RiskCtrlModule(const RiskCtrlModule&&) = delete;
  RiskCtrlModule& operator=(const RiskCtrlModule&&) = delete;

  explicit RiskCtrlModule(TDSrv* tdSrv, std::uint32_t no);

 public:
  int init();

 private:
  void initRiskCtrlModuleConf(const YAML::Node& node);

  void initSegmentGroup(std::uint32_t threadPoolSize);
  void initRiskCtrlStatusUpdatersGroup(std::uint32_t threadPoolSize);
  int initPnlMonitorRangeMgr(std::uint32_t threadPoolSize);
  int initSelfTradeCtrlRangeMgr(std::uint32_t threadPoolSize);
  int initTrdSymbolListMgr(std::uint32_t threadPoolSize);
  int initFlowCtrlRuleMgr(std::uint32_t threadPoolSize);
  void initPosMgr(std::uint32_t threadPoolSize);
  void initOrdMgr(std::uint32_t threadPoolSize);

  std::uint64_t getHashFromTask(const SHMIPCTaskSPtr& task,
                                const ConditionFieldGroup& conditionFieldGroup);

 public:
  int start();

 public:
  void stop();

 public:
  const RiskCtrlModuleConfSPtr& getTDSrvTaskDispatcherConf() {
    return tdSrvTaskDispatcherConf_;
  }

  TDSrvRiskPluginMgrSPtr& getTDSrvRiskPluginMgr() {
    return tdSrvRiskPluginMgr_;
  }

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block>&
  getTDSrvTaskDispatcher() {
    return tdSrvTaskDispatcher_;
  }

  std::vector<std::shared_ptr<bip::managed_shared_memory>>& getSegmentGroup() {
    return segmentGroup_;
  }

  const std::vector<RiskCtrlStatusUpdatersSPtr>&
  getRiskCtrlStatusUpdatersGroup() {
    return riskCtrlStatusUpdatersGroup_;
  }

  std::vector<PnlMonitorRangeMgrSPtr>& getPnlMonitorRangeMgrGroup() {
    return pnlMonitorRangeMgrGroup_;
  }

  std::vector<SelfTradeCtrlRangeMgrSPtr>& getSelfTradeCtrlRangeMgrGroup() {
    return selfTradeCtrlRangeMgrGroup_;
  }

  std::vector<TrdSymbolListMgrSPtr>& getTrdSymbolListMgrGroup() {
    return trdSymbolListMgrGroup_;
  }

  std::vector<FlowCtrlRuleMgrSPtr>& getFlowCtrlRuleMgrGroup() {
    return flowCtrlRuleMgrGroup_;
  }

  std::vector<TDPosMgrSPtr>& getPosMgrGroup() { return posMgrGroup_; }
  std::vector<TDOrdMgrSPtr>& getOrdMgrGroup() { return ordMgrGroup_; }

 private:
  TDSrv* tdSrv_{nullptr};
  std::uint32_t no_{0};

  ConditionFieldGroup conditionFieldGroup_;

  RiskCtrlModuleConfSPtr tdSrvTaskDispatcherConf_{nullptr};

  TDSrvRiskPluginMgrSPtr tdSrvRiskPluginMgr_{nullptr};

  TDGWTaskHandlerSPtr tdGWTaskHandler_{nullptr};
  StgEngTaskHandlerSPtr stgEngTaskHandler_{nullptr};

  std::vector<std::shared_ptr<bip::managed_shared_memory>> segmentGroup_;
  std::vector<RiskCtrlStatusUpdatersSPtr> riskCtrlStatusUpdatersGroup_;
  std::vector<PnlMonitorRangeMgrSPtr> pnlMonitorRangeMgrGroup_;
  std::vector<SelfTradeCtrlRangeMgrSPtr> selfTradeCtrlRangeMgrGroup_;
  std::vector<TrdSymbolListMgrSPtr> trdSymbolListMgrGroup_;
  std::vector<FlowCtrlRuleMgrSPtr> flowCtrlRuleMgrGroup_;
  std::vector<TDPosMgrSPtr> posMgrGroup_;
  std::vector<TDOrdMgrSPtr> ordMgrGroup_;

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> tdSrvTaskDispatcher_{
      nullptr};
};

}  // namespace bq::td::srv
