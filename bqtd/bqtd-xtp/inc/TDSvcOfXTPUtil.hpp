/*!
 * \file TDSvcOfXTPUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#pragma once

#include "API.hpp"
#include "def/BQConst.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::svc {
class TDSvcOfCN;
}

namespace bq::td::svc::xtp {

OrderStatus GetOrderStatus(XTP_ORDER_STATUS_TYPE exchOrderStatus);

XTP_MARKET_TYPE GetExchMarketCode(MarketCode marketCode);
MarketCode GetMarketCode(XTP_MARKET_TYPE exchMarketCode);

int GetExchSide(Side side);
Side GetSide(int exchSide);

}  // namespace bq::td::svc::xtp
