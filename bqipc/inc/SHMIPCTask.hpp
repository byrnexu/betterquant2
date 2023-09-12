/*!
 * \file SHMIPCTask.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/DefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

enum class CopyIPCData { True = 1, False = 2 };

struct SHMIPCTask {
  SHMIPCTask() = default;
  SHMIPCTask(const void* origData, std::size_t origDataLen,
             CopyIPCData copyIPCData = CopyIPCData::True) {
    if (copyIPCData == CopyIPCData::True) {
      data_ = malloc(origDataLen);
      memcpy(data_, origData, origDataLen);
    } else {
      data_ = const_cast<void*>(origData);
    }
    len_ = origDataLen;
  }
  void* data_{nullptr};
  std::size_t len_{0};

  ~SHMIPCTask() { SAFE_FREE(data_); }
};
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

template <typename Msg>
std::shared_ptr<Msg> MakeMsgSPtrByTask(
    const SHMIPCTaskSPtr& task, CopyIPCData copyIPCData = CopyIPCData::False) {
  Msg* msg = nullptr;
  if (copyIPCData == CopyIPCData::False) {
    msg = static_cast<Msg*>(task->data_);
    std::shared_ptr<Msg> ret(msg);
    task->data_ = nullptr;
    task->len_ = 0;
    return ret;
  } else {
    auto buf = malloc(task->len_);
    memcpy(buf, task->data_, task->len_);
    msg = static_cast<Msg*>(buf);
    std::shared_ptr<Msg> ret(msg);
    buf = nullptr;
    return ret;
  }
}

template <typename Msg>
std::shared_ptr<Msg> MakeMsgSPtrByTask(const void* buf) {
  auto msg = static_cast<Msg*>(const_cast<void*>(buf));
  std::shared_ptr<Msg> ret(msg);
  buf = nullptr;
  return ret;
}

}  // namespace bq
