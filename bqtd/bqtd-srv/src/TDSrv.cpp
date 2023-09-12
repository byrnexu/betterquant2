/*!
 * \file TDSrv.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrv.hpp"

#include "AssetsMgr.hpp"
#include "ClientChannelGroup.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "OrderPreProc.hpp"
#include "PosMgr.hpp"
#include "PosMgrRestorer.hpp"
#include "RiskCtrlConfMonitor.hpp"
#include "RiskCtrlModule.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCTask.hpp"
#include "SHMSrv.hpp"
#include "StgEngTaskHandler.hpp"
#include "TDGWTaskHandler.hpp"
#include "TDSrvConst.hpp"
#include "TDSrvRiskPluginMgr.hpp"
#include "db/DBEngConst.hpp"
#include "db/TBLFlowCtrlRule.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "db/TBLPnlMonitorRange.hpp"
#include "db/TBLSelfTradeCtrlRange.hpp"
#include "db/TBLTrdSymbolList.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfAssets.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/PosInfo.hpp"
#include "def/SyncTask.hpp"
#include "util/BQUtil.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/ScheduleTaskBundle.hpp"
#include "util/Scheduler.hpp"
#include "util/StdExt.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

int TDSrv::prepareInit() {
  if (const auto ret = Config::get_mutable_instance().init(configFilename_);
      ret != 0) {
    const auto statusMsg = fmt::format("Prepare init failed.");
    std::cerr << statusMsg << std::endl;
    return ret;
  }

  if (const auto ret = InitLogger(configFilename_); ret != 0) {
    const auto statusMsg =
        fmt::format("Init td srv failed because of init logger failed. {}",
                    configFilename_);
    std::cerr << statusMsg << std::endl;
    return ret;
  }

  return 0;
}

int TDSrv::doInit() {
  if (const auto ret = initDBEng(); ret != 0) {
    LOG_E("Do init failed. ");
    return ret;
  }

  //! 如果仓位不正确，那么重建
  posMgrRestorer_ = std::make_shared<PosMgrRestorer>(this);
  if (const auto ret = posMgrRestorer_->exec(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  //! 从数据库获取pnl监控范围
  initPnlMonitorRangeGroup();

  //! 从数据库获取自成交控制范围
  initSelfTradeCtrlRangeGroup();

  //! 从数据库获取黑白名单
  initTrdSymbolListGroup();

  //! 从数据库获取流控规则
  initFlowCtrlRuleGroup();

  //! 从数据库获取代码表
  initTBLMonitorOfSymbolInfo();
  LOG_I("Begin to load symbol info table ...");
  if (const auto ret = tblMonitorOfSymbolInfo_->start(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  tdGWGroup_ = std::make_shared<ClientChannelGroup>("TDGW");
  stgEngGroup_ = std::make_shared<ClientChannelGroup>("StgEng");

  orderPreProc_ = std::make_shared<OrderPreProc>(this);
  if (const auto statusCode = orderPreProc_->init(); statusCode != 0) {
    return statusCode;
  }

  //! 初始化风控模组组合
  initRiskCtrlModuleComb();

  //! 初始化服务端
  initSHMSrv();

  riskCtrlConfMonitor_ = std::make_shared<RiskCtrlConfMonitor>(this);

  //! 创建计划任务模块
  scheduleTaskBundle_ = std::make_shared<ScheduleTaskBundle>();
  initScheduleTaskBundle();
  scheduleTaskBundleExecutor_ = std::make_shared<Scheduler>(
      std::string("TD_SRV"),
      [this]() { ExecScheduleTaskBundle(getScheduleTaskBundle()); }, 1 * 1000);

  return 0;
}

int TDSrv::initDBEng() {
  const auto dbEngParam = SetParam(db::DEFAULT_DB_ENG_PARAM,
                                   CONFIG["dbEngParam"].as<std::string>());
  int retOfMakeDBEng = 0;
  std::tie(retOfMakeDBEng, dbEng_) = db::MakeDBEng(
      dbEngParam, [](db::DBTaskSPtr& dbTask, const StringSPtr& dbExecRet) {
        LOG_D("Exec sql finished. [{}] [exec result = {}]", dbTask->toStr(),
              *dbExecRet);
      });
  if (retOfMakeDBEng != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfMakeDBEng;
  }

  if (auto retOfInit = getDBEng()->init(); retOfInit != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfInit;
  }

  return 0;
}

void TDSrv::initPnlMonitorRangeGroup() {
  const auto sql = fmt::format("SELECT * FROM {} where enable = 1; ",
                               TBLPnlMonitorRange::TableName);
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLPnlMonitorRange>::ExecSql(getDBEng(), sql);
  for (const auto& tblRec : *tblRecSet) {
    auto rec = tblRec.second;
    pnlMonitorRangeGroup_.emplace_back(rec->getRecWithAllFields());
  }
}

void TDSrv::initSelfTradeCtrlRangeGroup() {
  const auto sql = fmt::format("SELECT * FROM {} where enable = 1; ",
                               TBLSelfTradeCtrlRange::TableName);
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLSelfTradeCtrlRange>::ExecSql(getDBEng(), sql);
  for (const auto& tblRec : *tblRecSet) {
    auto rec = tblRec.second;
    selfTradeCtrlRangeGroup_.emplace_back(rec->getRecWithAllFields());
  }
}

void TDSrv::initTrdSymbolListGroup() {
  const auto sql = fmt::format("SELECT * FROM {} where enable = 1; ",
                               TBLTrdSymbolList::TableName);
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLTrdSymbolList>::ExecSql(getDBEng(), sql);
  for (const auto& tblRec : *tblRecSet) {
    auto rec = tblRec.second;
    trdSymbolListGroup_.emplace_back(rec->getRecWithAllFields());
  }
}

void TDSrv::initFlowCtrlRuleGroup() {
  const auto sql = fmt::format("SELECT * FROM {} where enable = 1; ",
                               TBLFlowCtrlRule::TableName);
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLFlowCtrlRule>::ExecSql(getDBEng(), sql);
  for (const auto& tblRec : *tblRecSet) {
    auto rec = tblRec.second;
    flowCtrlRuleGroup_.emplace_back(rec->getRecWithAllFields());
  }
}

void TDSrv::initTBLMonitorOfSymbolInfo() {
  const auto milliSecIntervalOfTBLMonitorOfSymbolInfo =
      CONFIG["milliSecIntervalOfTBLMonitorOfSymbolInfo"].as<std::uint32_t>();

  const auto tblMonitorOfSymbolInfo =
      CONFIG["tblMonitorOfSymbolInfo"].as<std::string>();

  const auto sql =
      fmt::format("SELECT * FROM {} where {};", TBLSymbolInfo::TableName,
                  tblMonitorOfSymbolInfo);

  const auto monitorSymbolTableChanges =
      CONFIG["monitorSymbolTableChanges"].as<bool>(true);
  const auto enableMonitoring = monitorSymbolTableChanges
                                    ? db::EnableMonitoring::True
                                    : db::EnableMonitoring::False;

  tblMonitorOfSymbolInfo_ = std::make_shared<db::TBLMonitorOfSymbolInfo>(
      getDBEng(), milliSecIntervalOfTBLMonitorOfSymbolInfo, sql, nullptr,
      enableMonitoring);
}

int TDSrv::initRiskCtrlModuleComb() {
  for (std::uint32_t no = 0; no < CONFIG["riskCtrlModuleComb"].size(); ++no) {
    auto riskCtrlModule = std::make_shared<RiskCtrlModule>(this, no);
    riskCtrlModuleComb_.emplace_back(riskCtrlModule);
    const auto ret = riskCtrlModule->init();
    if (ret != 0) {
      LOG_E("Do init failed. [ret = {}]", ret);
      return ret;
    }
  }
  return 0;
}

void TDSrv::initSHMSrv() {
  const auto tdGWAddr =
      fmt::format("{}@{}", AppName, CONFIG["tdGWChannel"].as<std::string>());
  shmSrvOfTDGW_ = std::make_shared<SHMSrv>(
      tdGWAddr, [this](const auto* shmBuf, std::size_t shmBufLen) {
        auto task = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
        orderPreProc_->handle(task);
      });

  const auto stgEngAddr =
      fmt::format("{}@{}", AppName, CONFIG["stgEngChannel"].as<std::string>());
  shmSrvOfStgEng_ = std::make_shared<SHMSrv>(
      stgEngAddr, [this](const auto* shmBuf, std::size_t shmBufLen) {
        auto task = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
        orderPreProc_->handle(task);
      });

  plugInChannel_ = CONFIG["plugInChannel"].as<std::string>();
  const auto plugInAddr = fmt::format("{}@{}", AppName, plugInChannel_);
  shmSrvOfPlugIn_ = std::make_shared<SHMSrv>(
      plugInAddr, [this](const auto* shmBuf, std::size_t shmBufLen) {
        const auto msgId = static_cast<const SHMHeader*>(shmBuf)->msgId_;
        LOG_W("Risk ctrl plug in recv msg {} - {}", msgId, GetMsgName(msgId));
      });

  topicOfTriggerRiskCtrl_ =
      fmt::format("{}{}TriggerRiskCrtl", plugInChannel_, SEP_OF_TOPIC);
}

void TDSrv::initScheduleTaskBundle() {
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "removeExpiredChannel",
      [this]() {
        getTDGWGroup()->removeExpiredChannel();
        getStgEngGroup()->removeExpiredChannel();
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(5000)));

  const auto milliSecIntervalOfSyncTask =
      CONFIG["milliSecIntervalOfSyncTask"].as<std::uint32_t>();
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "syncTask",
      [this]() {
        handleSyncTaskGroup();
        return true;
      },
      ExecAtStartup::False, milliSecIntervalOfSyncTask));

  const auto milliSecIntervalRiskCtrlConfMonitor =
      CONFIG["milliSecIntervalRiskCtrlConfMonitor"].as<std::uint32_t>(5000);
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "riskCtrlConfMonitor ",
      [this]() {
        riskCtrlConfMonitor_->check();
        return true;
      },
      ExecAtStartup::False, milliSecIntervalRiskCtrlConfMonitor));
}

int TDSrv::doRun() {
  getDBEng()->start();

  orderPreProc_->start();
  for (auto& riskCtrlModule : riskCtrlModuleComb_) {
    riskCtrlModule->start();
  }

  shmSrvOfTDGW_->start();
  shmSrvOfStgEng_->start();
  shmSrvOfPlugIn_->start();

  if (const auto ret = scheduleTaskBundleExecutor_->start(); ret != 0) {
    LOG_E("Start scheduler of multi task failed.");
    return ret;
  }

  return 0;
}

void TDSrv::cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                               SyncToRiskMgr syncToRiskMgr, SyncToDB syncToDB) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    syncTaskGroup_.emplace_back(
        std::make_shared<SyncTask>(msgId, task, syncToRiskMgr, syncToDB));
  }
}

void TDSrv::handleSyncTaskGroup() {
  std::vector<SyncTaskSPtr> taskGroup;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    std::swap(taskGroup, syncTaskGroup_);
  }

  if (taskGroup.size() > 100) {
    LOG_W("Too many unprocessed task of sync. [num = {}]", taskGroup.size());
  }

  for (const auto& rec : taskGroup) {
    if (rec->syncToDB_ == SyncToDB::False) continue;

    if (rec->msgId_ == MSG_ID_ON_ORDER ||  //
        rec->msgId_ == MSG_ID_ON_ORDER_RET ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      const auto orderInfo = std::any_cast<OrderInfoSPtr>(rec->task_);
      const auto identity = GET_RAND_STR();
      const auto sql = orderInfo->getSqlOfUSPOrderInfoUpdate();
      const auto [ret, execRet] = getDBEng()->asyncExec(identity, sql);
      if (ret != 0) {
        LOG_W("Sync order info to db failed. [{}]", sql);
      }

    } else if (rec->msgId_ == MSG_ID_SYNC_POS_INFO) {
      const auto posChgInfo = std::any_cast<PosChgInfoSPtr>(rec->task_);
      for (const auto& posInfo : *posChgInfo) {
        const auto identity = GET_RAND_STR();
        const auto sql = posInfo->getSqlOfReplace();
        const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
        if (ret != 0) {
          LOG_W("Replace pos info from db failed. [{}]", sql);
        }
      }

    } else {
      LOG_W("Unhandled task of sync to db. {} - {}", rec->msgId_,
            GetMsgName(rec->msgId_))
    }
  }
}

void TDSrv::doExit(const boost::system::error_code* ec, int signalNum) {
  scheduleTaskBundleExecutor_->stop();

  shmSrvOfPlugIn_->stop();
  shmSrvOfStgEng_->stop();
  shmSrvOfTDGW_->stop();

  for (auto& tdSrvTaskDispatcher : riskCtrlModuleComb_) {
    tdSrvTaskDispatcher->stop();
  }
  orderPreProc_->stop();

  tblMonitorOfSymbolInfo_->stop();

  getDBEng()->stop();
}

std::vector<db::pnlMonitorRange::RecordSPtr> TDSrv::getPnlMonitorRangeMgrGroup(
    const std::string& step) const {
  std::vector<db::pnlMonitorRange::RecordSPtr> ret;
  {
    std::lock_guard<std::mutex> guard(mtxPnlMonitorRangeGroup_);
    for (const auto& rec : pnlMonitorRangeGroup_) {
      if (rec->step != step) continue;
      ret.emplace_back(rec);
    }
  }
  return ret;
}

std::vector<db::selfTradeCtrlRange::RecordSPtr>
TDSrv::getSelfTradeCtrlRangeMgrGroup(const std::string& step) const {
  std::vector<db::selfTradeCtrlRange::RecordSPtr> ret;
  {
    std::lock_guard<std::mutex> guard(mtxSelfTradeCtrlRangeGroup_);
    for (const auto& rec : selfTradeCtrlRangeGroup_) {
      if (rec->step != step) continue;
      ret.emplace_back(rec);
    }
  }
  return ret;
}

std::vector<db::trdSymbolList::RecordSPtr> TDSrv::getTrdSymbolListGroup(
    const std::string& step) const {
  std::vector<db::trdSymbolList::RecordSPtr> ret;
  {
    std::lock_guard<std::mutex> guard(mtxTrdSymbolListGroup_);
    for (const auto& rec : trdSymbolListGroup_) {
      if (rec->step != step) continue;
      ret.emplace_back(rec);
    }
  }
  return ret;
}

std::vector<db::flowCtrlRule::RecordSPtr> TDSrv::getFlowCtrlRuleGroup(
    const std::string& step) const {
  std::vector<db::flowCtrlRule::RecordSPtr> ret;
  {
    std::lock_guard<std::mutex> guard(mtxFlowCtrlRuleGroup_);
    for (const auto& rec : flowCtrlRuleGroup_) {
      if (rec->step != step) continue;
      ret.emplace_back(rec);
    }
  }
  return ret;
}

}  // namespace bq::td::srv
