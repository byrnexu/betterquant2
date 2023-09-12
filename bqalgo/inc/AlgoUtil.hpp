/*!
 * \file AlgoUtil.hpp
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

namespace bq {
struct Bid1Ask1;
using Bid1Ask1SPtr = std::shared_ptr<Bid1Ask1>;
}  // namespace bq

namespace bq::algo {

enum class Urgency;

std::tuple<int, std::vector<Decimal>> splitTotalSize(Decimal totalSize,
                                                     uint32_t splitNum,
                                                     Decimal prec);

Decimal GetPriceOfUrgency(const Bid1Ask1SPtr& bid1ask1, Side side,
                          Urgency urgency, std::uint32_t tickerOffset,
                          Decimal precOfOrderPrice);

std::tuple<Urgency, std::uint32_t> UpgradeUrgency(
    Urgency curUrgency, std::uint32_t tickerOffset = 0);

}  // namespace bq::algo
