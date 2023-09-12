/*!
 * \file MDSvcOfCTPUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "MDSvcOfCTPUtil.hpp"

#include "util/Datetime.hpp"
#include "util/Logger.hpp"

namespace bq::md::svc::ctp {

SymbolType GetSymbolType(TThostFtdcAPIProductClassType exchSymbolType) {
  switch (exchSymbolType) {
    case THOST_FTDC_APC_FutureSingle:
      return SymbolType::CN_Futures;

    case THOST_FTDC_APC_OptionSingle:
      return SymbolType::CN_FuturesOptions;

    case THOST_FTDC_PC_Combination:
      return SymbolType::CN_FuturesComb;

    case THOST_FTDC_PC_Spot:
      return SymbolType::CN_SpotFutures;

    case THOST_FTDC_PC_EFP:
      return SymbolType::CN_FuturesEFP;

    case THOST_FTDC_PC_SpotOption:
      return SymbolType::CN_SpotOptions;

    case THOST_FTDC_PC_TAS:
      return SymbolType::CN_FuturesTAS;

    case THOST_FTDC_PC_MI:
      return SymbolType::CN_FuturesMI;

    default:
      return SymbolType::Others;
  }
}

}  // namespace bq::md::svc::ctp
