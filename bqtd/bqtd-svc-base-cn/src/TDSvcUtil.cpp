/*!
 * \file TDSvcUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSvcUtil.hpp"

#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "TDSvcOfCN.hpp"
#include "db/TBLTrdSymbol.hpp"
#include "def/AssetInfo.hpp"
#include "def/DataStruOfOthers.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::td::svc {

//! 用于创建定时查询资产和未完结订单信号
SHMIPCAsyncTaskSPtr MakeTDSrvSignal(MsgId msgId, const std::any& arg) {
  TDSrvSignal tdSrvSignal(msgId);
  const auto task =
      std::make_shared<SHMIPCTask>(&tdSrvSignal, sizeof(TDSrvSignal));
  const auto ret = std::make_shared<SHMIPCAsyncTask>(task, arg);
  return ret;
}

//! 用于定时同步资产到riskMgr
void NotifyAssetInfo(const SHMCliSPtr& shmCli, AcctId acctId,
                     const UpdateInfoOfAssetGroupSPtr& updateInfoOfAssetGroup) {
  const auto extDataLen =
      updateInfoOfAssetGroup->assetInfoGroupAdd_->size() * sizeof(AssetInfo) +
      updateInfoOfAssetGroup->assetInfoGroupDel_->size() * sizeof(AssetInfo) +
      updateInfoOfAssetGroup->assetInfoGroupChg_->size() * sizeof(AssetInfo);
  const auto shmBufLen = sizeof(AssetInfoNotify) + extDataLen;

  shmCli->asyncSendMsgWithZeroCopy(
      [&](void* shmBuf) {
        auto assetInfoNotify = static_cast<AssetInfoNotify*>(shmBuf);
        assetInfoNotify->acctId_ = acctId;
        assetInfoNotify->addNum_ =
            updateInfoOfAssetGroup->assetInfoGroupAdd_->size();
        assetInfoNotify->delNum_ =
            updateInfoOfAssetGroup->assetInfoGroupDel_->size();
        assetInfoNotify->chgNum_ =
            updateInfoOfAssetGroup->assetInfoGroupChg_->size();
        assetInfoNotify->extDataLen_ = extDataLen;

        char* pos = static_cast<char*>(shmBuf) + sizeof(AssetInfoNotify);
        for (const auto& rec : *updateInfoOfAssetGroup->assetInfoGroupAdd_) {
          memcpy(pos, rec.get(), sizeof(AssetInfo));
          pos += sizeof(AssetInfo);
        }
        for (const auto& rec : *updateInfoOfAssetGroup->assetInfoGroupDel_) {
          memcpy(pos, rec.get(), sizeof(AssetInfo));
          pos += sizeof(AssetInfo);
        }
        for (const auto& rec : *updateInfoOfAssetGroup->assetInfoGroupChg_) {
          memcpy(pos, rec.get(), sizeof(AssetInfo));
          pos += sizeof(AssetInfo);
        }
      },
      MSG_ID_SYNC_ASSETS, shmBufLen);
}

bool CheckIfUpdateOrderInfo(int secOffOfUpdateTimeInOrd,
                            int secAgoTheOrderCouldBeSync) {
  //! 将secOffOfUpdateTimeInOrd和当前时间比较，是否已经超过seconds秒
  const auto now = boost::posix_time::second_clock::local_time();
  auto secOfNow = GetSecOffsetOfDay(now);
  if (secOffOfUpdateTimeInOrd - secOfNow > 3600) {
    LOG_W(
        "Sec offset in date of update time in order greater than the "
        "sec offset in date of now more than 3600 seconds. [now = {}]",
        boost::posix_time::to_iso_string(now));
    secOfNow += 86400;
  }

  if (secOfNow > secOffOfUpdateTimeInOrd) {
    if (secOfNow - secOffOfUpdateTimeInOrd > secAgoTheOrderCouldBeSync)
      return true;
  }

  return false;
}

//! 类似CTP查到的订单信息里没有updateTime所以用下面的方式实现延迟推送
bool CheckIfUpdateOrderInfo(TDSvcOfCN* tdSvc,
                            const OrderInfoSPtr& orderInfoFromExch,
                            std::uint64_t secAgoTheOrderCouldBeSync) {
  //! 订单id和最后变动时间对照表
  static std::map<OrderId, std::uint64_t> orderId2UpdateTimeGroup;
  bool orderIsUpdated =
      tdSvc->getOrdMgr()->compAndCheckIfUpdate<LockFunc::True>(
          orderInfoFromExch);
  if (!orderIsUpdated) {
    //! 如果订单状态没有变得更加新
    orderId2UpdateTimeGroup.erase(orderInfoFromExch->orderId_);
    return false;
  }

  const auto now = GetTotalSecSince1970();

  //! 订单状态变得更加新
  const auto iter = orderId2UpdateTimeGroup.find(  //
      orderInfoFromExch->orderId_);

  if (iter == std::end(orderId2UpdateTimeGroup)) {
    //! 第一次记录此订单状态变化，只记录不做任何处理
    orderId2UpdateTimeGroup.emplace(orderInfoFromExch->orderId_, now);
    if (orderId2UpdateTimeGroup.size() % 1000) {
      LOG_W("Size of order id to update time is {}",
            orderId2UpdateTimeGroup.size());
    }
    LOG_I(
        "It is found that the order status has become more recent, "
        "and the order information is updated when the status changes "
        "for a long enough time. {}",
        orderInfoFromExch->toShortStr())
    return false;
  }

  const auto prevUpdateTime = iter->second;
  if (now - prevUpdateTime <= secAgoTheOrderCouldBeSync) {
    return false;
  }

  //! 第二次发现订单状态变得更加新并且和第一次的时间间隔足够大
  orderId2UpdateTimeGroup.erase(orderInfoFromExch->orderId_);
  LOG_I(
      "It is found that the order status change time exceeds {} seconds, "
      "and the order status change is started to be updated. {}",
      secAgoTheOrderCouldBeSync, orderInfoFromExch->toShortStr());

  return true;
}

}  // namespace bq::td::svc
