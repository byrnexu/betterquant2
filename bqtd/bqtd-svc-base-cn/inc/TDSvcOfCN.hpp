/*!
 * \file TDSvcOfCN.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMIPCMsgId.hpp"
#include "TDSvcDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/SvcBase.hpp"

namespace bq {
class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

class SignalHandler;
using SignalHandlerSPtr = std::shared_ptr<SignalHandler>;

struct ScheduleTask;
using ScheduleTaskSPtr = std::shared_ptr<ScheduleTask>;
using ScheduleTaskBundle = std::vector<ScheduleTaskSPtr>;
using ScheduleTaskBundleSPtr = std::shared_ptr<ScheduleTaskBundle>;

class AssetsMgr;
using AssetsMgrSPtr = std::shared_ptr<AssetsMgr>;

struct SyncTask;
using SyncTaskSPtr = std::shared_ptr<SyncTask>;
using SyncTaskGroup = std::vector<SyncTaskSPtr>;
using SyncTaskGroupSPtr = std::shared_ptr<SyncTaskGroup>;

class FlowCtrlSvc;
using FlowCtrlSvcSPtr = std::shared_ptr<FlowCtrlSvc>;

class SHMCli;
using SHMCliSPtr = std::shared_ptr<SHMCli>;

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

template <typename Task>
struct AsyncTask;

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

enum class SyncToRiskMgr;
enum class SyncToDB;

class ExceedFlowCtrlHandler;
using ExceedFlowCtrlHandlerSPtr = std::shared_ptr<ExceedFlowCtrlHandler>;

class OpenedContractGroup;
using OpenedContractGroupSPtr = std::shared_ptr<OpenedContractGroup>;
}  // namespace bq

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
class TBLMonitorOfSymbolInfo;
using TBLMonitorOfSymbolInfoSPtr = std::shared_ptr<TBLMonitorOfSymbolInfo>;
}  // namespace bq::db

namespace bq::td {
class TrdSymbolCache;
using TrdSymbolCacheSPtr = std::shared_ptr<TrdSymbolCache>;
class ExternalStatusCodeCache;
using ExternalStatusCodeCacheSPtr = std::shared_ptr<ExternalStatusCodeCache>;
class MapRelOfCliIdAndOrdId;
using MapRelOfCliIdAndOrdIdSPtr = std::shared_ptr<MapRelOfCliIdAndOrdId>;
}  // namespace bq::td

namespace bq::td::svc {

class TDSrvTaskHandler;
using TDSrvTaskHandlerSPtr = std::shared_ptr<TDSrvTaskHandler>;

class SimedOrderInfoHandler;
using SimedOrderInfoHandlerSPtr = std::shared_ptr<SimedOrderInfoHandler>;

class TDGateway;
using TDGatewaySPtr = std::shared_ptr<TDGateway>;

class TDSvcOfCN : public SvcBase {
 public:
  using SvcBase::SvcBase;

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  void initSegmentData();
  int initDBEng();

  void initTBLMonitorOfSymbolInfo();
  void initTrdSymbolCache();
  void initExternalStatusCodeCache();

  int queryMaxNoUsedToCalcPos();

  void initAssetsMgr();
  void initOrdMgr();

  int initTDSrvTaskDispatcher();
  void initSHMCliOfTDSrv();
  void initSHMCliOfRiskMgr();

 private:
  void initScheduleTaskBundle() {
    beforeInitScheduleTaskBundle();
    doInitScheduleTaskBundle();
  };

  virtual void beforeInitScheduleTaskBundle();
  virtual void doInitScheduleTaskBundle(){};

 public:
  int doRun() final;

 private:
  void sendTDGWReg();

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 public:
  AcctId getAcctId() const { return acctId_; }
  std::string getApiName() const { return apiName_; }

  std::uint64_t getNextNoUsedToCalcPos() { return ++maxNoUsedToCalcPos_; }

  std::string getAppName() const { return appName_; }

 public:
  db::DBEngSPtr getDBEng() const { return dbEng_; }

  db::TBLMonitorOfSymbolInfoSPtr getTBLMonitorOfSymbolInfo() const {
    return tblMonitorOfSymbolInfo_;
  }

  TrdSymbolCacheSPtr getTrdSymbolCache() const { return trdSymbolCache_; }
  ExternalStatusCodeCacheSPtr getExternalStatusCodeCache() const {
    return externalStatusCodeCache_;
  }

  AssetsMgrSPtr getAssetsMgr() const { return assetsMgr_; }
  TDOrdMgrSPtr getOrdMgr() const { return ordMgr_; }

  SimedOrderInfoHandlerSPtr getSimedOrderInfoHandler() const {
    return simedOrderInfoHandler_;
  }

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> getTDSrvTaskDispatcher()
      const {
    return tdSrvTaskDispatcher_;
  }

  SHMCliSPtr getSHMCliOfTDSrv() const { return shmCliOfTDSrv_; }
  SHMCliSPtr getSHMCliOfRiskMgr() const { return shmCliOfRiskMgr_; }

  FlowCtrlSvcSPtr getFlowCtrlSvc() const { return flowCtrlSvc_; }
  ExceedFlowCtrlHandlerSPtr getExceedFlowCtrlHandler() const {
    return exceedFlowCtrlHandler_;
  }

  void cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                          SyncToRiskMgr syncToRiskMgr, SyncToDB syncToDB);
  void handleSyncTaskGroup();

  ScheduleTaskBundleSPtr& getScheduleTaskBundle() {
    return scheduleTaskBundle_;
  };

  TDGatewaySPtr getTDGateway() const { return tdGateway_; }
  void setTDGateway(const TDGatewaySPtr& value) { tdGateway_ = value; }

  std::string getTradingDay() const { return tradingDay_; }
  void setTradingDay(const std::string& value) { tradingDay_ = value; }

  MapRelOfCliIdAndOrdIdSPtr getMapRelOfCliIdAndOrdId() const {
    return mapRelOfCliIdAndOrdId_;
  }
  OpenedContractGroupSPtr getOpenedContractGroup() const {
    return openedContractGroup_;
  }
  std::string getIdentityOfMapRelOfCliIdAndOrdId() const;
  std::string getIdentityOfOpenContractGroup() const;

  FeeInfoCacheSPtr getFeeInfoCache() const { return feeInfoCache_; }

 private:
  AcctId acctId_;
  std::string apiName_;

  std::uint64_t maxNoUsedToCalcPos_{0};

  //! "TD-XTP-INSTANCE-10001@TD@TDGWChannel@Trade"
  std::string appName_;

  db::DBEngSPtr dbEng_{nullptr};
  db::TBLMonitorOfSymbolInfoSPtr tblMonitorOfSymbolInfo_{nullptr};
  TrdSymbolCacheSPtr trdSymbolCache_{nullptr};
  ExternalStatusCodeCacheSPtr externalStatusCodeCache_{nullptr};

  AssetsMgrSPtr assetsMgr_{nullptr};
  TDOrdMgrSPtr ordMgr_{nullptr};

  SimedOrderInfoHandlerSPtr simedOrderInfoHandler_{nullptr};

  TDSrvTaskHandlerSPtr tdSrvTaskHandler_{nullptr};
  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> tdSrvTaskDispatcher_{
      nullptr};
  SHMCliSPtr shmCliOfTDSrv_{nullptr};
  SHMCliSPtr shmCliOfRiskMgr_{nullptr};

  FlowCtrlSvcSPtr flowCtrlSvc_{nullptr};
  ExceedFlowCtrlHandlerSPtr exceedFlowCtrlHandler_{nullptr};

  SyncTaskGroupSPtr syncTaskGroup_{nullptr};
  std::ext::spin_mutex mtxSyncTaskGroup_;

  ScheduleTaskBundleSPtr scheduleTaskBundle_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutor_{nullptr};

  TDGatewaySPtr tdGateway_{nullptr};
  std::string tradingDay_;

  MapRelOfCliIdAndOrdIdSPtr mapRelOfCliIdAndOrdId_{nullptr};
  OpenedContractGroupSPtr openedContractGroup_{nullptr};

  FeeInfoCacheSPtr feeInfoCache_{nullptr};
};

}  // namespace bq::td::svc
