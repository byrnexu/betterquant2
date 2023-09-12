#include "OrderPreProc.hpp"

#include "Config.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "SHMIPC.hpp"
#include "TDSrv.hpp"
#include "db/TBLAcctGrpOfSharedPos.hpp"
#include "db/TBLOrderInfo.hpp"
#include "db/TBLPosInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLTrdAcctInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/BQUtil.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

OrderPreProc::OrderPreProc(TDSrv* tdSrv) : tdSrv_(tdSrv) {}

int OrderPreProc::init() {
  //! 初始化 taskDispatcherParam
  const auto param =
      CONFIG["orderPreProcTaskDispatcherParam"].as<std::string>();
  const auto paramInStrFmt = SetParam(DEFAULT_TASK_DISPATCHER_PARAM, param);
  const auto [ret, orderPreProcTaskDispatcherParam] =
      MakeTaskDispatcherParam(paramInStrFmt);
  if (ret != 0) {
    LOG_E("Init taskdispatcher failed. {}", paramInStrFmt);
    return ret;
  }

  const auto makeAsyncTask = [this](const auto& task) {
    const auto asyncTask = std::make_shared<SHMIPCAsyncTask>(task, std::any());
    return std::make_tuple(0, asyncTask);
  };

  //! 因为业务不复杂，所以用0号线程处理即可
  const auto getThreadForAsyncTask =
      [](const auto& asyncTask, auto taskSpecificThreadPoolSize) { return 0; };

  const auto handleAsyncTask = [this](auto& asyncTask) {
    const auto shmHeader =
        static_cast<const SHMHeader*>(asyncTask->task_->data_);
    switch (shmHeader->msgId_) {
      case MSG_ID_ON_ACCT_INFO_CHG:
        handleOnAcctInfoChg();
        break;
      case MSG_ID_ON_ORDER:
        handleOnOrder(asyncTask);
        break;
      case MSG_ID_ON_ORDER_RET:
        handleOnOrderRet(asyncTask);
        break;

      default:
        break;
    }
  };

  orderPreProcTaskDispatcher_ =
      std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
          orderPreProcTaskDispatcherParam, makeAsyncTask, getThreadForAsyncTask,
          handleAsyncTask);
  orderPreProcTaskDispatcher_->init();

  initPosMgr();
  initOrdMgr();
  initAcctInfo();

  return 0;
}

void OrderPreProc::initPosMgr() {
  LOG_I("Begin to init posmgr for order pre process.");
  const auto sql = fmt::format("SELECT * FROM {}", TBLPosInfo::TableName);
  posMgr_ = std::make_shared<OPPosMgr>();
  posMgr_->init(CONFIG, tdSrv_->getDBEng(), sql);
}

void OrderPreProc::initOrdMgr() {
  LOG_I("Begin to init ordmgr for order pre process.");
  const auto filled = magic_enum::enum_integer(OrderStatus::Filled);
  const auto sql = fmt::format("SELECT * FROM {} WHERE `orderStatus` < {}; ",
                               TBLOrderInfo::TableName, filled);
  ordMgr_ = std::make_shared<OPOrdMgr>();
  ordMgr_->init(CONFIG, tdSrv_->getDBEng(), sql);
}

void OrderPreProc::initAcctInfo() {
  LOG_I("Begin to init account info.");
  initAcctGrpOfSharedPos();
  initTrdAcctId2AcctId();
}

void OrderPreProc::initTrdAcctId2AcctId() {
  const auto sql = fmt::format("SELECT * FROM {} WHERE isDel = 0;",
                               TBLTrdAcctInfo::TableName);
  const auto [statusCode, tblRecSet] =
      db::TBLRecSetMaker<TBLTrdAcctInfo>::ExecSql(tdSrv_->getDBEng(), sql);
  if (statusCode != 0) {
    LOG_W("Init trd acct id to acct id info failed. {}", sql);
    return;
  }

  trdAcctId2AcctId_.clear();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    trdAcctId2AcctId_.emplace(rec->trdAcctId, rec->acctId);
  }

  acctId2TrdAcctId_.clear();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    const auto iter = acctId2TrdAcctId_.find(rec->acctId);
    if (iter == std::end(acctId2TrdAcctId_)) {
      acctId2TrdAcctId_[rec->acctId] = rec->trdAcctId;
    } else {
      if (rec->trdAcctId < iter->second) {
        acctId2TrdAcctId_[rec->acctId] = rec->trdAcctId;
      }
    }
  }

  LOG_I("Init trd acct id to acct id info success. [size = {}]",
        trdAcctId2AcctId_.size());
}

void OrderPreProc::initAcctGrpOfSharedPos() {
  const auto sql =
      fmt::format("SELECT * FROM {};", TBLAcctGrpOfSharedPos::TableName);
  const auto [statusCode, tblRecSet] =
      db::TBLRecSetMaker<TBLAcctGrpOfSharedPos>::ExecSql(tdSrv_->getDBEng(),
                                                         sql);
  if (statusCode != 0) {
    LOG_W("Init acct group of shared pos failed. {}", sql);
    return;
  }

  acctId2AcctGrpId_.clear();
  acctGrpId2AcctId_.clear();

  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    acctId2AcctGrpId_.emplace(rec->acctId, rec->acctGrpId);
    acctGrpId2AcctId_.emplace(rec->acctGrpId, rec->acctId);
  }
  LOG_I("Init acct group of shared pos success. [size = {}]",
        acctGrpId2AcctId_.size());
}

void OrderPreProc::start() { orderPreProcTaskDispatcher_->start(); }
void OrderPreProc::stop() { orderPreProcTaskDispatcher_->stop(); }

void OrderPreProc::handle(const SHMIPCTaskSPtr& task) {
  const auto shmHeader = static_cast<const SHMHeader*>(task->data_);
  //! 只有下单需要借仓处理，只有下单和回报才有可能影响到 OrdMgr 和 PosMgr
  if (shmHeader->msgId_ == MSG_ID_ON_ORDER ||
      shmHeader->msgId_ == MSG_ID_ON_ORDER_RET) {
    const auto marketCode =
        static_cast<const OrderInfo*>(task->data_)->marketCode_;
    if (IsCNMarketOfFutures(marketCode)) {
      orderPreProcTaskDispatcher_->dispatch(const_cast<SHMIPCTaskSPtr&>(task));
      return;
    }
  }

  //! 数据库里账户信息发生变化，重新加载
  if (shmHeader->msgId_ == MSG_ID_ON_ACCT_INFO_CHG) {
    orderPreProcTaskDispatcher_->dispatch(const_cast<SHMIPCTaskSPtr&>(task));
    return;
  }

  if (!tdSrv_->getRiskCtrlModuleComb().empty()) {
    tdSrv_->getRiskCtrlModuleComb()[0]->getTDSrvTaskDispatcher()->dispatch(
        const_cast<SHMIPCTaskSPtr&>(task));
  }
}

void OrderPreProc::handleOnAcctInfoChg() { initAcctInfo(); }

void OrderPreProc::handleOnOrder(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  auto order = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);

  //! 确定开平方向、账号和交易账号
  if (const auto statusCode = handleSharedPos(order); statusCode != 0) {
    return;
  }

  //! order必须有了正确的开平之后才能放入OrdMgr，这样OrdMgr才能正确的处理可借仓位，所以先调用
  //! handleSharedPos
  if (const auto statusCode = addToOrdMgr(order); statusCode != 0) {
    return;
  }

  //! 转发订单给风控处理模组组合
  if (!tdSrv_->getRiskCtrlModuleComb().empty()) {
    auto task = std::make_shared<SHMIPCTask>(order.get(), order->size());
    tdSrv_->getRiskCtrlModuleComb()[0]->getTDSrvTaskDispatcher()->dispatch(
        task);
  }
}

int OrderPreProc::handleSharedPos(OrderInfoSPtr& order) {
  //! 说明已经指定开平方向了，不需要再做借仓处理
  if (order->posDirection_ != PosDirection::Others) {
    return 0;
  }

  //! 检查交易账号所属账号有没有足够的可借仓位
  const auto posCanBeBorrowed = getPosCanBeBorrowed(order, order->acctId_);
  if (DEC::GE(posCanBeBorrowed, order->orderSize_)) {
    //! 如果有足够的可借仓位，那就借
    LOG_I("Account {} has {} pos for borrow. {}", order->acctId_,
          posCanBeBorrowed, order->toShortStr());
    order->posDirection_ = PosDirection::Close;
    return 0;
  }

  //! 先检查当前交易账号所属账号 order->acctId_ 是否有可借账户列表
  const auto iter = acctId2AcctGrpId_.find(order->acctId_);
  if (iter == std::end(acctId2AcctGrpId_)) {
    //! 如果没有可借账户列表，那么返回可借仓位不足
    LOG_W(
        "There are not enough available positions for borrowing because the "
        "account does not have shared pos. {}",
        order->toShortStr());
    order->orderStatus_ = OrderStatus::Failed;
    order->statusCode_ = SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW;

    //! 发送废单信息给策略引擎
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&, this](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *order);
          LOG_W("Forward order ret {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        order->stgId_, MSG_ID_ON_ORDER_RET, order->size());

    tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, order, SyncToRiskMgr::False,
                               SyncToDB::True);
    return order->statusCode_;
  }

  //! 如果当前交易账号所属账号有可借账户列表
  const auto acctGrpId = iter->second;
  //! 获取可解账户列表
  const auto range = acctGrpId2AcctId_.equal_range(acctGrpId);
  //! 遍历可借账户列表，看看哪个账户有足够的仓位可借
  for (auto iter = range.first; iter != range.second; ++iter) {
    const auto acctId = iter->second;
    //! order->acctId_ 是否有足够的可借仓位上面已经检查过了
    if (acctId == order->acctId_) continue;

    //! 获取当前账号 acctId 可借仓位
    const auto posCanBeBorrowed = getPosCanBeBorrowed(order, acctId);

    //! 当前账号 acctId 有足够的可借仓位
    if (DEC::GE(posCanBeBorrowed, order->orderSize_)) {
      //! 设定真正交易的账号和交易账号
      LOG_I("Account {} has {} pos for borrow. {}", acctId, posCanBeBorrowed,
            order->toShortStr());
      order->acctId_ = acctId;
      order->trdAcctId_ = acctId2TrdAcctId_[acctId];
      order->posDirection_ = PosDirection::Close;
      return 0;
    }
  }

  LOG_W(
      "All accounts with shared pos do not have enough available positions "
      "for borrowing or no account for borrow. {}",
      order->toShortStr());
  order->orderStatus_ = OrderStatus::Failed;
  order->statusCode_ = SCODE_TD_SRV_INSUFFICIENT_POS_FOR_BORROW;

  //! 发送废单信息给策略引擎
  tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
      [&, this](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *order);
        LOG_W("Forward order ret {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
      },
      order->stgId_, MSG_ID_ON_ORDER_RET, order->size());

  tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, order, SyncToRiskMgr::False,
                             SyncToDB::True);
  return order->statusCode_;
}

int OrderPreProc::addToOrdMgr(const OrderInfoSPtr& order) {
  const auto statusCode =
      ordMgr_->add<LockFunc::False, DeepClone::False>(order);

  if (statusCode != 0) {
    LOG_W("Handle msg id on order failed. {}", order->toShortStr());
    order->orderStatus_ = OrderStatus::Failed;
    order->statusCode_ = statusCode;

    //! 发送废单信息给策略引擎
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&, this](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *order);
          LOG_W("Forward order ret {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        order->stgId_, MSG_ID_ON_ORDER_RET, order->size());

    tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, order, SyncToRiskMgr::False,
                               SyncToDB::True);
    return statusCode;
  }

  return 0;
}

Decimal OrderPreProc::getPosCanBeBorrowed(OrderInfoSPtr& order, AcctId acctId) {
  const auto holdPos = posMgr_->getHoldPos<LockFunc::False>(order, acctId);
  if (DEC::ZERO(holdPos)) {
    return 0;
  } else if (DEC::GT(holdPos, 0.0)) {
    //! 说明有可用的多头头寸
    if (order->side_ == Side::Bid) {
      //! 但是目前要做多，需要平空，因此需要空头头寸
      return 0;
    }
  } else {
    // 说明有可用的空头头寸
    if (order->side_ == Side::Ask) {
      //! 但是目前要做空，需要平多，因此需要多头头寸
      return 0;
    }
  }
  const auto frozenPos = ordMgr_->getFrozenPos<LockFunc::False>(order, acctId);
  const auto posCanBeBorrowed = std::fabs(holdPos) - frozenPos;
  return posCanBeBorrowed;
}

void OrderPreProc::handleOnOrderRet(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  auto order = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);

  updateOrdAndPosMgr(order);
  if (!tdSrv_->getRiskCtrlModuleComb().empty()) {
    auto task = std::make_shared<SHMIPCTask>(order.get(), order->size());
    tdSrv_->getRiskCtrlModuleComb()[0]->getTDSrvTaskDispatcher()->dispatch(
        task);
  }
}

void OrderPreProc::updateOrdAndPosMgr(const OrderInfoSPtr& order) {
  //! 更新 PosMgr
  const auto [isTheOrderCanBeUsedCalcPos, orderInfoInOrdMgr] =
      ordMgr_->updateByOrderInfoFromTDGW<LockFunc::False>(order);
  if (isTheOrderCanBeUsedCalcPos == IsTheOrderCanBeUsedCalcPos::True) {
    LOG_I("Current order can be used to calc pos, begin to calc pos. {}",
          order->toShortStr());
    posMgr_->updateByOrderInfoFromTDGW<LockFunc::False>(order);
  }
}

}  // namespace bq::td::srv
