/*!
 * \file TDSrv.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMIPCDef.hpp"
#include "SHMIPCMsgId.hpp"
#include "db/DBEngDef.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/SvcBase.hpp"

namespace bq::db::pnlMonitorRange {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::pnlMonitorRange

namespace bq::db::selfTradeCtrlRange {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::selfTradeCtrlRange

namespace bq::db::trdSymbolList {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::trdSymbolList

namespace bq::db::flowCtrlRule {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::flowCtrlRule

namespace bq {

class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

struct ScheduleTask;
using ScheduleTaskSPtr = std::shared_ptr<ScheduleTask>;

using ScheduleTaskBundle = std::vector<ScheduleTaskSPtr>;
using ScheduleTaskBundleSPtr = std::shared_ptr<ScheduleTaskBundle>;

struct SyncTask;
using SyncTaskSPtr = std::shared_ptr<SyncTask>;

enum class SyncToRiskMgr;
enum class SyncToDB;

}  // namespace bq

namespace bq::db {
class TBLMonitorOfSymbolInfo;
using TBLMonitorOfSymbolInfoSPtr = std::shared_ptr<TBLMonitorOfSymbolInfo>;
}  // namespace bq::db

namespace bq::td::srv {

class PosMgrRestorer;
using PosMgrRestorerSPtr = std::shared_ptr<PosMgrRestorer>;

class OrderPreProc;
using OrderPreProcSPtr = std::shared_ptr<OrderPreProc>;

class ClientChannelGroup;
using ClientChannelGroupSPtr = std::shared_ptr<ClientChannelGroup>;

class RiskCtrlModule;
using RiskCtrlModuleSPtr = std::shared_ptr<RiskCtrlModule>;

class RiskCtrlConfMonitor;
using RiskCtrlConfMonitorSPtr = std::shared_ptr<RiskCtrlConfMonitor>;

class TDSrv : public SvcBase {
 public:
  using SvcBase::SvcBase;

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  int initDBEng();

  void initPnlMonitorRangeGroup();
  void initSelfTradeCtrlRangeGroup();
  void initTrdSymbolListGroup();
  void initFlowCtrlRuleGroup();
  void initTBLMonitorOfSymbolInfo();

  int initRiskCtrlModuleComb();
  void initSHMSrv();

  void initScheduleTaskBundle();

 public:
  int doRun() final;

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 public:
  db::DBEngSPtr getDBEng() const { return dbEng_; }

  std::vector<db::pnlMonitorRange::RecordSPtr> getPnlMonitorRangeMgrGroup(
      const std::string& step) const;

  std::vector<db::selfTradeCtrlRange::RecordSPtr> getSelfTradeCtrlRangeMgrGroup(
      const std::string& step) const;

  std::vector<db::trdSymbolList::RecordSPtr> getTrdSymbolListGroup(
      const std::string& step) const;

  std::vector<db::flowCtrlRule::RecordSPtr> getFlowCtrlRuleGroup(
      const std::string& step) const;

  const db::TBLMonitorOfSymbolInfoSPtr& getTBLMonitorOfSymbolInfo() const {
    return tblMonitorOfSymbolInfo_;
  }

  OrderPreProcSPtr& getOrderPreProc() { return orderPreProc_; }

  const std::vector<RiskCtrlModuleSPtr>& getRiskCtrlModuleComb() {
    return riskCtrlModuleComb_;
  }

  bool isFirstRiskCtrlModule(std::uint32_t no) { return no == 0; }
  bool isLastRiskCtrlModule(std::uint32_t no) {
    return no == riskCtrlModuleComb_.size() - 1;
  }

  const ClientChannelGroupSPtr& getTDGWGroup() const { return tdGWGroup_; }
  const ClientChannelGroupSPtr& getStgEngGroup() const { return stgEngGroup_; }

  SHMSrvSPtr& getSHMSrvOfTDGW() { return shmSrvOfTDGW_; }
  SHMSrvSPtr& getSHMSrvOfStgEng() { return shmSrvOfStgEng_; }
  SHMSrvSPtr& getSHMSrvOfPlugIn() { return shmSrvOfPlugIn_; }

  const std::string& getPlugInChannel() const { return plugInChannel_; }
  const std::string& getTopicOfTriggerRiskCtrl() const {
    return topicOfTriggerRiskCtrl_;
  }

  void cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                          SyncToRiskMgr syncToRiskMgr, SyncToDB syncToDB);
  void handleSyncTaskGroup();

  ScheduleTaskBundleSPtr& getScheduleTaskBundle() {
    return scheduleTaskBundle_;
  };

 private:
  db::DBEngSPtr dbEng_{nullptr};

  std::vector<db::pnlMonitorRange::RecordSPtr> pnlMonitorRangeGroup_;
  mutable std::mutex mtxPnlMonitorRangeGroup_;

  std::vector<db::selfTradeCtrlRange::RecordSPtr> selfTradeCtrlRangeGroup_;
  mutable std::mutex mtxSelfTradeCtrlRangeGroup_;

  std::vector<db::trdSymbolList::RecordSPtr> trdSymbolListGroup_;
  mutable std::mutex mtxTrdSymbolListGroup_;

  std::vector<db::flowCtrlRule::RecordSPtr> flowCtrlRuleGroup_;
  mutable std::mutex mtxFlowCtrlRuleGroup_;

  db::TBLMonitorOfSymbolInfoSPtr tblMonitorOfSymbolInfo_{nullptr};

  PosMgrRestorerSPtr posMgrRestorer_{nullptr};
  OrderPreProcSPtr orderPreProc_{nullptr};

  std::vector<RiskCtrlModuleSPtr> riskCtrlModuleComb_;

  ClientChannelGroupSPtr tdGWGroup_{nullptr};
  ClientChannelGroupSPtr stgEngGroup_{nullptr};

  SHMSrvSPtr shmSrvOfTDGW_{nullptr};
  SHMSrvSPtr shmSrvOfStgEng_{nullptr};
  SHMSrvSPtr shmSrvOfPlugIn_{nullptr};

  std::string plugInChannel_;
  std::string topicOfTriggerRiskCtrl_;

  std::vector<SyncTaskSPtr> syncTaskGroup_;
  std::ext::spin_mutex mtxSyncTaskGroup_;

  RiskCtrlConfMonitorSPtr riskCtrlConfMonitor_{nullptr};

  ScheduleTaskBundleSPtr scheduleTaskBundle_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutor_{nullptr};
};

}  // namespace bq::td::srv
