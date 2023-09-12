/*!
 * \file RiskMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "RiskMgrDef.hpp"
#include "SHMIPCDef.hpp"
#include "db/DBEngDef.hpp"
#include "def/ConstIF.hpp"
#include "util/Pch.hpp"
#include "util/SvcBase.hpp"

namespace bq {
class AssetsMgr;
using AssetsMgrSPtr = std::shared_ptr<AssetsMgr>;

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

class SubMgr;
using SubMgrSPtr = std::shared_ptr<SubMgr>;

class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

struct ScheduleTask;
using ScheduleTaskSPtr = std::shared_ptr<ScheduleTask>;
using ScheduleTaskBundle = std::vector<ScheduleTaskSPtr>;
using ScheduleTaskBundleSPtr = std::shared_ptr<ScheduleTaskBundle>;

class MarketDataCache;
using MarketDataCacheSPtr = std::shared_ptr<MarketDataCache>;

}  // namespace bq

namespace bq::db {
class TBLMonitorOfSymbolInfo;
using TBLMonitorOfSymbolInfoSPtr = std::shared_ptr<TBLMonitorOfSymbolInfo>;
}  // namespace bq::db

namespace bq::riskmgr {

class TDGWTaskHandler;
using TDGWTaskHandlerSPtr = std::shared_ptr<TDGWTaskHandler>;

class StgEngTaskHandler;
using StgEngTaskHandlerSPtr = std::shared_ptr<StgEngTaskHandler>;

class ClientChannelGroup;
using ClientChannelGroupSPtr = std::shared_ptr<ClientChannelGroup>;

class PubSvc;
using PubSvcSPtr = std::shared_ptr<PubSvc>;

class RiskMgr : public SvcBase {
 public:
  using SvcBase::SvcBase;

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  int initDBEng();
  void initTBLMonitorOfSymbolInfo();
  void initPosMgr();
  void initAssetsMgr();
  void initOrdMgr();
  int initRiskMgrTaskDispatcher();
  void initSHMSrv();
  void initScheduleTaskBundle();

 public:
  int doRun() final;

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 public:
  db::DBEngSPtr getDBEng() const { return dbEng_; }

  db::TBLMonitorOfSymbolInfoSPtr getTBLMonitorOfSymbolInfo() const {
    return tblMonitorOfSymbolInfo_;
  }

  MarketDataCacheSPtr getMarketDataCache() const { return marketDataCache_; }
  SubMgrSPtr getSubMgr() const { return subMgr_; }

  RiskPosMgrSPtr getPosMgr() const { return posMgr_; }
  AssetsMgrSPtr getAssetsMgr() const { return assetsMgr_; }
  RiskOrdMgrSPtr getOrdMgr() const { return ordMgr_; }

  PubSvcSPtr getPubSvc() const { return pubSvc_; }

  ClientChannelGroupSPtr getTDGWGroup() const { return tdGWGroup_; }
  ClientChannelGroupSPtr getStgEngGroup() const { return stgEngGroup_; }

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block>
  getRiskMgrTaskDispatcher() const {
    return riskMgrTaskDispatcher_;
  }

  SHMSrvSPtr getSHMSrvOfTDGW() const { return shmSrvOfTDGW_; }
  SHMSrvSPtr getSHMSrvOfStgEng() const { return shmSrvOfStgEng_; }
  SHMSrvSPtr getSHMSrvOfPub() const { return shmSrvOfPub_; }

  ScheduleTaskBundleSPtr& getScheduleTaskBundle() {
    return scheduleTaskBundle_;
  };

 private:
  db::DBEngSPtr dbEng_{nullptr};
  db::TBLMonitorOfSymbolInfoSPtr tblMonitorOfSymbolInfo_{nullptr};

  MarketDataCacheSPtr marketDataCache_{nullptr};
  SubMgrSPtr subMgr_{nullptr};

  RiskPosMgrSPtr posMgr_{nullptr};
  AssetsMgrSPtr assetsMgr_{nullptr};
  RiskOrdMgrSPtr ordMgr_{nullptr};

  PubSvcSPtr pubSvc_{nullptr};

  ClientChannelGroupSPtr tdGWGroup_{nullptr};
  ClientChannelGroupSPtr stgEngGroup_{nullptr};

  TDGWTaskHandlerSPtr tdGWTaskHandler_{nullptr};
  StgEngTaskHandlerSPtr stgEngTaskHandler_{nullptr};

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> riskMgrTaskDispatcher_{
      nullptr};

  SHMSrvSPtr shmSrvOfTDGW_{nullptr};
  SHMSrvSPtr shmSrvOfStgEng_{nullptr};
  SHMSrvSPtr shmSrvOfPub_{nullptr};

  ScheduleTaskBundleSPtr scheduleTaskBundle_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutor_{nullptr};
};

}  // namespace bq::riskmgr
