/*!
 * \file TDGWTaskHandler.hpp
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
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class TDGWTaskHandler {
 public:
  TDGWTaskHandler(const TDGWTaskHandler&) = delete;
  TDGWTaskHandler& operator=(const TDGWTaskHandler&) = delete;
  TDGWTaskHandler(const TDGWTaskHandler&&) = delete;
  TDGWTaskHandler& operator=(const TDGWTaskHandler&&) = delete;

  explicit TDGWTaskHandler(TDSrv* tdSrv, std::uint32_t no);

 public:
  void handleAsyncTask(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void handleMsgIdOnOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleMsgIdOnCancelOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);

  void handleMsgIdSyncAssets(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleMsgIdOnTDGWReg(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  TDSrv* tdSrv_;
  std::uint32_t no_{0};
};

}  // namespace bq::td::srv
