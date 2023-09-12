/*!
 * \file RawTD.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/02
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQConstIF.hpp"
#include "def/DefIF.hpp"
#include "util/Datetime.hpp"
#include "util/Pch.hpp"

namespace bq {

template <typename Task>
struct AsyncTask;

struct RawTD {
  RawTD(MsgType msgType, void* data, std::uint32_t dataLen) {
    msgType_ = msgType;
    localTs_ = GetTotalUSSince1970();

    dataLen_ = dataLen;
    data_ = malloc(dataLen);
    memcpy(data_, data, dataLen_);
  }

  ~RawTD() { SAFE_FREE(data_); }

  MsgType msgType_;
  std::uint64_t localTs_;

  std::uint32_t dataLen_{0};
  void* data_{nullptr};
};
using RawTDSPtr = std::shared_ptr<RawTD>;

using RawTDAsyncTask = AsyncTask<RawTDSPtr>;
using RawTDAsyncTaskSPtr = std::shared_ptr<RawTDAsyncTask>;

}  // namespace bq
