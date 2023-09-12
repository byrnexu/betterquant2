/*!
 * \file BQXTPUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#pragma once

#include "API.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc::xtp {

std::tuple<int, XTP_PROTOCOL_TYPE> GetXTPProtocolType(std::string protocolType);

MarketCode GetMarketCode(XTP_EXCHANGE_TYPE exchangeType);
XTP_EXCHANGE_TYPE GetExchMarketCode(MarketCode marketCode);

SymbolType GetSymbolType(XTP_SECURITY_TYPE exchSymbolType);
SymbolState GetSymbolState(XTP_SECURITY_STATUS exchSymbolState);

Side GetSideFromTrades(MarketCode marketCode, char trade_flag);
Side GetSideFromOrders(MarketCode marketCode, char side);

}  // namespace bq::md::svc::xtp
