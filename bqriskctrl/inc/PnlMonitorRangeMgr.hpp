/*!
 * \file PnlMonitorRangeMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "PnlMonitorDef.hpp"
#include "RiskCtrlDataMgr.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq::td::srv {
class TDSrv;
}

namespace bq {

class PosSnapshot;
using PosSnapshotSPtr = std::shared_ptr<PosSnapshot>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class SubMgr;
using SubMgrSPtr = std::shared_ptr<SubMgr>;

using Key2PnlMonitorRangeGroup = std::map<std::string, PnlMonitorRangeSPtr>;

struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;

using Condition2PnlGroup = std::map<std::string, PnlSPtr>;
using Condition2PnlGroupSPtr = std::shared_ptr<Condition2PnlGroup>;

class PnlMonitorRangeMgr : public RiskCtrlDataMgr {
 public:
  PnlMonitorRangeMgr(const PnlMonitorRangeMgr&) = delete;
  PnlMonitorRangeMgr& operator=(const PnlMonitorRangeMgr&) = delete;
  PnlMonitorRangeMgr(const PnlMonitorRangeMgr&&) = delete;
  PnlMonitorRangeMgr& operator=(const PnlMonitorRangeMgr&&) = delete;

  explicit PnlMonitorRangeMgr(const std::string& step, std::uint32_t threadNo,
                              const std::string& fieldGroupUsedToGenHash);
  ~PnlMonitorRangeMgr();

 public:
  std::tuple<int, std::string> init(
      const std::vector<db::pnlMonitorRange::RecordSPtr>& recSet);

 private:
  std::tuple<int, std::string> handleCurPnlMonitorRec(
      const db::pnlMonitorRange::RecordSPtr& rec);

 private:
  void initSubMgr();
  void onDataRecv(const void* shmBuf, std::size_t shmBufLen);

  Condition2PnlGroupSPtr initCondition2PnlGroup();
  void initPnlInCondition2PnlGroup(Condition2PnlGroupSPtr& condition2PnlGroup);
  void updateCondition2PnlGroup(
      const Condition2PnlGroupSPtr& condition2PnlGroup);

  PnlSPtr getPnl(const std::string& condition);

 public:
  std::tuple<int, std::string> checkIfTriggerRiskCtrl(
      const OrderInfoSPtr& orderInfo, std::uint32_t secDelayOfPrice);

 private:
  std::tuple<int, std::string> handleTableChgTypeOfAdd(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfDel(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfChg(const Doc& doc);

  db::pnlMonitorRange::RecordSPtr makeRec(const Doc& doc);

 private:
  td::srv::TDSrv* tdSrv_{nullptr};

  // clang-format off
  //! 
  //! 核心逻辑：
  //! 
  //! 收到订单之后遍历 Key2PnlMonitorRangeGroup，找出触发风控的condition，再根据 
  //! condition 也就是 "acctId=10000&trdAcctId=100000", 从 condition2PnlGroup_ 
  //! 找到 pnl
  //! 
  //! key 是 "acctId&trdAcctId" key 就是创建的时候用到，用来归集同一类风控范围
  //! val 是 PnlMonitorRangeSPtr 
  //!
  //! PnlMonitorRange
  //! {
  //!   conditionGroupInStrFmt_ : "acctId&trdAcctId" ,
  //!   conditionFieldGroup_ : ["acctId", "trdAcctId"]
  //!   hash2PnlThresholdGroup_ : {
  //!                               {
  //!                                 hash of "acctId=10000&trdAcctId=100000", 
  //!                                 {"acctId=10000&trdAcctId=100000", pnlType, limitValue}
  //!                               }, ...
  //!                             }
  //!
  // clang-format on
  Key2PnlMonitorRangeGroup key2PnlMonitorRangeGroup_;
  std::ext::spin_mutex mtxKey2PnlMonitorRangeGroup_;

  //! 来了一条行情之后结合key2PnlMonitorRangeGroup_初始化conditionFieldGroup_，
  //! 然后再填充其中的pnl用于风控
  Condition2PnlGroupSPtr condition2PnlGroup_{nullptr};
  std::ext::spin_mutex mtxCondition2PnlGroup_;

  SubMgrSPtr subMgr_{nullptr};
  PosSnapshotSPtr posSnapshot_{nullptr};
};

}  // namespace bq
