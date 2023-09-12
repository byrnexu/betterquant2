/*!
 * \file MDSvcOfXTPUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "MDSvcOfXTPUtil.hpp"

namespace bq::md::svc::xtp {

std::tuple<int, XTP_PROTOCOL_TYPE> GetXTPProtocolType(
    std::string protocolType) {
  boost::to_lower(protocolType);
  if (protocolType == "tcp") {
    return {0, XTP_PROTOCOL_TCP};
  } else if (protocolType == "udp") {
    return {0, XTP_PROTOCOL_UDP};
  } else {
    return {-1, XTP_PROTOCOL_TCP};
  }
}

MarketCode GetMarketCode(XTP_EXCHANGE_TYPE exchangeType) {
  switch (exchangeType) {
    case XTP_EXCHANGE_SH:
      return MarketCode::SSE;
    case XTP_EXCHANGE_SZ:
      return MarketCode::SZSE;
    default:
      return MarketCode::Others;
  }
}

XTP_EXCHANGE_TYPE GetExchMarketCode(MarketCode marketCode) {
  switch (marketCode) {
    case MarketCode::SSE:
      return XTP_EXCHANGE_SH;
    case MarketCode::SZSE:
      return XTP_EXCHANGE_SZ;
    default:
      return XTP_EXCHANGE_UNKNOWN;
  }
}

SymbolType GetSymbolType(XTP_SECURITY_TYPE exchSymbolType) {
  switch (exchSymbolType) {
    case XTP_SECURITY_MAIN_BOARD:
      //! 主板股票
      return SymbolType::CN_MainBoard;

    case XTP_SECURITY_SECOND_BOARD:
      //! 中小板股票
      return SymbolType::CN_SecondBoard;

    case XTP_SECURITY_STARTUP_BOARD:
      //! 创业板股票
      return SymbolType::CN_StartupBoard;

    case XTP_SECURITY_INDEX:
      //! 指数
      return SymbolType::CN_Index;

    case XTP_SECURITY_TECH_BOARD:
      //! 科创板股票(上海)
      return SymbolType::CN_TechBoard;

    case XTP_SECURITY_STATE_BOND:
      //! 国债
      return SymbolType::CN_StateBond;

    case XTP_SECURITY_ENTERPRICE_BOND:
      //! 企业债
      return SymbolType::CN_EnterpriseBond;

    case XTP_SECURITY_COMPANEY_BOND:
      //! 公司债
      return SymbolType::CN_CompanyBond;

    case XTP_SECURITY_CONVERTABLE_BOND:
      //! 转换债券
      return SymbolType::CN_ConvertableBond;

    case XTP_SECURITY_NATIONAL_BOND_REVERSE_REPO:
      //! 国债逆回购
      return SymbolType::CN_NationalBondReverseRepo;

    case XTP_SECURITY_ETF_SINGLE_MARKET_STOCK:
      //! 本市场股票 ETF
      return SymbolType::CN_ETF_SingleMarketBond;

    case XTP_SECURITY_ETF_INTER_MARKET_STOCK:
      //! 跨市场股票 ETF
      return SymbolType::CN_ETF_InterMarketStock;

    case XTP_SECURITY_ETF_CROSS_BORDER_STOCK:
      // 跨境股票 ETF
      return SymbolType::CN_ETF_CrossBorderStock;

    case XTP_SECURITY_ETF_SINGLE_MARKET_BOND:
      //! 本市场实物债券 ETF
      return SymbolType::CN_ETF_SingleMarketBond;

    case XTP_SECURITY_ETF_GOLD:
      //! 黄金 ETF
      return SymbolType::CN_ETF_Gold;

    case XTP_SECURITY_STRUCTURED_FUND_CHILD:
      //! 分级基金子基金
      return SymbolType::CN_StructuredFundChild;

    case XTP_SECURITY_SZSE_RECREATION_FUND:
      //! 深交所仅申赎基金
      return SymbolType::CN_SZSE_RecreationFund;

    case XTP_SECURITY_STOCK_OPTION:
      //! 个股期权
      return SymbolType::CN_StockOption;

    case XTP_SECURITY_ETF_OPTION:
      //! ETF期权
      return SymbolType::CN_ETF_Option;

    case XTP_SECURITY_ALLOTMENT:
      //! 配股
      return SymbolType::CN_Allotment;

    case XTP_SECURITY_MONETARY_FUND_SHCR:
      //! 上交所申赎型货币基金
      return SymbolType::CN_MonetaryFundSHTR;

    case XTP_SECURITY_MONETARY_FUND_SHTR:
      //! 上交所交易型货币基金
      return SymbolType::CN_MonetaryFundSHTR;

    case XTP_SECURITY_MONETARY_FUND_SZ:
      //! 深交所货币基金
      return SymbolType::CN_MonetaryFundSZ;

    default:
      return SymbolType::Others;
  }
}

SymbolState GetSymbolState(XTP_SECURITY_STATUS exchSymbolState) {
  switch (exchSymbolState) {
    case XTP_SECURITY_STATUS_ST:
      //! 风险警示板
      return CN_StatusST;

    case XTP_SECURITY_STATUS_N_IPO:
      //! 首日上市
      return CN_StatusNIPO;

    case XTP_SECURITY_STATUS_COMMON:
      //! 普通
      return CN_StatusCommon;

    case XTP_SECURITY_STATUS_RESUME:
      //! 恢复上市
      return CN_StatusResume;

    case XTP_SECURITY_STATUS_DELISTING:
      //! 退市整理期
      return CN_StatusDelisting;

    default:
      return SymbolState::Others;
  }
}

Side GetSideFromTrades(MarketCode marketCode, char trade_flag) {
  if (marketCode == MarketCode::SSE) {
    if (trade_flag == 'B') {
      return Side::Bid;
    } else if (trade_flag == 'S') {
      return Side::Ask;
    } else {
      return Side::Others;
    }
  } else if (marketCode == MarketCode::SZSE) {
    return Side::Others;
  } else {
    return Side::Others;
  }
}

Side GetSideFromOrders(MarketCode marketCode, char side) {
  if (marketCode == MarketCode::SSE) {
    if (side == 'B') {
      return Side::Bid;
    } else if (side == 'S') {
      return Side::Ask;
    } else {
      return Side::Others;
    }
  } else if (marketCode == MarketCode::SZSE) {
    if (side == '1') {
      return Side::Bid;
    } else if (side == '2') {
      return Side::Ask;
    } else {
      return Side::Others;
    }
  } else {
    return Side::Others;
  }
}

}  // namespace bq::md::svc::xtp
