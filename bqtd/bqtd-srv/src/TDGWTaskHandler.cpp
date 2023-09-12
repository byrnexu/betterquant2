/*!
 * \file TDGWTaskHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDGWTaskHandler.hpp"

#include "AssetsMgr.hpp"
#include "ClientChannelGroup.hpp"
#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "RiskCtrlModule.hpp"
#include "RiskCtrlStatusUpdaters.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCTask.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "TDSrv.hpp"
#include "TDSrvRiskPluginMgr.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/SyncTask.hpp"
#include "util/Datetime.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::srv {

TDGWTaskHandler::TDGWTaskHandler(TDSrv* tdSrv, std::uint32_t no)
    : tdSrv_(tdSrv), no_(no) {}

void TDGWTaskHandler::handleAsyncTask(const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto shmHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_ORDER_RET:
      handleMsgIdOnOrderRet(asyncTask);
      break;
    case MSG_ID_ON_CANCEL_ORDER_RET:
      handleMsgIdOnCancelOrderRet(asyncTask);
      break;
    case MSG_ID_ON_TDGW_REG:
      handleMsgIdOnTDGWReg(asyncTask);
      break;
    default:
      LOG_W("[{}] Unable to process msgId {}.", no_, shmHeader->msgId_);
      break;
  }
}

void TDGWTaskHandler::handleMsgIdOnOrderRet(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto ordRet = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);
#ifndef OPT_LOG
  LOG_I("[{}] Recv order ret {}", no_, ordRet->toShortStr());
#endif

  //! 优先发送回报给策略
  if (tdSrv_->isFirstRiskCtrlModule(no_)) {
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordRet);
#ifndef OPT_LOG
          LOG_I("[{}] Forward order ret {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        ordRet->stgId_, MSG_ID_ON_ORDER_RET, ordRet->size());
  }

  const auto threadNo = std::ext::tls_get<ThreadInfo>().no_;

  //! syncToDB动作已经在tdSrv做了，所以这里就不再syncToDB
  const auto [isTheOrderCanBeUsedCalcPos, orderInfoInOrdMgr] =
      tdSrv_->getRiskCtrlModuleComb()[no_]
          ->getOrdMgrGroup()[threadNo]
          ->updateByOrderInfoFromTDGW<LockFunc::False>(ordRet);

  if (isTheOrderCanBeUsedCalcPos == IsTheOrderCanBeUsedCalcPos::True) {
    LOG_I("[{}] Current order can be used to calc pos, begin to calc pos. {}",
          no_, ordRet->toShortStr());

    //! 更新PosMgr
    const auto posChgInfo =
        tdSrv_->getRiskCtrlModuleComb()[no_]
            ->getPosMgrGroup()[threadNo]
            ->updateByOrderInfoFromTDGW<LockFunc::False>(ordRet);

    //! 放进入库线程异步处理，所以这里posChgInfo需要复制一个副本
    auto posInfGroup = std::make_shared<PosChgInfo>();
    posInfGroup->reserve(posChgInfo->size());
    std::for_each(
        std::begin(*posChgInfo), std::end(*posChgInfo),
        [&](const auto& posInfo) {
          posInfGroup->emplace_back(std::make_shared<PosInfo>(*posInfo));
        });

    //! 在第一个风控模组更新仓位即可
    if (tdSrv_->isFirstRiskCtrlModule(no_)) {
      tdSrv_->cacheSyncTaskGroup(MSG_ID_SYNC_POS_INFO, posInfGroup,
                                 SyncToRiskMgr::False, SyncToDB::True);
    }
  }

  //! 风控插件轮流处理
  const auto [statusCode, details] =
      tdSrv_->getRiskCtrlModuleComb()[no_]->getTDSrvRiskPluginMgr()->onOrderRet(
          ordRet, no_, threadNo);
  if (statusCode != 0) {
    LOG_W("[{}] Risk check order ret failed. {}", no_, details);

    //! 回滚订单状态变化
    tdSrv_->getRiskCtrlModuleComb()[no_]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->batchRollback();
    LOG_T("[{}] Batch rollback risk ctrl status. {}", no_, details);

    return;
  }

  //! 批量更新风控状态变化
  tdSrv_->getRiskCtrlModuleComb()[no_]
      ->getRiskCtrlStatusUpdatersGroup()[threadNo]
      ->batchExecute();
  LOG_T("[{}] Batch update risk ctrl status generate by order {}", no_,
        ordRet->toShortStr());

  //! 将消息丢给下一个风控模组
  if (!tdSrv_->isLastRiskCtrlModule(no_)) {
    auto task = std::make_shared<SHMIPCTask>(ordRet.get(), ordRet->size());
    tdSrv_->getRiskCtrlModuleComb()[no_ + 1]
        ->getTDSrvTaskDispatcher()
        ->dispatch(task);
  }
}

void TDGWTaskHandler::handleMsgIdOnCancelOrderRet(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  auto ordRet = MakeMsgSPtrByTask<OrderInfo>(asyncTask->task_);
  LOG_I("[{}] Recv cancel order ret {}", no_, ordRet->toShortStr());

  //! 优先发送应答给策略
  if (tdSrv_->isFirstRiskCtrlModule(no_)) {
    tdSrv_->getSHMSrvOfStgEng()->pushMsgWithZeroCopy(
        [&](void* shmBuf) {
          InitMsgBodyExt(shmBuf, *ordRet);
#ifndef OPT_LOG
          LOG_I("[{}] Forward cancel order ret {}", no_,
                static_cast<OrderInfo*>(shmBuf)->toShortStr());
#endif
        },
        ordRet->stgId_, MSG_ID_ON_CANCEL_ORDER_RET, ordRet->size());
  }

  //! 风控插件轮流检查
  const auto threadNo = std::ext::tls_get<ThreadInfo>().no_;
  const auto [statusCode, details] =
      tdSrv_->getRiskCtrlModuleComb()[no_]
          ->getTDSrvRiskPluginMgr()
          ->onCancelOrderRet(ordRet, no_, threadNo);
  if (statusCode != 0) {
    LOG_W("[{}] Risk check cancel order ret failed. {}", no_, details);

    //! 回滚订单状态变化
    tdSrv_->getRiskCtrlModuleComb()[no_]
        ->getRiskCtrlStatusUpdatersGroup()[threadNo]
        ->batchRollback();
    LOG_T("[{}] Batch rollback risk ctrl status. {}", no_, details);

    return;
  }

  //! 批量更新风控状态变化，优先发送报单
  tdSrv_->getRiskCtrlModuleComb()[no_]
      ->getRiskCtrlStatusUpdatersGroup()[threadNo]
      ->batchExecute();
  LOG_T("[{}] Batch update risk ctrl status generate by cancel order ret {}",
        no_, ordRet->toShortStr());

  //! 将消息丢给下一个风控模组
  if (!tdSrv_->isLastRiskCtrlModule(no_)) {
    auto task = std::make_shared<SHMIPCTask>(ordRet.get(), ordRet->size());
    tdSrv_->getRiskCtrlModuleComb()[no_ + 1]
        ->getTDSrvTaskDispatcher()
        ->dispatch(task);
  }
}

void TDGWTaskHandler::handleMsgIdOnTDGWReg(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  const auto msgHeader = static_cast<const SHMHeader*>(asyncTask->task_->data_);
  LOG_D("[{}] Recv msg {}. [channel = {}]", no_, GetMsgName(msgHeader->msgId_),
        msgHeader->clientChannel_);

  tdSrv_->getTDGWGroup()->update(msgHeader->clientChannel_);
  tdSrv_->getSHMSrvOfTDGW()->sendRspWithZeroCopy([&](void* shmBuf) {},
                                                 msgHeader, sizeof(TDGWReg));
}

}  // namespace bq::td::srv
