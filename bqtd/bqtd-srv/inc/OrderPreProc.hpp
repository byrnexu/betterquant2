#pragma once

#include "TDSrvDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/OrderInfoIF.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"

namespace bq {

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class OrderPreProc {
 public:
  OrderPreProc(const OrderPreProc&) = delete;
  OrderPreProc& operator=(const OrderPreProc&) = delete;
  OrderPreProc(OrderPreProc&&) = delete;
  OrderPreProc& operator=(OrderPreProc&&) = delete;

  explicit OrderPreProc(TDSrv* tdSrv);

 public:
  int init();
  void start();
  void stop();

  void handle(const SHMIPCTaskSPtr& asyncTask);

 private:
  void initPosMgr();
  void initOrdMgr();

 private:
  void initAcctInfo();
  void initTrdAcctId2AcctId();
  void initAcctGrpOfSharedPos();

 private:
  void handleOnAcctInfoChg();

  void handleOnOrder(const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask);
  int handleSharedPos(OrderInfoSPtr& order);
  int addToOrdMgr(const OrderInfoSPtr& order);
  Decimal getPosCanBeBorrowed(OrderInfoSPtr& order, AcctId acctId);

  void handleOnOrderRet(const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask);
  void updateOrdAndPosMgr(const OrderInfoSPtr& order);

 private:
  TDSrv* tdSrv_{nullptr};

  OPPosMgrSPtr posMgr_{nullptr};
  OPOrdMgrSPtr ordMgr_{nullptr};

  std::map<TrdAcctId, AcctId> trdAcctId2AcctId_;
  std::map<AcctId, TrdAcctId> acctId2TrdAcctId_;

  std::map<AcctId, AcctGrpId> acctId2AcctGrpId_;
  std::multimap<AcctGrpId, AcctId> acctGrpId2AcctId_;

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block>
      orderPreProcTaskDispatcher_{nullptr};
};

}  // namespace bq::td::srv
