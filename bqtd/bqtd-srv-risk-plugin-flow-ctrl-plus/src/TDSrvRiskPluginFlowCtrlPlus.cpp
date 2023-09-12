/*!
 * \file TDSrvRiskPluginFlowCtrlPlus.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvRiskPluginFlowCtrlPlus.hpp"

#include "AssetsMgr.hpp"
#include "FlowCtrlConst.hpp"
#include "FlowCtrlDef.hpp"
#include "FlowCtrlOnStepOnCancelOrder.hpp"
#include "FlowCtrlOnStepOnCancelOrderRet.hpp"
#include "FlowCtrlOnStepOnOrder.hpp"
#include "FlowCtrlOnStepOnOrderRet.hpp"
#include "FlowCtrlRuleMgr.hpp"
#include "FlowCtrlUtil.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "SHMIPCUtil.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginFlowCtrlPlus.hpp"
#include "db/DBE.hpp"
#include "db/TBLMonitorOfFlowCtrlRule.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/SHMUtil.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

int TDSrvRiskPluginFlowCtrlPlus::doLoad() {
  initFlowCtrlTargetState();

  //! 创建流控处理对象
  flowCtrlOnStepOnOrder_ = std::make_shared<FlowCtrlOnStepOnOrder>(this);
  flowCtrlOnStepOnCancelOrder_ =
      std::make_shared<FlowCtrlOnStepOnCancelOrder>(this);
  flowCtrlOnStepOnOrderRet_ = std::make_shared<FlowCtrlOnStepOnOrderRet>(this);
  flowCtrlOnStepOnCancelOrderRet_ =
      std::make_shared<FlowCtrlOnStepOnCancelOrderRet>(this);

  return 0;
}

void TDSrvRiskPluginFlowCtrlPlus::initFlowCtrlTargetState() {
  flowCtrlTargetState_ = std::make_shared<FlowCtrlTargetState>();

  flowCtrlTargetState_->orderSizeEachTime_ =
      node_["orderSizeEachTime"].as<bool>(true);

  flowCtrlTargetState_->orderSizeTotal_ =  //
      node_["orderSizeTotal"].as<bool>(true);

  flowCtrlTargetState_->orderAmtEachTime_ =
      node_["orderAmtEachTime"].as<bool>(true);

  flowCtrlTargetState_->orderAmtTotal_ =  //
      node_["orderAmtTotal"].as<bool>(true);

  flowCtrlTargetState_->orderTimesTotal_ =  //
      node_["orderTimesTotal"].as<bool>(true);

  flowCtrlTargetState_->orderTimesWithinTime_ =
      node_["orderTimesWithinTime"].as<bool>(true);

  flowCtrlTargetState_->cancelOrderTimesTotal_ =
      node_["cancelOrderTimesTotal"].as<bool>(true);

  flowCtrlTargetState_->cancelOrderTimesWithinTime_ =
      node_["cancelOrderTimesWithinTime"].as<bool>(true);

  flowCtrlTargetState_->rejectOrderTimesTotal_ =
      node_["rejectOrderTimesTotal"].as<bool>(true);

  flowCtrlTargetState_->rejectOrderTimesWithinTime_ =
      node_["rejectOrderTimesWithinTime"].as<bool>(true);

  flowCtrlTargetState_->holdVolTotal_ = node_["holdVolTotal"].as<bool>(true);
  flowCtrlTargetState_->holdAmtTotal_ = node_["holdAmtTotal"].as<bool>(true);
  flowCtrlTargetState_->openTDayTotal_ = node_["openTDayTotal"].as<bool>(true);
}

void TDSrvRiskPluginFlowCtrlPlus::doUnload() {}

boost::dll::fs::path TDSrvRiskPluginFlowCtrlPlus::getLocation() const {
  return boost::dll::this_line_location();
}

TDSrvRiskPluginFlowCtrlPlus::TDSrvRiskPluginFlowCtrlPlus(TDSrv* tdSrv)
    : TDSrvRiskPlugin(tdSrv) {}

int TDSrvRiskPluginFlowCtrlPlus::doOnRiskCtrlConfChg(const std::string& step,
                                                     const Doc& doc,
                                                     const std::string& conf,
                                                     std::uint32_t combNo,
                                                     std::uint32_t threadNo) {
  auto& flowCtrlRuleMgr = getTDSrv()
                              ->getRiskCtrlModuleComb()[combNo]
                              ->getFlowCtrlRuleMgrGroup()[threadNo];
  const auto [statusCode, statusMsg] = flowCtrlRuleMgr->update(doc);
  if (statusCode != 0) {
    L_W(logger(), statusMsg);
    return statusCode;
  }

  L_I(logger(), "Step of {} recv {}", step, conf);

  if (doc["tableChgType"].GetString() == ENUM_TO_STR(TableChgType::Chg)) {
    updateTsQueOfLimitValueInSHM(step, doc, conf, combNo, threadNo);
  }

  return 0;
}

void TDSrvRiskPluginFlowCtrlPlus::updateTsQueOfLimitValueInSHM(
    const std::string& step, const Doc& doc, const std::string& conf,
    std::uint32_t combNo, std::uint32_t threadNo) {
  const auto& target2FlowCtrlRuleGroup =
      getTDSrv()
          ->getRiskCtrlModuleComb()[combNo]
          ->getFlowCtrlRuleMgrGroup()[threadNo]
          ->getTarget2FlowCtrlRuleGroup();

  for (const auto& rec : *target2FlowCtrlRuleGroup) {
    const auto& rule = rec.second;
    if (rule->limitType_ != FlowCtrlLimitType::NumLimitWithinTime) {
      continue;
    }

    bool createTSQue = true;
    //! 下面代码如果配置中 tsQue 长度发生变化，那么会重建
    for (const auto& rec : rule->conditionValue2LimitValueGroup_) {
      //! conditionValueInStrFmt = "acctId=10000&trdAcctId=100000"
      const auto& conditionValueInStrFmt = rec.first;
      openOrCtorLimitValue(combNo, threadNo, rule, conditionValueInStrFmt,
                           createTSQue);
    }
  }
}

//!
//! 打开或者构建limitValueInSHM，
//! 如果limitValueInSHM已经存在且其中tsQue的长度发送变化，那么重建
//!
LimitValueInSHM* TDSrvRiskPluginFlowCtrlPlus::openOrCtorLimitValue(
    std::uint32_t combNo, std::uint32_t threadNo, const FlowCtrlRuleSPtr& rule,
    const std::string& conditionValueInStrFmt, bool createTSQue) {
  auto& segment =
      getTDSrv()->getRiskCtrlModuleComb()[combNo]->getSegmentGroup()[threadNo];

  //! create limitValueInSHM
  //! 10002-OrderSizeTotal-acctId-OrderSizeTotal-
  //! acctId=10000&marketCode=SZSE&symbolCode=000002- // prefix
  //! acctId=10000&marketCode=SZSE&symbolCode=000002  // conditionValueInStrFmt
  //
  const auto keyOfLimitValue = fmt::format(
      "{}{}", rule->prefixOfLimitValueKey(), conditionValueInStrFmt);
  const auto ret = segment->find<LimitValueInSHM>(keyOfLimitValue.c_str());

  auto limitValueInSHM = ret.first;
  if (limitValueInSHM != nullptr) {
    //! 如果limitValueInSHM已经存在于共享内存中
    L_T(logger(), "Get limit value {} in thread {}-{}. {} {} [len = {}]",
        keyOfLimitValue, combNo, threadNo, FreeSegmentPerDesc(*segment),
        rule->toStr(), ret.second);

    if (createTSQue) {
      //! 如果需要创建流控时间队列
      const auto queueSizeInSHM = limitValueInSHM->tsQue_->size();
      const auto queueSizeInConf = rule->limitValueInConf_.tsQue_.size();

      //! 如果数据库中流控时间队列的长度发生变化，那么重建队列
      if (queueSizeInSHM != queueSizeInConf) {
        limitValueInSHM->tsQue_ = bip::make_managed_shared_ptr(
            segment->construct<TSQueInSHM>(bip::anonymous_instance)(
                boost::circular_buffer<std::uint64_t, Uint64Alloc>(
                    queueSizeInConf, segment->get_allocator<std::uint64_t>())),
            *segment);

        L_I(logger(),
            "Create ts que of limit value {} in thread {}-{} "
            "because of queue size channged from {} to {}. {} {}",
            keyOfLimitValue, combNo, threadNo, queueSizeInSHM, queueSizeInConf,
            FreeSegmentPerDesc(*segment), rule->toStr());

        if (limitValueInSHM->tsQue_->empty()) {
          limitValueInSHM->reset(queueSizeInConf);
        }
      }
    }

    return limitValueInSHM;
  }

  limitValueInSHM =
      segment->construct<LimitValueInSHM>(keyOfLimitValue.c_str())();

  L_I(logger(), "Create limit value {} in thread {}-{}. {} {}", keyOfLimitValue,
      combNo, threadNo, FreeSegmentPerDesc(*segment), rule->toStr());

  //! init LimitValueInSHM->tsQue_
  if (createTSQue) {
    const auto queueSize = rule->limitValueInConf_.tsQue_.size();

    limitValueInSHM->tsQue_ = bip::make_managed_shared_ptr(
        segment->construct<TSQueInSHM>(bip::anonymous_instance)(
            boost::circular_buffer<std::uint64_t, Uint64Alloc>(
                queueSize, segment->get_allocator<std::uint64_t>())),
        *segment);

    L_I(logger(), "Create ts que of limit value {} in thread {}-{}. {} {}",
        keyOfLimitValue, combNo, threadNo, FreeSegmentPerDesc(*segment),
        rule->toStr());

    if (limitValueInSHM->tsQue_->empty()) {
      limitValueInSHM->reset(queueSize);
    }
  }

  return limitValueInSHM;
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrlPlus::doOnOrder(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  const auto ret =
      flowCtrlOnStepOnOrder_->handleFlowCtrl(orderInfo, combNo, threadNo);
  return ret;
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrlPlus::doOnCancelOrder(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  const auto ret =
      flowCtrlOnStepOnCancelOrder_->handleFlowCtrl(orderInfo, combNo, threadNo);
  return ret;
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrlPlus::doOnOrderRet(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  const auto ret =
      flowCtrlOnStepOnOrderRet_->handleFlowCtrl(orderInfo, combNo, threadNo);
  return ret;
}

std::tuple<int, std::string> TDSrvRiskPluginFlowCtrlPlus::doOnCancelOrderRet(
    const OrderInfoSPtr& orderInfo, std::uint32_t combNo,
    std::uint32_t threadNo) {
  const auto ret = flowCtrlOnStepOnCancelOrderRet_->handleFlowCtrl(
      orderInfo, combNo, threadNo);
  return ret;
}

}  // namespace bq::td::srv
