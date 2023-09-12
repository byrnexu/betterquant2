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

struct SmartOrderParams {
  TrdAcctId trdAcctId_;
  MarketCode marketCode_;
  std::string symbolCode_;
  Side side_;
  PosDirection posDirection_;
  Decimal orderSize_{1};
  Urgency initialUrgency_;
  std::uint32_t tickerOffset_{0};
  CloseTDayStg closeTDayStg_{CloseTDayStg::RejectCloseTDay};
  PriceRange priceRange_;
};
using SmartOrderParamsSPtr = std::shared_ptr<SmartOrderParams>;

}  // namespace bq::algo
