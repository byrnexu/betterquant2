/*!
 * \file TDSvcOfXTPUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDSvcOfXTPUtil.hpp"

#include "Config.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfXTPConst.hpp"
#include "TDSvcUtil.hpp"
#include "def/BQConst.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq::td::svc::xtp {

OrderStatus GetOrderStatus(XTP_ORDER_STATUS_TYPE exchOrderStatus) {
  switch (exchOrderStatus) {
    case XTP_ORDER_STATUS_ALLTRADED:
      return OrderStatus::Filled;
    case XTP_ORDER_STATUS_PARTTRADEDQUEUEING:
      return OrderStatus::PartialFilled;
    case XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING:
      return OrderStatus::PartialFilledCanceled;
    case XTP_ORDER_STATUS_NOTRADEQUEUEING:
      return OrderStatus::ConfirmedByExch;
    case XTP_ORDER_STATUS_CANCELED:
      return OrderStatus::Canceled;
    case XTP_ORDER_STATUS_REJECTED:
      return OrderStatus::Failed;
    case XTP_ORDER_STATUS_UNKNOWN:
      return OrderStatus::Others;
  }
  return OrderStatus::Others;
}

XTP_MARKET_TYPE GetExchMarketCode(MarketCode marketCode) {
  switch (marketCode) {
    case MarketCode::SSE:
      return XTP_MKT_SH_A;
    case MarketCode::SZSE:
      return XTP_MKT_SZ_A;
    default:
      return XTP_MKT_UNKNOWN;
  }
}

MarketCode GetMarketCode(XTP_MARKET_TYPE exchMarketCode) {
  switch (exchMarketCode) {
    case XTP_MKT_SH_A:
      return MarketCode::SSE;
    case XTP_MKT_SZ_A:
      return MarketCode::SZSE;
    default:
      return MarketCode::Others;
  }
}

int GetExchSide(Side side) {
  switch (side) {
    case Side::Bid:
      return XTP_SIDE_BUY;
    case Side::Ask:
      return XTP_SIDE_SELL;
    default:
      return XTP_SIDE_UNKNOWN;
  }
}

Side GetSide(int exchSide) {
  switch (exchSide) {
    case XTP_SIDE_BUY:
      return Side::Bid;
    case XTP_SIDE_SELL:
      return Side::Ask;
    default:
      return Side::Others;
  }
}

}  // namespace bq::td::svc::xtp
