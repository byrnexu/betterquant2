/*!
 * \file HttpCliOfExch.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "HttpCliOfExch.hpp"

#include "AssetsMgr.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDSvc.hpp"
#include "TDSvcUtil.hpp"
#include "WSCliOfExch.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/SyncTask.hpp"
#include "def/TDWSCliAsyncTaskArg.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/Logger.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::svc {

//!
//! ordReq来自报单请求的DeepClone::True副本
//!
void HttpCliOfExch::handleRspOfOrder(OrderInfoSPtr ordReq, cpr::Response rsp) {
  const auto [failed, externalStatusCode, externalStatusMsg] =
      rspOfOrderIsFailed(rsp.text);
  if (failed) {
    const auto statusMsg =
        fmt::format("Handle order status rsp of failed. {} {}", rsp.text,
                    ordReq->toShortStr());
    LOG_W(statusMsg);

    ordReq->statusCode_ =
        tdSvc_->getExternalStatusCodeCache()->getAndSetStatusCodeIfNotExists(
            tdSvc_->getMarketCode(), ordReq->userId_,
            fmt::format("{}", externalStatusCode), externalStatusMsg, -1);

    //! 此处订单肯定已经完结
    if (ordReq->statusCode_ != 0) {
      ordReq->orderStatus_ = OrderStatus::Failed;

      const auto [isTheOrderInfoUpdated, orderInfoInOrdMgr] =
          tdSvc_->getOrdMgr()
              ->updateByOrderInfoFromExch<LockFunc::True, DeepClone::True>(
                  ordReq, tdSvc_->getNextNoUsedToCalcPos());
      if (isTheOrderInfoUpdated == IsSomeFieldOfOrderUpdated::False) {
        return;
      }

      //!
      //! 这个回报没有经过状态机，因为是废单，肯定没有后续了，但是要注意一种情况，
      //! 就是这个废单应答回来的慢，同步订单的https请求已经查到该订单不存在，这
      //! 样就会有两个废单回报。以后可以在这里加一个状态机检查。
      //!
      //! 20220823 增加了状态机检查
      //!
      tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
          [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *orderInfoInOrdMgr); },
          MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr->size());

      tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_ORDER_RET, orderInfoInOrdMgr,
                                 SyncToRiskMgr::True, SyncToDB::True);
      //!
      //! orderinfo只会在当前线程被使用都DeepClone::False，一旦进入被http请求使用
      //! 可能会被带入异步应答或者入库再或者保存到OrdMgr，有可能会被其他线程使用的
      //! 情况下，都deepclone::true
      //!
    }

    return;
  }

  LOG_D("{} {}", rsp.text, ordReq->toShortStr());
}

//!
//! ordReq来自请求
//!
void HttpCliOfExch::handleRspOfCancelOrder(OrderInfoSPtr ordReq,
                                           cpr::Response rsp) {
  const auto [failed, externalStatusCode, externalStatusMsg] =
      rspOfCancelOrderIsFailed(rsp.text);
  if (failed) {
    const auto statusMsg =
        fmt::format("Handle cancel order rsp of failed. {}", rsp.text);
    LOG_W("{} {}", statusMsg, ordReq->toShortStr());

    ordReq->statusCode_ =
        tdSvc_->getExternalStatusCodeCache()->getAndSetStatusCodeIfNotExists(
            tdSvc_->getMarketCode(), ordReq->userId_,
            fmt::format("{}", externalStatusCode), externalStatusMsg, -1);

    tdSvc_->getSHMCliOfTDSrv()->asyncSendMsgWithZeroCopy(
        [&](void* shmBuf) { InitMsgBodyExt(shmBuf, *ordReq); },
        MSG_ID_ON_CANCEL_ORDER_RET, ordReq->size());

    // not sync to db
    tdSvc_->cacheSyncTaskGroup(MSG_ID_ON_CANCEL_ORDER_RET, ordReq,
                               SyncToRiskMgr::True, SyncToDB::False);
    return;
  }
}

void HttpCliOfExch::syncAssetsSnapshot() {
  const auto assetInfoGroupFromExch = doSyncAssetsSnapshot();
  if (assetInfoGroupFromExch.empty()) {
    return;
  }

  //! 这里生成keyHash_是为了子类和keyHash_无关
  AssetInfoGroupSPtr assetInfoGroup = std::make_shared<AssetInfoGroup>();
  for (const auto& rec : assetInfoGroupFromExch) {
    rec->initKeyHash();
    assetInfoGroup->emplace(rec->keyHash_, rec);
  }

  //! 用从交易所查到的资产信息和AssetsMgr中的资产信息比较
  const auto updateInfoOfAssetGroup =
      tdSvc_->getAssetsMgr()->compareWithAssetsSnapshot(assetInfoGroup);

  //! 发送变化到 TDSrv 和 RiskMgr
  if (!updateInfoOfAssetGroup->empty()) {
    NotifyAssetInfo(tdSvc_->getSHMCliOfTDSrv(), tdSvc_->getAcctId(),
                    updateInfoOfAssetGroup);

    tdSvc_->cacheSyncTaskGroup(MSG_ID_SYNC_ASSETS, updateInfoOfAssetGroup,
                               SyncToRiskMgr::True, SyncToDB::True);
    updateInfoOfAssetGroup->print();
  }
}

//! shmIPCAsyncTask包含SHMIPCTask和OrderInfoInOrdMgr
void HttpCliOfExch::syncUnclosedOrderInfo(
    SHMIPCAsyncTaskSPtr& shmIPCAsyncTask) {
  const auto orderInfoFromExch = doSyncUnclosedOrderInfo(shmIPCAsyncTask);
  if (!orderInfoFromExch) {
    return;
  }
  const auto wsCliAsyncTaskArg = std::make_shared<WSCliAsyncTaskArg>(
      MsgType::SyncUnclosedOrder, orderInfoFromExch);
  //!
  //! asyncTask包含一个空的TaskFromSrv和wsCliAsyncTaskArg，wsCliAsyncTaskArg包含
  //! MsgType和std::any类型的extData（数据内容为orderInfoFromExch）
  //!
  auto asyncTask = std::make_shared<WSCliAsyncTask>(nullptr, wsCliAsyncTaskArg);
  tdSvc_->getWSCliOfExch()->getTaskDispatcher()->dispatch(asyncTask);
}

}  // namespace bq::td::svc
