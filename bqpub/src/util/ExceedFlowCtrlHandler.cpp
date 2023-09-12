/*!
 * \file ExceedFlowCtrlHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/ExceedFlowCtrlHandler.hpp"

#include "SHMIPC.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq {

void ExceedFlowCtrlHandler::saveExceedFlowCtrlTask(
    const SHMIPCAsyncTaskSPtr& asyncTask) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxExceedFlowCtrlTaskInfoGroup_);
    exceedFlowCtrlTaskInfoGroup_.emplace_back(asyncTask);
    if (exceedFlowCtrlTaskInfoGroup_.size() % 10 == 0) {
      LOG_W("Exceed flow ctrl task num {}.",
            exceedFlowCtrlTaskInfoGroup_.size());
    }
  }
}

//!
//! TDFrontTaskHandler检查是否超出流控，超流控就调用上面的saveExceedFlowCtrlTask
//! 在定时器中定时调用下面的函数handleExceedFlowCtrlTask，检查每个之前超流控的函
//! 数现在调用是否不再超流控
//!
void ExceedFlowCtrlHandler::handleExceedFlowCtrlTask() {
  std::deque<SHMIPCAsyncTaskSPtr> exceedFlowCtrlTaskInfoGroupClone_;
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxExceedFlowCtrlTaskInfoGroup_);
    exceedFlowCtrlTaskInfoGroupClone_.assign(
        std::begin(exceedFlowCtrlTaskInfoGroup_),
        std::end(exceedFlowCtrlTaskInfoGroup_));
    exceedFlowCtrlTaskInfoGroup_.clear();
  }

  for (auto& asyncTask : exceedFlowCtrlTaskInfoGroupClone_) {
    cbExceedFlowCtrlTaskDispatch_(asyncTask);
  }
}

}  // namespace bq
