/*!
 * \file RiskCtrlConfMonitor.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/12
 *
 * \brief
 *
 * 1. 定时检测数据库变化
 * 2. 发现变化的话生成json格式的数据
 *
 */

#include "RiskCtrlConfMonitor.hpp"

#include "CommonIPCData.hpp"
#include "OrderPreProc.hpp"
#include "RiskCtrlModule.hpp"
#include "SHMIPCTask.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginConst.hpp"
#include "db/DBE.hpp"
#include "db/TBLAcctGrpOfSharedPos.hpp"
#include "db/TBLTrdAcctInfo.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

RiskCtrlConfMonitor::RiskCtrlConfMonitor(TDSrv* tdSrv)
    : tdSrv_(tdSrv),
      tblRecSetOfTrdSymbolList_(
          std::make_shared<db::TBLRecSet<TBLTrdSymbolList>>()),
      tblRecSetOfSelfTradeCtrlRange_(
          std::make_shared<db::TBLRecSet<TBLSelfTradeCtrlRange>>()),
      tblRecSetOfPnlMonitorRange_(
          std::make_shared<db::TBLRecSet<TBLPnlMonitorRange>>()),
      tblRecSetOfFlowCtrlRule_(
          std::make_shared<db::TBLRecSet<TBLFlowCtrlRule>>()) {}

void RiskCtrlConfMonitor::check() {
  ++checkTimes_;
  checkTrdSymbolList();
  checkSelfTradeCtrlRange();
  checkPnlMonitorRange();
  checkFlowCtrlRule();
  checkAcctInfo();
}

void RiskCtrlConfMonitor::checkTrdSymbolList() {
  LOG_D("Begin to check table {}", TBLTrdSymbolList::TableName);

  const auto sql = fmt::format("SELECT * FROM {} where enable = 1;",
                               TBLTrdSymbolList::TableName);

  const auto [ret, newTBLRecSet] =
      db::TBLRecSetMaker<TBLTrdSymbolList>::ExecSql(tdSrv_->getDBEng(), sql);

  if (newTBLRecSet->size() > 1000) {
    LOG_W("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLTrdSymbolList::TableName);
  } else {
    LOG_D("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLTrdSymbolList::TableName);
  }

  db::TBLRecSetSPtr<TBLTrdSymbolList> tblRecSetAdd;
  db::TBLRecSetSPtr<TBLTrdSymbolList> tblRecSetDel;
  db::TBLRecSetSPtr<TBLTrdSymbolList> tblRecSetChg;
  std::tie(tblRecSetAdd, tblRecSetDel, tblRecSetChg) =
      TBLRecSetCompare(newTBLRecSet, tblRecSetOfTrdSymbolList_);

  for (const auto& rec : *tblRecSetAdd) {
    if (checkTimes_ > 1) {
      LOG_I("Add {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_TRD_SYMBOL_LIST, TableChgType::Add,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetDel) {
    if (checkTimes_ > 1) {
      LOG_I("Del {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_TRD_SYMBOL_LIST, TableChgType::Del,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetChg) {
    if (checkTimes_ > 1) {
      LOG_I("Chg {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_TRD_SYMBOL_LIST, TableChgType::Chg,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  tblRecSetOfTrdSymbolList_ = newTBLRecSet;
}

void RiskCtrlConfMonitor::checkSelfTradeCtrlRange() {
  LOG_D("Begin to check table {}", TBLSelfTradeCtrlRange::TableName);

  const auto sql = fmt::format("SELECT * FROM {} where enable = 1;",
                               TBLSelfTradeCtrlRange::TableName);

  const auto [ret, newTBLRecSet] =
      db::TBLRecSetMaker<TBLSelfTradeCtrlRange>::ExecSql(tdSrv_->getDBEng(),
                                                         sql);

  if (newTBLRecSet->size() > 1000) {
    LOG_W("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLSelfTradeCtrlRange::TableName);
  } else {
    LOG_D("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLSelfTradeCtrlRange::TableName);
  }

  db::TBLRecSetSPtr<TBLSelfTradeCtrlRange> tblRecSetAdd;
  db::TBLRecSetSPtr<TBLSelfTradeCtrlRange> tblRecSetDel;
  db::TBLRecSetSPtr<TBLSelfTradeCtrlRange> tblRecSetChg;
  std::tie(tblRecSetAdd, tblRecSetDel, tblRecSetChg) =
      TBLRecSetCompare(newTBLRecSet, tblRecSetOfSelfTradeCtrlRange_);

  for (const auto& rec : *tblRecSetAdd) {
    if (checkTimes_ > 1) {
      LOG_I("Add {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_SELF_TRADE_CTRL, TableChgType::Add,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetDel) {
    if (checkTimes_ > 1) {
      LOG_I("Del {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_SELF_TRADE_CTRL, TableChgType::Del,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetChg) {
    if (checkTimes_ > 1) {
      LOG_I("Chg {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_SELF_TRADE_CTRL, TableChgType::Chg,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  tblRecSetOfSelfTradeCtrlRange_ = newTBLRecSet;
}

void RiskCtrlConfMonitor::checkPnlMonitorRange() {
  LOG_D("Begin to check table {}", TBLPnlMonitorRange::TableName);

  const auto sql = fmt::format("SELECT * FROM {} where enable = 1;",
                               TBLPnlMonitorRange::TableName);

  const auto [ret, newTBLRecSet] =
      db::TBLRecSetMaker<TBLPnlMonitorRange>::ExecSql(tdSrv_->getDBEng(), sql);

  if (newTBLRecSet->size() > 1000) {
    LOG_W("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLPnlMonitorRange::TableName);
  } else {
    LOG_D("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLPnlMonitorRange::TableName);
  }

  db::TBLRecSetSPtr<TBLPnlMonitorRange> tblRecSetAdd;
  db::TBLRecSetSPtr<TBLPnlMonitorRange> tblRecSetDel;
  db::TBLRecSetSPtr<TBLPnlMonitorRange> tblRecSetChg;
  std::tie(tblRecSetAdd, tblRecSetDel, tblRecSetChg) =
      TBLRecSetCompare(newTBLRecSet, tblRecSetOfPnlMonitorRange_);

  for (const auto& rec : *tblRecSetAdd) {
    if (checkTimes_ > 1) {
      LOG_I("Add {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_PNL_MONITOR, TableChgType::Add,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetDel) {
    if (checkTimes_ > 1) {
      LOG_I("Del {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_PNL_MONITOR, TableChgType::Del,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetChg) {
    if (checkTimes_ > 1) {
      LOG_I("Chg {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_PNL_MONITOR, TableChgType::Chg,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  tblRecSetOfPnlMonitorRange_ = newTBLRecSet;
}

void RiskCtrlConfMonitor::checkFlowCtrlRule() {
  LOG_D("Begin to check table {}", TBLFlowCtrlRule::TableName);

  const auto sql = fmt::format("SELECT * FROM {} where enable = 1;",
                               TBLFlowCtrlRule::TableName);

  const auto [ret, newTBLRecSet] =
      db::TBLRecSetMaker<TBLFlowCtrlRule>::ExecSql(tdSrv_->getDBEng(), sql);

  if (newTBLRecSet->size() > 1000) {
    LOG_W("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLFlowCtrlRule::TableName);
  } else {
    LOG_D("Get {} numbers of rec from table {}.", newTBLRecSet->size(),
          TBLFlowCtrlRule::TableName);
  }

  db::TBLRecSetSPtr<TBLFlowCtrlRule> tblRecSetAdd;
  db::TBLRecSetSPtr<TBLFlowCtrlRule> tblRecSetDel;
  db::TBLRecSetSPtr<TBLFlowCtrlRule> tblRecSetChg;
  std::tie(tblRecSetAdd, tblRecSetDel, tblRecSetChg) =
      TBLRecSetCompare(newTBLRecSet, tblRecSetOfFlowCtrlRule_);

  for (const auto& rec : *tblRecSetAdd) {
    if (checkTimes_ > 1) {
      LOG_I("Add {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_FLOW_CTRL_PLUS, TableChgType::Add,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetDel) {
    if (checkTimes_ > 1) {
      LOG_I("Del {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_FLOW_CTRL_PLUS, TableChgType::Del,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  for (const auto& rec : *tblRecSetChg) {
    if (checkTimes_ > 1) {
      LOG_I("Chg {}", rec.second->getJsonStrOfAllFields());
      notifyChgToRiskPlugin(PLUGIN_FLOW_CTRL_PLUS, TableChgType::Chg,
                            rec.second->getJsonStrOfAllFields());
    }
  }

  tblRecSetOfFlowCtrlRule_ = newTBLRecSet;
}

void RiskCtrlConfMonitor::notifyChgToRiskPlugin(
    const std::string& pluginName, TableChgType tableChgType,
    const std::string& jsonDataInDB) {
  auto notifyCont =
      fmt::format(R"({{"pluginName":"{}","tableChgType":"{}",)", pluginName,
                  magic_enum::enum_name(tableChgType));
  notifyCont.append(R"("data":)").append(jsonDataInDB).append("}");
  LOG_I("Try to notify risk ctrl plugin chg. {}", notifyCont);

  const auto commonIPCTask =
      MakeCommonIPCData(notifyCont, MSG_ID_ON_RISK_CTRL_CONF_CHG);
  auto task = std::make_shared<SHMIPCTask>(
      commonIPCTask, sizeof(CommonIPCData) + commonIPCTask->dataLen_,
      CopyIPCData::False);
  auto asyncTask = std::make_shared<SHMIPCAsyncTask>(task, std::any());
  tdSrv_->getRiskCtrlModuleComb()[0]
      ->getTDSrvTaskDispatcher()
      ->dispatchToAllThread(asyncTask);
}

void RiskCtrlConfMonitor::checkAcctInfo() {
  auto getUpateTime = [this](const std::string& tableName) -> double {
    const auto identity = GET_RAND_STR();

    const auto sql = fmt::format(
        "SELECT MAX(UNIX_TIMESTAMP(updateTime)) AS updateTime FROM {};",
        tableName);

    const auto [statusCode, execRet] =
        tdSrv_->getDBEng()->syncExec(identity, sql);

    Doc doc;
    doc.Parse(execRet.data());

    if (doc["recordSetGroup"].Size() == 0) {
      return 0;
    }

    if (doc["recordSetGroup"][0].Size() == 0) {
      return 0;
    }

    auto ret = doc["recordSetGroup"][0][0]["updateTime"].GetString();
    if (std::strlen(ret) == 0) ret = "0";
    return CONV(double, ret);
  };

  const auto updateTimeOfSharedPos =
      getUpateTime(TBLAcctGrpOfSharedPos::TableName);
  const auto updateTimeOfTrdAcct = getUpateTime(TBLTrdAcctInfo::TableName);
  const auto updateTime = std::max(updateTimeOfSharedPos, updateTimeOfTrdAcct);

  if (updateTimeOfSharedPos_ != updateTimeOfSharedPos ||
      updateTimeOfTrdAcct_ != updateTimeOfTrdAcct) {
    notifyChgToOrderPreProc();
    updateTimeOfSharedPos_ = updateTimeOfSharedPos;
    updateTimeOfTrdAcct_ = updateTimeOfTrdAcct;
  }
}

void RiskCtrlConfMonitor::notifyChgToOrderPreProc() {
  LOG_I("Try to notify account info chg. ");
  auto notifyCont = "{}";
  const auto commonIPCTask =
      MakeCommonIPCData(notifyCont, MSG_ID_ON_ACCT_INFO_CHG);
  auto task = std::make_shared<SHMIPCTask>(
      commonIPCTask, sizeof(CommonIPCData) + commonIPCTask->dataLen_,
      CopyIPCData::False);
  tdSrv_->getOrderPreProc()->handle(task);
}

}  // namespace bq::td::srv
