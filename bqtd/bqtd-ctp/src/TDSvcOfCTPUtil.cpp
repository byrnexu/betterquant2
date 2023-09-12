/*!
 * \file TDSvcOfCTPUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#include "TDSvcOfCTPUtil.hpp"

#include "API.hpp"
#include "Config.hpp"
#include "TDSvcOfCN.hpp"
#include "TDSvcOfCTPConst.hpp"
#include "TDSvcUtil.hpp"
#include "def/BQConst.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq::td::svc::ctp {

TThostFtdcDirectionType GetExchSide(Side side) {
  switch (side) {
    case Side::Bid:
      return THOST_FTDC_D_Buy;
    case Side::Ask:
      return THOST_FTDC_D_Sell;
  }
  return THOST_FTDC_D_Buy;
}

Side GetExchSide(TThostFtdcDirectionType exchSide) {
  switch (exchSide) {
    case THOST_FTDC_D_Buy:
      return Side::Bid;
    case THOST_FTDC_D_Sell:
      return Side::Ask;
  }
  return Side::Bid;
}

std::tuple<int, TThostFtdcDirectionType, TThostFtdcOffsetFlagType>
GetExchSideInfo(const OrderInfoSPtr& orderInfo) {
  if (orderInfo->side_ == Side::Bid) {
    if (orderInfo->posDirection_ == PosDirection::Open) {
      return {0, THOST_FTDC_D_Buy, THOST_FTDC_OF_Open};
    } else if (orderInfo->posDirection_ == PosDirection::Close) {
      return {0, THOST_FTDC_D_Buy, THOST_FTDC_OF_Close};
    } else if (orderInfo->posDirection_ == PosDirection::CloseTDay) {
      return {0, THOST_FTDC_D_Buy, THOST_FTDC_OF_CloseToday};
    } else if (orderInfo->posDirection_ == PosDirection::CloseYDay) {
      return {0, THOST_FTDC_D_Buy, THOST_FTDC_OF_CloseYesterday};
    } else {
      LOG_W("Get exch side info failed. [side: {};  posDirection: {}]",
            magic_enum::enum_name(orderInfo->side_),
            magic_enum::enum_name(orderInfo->posDirection_));
    }
  } else if (orderInfo->side_ == Side::Ask) {
    if (orderInfo->posDirection_ == PosDirection::Open) {
      return {0, THOST_FTDC_D_Sell, THOST_FTDC_OF_Open};
    } else if (orderInfo->posDirection_ == PosDirection::Close) {
      return {0, THOST_FTDC_D_Sell, THOST_FTDC_OF_Close};
    } else if (orderInfo->posDirection_ == PosDirection::CloseTDay) {
      return {0, THOST_FTDC_D_Sell, THOST_FTDC_OF_CloseToday};
    } else if (orderInfo->posDirection_ == PosDirection::CloseYDay) {
      return {0, THOST_FTDC_D_Sell, THOST_FTDC_OF_CloseYesterday};
    } else {
      LOG_W("Get exch side info failed. [side: {};  posDirection: {}]",
            magic_enum::enum_name(orderInfo->side_),
            magic_enum::enum_name(orderInfo->posDirection_));
    }
  } else {
    LOG_W("Get exch side info failed. [side: {};  posDirection: {}]",
          magic_enum::enum_name(orderInfo->side_),
          magic_enum::enum_name(orderInfo->posDirection_));
    return {-1, '0', '0'};
  }

  return {-1, '0', '0'};
}

std::string GetExchange(MarketCode marketCode) {
  switch (marketCode) {
    case MarketCode::SHFE:
      return "SHFE";

    case MarketCode::CZCE:
      return "CZCE";

    case MarketCode::DCE:
      return "DCE";

    case MarketCode::CFFEX:
      return "CFFEX";

    case MarketCode::INE:
      return "INE";

    default:
      LOG_W("Get exchange failed. [marketCode: {}]", GetMarketName(marketCode));
      return "";
  }
}

MarketCode GetMarketCode(const char* exchange) {
  if (std::strcmp(exchange, "SHFE") == 0) {
    return MarketCode::SHFE;
  } else if (std::strcmp(exchange, "CZCE") == 0) {
    return MarketCode::CZCE;
  } else if (std::strcmp(exchange, "DCE") == 0) {
    return MarketCode::DCE;
  } else if (std::strcmp(exchange, "CFFEX") == 0) {
    return MarketCode::CFFEX;
  } else if (std::strcmp(exchange, "INE") == 0) {
    return MarketCode::INE;
  } else {
    LOG_W("Get market code failed. [exchange: {}]", exchange);
    return MarketCode::Others;
  }
}

OrderStatus GetOrderStatus(TThostFtdcOrderStatusType exchOrderStatus) {
  switch (exchOrderStatus) {
    case THOST_FTDC_OST_NoTradeQueueing:
      return OrderStatus::ConfirmedByExch;

    case THOST_FTDC_OST_PartTradedQueueing:
      return OrderStatus::PartialFilled;

    case THOST_FTDC_OST_AllTraded:
      return OrderStatus::Filled;

    case THOST_FTDC_OST_PartTradedNotQueueing:
      return OrderStatus::PartialFilledCanceled;

    case THOST_FTDC_OST_Canceled:
      return OrderStatus::Canceled;

    case THOST_FTDC_OST_NoTradeNotQueueing:
      //! 未成交不在队列中
    case THOST_FTDC_OST_Unknown:
      //! 未知
    case THOST_FTDC_OST_NotTouched:
      //! 尚未触发
    case THOST_FTDC_OST_Touched:
      //! 已触发
      return OrderStatus::Others;

    default:
      return OrderStatus::Others;
  }
}

}  // namespace bq::td::svc::ctp
