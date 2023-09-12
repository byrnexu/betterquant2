/*!
 * \file RiskCtrlConfMonitor.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/12
 *
 * \brief
 */

#pragma once

#include "db/TBLFlowCtrlRule.hpp"
#include "db/TBLPnlMonitorRange.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLSelfTradeCtrlRange.hpp"
#include "db/TBLTrdSymbolList.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::td::srv {

class TDSrv;

class RiskCtrlConfMonitor {
 public:
  RiskCtrlConfMonitor(const RiskCtrlConfMonitor&) = delete;
  RiskCtrlConfMonitor& operator=(const RiskCtrlConfMonitor&) = delete;
  RiskCtrlConfMonitor(const RiskCtrlConfMonitor&&) = delete;
  RiskCtrlConfMonitor& operator=(const RiskCtrlConfMonitor&&) = delete;

  explicit RiskCtrlConfMonitor(TDSrv* tdSrv);

 public:
  void check();

 private:
  void checkTrdSymbolList();
  void checkSelfTradeCtrlRange();
  void checkPnlMonitorRange();
  void checkFlowCtrlRule();
  void checkAcctInfo();

 private:
  void notifyChgToRiskPlugin(const std::string& pluginName,
                             TableChgType tableChgType,
                             const std::string& jsonDataInDB);
  void notifyChgToOrderPreProc();

 private:
  TDSrv* tdSrv_{nullptr};

  std::uint64_t checkTimes_{0};
  db::TBLRecSetSPtr<TBLTrdSymbolList> tblRecSetOfTrdSymbolList_;
  db::TBLRecSetSPtr<TBLSelfTradeCtrlRange> tblRecSetOfSelfTradeCtrlRange_;
  db::TBLRecSetSPtr<TBLPnlMonitorRange> tblRecSetOfPnlMonitorRange_;
  db::TBLRecSetSPtr<TBLFlowCtrlRule> tblRecSetOfFlowCtrlRule_;

  double updateTimeOfSharedPos_{0};
  double updateTimeOfTrdAcct_{0};
};

}  // namespace bq::td::srv
