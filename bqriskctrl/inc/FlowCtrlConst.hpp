/*!
 * \file FlowCtrlConst.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

//! 因为单位时间内下单数量和金额每次增长不是1，所以暂时不支持
enum class FlowCtrlTarget : std::uint8_t {
  //! 单笔下单数量
  OrderSizeEachTime,
  //! 累计下单数量
  OrderSizeTotal,

  //! 单笔下单金额
  OrderAmtEachTime,
  //! 累计下单金额
  OrderAmtTotal,

  //! 累计下单笔数
  OrderTimesTotal,
  //! 单位时间下单笔数
  OrderTimesWithinTime,

  //! 累计撤单笔数
  CancelOrderTimesTotal,
  //! 单位时间撤单笔数
  CancelOrderTimesWithinTime,

  //! 累计被拒单笔数
  RejectOrderTimesTotal,
  //! 单位时间被拒单笔数
  RejectOrderTimesWithinTime,

  //! 累计成交数量
  HoldVolTotal,
  //! 累计成交金额
  HoldAmtTotal,

  //! 当日累计开仓
  OpenTDayTotal,

  Others = UINT8_MAX - 1
};

enum class FlowCtrlAction : std::uint8_t {
  //! 拒单
  RejectOrder = 1,
  //! 通知
  PubTopic,

  Others = UINT8_MAX - 1
};

enum class FlowCtrlLimitType : std::uint8_t {
  NumLimitEachTime = 1,
  NumLimitTotal,
  NumLimitWithinTime,
  Others = UINT8_MAX - 1
};

const static std::string SEP_OF_LIMIT_VALUE = "/";
const static std::uint32_t MAX_TS_QUEUE_SIZE = 1000;

}  // namespace bq
