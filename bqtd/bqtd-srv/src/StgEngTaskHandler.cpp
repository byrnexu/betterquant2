/*!
 * \file StgEngTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgEngTaskHandler.hpp"

#include "ClientChannelGroup.hpp"
#include "CommonIPCData.hpp"
#include "FlowCtrlRuleMgr.hpp"
#include "OrdMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCTask.hpp"
#include "SHMIPCTopicName.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginMgr.hpp"
#include "TDSrvUtil.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "util/Datetime.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/Util.hpp"

namespace bq::td::srv {

StgEngTaskHandler::StgEngTaskHandler(TDSrv* tdSrv, std::uint32_t no)
    : tdSrv_(tdSrv), no_(no) {}

void StgEngTaskHandler::handleAsyncTask(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_RISK_CTRL_CONF_CHG:
      handleMsgIdOnRiskCtrlConfChg(asyncTask);
      break;
    case MSG_ID_ON_ORDER:
      handleMsgIdOnOrder(asyncTask);
      break;
    case MSG_ID_ON_CANCEL_ORDER:
      handleMsgIdOnCancelOrder(asyncTask);
      break;
    case MSG_ID_ON_STG_REG:
      handleMsgIdOnStgReg(asyncTask);
      break;
    default:
      LOG_W("[{}] Unable to process msgId {}.", no_, shmHeader->msgId_);
      break;
  }
}

void StgEngTaskHandler::handleMsgIdOnRiskCtrlConfChg(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto threadNo = std::ext::tls_get<ThreadInfo>().no_;
  /*
   *  默认CopyIPCData::False的情形asyncTask->task_的内存会被移动到commonIPCData，
   *  所以asyncTask被 投递到多个线程的时候就会异常，因此这里CopyIPCData为True。
   */
  auto commonIPCData =
      MakeMsgSPtrByTask<CommonIPCData>(asyncTask->task_, CopyIPCData::True);
  const auto conf = static_cast<const char*>(commonIPCData->data_);
  tdSrv_->getRiskCtrlModuleComb()[no_]
      ->getTDSrvRiskPluginMgr()
      ->onRiskCtrlConfChg(conf, no_, threadNo);

  if (!tdSrv_->isLastRiskCtrlModule(no_)) {
    //! 因为投递用的dispatchToAllThread所以第一个线程投递即可，不然会重复投递
    if (threadNo == 0) {
      auto task = std::make_shared<SHMIPCTask>(
          commonIPCData.get(), sizeof(CommonIPCData) + commonIPCData->dataLen_);
      auto asyncTask = std::make_shared<SHMIPCAsyncTask>(task, std::any());
      tdSrv_->getRiskCtrlModuleComb()[no_ + 1]
          ->getTDSrvTaskDispatcher()
          ->dispatchToAllThread(asyncTask);
    }
  }
}

void StgEngTaskHandler::handleMsgIdOnOrder(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  auto ordReq = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);
#ifndef OPT_LOG
  LOG_I("[{}] Recv order {}", no_, ordReq->toShortStr());
#endif

  if (tdSrv_->isFirstRiskCtrlModule(no_)) {
    if (tdSrv_->getTDGWGroup()->exists(ordReq->acctId_) == false) {
      LOG_W("[{}] Handle msg id on order failed. {}", no_,
            ordReq->toShortStr());
      ordReq->orderStatus_ = OrderStatus::Failed;
      ordReq->statusCode_ = SCODE_TD_SRV_TDGW_NOT_EXISTS;

      //! 发送废单信息给策略引擎
      tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
          [&, this](void* shmBuf) {
            InitMsgBodyExt(shmBuf, *ordReq);
            LOG_W("[{}] Forward order ret {}", no_,
                  static_cast<OrderInfo*>(shmBuf)->toShortStr());
          },
          ordReq->stgId_, MSG_ID_ON_ORDER_RET, ordReq->size());

      tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq,
                                 SyncToRiskMgr::False, SyncToDB::True);
      return;
    }
  }

  /*
   *
   * TDSrv 收到报单请求的时候，由于发起报单请求的时候，在 StgEng 已经将该订单入
   * 库，如果临时创建线程，在线程启动的时候OrdMgr加载未完结订单，此订单也会被加
   * 载，但是因为收到报单请求会调用 OrdMgr::Add，这时就会 Add 失败，所以增加了
   * 这个配置参数 preCreateTaskSpecificThreadPool_，事先创建好线程并加载未完结
   * 订单。
   *
   */

  const auto threadNo = std::ext::tls_get<ThreadInfo>().no_;
  const auto ret = tdSrv_->getRiskCtrlModuleComb()[no_]
                       ->getOrdMgrGroup()[threadNo]
                       ->add<LockFunc::False, DeepClone::False>(ordReq);
  if (ret != 0) {
    LOG_W("[{}] Handle msg id on order failed. {}", no_, ordReq->toShortStr());
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = ret;

    //! 发送废单信息给策略引擎
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&, this](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
          LOG_I("[{}] Forward order ret {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        ordReq->stgId_, MSG_ID_ON_ORDER_RET, ordReq->size());

    tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq,
                               SyncToRiskMgr::False, SyncToDB::True);
    return;
  }

  //! 风控插件轮流检查
  const auto [statusCode, details] =
      tdSrv_->getRiskCtrlModuleComb()[no_]->getTDSrvRiskPluginMgr()->onOrder(
          ordReq, no_, threadNo);

  if (statusCode != 0) {
    LOG_W("[{}] Risk check order failed. [{} - {}] {}", no_, statusCode,
          GetStatusMsg(statusCode), ordReq->toShortStr());
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = statusCode;

    //! 发送废单信息给策略引擎
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
          LOG_I("[{}] Forward order ret {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        ordReq->stgId_, MSG_ID_ON_ORDER_RET, ordReq->size());

    //! PubTopic
    pubTopicOfTriggerRiskCtrl(details, ordReq);

    //! 回滚风控内部状态
    tdSrv_->getRiskCtrlModuleComb()[no_]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->batchRollback();
    LOG_T("[{}] Batch rollback risk ctrl status generate by order {}", no_,
          ordReq->toShortStr());

    //! 移除OrdMgr中的订单
    tdSrv_->getRiskCtrlModuleComb()[no_]
        ->getOrdMgrGroup()[threadNo]
        ->remove<LockFunc::False>(ordReq->orderId_);

    //! cache task of sync group
    tdSrv_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq,
                               SyncToRiskMgr::False, SyncToDB::True);
    return;
  }

  if (tdSrv_->isLastRiskCtrlModule(no_)) {
    tdSrv_->getSHMSrvOfTDGW()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
          LOG_I("[{}] Forward order {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        ordReq->acctId_, MSG_ID_ON_ORDER, ordReq->size());

  } else {
    auto task = std::make_shared<SHMIPCTask>(ordReq.get(), ordReq->size());
    tdSrv_->getRiskCtrlModuleComb()[no_ + 1]
        ->getTDSrvTaskDispatcher()
        ->dispatch(task);
  }

  //! 批量更新风控状态变化，优先发送报单
  tdSrv_->getRiskCtrlModuleComb()[no_]
      ->getRiskCtrlStatusUpdatersGroup()[threadNo]
      ->batchExecute();
  LOG_T("[{}] Batch update risk ctrl status generate by order {}", no_,
        ordReq->toShortStr());

//! Order Total num: 100; avg: 24.54000; med: 23; min: 15; max: 105
#ifdef PERF_TEST
  EXEC_PERF_TEST("Order", ordReq->orderTime_, 100, 10);
#endif
}

void StgEngTaskHandler::handleMsgIdOnCancelOrder(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  auto ordReq = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);

  if (tdSrv_->isFirstRiskCtrlModule(no_)) {
    if (tdSrv_->getTDGWGroup()->exists(ordReq->acctId_) == false) {
      LOG_W("[{}] Handle msg id on cancel order failed. {}", no_,
            ordReq->toShortStr());
      ordReq->statusCode_ = SCODE_TD_SRV_TDGW_NOT_EXISTS;

      //! 发送撤单失败信息给策略引擎
      tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
          [&](void* shmBuf) {
            InitMsgBodyExt(shmBuf, *ordReq);
            LOG_I("[{}] Forward cancel order ret {}", no_,
                  static_cast<OrderInfo*>(shmBuf)->toShortStr());
          },
          ordReq->stgId_, MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());

      return;
    }
  }

  //! 风控插件轮流检查
  const auto threadNo = std::ext::tls_get<ThreadInfo>().no_;
  const auto [statusCode, details] = tdSrv_->getRiskCtrlModuleComb()[no_]
                                         ->getTDSrvRiskPluginMgr()
                                         ->onCancelOrder(ordReq, no_, threadNo);
  if (statusCode != 0) {
    LOG_W("[{}] Risk check cancel order failed. [{} - {}] {}", no_, statusCode,
          GetStatusMsg(statusCode), ordReq->toShortStr());
    ordReq->statusCode_ = statusCode;

    //! 发送撤单失败信息给策略引擎
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
          LOG_I("[{}] Forward cancel order ret {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        ordReq->stgId_, MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());

    //! PubTopic
    pubTopicOfTriggerRiskCtrl(details, ordReq);

    //! 回滚订单状态变化
    tdSrv_->getRiskCtrlModuleComb()[no_]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->batchRollback();
    LOG_T("[{}] Batch rollback risk ctrl status generate by order {}", no_,
          ordReq->toShortStr());

    return;
  }

  if (tdSrv_->isLastRiskCtrlModule(no_)) {
    tdSrv_->getSHMSrvOfTDGW()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
          LOG_I("[{}] Forward cancel order {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        ordReq->acctId_, MSG_ID_ON_CANCEL_ORDER, ordReq->size());

  } else {
    auto task = std::make_shared<SHMIPCTask>(ordReq.get(), ordReq->size());
    tdSrv_->getRiskCtrlModuleComb()[no_ + 1]
        ->getTDSrvTaskDispatcher()
        ->dispatch(task);
  }

  //! 批量更新风控状态变化，优先发送撤单应答
  tdSrv_->getRiskCtrlModuleComb()[no_]
      ->getRiskCtrlStatusUpdatersGroup()[threadNo]
      ->batchExecute();
  LOG_T("[{}] Batch update risk ctrl status generate by order {}", no_,
        ordReq->toShortStr());
}

void StgEngTaskHandler::handleMsgIdOnStgReg(
    const AsyncTaskSPtr<SHMIPCTaskSPtr>& asyncTask) {
  const auto reqHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  LOG_D("[{}] Recv msg {}. [channel = {}]", no_, GetMsgName(reqHeader->msgId_),
        reqHeader->clientChannel_);

  tdSrv_->getStgEngGroup()->update(reqHeader->clientChannel_);
  tdSrv_->getSHMSrvOfStgEng()->sendRspWithZeroCopy([&](void* shmBuf) {},
                                                   reqHeader, sizeof(StgReg));
}

void StgEngTaskHandler::pubTopicOfTriggerRiskCtrl(const std::string& details,
                                                  const OrderInfoSPtr& order) {
  const auto& topicPrefix = tdSrv_->getTopicOfTriggerRiskCtrl();

  //! RISK@PlugInChannel@Trade@TriggerRiskCrtl@StgId@10000
  const auto topicOfStg = fmt::format(
      "{}{}StgId{}{}", topicPrefix, SEP_OF_TOPIC, SEP_OF_TOPIC, order->stgId_);
  PubTopic(tdSrv_->getSHMSrvOfPlugIn(), TOPIC_NAME_TRIGGER_RISK_CTRL,
           topicOfStg, details);

  //! RISK@PlugInChannel@Trade@TriggerRiskCrtl@StgId@10000@StgInstId@1
  const auto topicOfStgInst =
      fmt::format("{}{}StgId{}{}StgInstId{}{}", topicPrefix, SEP_OF_TOPIC,
                  SEP_OF_TOPIC, order->stgId_, SEP_OF_TOPIC, order->stgInstId_);
  PubTopic(tdSrv_->getSHMSrvOfPlugIn(), TOPIC_NAME_TRIGGER_RISK_CTRL,
           topicOfStgInst, details);

  //! RISK@PlugInChannel@Trade@TriggerRiskCrtl@AcctId@10000
  const auto topicOfAcct =
      fmt::format("{}{}AcctId{}{}", topicPrefix, SEP_OF_TOPIC, SEP_OF_TOPIC,
                  order->acctId_);
  PubTopic(tdSrv_->getSHMSrvOfPlugIn(), TOPIC_NAME_TRIGGER_RISK_CTRL,
           topicOfAcct, details);

  //! RISK@PlugInChannel@Trade@TriggerRiskCrtl@AcctId@10000@TrdAcctId@1
  const auto topicOfTrdAcct = fmt::format(
      "{}{}AcctId{}{}TrdAcctId{}{}", topicPrefix, SEP_OF_TOPIC, SEP_OF_TOPIC,
      order->acctId_, SEP_OF_TOPIC, order->trdAcctId_);
  PubTopic(tdSrv_->getSHMSrvOfPlugIn(), TOPIC_NAME_TRIGGER_RISK_CTRL,
           topicOfTrdAcct, details);
}

}  // namespace bq::td::srv
