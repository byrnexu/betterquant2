/*!
 * \file StgEngImpl.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMIPCMsgId.hpp"
#include "StgEngDef.hpp"
#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"
#include "def/ConstIF.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/SvcBase.hpp"

namespace YAML {
class Node;
using NodeSPtr = std::shared_ptr<Node>;
}  // namespace YAML

namespace bq {
enum class LogLevel;
enum class ExecAtStartup;

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
class SHMCli;
using SHMCliSPtr = std::shared_ptr<SHMCli>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct PosOfStgInst;
using PosOfStgInstSPtr = std::shared_ptr<PosOfStgInst>;

using PosOfStgInstGroup = std::vector<PosOfStgInstSPtr>;
using PosOfStgInstGroupSPtr = std::shared_ptr<PosOfStgInstGroup>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class ProductInfoCache;
using ProductInfoCacheSPtr = std::shared_ptr<ProductInfoCache>;

class StgInfoCache;
using StgInfoCacheSPtr = std::shared_ptr<StgInfoCache>;

class AcctInfoCache;
using AcctInfoCacheSPtr = std::shared_ptr<AcctInfoCache>;

class TrdAcctInfoCache;
using TrdAcctInfoCacheSPtr = std::shared_ptr<TrdAcctInfoCache>;

class MarketDataCache;
using MarketDataCacheSPtr = std::shared_ptr<MarketDataCache>;

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

template <typename Task>
struct AsyncTask;
template <typename Task>
using AsyncTaskSPtr = std::shared_ptr<AsyncTask<Task>>;

class SubMgr;
using SubMgrSPtr = std::shared_ptr<SubMgr>;

class TopicMgr;
using TopicMgrSPtr = std::shared_ptr<TopicMgr>;

struct SyncTask;
using SyncTaskSPtr = std::shared_ptr<SyncTask>;

class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

struct ScheduleTask;
using ScheduleTaskSPtr = std::shared_ptr<ScheduleTask>;
using ScheduleTaskBundle = std::vector<ScheduleTaskSPtr>;
using ScheduleTaskBundleSPtr = std::shared_ptr<ScheduleTaskBundle>;

struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;

enum class SyncToRiskMgr;
enum class SyncToDB;

struct SimedTDInfo;
using SimedTDInfoSPtr = std::shared_ptr<SimedTDInfo>;
}  // namespace bq

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;

class TBLMonitorOfStgInstInfo;
using TBLMonitorOfStgInstInfoSPtr = std::shared_ptr<TBLMonitorOfStgInstInfo>;
class TBLMonitorOfSymbolInfo;
using TBLMonitorOfSymbolInfoSPtr = std::shared_ptr<TBLMonitorOfSymbolInfo>;
}  // namespace bq::db

namespace bq::tdeng {
class TDEngConnpool;
using TDEngConnpoolSPtr = std::shared_ptr<TDEngConnpool>;
}  // namespace bq::tdeng

namespace bq::algo {
class AlgoMgr;
using AlgoMgrSPtr = std::shared_ptr<AlgoMgr>;
}  // namespace bq::algo

namespace bq::stg {

struct TaskOfFixedTime;
using TaskOfFixedTimeSPtr = std::shared_ptr<TaskOfFixedTime>;

class StgInstTaskHandlerImpl;
using StgInstTaskHandlerImplSPtr = std::shared_ptr<StgInstTaskHandlerImpl>;

class WebSrvTaskHandler;
using WebSrvTaskHandlerSPtr = std::shared_ptr<WebSrvTaskHandler>;

class DynCandleSvc;
using DynCandleSvcSPtr = std::shared_ptr<DynCandleSvc>;

class SysInstructionSvc;
using SysInstructionSvcSPtr = std::shared_ptr<SysInstructionSvc>;

class StgEngImpl;
using StgEngImplSPtr = std::shared_ptr<StgEngImpl>;

class StgEngImpl : public SvcBase {
 public:
  StgEngImpl(const StgEngImpl&) = delete;
  StgEngImpl& operator=(const StgEngImpl&) = delete;
  StgEngImpl(const StgEngImpl&&) = delete;
  StgEngImpl& operator=(const StgEngImpl&&) = delete;

  explicit StgEngImpl(const std::string& configFilename);

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  int initDBEng();
  void initDftStgInstInfo();

  int initTDEng();
  void initTBLMonitorOfStgInstInfo();
  void initTBLMonitorOfSymbolInfo();
  void initSubMgr();
  void initTopicMgr();
  void initSHMCliOfTDSrv();
  void initSHMCliOfRiskMgr();
  void initSHMCliOfWebSrv();
  void initOrdMgr();
  void initPosMgr();
  int initStgInstTaskDispatcher();

  void initScheduleTaskBundleOfTimer();
  void execTaskBunndleOfTimer();

  void initScheduleTaskBundle();

 public:
  const TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block>&
  getStgInstTaskDispatcher() {
    return stgInstTaskDispatcher_;
  }

 public:
  StgInstTaskHandlerImplSPtr getStgInstTaskHandler() {
    return stgInstTaskHandler_;
  }

  void installStgInstTaskHandler(const StgInstTaskHandlerImplSPtr& value) {
    stgInstTaskHandler_ = value;
  }

 private:
  int doRun();

 private:
  void sendStgStartSignal();
  void sendStgInstStartSignal();
  void sendStgReg();

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 private:
  void sendStgStopSignal();
  void sendStgInstStopSignal();

 public:
  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, PosDirection posDirection,
      Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(
      StgInstId stgInstId, MarketCode marketCode, const std::string& symbolCode,
      Side side, PosDirection posDirection, Decimal orderPrice,
      Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, OrderId orderId = 0,
      const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, Decimal orderPrice,
      Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(
      StgInstId stgInstId, MarketCode marketCode, const std::string& symbolCode,
      Side side, Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, OrderId orderId = 0,
      const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(OrderInfoSPtr& orderInfo);

 public:
  int cancelOrder(OrderId orderId);

  std::vector<OrderId> cancelAllOrderOfStg();
  std::vector<OrderId> cancelAllOrderOfStgInst(
      const StgInstInfoSPtr& stgInstInfo);
  std::vector<OrderId> cancelAllOrderOfStgInst(StgInstId stgInsId);
  std::vector<OrderId> cancelAllOrderOfAlgo(AlgoId algoId);

 public:
  //! algoName仅仅是一个助记词
  std::tuple<int, AlgoId> algoOrder(const StgInstInfoSPtr& stgInstInfo,
                                    const std::string& algoType,
                                    const std::string& algoName,
                                    std::uint32_t lifetime,
                                    const std::string& algoParamsInJsonFmt);
  int cancelAlgoOrder(AlgoId algoId);
  std::string getProgressOfAlgoOrder(AlgoId algoId);

 public:
  int sub(StgInstId subscriber, const std::string& topic);
  int unSub(StgInstId subscriber, const std::string& topic);

 public:
  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t tsBegin, std::uint64_t tsEnd);

  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext = "");

  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

 public:
  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           const std::string& execTime,
                           std::uint32_t timeZone = 8);

  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           ExecAtStartup execAtStartUp,
                           std::uint32_t milliSecInterval,
                           std::uint64_t maxExecTimes = UINT64_MAX);

  void uninstallStgInstTimer(StgInstId stgInstId, const std::string& timerName);

 public:
  std::tuple<int, OrderInfoSPtr> getOrderInfo(OrderId orderId) const;

  std::vector<OrderInfoSPtr> getUnclosedOrderInfoGroup(
      const StgInstInfoSPtr& stgInstInfo);

  PosOfStgInstGroupSPtr getPosOfStgInst(const StgInstInfoSPtr& stgInstInfo);

 public:
  bool saveStgPrivateData(StgInstId stgInstId, const std::string& jsonStr);
  std::string loadStgPrivateData(StgInstId stgInstId);

 public:
  std::tuple<int, std::string> syncExecSql(const std::string& sql);
  void saveToDB(const PnlSPtr& pnl);

 public:
  const YAML::Node& getConfig() { return config_; }

  StgId getStgId() const { return stgId_; }
  std::string getAppName() const { return appName_; }

  db::DBEngSPtr getDBEng() const { return dbEng_; }
  tdeng::TDEngConnpoolSPtr getTDEngConnpool() const { return tdEngConnpool_; }

  StgInstInfoSPtr getDftStgInstInfo() const { return dftStgInstInfo_; }

  db::TBLMonitorOfStgInstInfoSPtr getTBLMonitorOfStgInstInfo() const {
    return tblMonitorOfStgInstInfo_;
  }

  db::TBLMonitorOfSymbolInfoSPtr getTBLMonitorOfSymbolInfo() const {
    return tblMonitorOfSymbolInfo_;
  }

  SysInstructionSvcSPtr& getSysInstructionSvc() { return sysInstructionSvc_; }
  algo::AlgoMgrSPtr& getAlgoMgr() { return algoMgr_; }

  MarketDataCacheSPtr getMarketDataCache() const { return marketDataCache_; }

  StgOrdMgrSPtr& getOrdMgr() { return ordMgr_; }
  StgPosMgrSPtr& getPosMgr() { return posMgr_; }

  SubMgrSPtr& getSubMgr() { return subMgr_; }
  TopicMgrSPtr& getTopicMgr() { return topicMgr_; };

  SHMCliSPtr getSHMCliOfWebSrv() const { return shmCliOfWebSrv_; }

 private:
  void resetBarrierOfStgStartSignal() {
    barrierOfStgStartSignal_ = std::make_shared<std::promise<void>>();
  }

 public:
  std::shared_ptr<std::promise<void>>& getBarrierOfStgStartSignal() {
    return barrierOfStgStartSignal_;
  }

  void cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                          SyncToRiskMgr syncToRiskMgr, SyncToDB syncToDB);

 public:
  void logTrace(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logDebug(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logInfo(const std::string& fmt, const std::vector<std::string>& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logWarn(const std::string& fmt, const std::vector<std::string>& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logError(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logCritical(const std::string& fmt, const std::vector<std::string>& args,
                   const StgInstInfoSPtr& stgInstInfo = nullptr,
                   NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logTrace(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logDebug(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logInfo(const std::string& fmt,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logWarn(const std::string& fmt,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logError(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logCritical(const std::string& fmt,
                   const StgInstInfoSPtr& stgInstInfo = nullptr,
                   NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

 private:
  void handleSyncTaskGroup();

  ScheduleTaskBundleSPtr getScheduleTaskBundleOfTimer();

 private:
  YAML::Node config_;

  StgId stgId_{1};
  std::string appName_;

  std::string rootDirOfStgPrivateData_;

  db::DBEngSPtr dbEng_{nullptr};
  tdeng::TDEngConnpoolSPtr tdEngConnpool_{nullptr};

  StgInstInfoSPtr dftStgInstInfo_{nullptr};

  db::TBLMonitorOfStgInstInfoSPtr tblMonitorOfStgInstInfo_{nullptr};
  db::TBLMonitorOfSymbolInfoSPtr tblMonitorOfSymbolInfo_{nullptr};

  SysInstructionSvcSPtr sysInstructionSvc_{nullptr};
  algo::AlgoMgrSPtr algoMgr_{nullptr};

  ProductInfoCacheSPtr productInfoCache_{nullptr};
  StgInfoCacheSPtr stgInfoCache_{nullptr};
  AcctInfoCacheSPtr acctInfoCache_{nullptr};
  TrdAcctInfoCacheSPtr trdAcctInfoCache_{nullptr};
  MarketDataCacheSPtr marketDataCache_{nullptr};

  WebSrvTaskHandlerSPtr webSrvTaskHandler_{nullptr};

  DynCandleSvcSPtr dynCandle_{nullptr};

  StgOrdMgrSPtr ordMgr_{nullptr};
  StgPosMgrSPtr posMgr_{nullptr};
  SubMgrSPtr subMgr_{nullptr};
  TopicMgrSPtr topicMgr_{nullptr};

  SHMCliSPtr shmCliOfTDSrv_{nullptr};
  SHMCliSPtr shmCliOfRiskMgr_{nullptr};
  SHMCliSPtr shmCliOfWebSrv_{nullptr};

  StgInstTaskHandlerImplSPtr stgInstTaskHandler_{nullptr};
  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> stgInstTaskDispatcher_{
      nullptr};

  std::shared_ptr<std::promise<void>> barrierOfStgStartSignal_{nullptr};

  std::vector<SyncTaskSPtr> syncTaskGroup_;
  std::ext::spin_mutex mtxSyncTaskGroup_;

  boost::posix_time::ptime prevExecTaskBunndleOfTimer_;

  std::vector<TaskOfFixedTimeSPtr> taskOfFixedTimeGroup_;
  std::ext::spin_mutex mtxTaskOfFixedTimeGroup_;

  ScheduleTaskBundleSPtr scheduleTaskBundleOfTimer_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutorOfTimer_{nullptr};
  std::ext::spin_mutex mtxScheduleTaskBundleOfTimer_;

  ScheduleTaskBundleSPtr scheduleTaskBundle_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutor_{nullptr};
};

}  // namespace bq::stg
