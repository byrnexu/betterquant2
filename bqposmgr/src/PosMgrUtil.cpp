/*!
 * \file PosMgrUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "PosMgrUtil.hpp"

#include "util/Logger.hpp"

namespace bq {

Decimal calcPnlOfCloseShort(SymbolType symbolType, Decimal openPrice,
                            Decimal closePrice, Decimal closePos,
                            std::uint32_t parValue) {
  Decimal ret = 0;

  //! 计算被平的空头仓位的已实现盈亏
  if (symbolType == SymbolType::Spot ||
      symbolType == SymbolType::CN_MainBoard ||
      symbolType == SymbolType::CN_SecondBoard ||
      symbolType == SymbolType::CN_StartupBoard ||
      symbolType == SymbolType::CN_TechBoard) {
    //! 现货 空头已实现盈亏=平仓数量*（平均开仓价格－平仓价格）
    ret = closePos * (openPrice - closePrice);

  } else if (symbolType == SymbolType::CN_Futures) {
    ret = static_cast<Decimal>(parValue) * closePos * (openPrice - closePrice);

  } else if (symbolType == SymbolType::Perp ||
             symbolType == SymbolType::Futures) {
    //! u本位合约 空头已实现盈亏=面值*平仓数量*（平均开仓价格－平仓价格）
    ret = closePos * (openPrice - closePrice);

  } else if (symbolType == SymbolType::CPerp ||
             symbolType == SymbolType::CFutures) {
    //! 币本位合约 空头已实现盈亏=面值*平仓数量*（1/平仓价格－1/平均开仓价格）
    ret = static_cast<Decimal>(parValue) * closePos *
          (1.0 / closePrice - 1.0 / openPrice);

  } else {
    LOG_W("Unhandled symbolType {}.", magic_enum::enum_name(symbolType));
  }

  return ret;
}

Decimal calcPnlOfCloseLong(SymbolType symbolType, Decimal openPrice,
                           Decimal closePrice, Decimal closePos,
                           std::uint32_t parValue) {
  Decimal ret = 0;

  //! 计算被平的多头仓位的已实现盈亏
  if (symbolType == SymbolType::Spot ||
      symbolType == SymbolType::CN_MainBoard ||
      symbolType == SymbolType::CN_SecondBoard ||
      symbolType == SymbolType::CN_StartupBoard ||
      symbolType == SymbolType::CN_TechBoard) {
    //! 现货 多头已实现盈亏=平仓数量*（平仓价格－平均开仓价格）
    ret = closePos * (closePrice - openPrice);

  } else if (symbolType == SymbolType::CN_Futures) {
    ret = static_cast<Decimal>(parValue) * closePos * (closePrice - openPrice);

  } else if (symbolType == SymbolType::Perp ||
             symbolType == SymbolType::Futures) {
    //! u本位合约 多头已实现盈亏=面值*平仓数量*（平仓价格－平均开仓价格）
    ret = closePos * (closePrice - openPrice);

  } else if (symbolType == SymbolType::CPerp ||
             symbolType == SymbolType::CFutures) {
    //! 币本位合约 多头已实现盈亏=面值*平仓数量*（1/平均开仓价格－1/平仓价格）
    ret = static_cast<Decimal>(parValue) * closePos *
          (1.0 / openPrice - 1.0 / closePrice);

  } else {
    LOG_W("Unhandled symbolType {}.", magic_enum::enum_name(symbolType));
  }

  return ret;
}

}  // namespace bq
