/*!
 * \file ExceedFlowCtrlHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
template <typename Task>
struct AsyncTask;
using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;
using CBExceedFlowCtrlTaskDispatch = std::function<void(SHMIPCAsyncTaskSPtr&)>;

class ExceedFlowCtrlHandler {
 public:
  ExceedFlowCtrlHandler(const ExceedFlowCtrlHandler&) = delete;
  ExceedFlowCtrlHandler& operator=(const ExceedFlowCtrlHandler&) = delete;
  ExceedFlowCtrlHandler(const ExceedFlowCtrlHandler&&) = delete;
  ExceedFlowCtrlHandler& operator=(const ExceedFlowCtrlHandler&&) = delete;

  explicit ExceedFlowCtrlHandler(
      const CBExceedFlowCtrlTaskDispatch& cbExceedFlowCtrlTaskDispatch)
      : cbExceedFlowCtrlTaskDispatch_(cbExceedFlowCtrlTaskDispatch) {}

 public:
  void saveExceedFlowCtrlTask(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleExceedFlowCtrlTask();

 private:
  std::deque<SHMIPCAsyncTaskSPtr> exceedFlowCtrlTaskInfoGroup_;
  std::ext::spin_mutex mtxExceedFlowCtrlTaskInfoGroup_;

  CBExceedFlowCtrlTaskDispatch cbExceedFlowCtrlTaskDispatch_;
};

}  // namespace bq
