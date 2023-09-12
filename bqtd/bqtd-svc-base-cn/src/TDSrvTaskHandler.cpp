/*!
 * \file TDSrvTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSrvTaskHandler.hpp"

#include "AssetsMgr.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "SHMIPC.hpp"
#include "SimedOrderInfoHandler.hpp"
#include "TDGateway.hpp"
#include "TDSvcDef.hpp"
#include "TDSvcOfCN.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/OrderInfoIF.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/SyncTask.hpp"
#include "util/Datetime.hpp"
#include "util/ExceedFlowCtrlHandler.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Logger.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/TrdSymbolCache.hpp"

namespace bq::td::svc {

TDSrvTaskHandler::TDSrvTaskHandler(TDSvcOfCN* tdSvc) : tdSvc_(tdSvc) {
  isSimedMode_ = CONFIG["simedMode"]["enabled"].as<bool>(false);
}

void TDSrvTaskHandler::handleAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_ORDER:
      handleMsgIdOnOrder(asyncTask);
      break;
    case MSG_ID_ON_CANCEL_ORDER:
      handleMsgIdOnCancelOrder(asyncTask);
      break;

    case MSG_ID_SYNC_UNCLOSED_ORDER_INFO:
      handleMsgIdSyncUnclosedOrderInfo(asyncTask);
      break;
    case MSG_ID_SYNC_ASSETS_SNAPSHOT:
      handleMsgIdSyncAssetsSnapshot(asyncTask);
      break;

    case MSG_ID_ON_TDGW_REG:
      handleMsgIdOnTDGWReg(asyncTask);
      break;

    case MSG_ID_ON_TEST_ORDER:
      handleMsgIdTestOrder(asyncTask);
      break;
    case MSG_ID_ON_TEST_CANCEL_ORDER:
      handleMsgIdTestCancelOrder(asyncTask);
      break;

    default:
      LOG_W("Unable to process msgId {}.", shmHeader->msgId_);
      break;
  }
}

void TDSrvTaskHandler::handleMsgIdOnOrder(SHMIPCAsyncTaskSPtr& asyncTask) {
  //! 获取下单请求
  auto ordReq = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);
#ifndef OPT_LOG
  LOG_I("Recv order {}", ordReq->toShortStr());
#endif

  //! 检查是否超流控
  bool exceedFlowCtrl =
      tdSvc_->getFlowCtrlSvc()->exceedFlowCtrl(GetMsgName(MSG_ID_ON_ORDER));
  if (exceedFlowCtrl) {
    LOG_W("Order exceed flow ctrl. {}", ordReq->toShortStr());

    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = SCODE_TD_SVC_EXCEED_FLOW_CTRL;

    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_ORDER_RET, ordReq->size());

    //! ordReq只在当前线程被使用，因此无需DeepClone::True
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq, SyncToRiskMgr::True,
                               SyncToDB::True);
    return;
  }

  //! 维护交易过的代码列表，写入数据库过程是同步，因此某一个品种第一次报单会慢一些
  tdSvc_->getTrdSymbolCache()->add(ordReq, SyncToDB::True);

  //!
  //! 如果这里不是DeepClone::True，那么假如syncToDB之前卡住，后面回报回来其他
  //! 线程正在修改ordReq的状态，此时入库，可能会不正常。如果是DeepClone::True
  //! 此时其他回报线程将订单状态完结并入库，这时下面的syncToDB不再卡住，这时
  //! 数据库中订单的状态就会从完结变成Pending，所以syncToDB也需要一个状态检查
  //! 因为这个过程是异步的。
  //!
  //! 另外这个add动作一定要在报单请求的前面，不然回报回来太快的话OrdMgr::add还
  //! 没有调用，回报在OrdMgr中找不到报单记录。
  //!
  ordReq->orderStatus_ = OrderStatus::Pending;
  if (const auto ret =
          tdSvc_->getOrdMgr()->add<LockFunc::True, DeepClone::True>(ordReq);
      ret != 0) {
    LOG_W("Handle op order failed. {}", ordReq->toShortStr());
    return;
  } else {
    LOG_I("OrdMgr add order. {}", ordReq->toShortStr());
  }

  //! 发送Pending状态给TDSrv
  tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        InitMsgBodyExt(shmBuf, *ordReq);
#ifndef OPT_LOG
        LOG_I("Send order ret. {}",
              static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
      },
      MSG_ID_ON_ORDER_RET, ordReq->size());

  //!
  //! 发送Pending状态给RiskMgr，后面下单失败的话订单字段值会变化，因此这里要复制
  //! 一个副本。
  //!
  tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET,
                             std::make_shared<OrderInfo>(*ordReq),
                             SyncToRiskMgr::True, SyncToDB::True);

  if (!isSimedMode_) {
    //! 如果是实盘模式
    handleMsgIdOnOrderInRealTDMode(ordReq);
  } else {
    //! 如果是仿真模式
    handleMsgIdOnOrderInSimedTDMode(ordReq);
  }

//! Order Total num: 100; avg: 44.64000; med: 41; min: 26; max: 214
#ifdef PERF_TEST
  EXEC_PERF_TEST("Order", ordReq->orderTime_, 100, 10);
  return;
#endif
}

void TDSrvTaskHandler::handleMsgIdOnCancelOrder(
    SHMIPCAsyncTaskSPtr& asyncTask) {
  //! 获取撤单请求
  auto ordReq = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);
  LOG_I("Recv cancel order {}", ordReq->toShortStr());

  //! 检查是否超流控
  bool exceedFlowCtrl = tdSvc_->getFlowCtrlSvc()->exceedFlowCtrl(
      GetMsgName(MSG_ID_ON_CANCEL_ORDER));
  if (exceedFlowCtrl) {
    LOG_W("Cancel order exceed flow ctrl. {}", ordReq->toShortStr());
    ordReq->statusCode_ = SCODE_TD_SVC_EXCEED_FLOW_CTRL;

    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());

    // not sync to db
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER_RET, ordReq,
                               SyncToRiskMgr::True, SyncToDB::False);
    return;
  }

  if (!isSimedMode_) {
    //! 如果是实盘模式
    handleMsgIdOnCancelOrderInRealTDMode(ordReq);
  } else {
    //! 如果是仿真模式
    handleMsgIdOnCancelOrderInSimedTDMode(ordReq);
  }
}

void TDSrvTaskHandler::handleMsgIdOnOrderInRealTDMode(OrderInfoSPtr& ordReq) {
  //! 实盘环境收到模拟盘订单
  if (!ordReq->isRealOrder()) {
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = SCODE_TD_SVC_REAL_RECV_SIMED_ORDER;
    LOG_W("Handle order in real td mode failed. [{} - {}] {}",
          ordReq->statusCode_, GetStatusMsg(ordReq->statusCode_),
          ordReq->toShortStr());
    tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_ORDER_RET, ordReq->size());

    //! ordReq只在当前线程被使用，因此无需DeepClone::True
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq, SyncToRiskMgr::True,
                               SyncToDB::True);
    return;
  }

  //! 这个ordReq不会被其他线程异步处理，所以这里无需clone一个副本
  if (const auto [ret, exchOrderId] = tdSvc_->getTDGateway()->order(ordReq);
      ret != 0) {
    //! 下单失败
    LOG_W("Handle order in real td mode failed. {}", ordReq->toShortStr());
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = ret;
    tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);

    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_ORDER_RET, ordReq->size());

    //! ordReq只在当前线程被使用，因此无需DeepClone::True
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq, SyncToRiskMgr::True,
                               SyncToDB::True);
    return;

  } else {
    //!
    //! 目前后台都根据clientId找到OrderId来处理回报，所以这里不用急着记录
    //! exchOrderId，但是考虑没收到委托确认（目前在委托确认赋值exchOrderId）
    //! 的情况下，该订单没有exchOrderId，但是syncUnclosedOrderInfo需要根据
    //! exchOrderId来同步订单情况，因此在这里对订单的exchOrderId赋值。最后
    //! 还是决定这里不更新exchOrderId，因为这里没有将exchOrderId更新到数据
    //! 库的逻辑。
    //!
  }
}

void TDSrvTaskHandler::handleMsgIdOnCancelOrderInRealTDMode(
    OrderInfoSPtr& ordReq) {
  //! 这个ordReq不会被其他线程异步处理，所以这里无需clone一个副本
  if (const auto ret = tdSvc_->getTDGateway()->cancelOrder(ordReq); ret != 0) {
    //! 撤单失败
    LOG_W("Handle op cancel order failed. {}", ordReq->toShortStr());
    ordReq->statusCode_ = ret;

    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordReq);
          LOG_I("Send cancel order ret. {}",
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
        },
        MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());

    // not sync to db
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER_RET, ordReq,
                               SyncToRiskMgr::True, SyncToDB::False);
    return;

  } else {
    //! 撤单成功
  }
}

void TDSrvTaskHandler::handleMsgIdOnOrderInSimedTDMode(OrderInfoSPtr& ordReq) {
  //! 模拟环境收到实盘订单
  if (ordReq->isRealOrder()) {
    ordReq->orderStatus_ = OrderStatus::Failed;
    ordReq->statusCode_ = SCODE_TD_SVC_SIMED_RECV_REAL_ORDER;
    LOG_W("Handle order in simed td mode failed. [{} - {}] {}",
          ordReq->statusCode_, GetStatusMsg(ordReq->statusCode_),
          ordReq->toShortStr());
    tdSvc_->getOrdMgr()->remove<LockFunc::True>(ordReq->orderId_);
    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_ORDER_RET, ordReq->size());

    //! ordReq只在当前线程被使用，因此无需DeepClone::True
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, ordReq, SyncToRiskMgr::True,
                               SyncToDB::True);
    return;
  }

  tdSvc_->getSimedOrderInfoHandler()->simOnOrder(ordReq);
}

void TDSrvTaskHandler::handleMsgIdOnCancelOrderInSimedTDMode(
    OrderInfoSPtr& ordReq) {
  tdSvc_->getSimedOrderInfoHandler()->simOnCancelOrder(ordReq);
}

//! 处理同步未完结订单
void TDSrvTaskHandler::handleMsgIdSyncUnclosedOrderInfo(
    SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto tdSrvSignal = static_cast<TDSrvSignal*>(asyncTask->task_->data_);
  LOG_D("Recv msg {}. [channel = {}]",
        GetMsgName(tdSrvSignal->shmHeader_.msgId_),
        tdSrvSignal->shmHeader_.clientChannel_);

  //! 检查同步未完结订单是否超流控
  bool exceedFlowCtrl = tdSvc_->getFlowCtrlSvc()->exceedFlowCtrl(
      GetMsgName(MSG_ID_SYNC_UNCLOSED_ORDER_INFO));
  if (exceedFlowCtrl) {
    tdSvc_->getExceedFlowCtrlHandler()->saveExceedFlowCtrlTask(asyncTask);
    return;
  }

  tdSvc_->getTDGateway()->syncUnclosedOrderInfo(asyncTask);
}

//! 处理同步资产快照信息
void TDSrvTaskHandler::handleMsgIdSyncAssetsSnapshot(
    SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto tdSrvSignal = static_cast<TDSrvSignal*>(asyncTask->task_->data_);
  LOG_D("Recv msg {}. [channel = {}]",
        GetMsgName(tdSrvSignal->shmHeader_.msgId_),
        tdSrvSignal->shmHeader_.clientChannel_);

  //! 检查同步资产信息是否超流控
  bool exceedFlowCtrl = tdSvc_->getFlowCtrlSvc()->exceedFlowCtrl(
      GetMsgName(MSG_ID_SYNC_ASSETS_SNAPSHOT));
  if (exceedFlowCtrl) {
    tdSvc_->getExceedFlowCtrlHandler()->saveExceedFlowCtrlTask(asyncTask);
    return;
  }

  tdSvc_->getTDGateway()->syncAssetsSnapshot();
}

void TDSrvTaskHandler::handleMsgIdOnTDGWReg(SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto msgHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  LOG_D("Recv msg {}. [channel = {}]", GetMsgName(msgHeader->msgId_),
        msgHeader->clientChannel_);
}

void TDSrvTaskHandler::handleMsgIdTestOrder(SHMIPCAsyncTaskSPtr& asyncTask) {
  LOG_D("{} trigged.", __func__);
  tdSvc_->getTDGateway()->testOrder();
}

void TDSrvTaskHandler::handleMsgIdTestCancelOrder(
    SHMIPCAsyncTaskSPtr& asyncTask) {
  LOG_D("{} trigged.", __func__);
  tdSvc_->getTDGateway()->testCancelOrder();
}

}  // namespace bq::td::svc
