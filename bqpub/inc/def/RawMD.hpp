/*!
 * \file RawMD.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/22
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/DefIF.hpp"
#include "util/Datetime.hpp"
#include "util/Pch.hpp"

namespace bq {

template <typename Task>
struct AsyncTask;

struct RawMD {
  RawMD(MsgType msgType, const void* data, std::uint32_t dataLen, bool isLast,
        std::uint64_t localTs = GetTotalUSSince1970()) {
    msgType_ = msgType;
    localTs_ = localTs;

    dataLen_ = dataLen;
    data_ = malloc(dataLen);
    memcpy(data_, data, dataLen_);

    isLast_ = isLast;

    switch (msgType) {
      case MsgType::Tickers:
        mdType_ = MDType::Tickers;
        break;
      case MsgType::Trades:
        mdType_ = MDType::Trades;
        break;
      case MsgType::Orders:
        mdType_ = MDType::Orders;
        break;
      case MsgType::Books:
        mdType_ = MDType::Books;
        break;
      case MsgType::Candle:
        mdType_ = MDType::Candle;
        break;
      case MsgType::Bid1Ask1:
        mdType_ = MDType::Bid1Ask1;
        break;
      case MsgType::LastPrice:
        mdType_ = MDType::LastPrice;
        break;
      case MsgType::DynCandle:
        mdType_ = MDType::DynCandle;
        break;
      default:
        mdType_ = MDType::Others;
        break;
    }
  }

  ~RawMD() {
    SAFE_FREE(data_);
    SAFE_FREE(dataAfterConv_);
  }

  MsgType msgType_;
  std::uint64_t localTs_;

  std::uint32_t dataLen_{0};
  void* data_{nullptr};

  bool isLast_{false};

  MDType mdType_;
  MarketCode marketCode_;
  SymbolType symbolType_;
  std::string symbolCode_;

  std::string topic_;
  TopicHash topicHash_;

  std::uint32_t dataAfterConvLen_{0};
  void* dataAfterConv_{nullptr};
};
using RawMDSPtr = std::shared_ptr<RawMD>;

using RawMDAsyncTask = AsyncTask<RawMDSPtr>;
using RawMDAsyncTaskSPtr = std::shared_ptr<RawMDAsyncTask>;

void InitTopicInfo(RawMDSPtr& rawMD);

}  // namespace bq
