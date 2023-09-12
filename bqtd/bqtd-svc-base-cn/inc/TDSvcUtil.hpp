/*!
 * \file TDSvcUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "TDSvcDef.hpp"
#include "def/BQDef.hpp"
#include "util/Pch.hpp"

namespace bq {
struct AssetInfo;
using AssetInfoSPtr = std::shared_ptr<AssetInfo>;

struct UpdateInfoOfAssetGroup;
using UpdateInfoOfAssetGroupSPtr = std::shared_ptr<UpdateInfoOfAssetGroup>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class SHMCli;
using SHMCliSPtr = std::shared_ptr<SHMCli>;

class TDSvcOfCN;
}  // namespace bq

namespace bq::db::trdSymbol {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;
}  // namespace bq::db::trdSymbol

namespace bq::td::svc {

SHMIPCAsyncTaskSPtr MakeTDSrvSignal(MsgId msgId,
                                    const std::any& arg = std::any());

void NotifyAssetInfo(const SHMCliSPtr& shmCli, AcctId acctId,
                     const UpdateInfoOfAssetGroupSPtr& updateInfoOfAssetGroup);

//! 类似XTP通过订单里的updateTime和本地时间比较实现延时推送
bool CheckIfUpdateOrderInfo(int secOffOfUpdateTimeInOrd,
                            int secAgoTheOrderCouldBeSync);

//! 类似CTP查到的订单信息里没有updateTime所以用下面的方式实现延迟推送
bool CheckIfUpdateOrderInfo(TDSvcOfCN* tdSvc,
                            const OrderInfoSPtr& orderInfoFromExch,
                            std::uint64_t secAgoTheOrderCouldBeSync);

}  // namespace bq::td::svc
