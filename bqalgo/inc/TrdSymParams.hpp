/*!
 * \file TrdSymParams.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/18
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

struct TrdSymParams {
  MarketCode marketCode_;
  std::string symbolCode_;

  Decimal upperLimitPrice_{0.0};
  Decimal lowerLimitPrice_{0.0};

  Decimal preClosePrice_{0.0};
  Decimal preSettlementPrice_{0.0};

  Decimal precOfBidOrderPrice_{0.0};
  Decimal precOfAskOrderPrice_{0.0};
  Decimal precOfOrderPrice_{0.0};

  Decimal precOfBidOrderVol_{0.0};
  Decimal precOfAskOrderVol_{0.0};
  Decimal precOfOrderVol_{0.0};

  Decimal minBidOrderVol_{0.0};
  Decimal minAskOrderVol_{0.0};
  Decimal minOrderVol_{0.0};

  Decimal maxBidOrderVol_{0.0};
  Decimal maxAskOrderVol_{0.0};
  Decimal maxOrderVol_{0.0};

  Decimal minBidOrderAmt_{0.0};
  Decimal minAskOrderAmt_{0.0};
  Decimal minOrderAmt_{0.0};

  Decimal maxBidOrderAmt_{0.0};
  Decimal maxAskOrderAmt_{0.0};
  Decimal maxOrderAmt_{0.0};

  int parValue_{0};
};

}  // namespace bq
