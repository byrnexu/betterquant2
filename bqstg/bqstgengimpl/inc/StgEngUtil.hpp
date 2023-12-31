/*!
 * \file StgEngUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMIPCMsgId.hpp"
#include "StgEngDef.hpp"

namespace bq {
struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
template <typename Task>

struct AsyncTask;
using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;

struct CommonIPCData;
using CommonIPCDataSPtr = std::shared_ptr<CommonIPCData>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::stg {

SHMIPCAsyncTaskSPtr MakeStgSignal(MsgId msgId, StgInstId stgInstId);
SHMIPCAsyncTaskSPtr MakeStgSignal(MsgId msgId, StgInstId stgInstId,
                                  const std::string& data);

std::tuple<int, StgInstId> GetStgInstId(const CommonIPCData* commonIPCData);

int InitPosSide(OrderInfoSPtr& orderInfo);

}  // namespace bq::stg
