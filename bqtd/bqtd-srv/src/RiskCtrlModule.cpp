/*!
 * \file RiskCtrlModule.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/17
 *
 * \brief
 */

#include "RiskCtrlModule.hpp"

#include "AssetsMgr.hpp"
#include "Config.hpp"
#include "FlowCtrlRuleMgr.hpp"
#include "OrdMgr.hpp"
#include "PnlMonitorRangeMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "SHMIPC.hpp"
#include "SelfTradeCtrlRangeMgr.hpp"
#include "StgEngTaskHandler.hpp"
#include "TDGWTaskHandler.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginMgr.hpp"
#include "TrdSymbolListMgr.hpp"
#include "db/DBEngConst.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "db/TBLOrderInfo.hpp"
#include "db/TBLPosInfo.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfAssets.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/PosInfo.hpp"
#include "util/BQUtil.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/StdExt.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

RiskCtrlModule::RiskCtrlModule(TDSrv* tdSrv, std::uint32_t no)
    : tdSrv_(tdSrv),
      no_(no),
      tdSrvTaskDispatcherConf_(std::make_shared<RiskCtrlModuleConf>()),
      tdSrvRiskPluginMgr_(std::make_shared<TDSrvRiskPluginMgr>(tdSrv_, no_)),
      tdGWTaskHandler_(std::make_shared<TDGWTaskHandler>(tdSrv_, no_)),
      stgEngTaskHandler_(std::make_shared<StgEngTaskHandler>(tdSrv_, no_)) {}

int RiskCtrlModule::init() {
  initRiskCtrlModuleConf(CONFIG["riskCtrlModuleComb"][no_]);

  //! 初始化 taskDispatcherParam
  const auto tdSrvTaskDispatcherParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               tdSrvTaskDispatcherConf_->tdSrvTaskDispatcherParam_);
  const auto [ret, tdSrvTaskDispatcherParam] =
      MakeTaskDispatcherParam(tdSrvTaskDispatcherParamInStrFmt);
  if (ret != 0) {
    LOG_E("[{}] Init taskdispatcher failed. {}", no_,
          tdSrvTaskDispatcherParamInStrFmt);
    return ret;
  }

  //! 获取线程池大小也就是分区数量
  const auto threadPoolSize =
      tdSrvTaskDispatcherParam->taskSpecificThreadPoolSize_;

  //! 初始化每个分区共享内存块数组
  initSegmentGroup(threadPoolSize);

  //! 初始化每个分区的风控更新函数队列
  initRiskCtrlStatusUpdatersGroup(threadPoolSize);

  int statusCode = 0;
  std::string statusMsg;

  //! 初始化pnl监控范围
  statusCode = initPnlMonitorRangeMgr(threadPoolSize);
  if (statusCode != 0) {
    return statusCode;
  }

  //! 初始化自成交控制范围
  statusCode = initSelfTradeCtrlRangeMgr(threadPoolSize);
  if (statusCode != 0) {
    return statusCode;
  }

  //! 初始化每个分区的黑白名单
  statusCode = initTrdSymbolListMgr(threadPoolSize);
  if (statusCode != 0) {
    return statusCode;
  }

  //! 初始化每个分区流控指标数组
  statusCode = initFlowCtrlRuleMgr(threadPoolSize);
  if (statusCode != 0) {
    return statusCode;
  }

  //! 初始化仓位管理服务数组
  initPosMgr(threadPoolSize);

  //! 初始化订单管理服务数组
  initOrdMgr(threadPoolSize);

  //! 获取当前分区名和分区字段数组
  const auto fieldGroupUsedToGenHash =
      tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_;
  std::tie(statusCode, statusMsg, conditionFieldGroup_) =
      MakeConditionFieldGroup(fieldGroupUsedToGenHash);
  if (statusCode != 0) {
    LOG_W("[{}] init td srv task dispatcher failed. [{}]", no_, statusMsg);
    return statusCode;
  }

  //! 获取带有条件数组的hash的异步任务
  const auto makeAsyncTask = [this](const auto& task) {
    const auto hash = getHashFromTask(task, conditionFieldGroup_);
    return std::make_tuple(0, std::make_shared<SHMIPCAsyncTask>(task, hash));
  };

  const auto getThreadForAsyncTask = [](const auto& asyncTask,
                                        auto taskSpecificThreadPoolSize) {
    const auto hash = std::any_cast<std::uint64_t>(asyncTask->arg_);
    const auto threadNo = hash % taskSpecificThreadPoolSize;
    return threadNo;
  };

  const auto handleAsyncTask = [this](auto& asyncTask) {
    const auto shmHeader =
        static_cast<const SHMHeader*>(asyncTask->task_->data_);
    switch (shmHeader->msgId_) {
      case MSG_ID_ON_RISK_CTRL_CONF_CHG:
        stgEngTaskHandler_->handleAsyncTask(asyncTask);
        break;
      case MSG_ID_ON_ORDER:
        stgEngTaskHandler_->handleAsyncTask(asyncTask);
        break;
      case MSG_ID_ON_CANCEL_ORDER:
        stgEngTaskHandler_->handleAsyncTask(asyncTask);
        break;
      case MSG_ID_ON_STG_REG:
        stgEngTaskHandler_->handleAsyncTask(asyncTask);
        break;

      case MSG_ID_ON_ORDER_RET:
        tdGWTaskHandler_->handleAsyncTask(asyncTask);
        break;
      case MSG_ID_ON_CANCEL_ORDER_RET:
        tdGWTaskHandler_->handleAsyncTask(asyncTask);
        break;
      case MSG_ID_ON_TDGW_REG:
        tdGWTaskHandler_->handleAsyncTask(asyncTask);
        break;

      default:
        LOG_W("[{}] Unable to process msgId {}.", no_, shmHeader->msgId_);
        break;
    }
  };

  tdSrvTaskDispatcher_ = std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
      tdSrvTaskDispatcherParam, makeAsyncTask, getThreadForAsyncTask,
      handleAsyncTask);
  tdSrvTaskDispatcher_->init();

  //! 构造的时候会用shm创建一些数据结构，因此放在后面
  tdSrvRiskPluginMgr_->load();

  return ret;
}

void RiskCtrlModule::initRiskCtrlModuleConf(const YAML::Node& node) {
  tdSrvTaskDispatcherConf_->step_ = node["step"].as<std::string>();
  tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_ =
      node["fieldGroupUsedToGenHash"].as<std::string>();
  tdSrvTaskDispatcherConf_->tdSrvTaskDispatcherParam_ =
      node["tdSrvTaskDispatcherParam"].as<std::string>();
  tdSrvTaskDispatcherConf_->riskCtrlpluginPath_ =
      node["riskCtrlpluginPath"].as<std::string>();
  tdSrvTaskDispatcherConf_->tdSrvRiskSegmentSize_ =
      node["tdSrvRiskSegmentSize"].as<std::uint32_t>();
}

void RiskCtrlModule::initSegmentGroup(std::uint32_t threadPoolSize) {
  const auto prefixOfSegment =
      fmt::format("TD-SRV-RISK-SEGMENT-{}-{}-",
                  boost::to_upper_copy(tdSrvTaskDispatcherConf_->step_), no_);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    const auto segmentName = fmt::format("{}{}", prefixOfSegment, threadNo);
    const auto segmentSize = tdSrvTaskDispatcherConf_->tdSrvRiskSegmentSize_;
    auto segment = std::make_shared<bip::managed_shared_memory>(
        bip::open_or_create, segmentName.c_str(), segmentSize);
    segmentGroup_.emplace_back(segment);
  }
}

void RiskCtrlModule::initRiskCtrlStatusUpdatersGroup(
    std::uint32_t threadPoolSize) {
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    const auto updaters = std::make_shared<RiskCtrlStatusUpdaters>();
    riskCtrlStatusUpdatersGroup_.emplace_back(updaters);
  }
}

int RiskCtrlModule::initPnlMonitorRangeMgr(std::uint32_t threadPoolSize) {
  const auto recPnlMonitorGroup =
      tdSrv_->getPnlMonitorRangeMgrGroup(tdSrvTaskDispatcherConf_->step_);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto pnlMonitorRangeMgr = std::make_shared<PnlMonitorRangeMgr>(
        tdSrvTaskDispatcherConf_->step_, threadNo,
        tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_);
    const auto [statusCode, statusMsg] =
        pnlMonitorRangeMgr->init(recPnlMonitorGroup);
    if (statusCode == 0) {
      pnlMonitorRangeMgrGroup_.emplace_back(pnlMonitorRangeMgr);
    } else {
      LOG_E("[{}] Init pnl mointor range mgr failed. [{} - {}}]", no_,
            statusCode, statusMsg);
      return statusCode;
    }
  }
  return 0;
}

int RiskCtrlModule::initSelfTradeCtrlRangeMgr(std::uint32_t threadPoolSize) {
  const auto recSelfTradeCtrlGroup =
      tdSrv_->getSelfTradeCtrlRangeMgrGroup(tdSrvTaskDispatcherConf_->step_);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto selfTradeCtrlRangeMgr = std::make_shared<SelfTradeCtrlRangeMgr>(
        tdSrvTaskDispatcherConf_->step_, threadNo,
        tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_);
    const auto [statusCode, statusMsg] =
        selfTradeCtrlRangeMgr->init(recSelfTradeCtrlGroup);
    if (statusCode == 0) {
      selfTradeCtrlRangeMgrGroup_.emplace_back(selfTradeCtrlRangeMgr);
    } else {
      LOG_E("[{}] Init self trade risk ctrl mgr failed. [{} - {}}]", no_,
            statusCode, statusMsg);
      return statusCode;
    }
  }
  return 0;
}

int RiskCtrlModule::initTrdSymbolListMgr(std::uint32_t threadPoolSize) {
  const auto recTrdSymbolListGroup =
      tdSrv_->getTrdSymbolListGroup(tdSrvTaskDispatcherConf_->step_);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto trdSymbolListMgr = std::make_shared<TrdSymbolListMgr>(
        tdSrvTaskDispatcherConf_->step_, threadNo,
        tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_);
    const auto [statusCode, statusMsg] =
        trdSymbolListMgr->init(recTrdSymbolListGroup);
    if (statusCode == 0) {
      trdSymbolListMgrGroup_.emplace_back(trdSymbolListMgr);
    } else {
      LOG_E("[{}] Init trd symbol list mgr failed. [{} - {}}]", no_, statusCode,
            statusMsg);
      return statusCode;
    }
  }
  return 0;
}

int RiskCtrlModule::initFlowCtrlRuleMgr(std::uint32_t threadPoolSize) {
  const auto recFlowCtrlRuleGroup =
      tdSrv_->getFlowCtrlRuleGroup(tdSrvTaskDispatcherConf_->step_);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto flowCtrlRuleMgr = std::make_shared<FlowCtrlRuleMgr>(
        tdSrvTaskDispatcherConf_->step_, threadNo,
        tdSrvTaskDispatcherConf_->fieldGroupUsedToGenHash_);
    const auto [statusCode, statusMsg] =
        flowCtrlRuleMgr->init(recFlowCtrlRuleGroup);
    if (statusCode == 0) {
      flowCtrlRuleMgrGroup_.emplace_back(flowCtrlRuleMgr);
    } else {
      LOG_E("[{}] Init flow ctrl rule mgr failed. [{} - {}]",  //
            no_, statusCode, statusMsg);
      return statusCode;
    }
  }
  return 0;
}

void RiskCtrlModule::initPosMgr(std::uint32_t threadPoolSize) {
  const auto sql = fmt::format("SELECT * FROM {}", TBLPosInfo::TableName);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto posMgr = std::make_shared<TDPosMgr>();
    LOG_I("Begin to init posmgr of thread {}-{}", no_, threadNo);
    posMgr->init(CONFIG, tdSrv_->getDBEng(), sql);
    posMgrGroup_.emplace_back(posMgr);
  }
}

void RiskCtrlModule::initOrdMgr(std::uint32_t threadPoolSize) {
  const auto filled = magic_enum::enum_integer(OrderStatus::Filled);
  const auto sql = fmt::format("SELECT * FROM {} WHERE `orderStatus` < {}; ",
                               TBLOrderInfo::TableName, filled);
  for (std::uint32_t threadNo = 0; threadNo < threadPoolSize; ++threadNo) {
    auto ordMgr = std::make_shared<TDOrdMgr>();
    LOG_I("Begin to init ordmgr of thread {}-{}", no_, threadNo);
    ordMgr->init(CONFIG, tdSrv_->getDBEng(), sql);
    ordMgrGroup_.emplace_back(ordMgr);
  }
}

//! 那些不再流转到下一风控模组的业务（比如策略注册），在0号线程里处理
std::uint64_t RiskCtrlModule::getHashFromTask(
    const SHMIPCTaskSPtr& task,
    const ConditionFieldGroup& conditionFieldGroup) {
  std::uint64_t ret = 0;
  const auto shmHeader = static_cast<const SHMHeader*>(task->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_ORDER:
    case MSG_ID_ON_CANCEL_ORDER:
    case MSG_ID_ON_ORDER_RET:
    case MSG_ID_ON_CANCEL_ORDER_RET: {
      const auto orderInfo = static_cast<const OrderInfo*>(task->data_);
      const auto s =
          MakeConditioFieldInfoInStrFmt(orderInfo, conditionFieldGroup);
      ret = XXH3_64bits(s.data(), s.size());
    } break;
    default:
      break;
  }
  return ret;
}

int RiskCtrlModule::start() {
  tdSrvTaskDispatcher_->start();
  return 0;
}

void RiskCtrlModule::stop() { tdSrvTaskDispatcher_->stop(); }

}  // namespace bq::td::srv
