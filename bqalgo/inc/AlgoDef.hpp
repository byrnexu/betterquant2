/*!
 * \file AlgoDef.hpp
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

enum class AlgoStatus {
  NotStarted,
  Started,
  Paused,
  Canceling,
  Canceled,
  ExecFailed,
  Finished,
  OutOfTime
};

enum class TryToCancelAllOrders { True, False };

enum class Urgency {
  PriceOfMaker,
  PriceOfMakerWith1Ticker,
  PriceOfMidPoint,
  PriceOfTaker
};

}  // namespace bq::algo
