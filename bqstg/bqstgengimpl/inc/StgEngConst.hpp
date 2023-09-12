/*!
 * \file StgEngConst.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::stg {

const static std::string prefixOfQueryHisMDBetween =
    "http://{}/v1/QueryHisMD/between";
const static std::string prefixOfQueryHisMDBefore =
    "http://{}/v1/QueryHisMD/before";
const static std::string prefixOfQueryHisMDAfter =
    "http://{}/v1/QueryHisMD/after";

const static std::string InstrCancelAllOrders = "cancelAllOrders";

}  // namespace bq::stg
