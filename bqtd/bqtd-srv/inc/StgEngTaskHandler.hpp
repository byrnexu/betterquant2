/*!
 * \file StgEngTaskHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {
struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
template <typename Task>
struct AsyncTask;
using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class StgEngTaskHandler {
 public:
  StgEngTaskHandler(const StgEngTaskHandler&) = delete;
  StgEngTaskHandler& operator=(const StgEngTaskHandler&) = delete;
  StgEngTaskHandler(const StgEngTaskHandler&&) = delete;
  StgEngTaskHandler& operator=(const StgEngTaskHandler&&) = delete;

  explicit StgEngTaskHandler(TDSrv* tdSrv, std::uint32_t no);

 public:
  void handleAsyncTask(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void handleMsgIdOnRiskCtrlConfChg(const SHMIPCAsyncTaskSPtr& asyncTask);

  void handleMsgIdOnOrder(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleMsgIdOnCancelOrder(const SHMIPCAsyncTaskSPtr& asyncTask);

  void handleMsgIdOnStgReg(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void pubTopicOfTriggerRiskCtrl(const std::string& details,
                                 const OrderInfoSPtr& order);

 private:
  TDSrv* tdSrv_;
  std::uint32_t no_;
};

}  // namespace bq::td::srv
