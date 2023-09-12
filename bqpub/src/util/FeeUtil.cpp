/*!
 * \file FeeUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/01
 *
 * \brief
 */

#include "util/FeeUtil.hpp"

#include "def/BQConst.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"

namespace bq {

Decimal calcFee(const OrderInfoSPTr& orderInfo, Decimal feeRatio,
                std::uint32_t parValue) {
  Decimal ret = 0;
  switch (orderInfo->symbolType_) {
    case SymbolType::CN_MainBoard:
    case SymbolType::CN_SecondBoard:
    case SymbolType::CN_StartupBoard:
    case SymbolType::CN_TechBoard:
      ret = feeRatio * orderInfo->dealSize_ * orderInfo->avgDealPrice_;
      break;

    case SymbolType::CN_Futures:
      ret = feeRatio * orderInfo->dealSize_ * orderInfo->avgDealPrice_ *
            static_cast<Decimal>(orderInfo->parValue_);
      break;

    case SymbolType::Spot:
      ret = orderInfo->side_ == Side::Bid
                ? feeRatio * orderInfo->dealSize_
                : feeRatio * orderInfo->dealSize_ * orderInfo->avgDealPrice_;
      break;

    case SymbolType::Perp:
    case SymbolType::Futures:
      //! u本位合约成交的单位是币的个数，收的手续费是u
      ret = feeRatio * orderInfo->dealSize_ * orderInfo->avgDealPrice_;
      break;

    case SymbolType::CPerp:
    case SymbolType::CFutures:
      //! 币本位合约成交的单位是合约的张数，收的手续费是币
      if (!DEC::ZERO(orderInfo->avgDealPrice_)) {
        ret = feeRatio * (orderInfo->dealSize_ * parValue) /
              orderInfo->avgDealPrice_;
      }
      break;

    default:
      break;
  }

  return ret >= 0 ? ret : ret * -1;
}

std::string getFeeCurrency(const OrderInfoSPTr& orderInfo,
                           const std::string& baseCurrency,
                           const std::string& quoteCurrency) {
  switch (orderInfo->symbolType_) {
    case SymbolType::CN_MainBoard:
    case SymbolType::CN_SecondBoard:
    case SymbolType::CN_StartupBoard:
    case SymbolType::CN_TechBoard:
    case SymbolType::CN_Futures:
      return CN_OFFICIAL_CURRENCY;

    case SymbolType::Spot:
      return orderInfo->side_ == Side::Bid ? baseCurrency : quoteCurrency;

    case SymbolType::Perp:
    case SymbolType::Futures:
      return quoteCurrency;

    case SymbolType::CPerp:
    case SymbolType::CFutures:
      return baseCurrency;

    default:
      break;
  }
  return "";
}

}  // namespace bq
