/*!
 * \file AlgoTWAPDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/17
 *
 * \brief
 */
#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::algo {

using PriceRange = std::tuple<Decimal, Decimal>;

struct TWAPParams {
  TrdAcctId trdAcctId_;
  MarketCode marketCode_;
  std::string symbolCode_;
  Side side_;
  PosDirection posDirection_;
  Decimal totalSize_{1};
  std::uint32_t splitNum_{1};
  std::uint32_t msIntervalOfSubOrder_{1000};
  std::uint32_t minMSIntervalOfOrderAndCancelOrder_{1000};
  Urgency initialUrgency_;
  std::uint32_t tickerOffset_{0};
  std::uint32_t cancelTimesOfUpgradeUrgency_;
  CloseTDayStg closeTDayStg_{CloseTDayStg::RejectCloseTDay};
  std::optional<PriceRange> priceRange_;
};
using TWAPParamsSPtr = std::shared_ptr<TWAPParams>;

}  // namespace bq::algo
