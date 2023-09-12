/*!
 * \file TDSvcOfCTPUtil.hpp
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

namespace bq::td::svc::ctp {

TThostFtdcDirectionType GetExchSide(Side side);
Side GetExchSide(TThostFtdcDirectionType exchSide);

std::tuple<int, TThostFtdcDirectionType, TThostFtdcOffsetFlagType>
GetExchSideInfo(const OrderInfoSPtr& orderInfo);

std::string GetExchange(MarketCode marketCode);
MarketCode GetMarketCode(const char* exchange);

OrderStatus GetOrderStatus(TThostFtdcOrderStatusType OrderStatus);

}  // namespace bq::td::svc::ctp
