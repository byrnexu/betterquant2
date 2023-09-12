/*!
 * \file PosInfo.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "def/PosInfo.hpp"

#include "db/TBLPosInfo.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/PnlIF.hpp"
#include "def/SymbolCodeIF.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"
#include "util/MarketDataCache.hpp"
#include "util/String.hpp"

namespace bq {

PosInfo::PosInfo() {}

PnlSPtr PosInfo::calcPnl(
    const MarketDataCacheSPtr& marketDataCache,
    const std::string& quoteCurrencyForCalc,
    const std::string& quoteCurrencyForConv,
    const std::string& origQuoteCurrencyOfUBasedContract) const {
  auto pnl = std::make_shared<Pnl>();
  pnl->quoteCurrencyForCalc_ = quoteCurrencyForCalc;

  if (symbolType_ == SymbolType::CN_MainBoard ||
      symbolType_ == SymbolType::CN_SecondBoard ||
      symbolType_ == SymbolType::CN_StartupBoard ||
      symbolType_ == SymbolType::CN_TechBoard ||
      symbolType_ == SymbolType::CN_Futures) {
    pnl->pnlUnReal_ = pnlUnReal_;
    pnl->pnlReal_ = pnlReal_;
    pnl->updateTime_ = updateTime_;
    pnl->fee_ = fee_;

  } else if (symbolType_ == SymbolType::Spot) {
    const auto [ret, baseCurrency, quoteCurrency] =
        SplitStrIntoTwoParts(std::string(symbolCode_), SEP_OF_SYMBOL_SPOT);
    //! calc pnl
    {
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, baseCurrency, quoteCurrency,
                    quoteCurrencyForCalc, quoteCurrencyForConv);
      if (statusCode == 0) {
        LOG_T("Calc price of {}-{} success. [price: {}; delay: {}s]",
              quoteCurrency, quoteCurrencyForCalc, lastPrice,
              (GetTotalSecSince1970() - updateTime / 1000000));
        //! pnlUnReal_ PubSvc中已经算好了，quoteCurrency和quoteCurrencyForCalc相
        //! 等的话lastPrice=1
        pnl->pnlUnReal_ = pnlUnReal_ / lastPrice;
        pnl->pnlReal_ = pnlReal_ / lastPrice;
        pnl->updateTime_ = updateTime_;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              quoteCurrency, SEP_OF_SYMBOL_SPOT, quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

    //! calc fee
    {
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, feeCurrency_, feeCurrency_,
                    quoteCurrencyForCalc, quoteCurrencyForConv);
      if (statusCode == 0) {
        LOG_T("Calc price of {}-{} success. [price: {}; delay: {}s]",
              feeCurrency_, quoteCurrencyForCalc, lastPrice,
              (GetTotalSecSince1970() - updateTime / 1000000));
        pnl->fee_ = fee_ / lastPrice;
        if (updateTime < pnl->updateTime_) pnl->updateTime_ = updateTime;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              feeCurrency_, SEP_OF_SYMBOL_SPOT, quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

  } else if (symbolType_ == SymbolType::Perp ||
             symbolType_ == SymbolType::Futures) {
    // calc pnl
    {
      //! pnl都是以USD或者USDT计价的
      const auto sep = symbolType_ == SymbolType::Perp ? SEP_OF_SYMBOL_PERP
                                                       : SEP_OF_SYMBOL_FUTURES;
      const auto baseCurrency =
          std::string(symbolCode_, strstr(symbolCode_, sep.c_str()));
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, baseCurrency,
                    origQuoteCurrencyOfUBasedContract, quoteCurrencyForCalc,
                    quoteCurrencyForConv);
      if (statusCode == 0) {
        pnl->pnlUnReal_ = pnlUnReal_ / lastPrice;
        pnl->pnlReal_ = pnlReal_ / lastPrice;
        pnl->updateTime_ = updateTime_;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              origQuoteCurrencyOfUBasedContract, SEP_OF_SYMBOL_SPOT,
              quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

    //! calc fee
    {
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, feeCurrency_, feeCurrency_,
                    quoteCurrencyForCalc, quoteCurrencyForConv);
      if (statusCode == 0) {
        pnl->fee_ = fee_ / lastPrice;
        if (updateTime < pnl->updateTime_) pnl->updateTime_ = updateTime;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              feeCurrency_, SEP_OF_SYMBOL_SPOT, quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

  } else if (symbolType_ == SymbolType::CPerp ||
             symbolType_ == SymbolType::CFutures) {
    // calc pnl
    {
      //! pnl都是以币本位计价的
      const auto sep = symbolType_ == SymbolType::Perp ? SEP_OF_SYMBOL_PERP
                                                       : SEP_OF_SYMBOL_FUTURES;
      const auto baseCurrency =
          std::string(symbolCode_, strstr(symbolCode_, sep.c_str()));
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, baseCurrency, baseCurrency,
                    quoteCurrencyForCalc, quoteCurrencyForConv);
      if (statusCode == 0) {
        pnl->pnlUnReal_ = pnlUnReal_ / lastPrice;
        pnl->pnlReal_ = pnlReal_ / lastPrice;
        pnl->updateTime_ = updateTime_;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              baseCurrency, SEP_OF_SYMBOL_SPOT, quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

    //! calc fee
    {
      const auto [statusCode, symbolGroupForCalc, updateTime, lastPrice] =
          CalcPrice(marketDataCache, marketCode_, feeCurrency_, feeCurrency_,
                    quoteCurrencyForCalc, quoteCurrencyForConv);
      if (statusCode == 0) {
        pnl->fee_ = fee_ / lastPrice;
        if (updateTime < pnl->updateTime_) pnl->updateTime_ = updateTime;
      } else {
        //! 获取价格失败，直接退出
        pnl->statusCode_ = statusCode;
        pnl->symbolGroupForCalc_ = std::move(symbolGroupForCalc);
        LOG_W("Calc pnl failed because of calc price of {}{}{} failed. {}",
              feeCurrency_, SEP_OF_SYMBOL_SPOT, quoteCurrencyForCalc, toStr());
        return pnl;
      }
    }

  } else {
    LOG_W("Unhandled symbolType {}.", magic_enum::enum_name(symbolType_));
  }

  return pnl;
}

std::string PosInfo::toStr() const {
  std::string ret;
  ret = fmt::format(
      "{} fee={}; pos={}; prePos={}; avgOpenPrice={}; pnlUnReal={}; pnlReal={}",
      getKey(), fee_, pos_, prePos_, avgOpenPrice_, pnlUnReal_, pnlReal_);
  return ret;
}

std::string PosInfo::getKey() const {
  const auto ret = fmt::format(  // add stgGrpId
      "{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}", productGrpId_,
      productId_, userId_, acctGrpId_, acctId_, trdAcctId_, stgGrpId_, stgId_,
      stgInstId_, algoId_, GetMarketName(marketCode_),
      magic_enum::enum_name(symbolType_), symbolCode_,
      magic_enum::enum_name(side_), magic_enum::enum_name(posSide_), parValue_,
      feeCurrency_);
  return ret;
}

std::string PosInfo::getKeyOfStgInst() const {
  const auto ret = fmt::format(
      "{}/{}/{}/{}/{}/{}/{}/{}/{}/{}", acctId_, trdAcctId_, algoId_,
      GetMarketName(marketCode_), magic_enum::enum_name(symbolType_),
      symbolCode_, magic_enum::enum_name(side_),
      magic_enum::enum_name(posSide_), parValue_, feeCurrency_);
  return ret;
}

std::string PosInfo::getTopicPrefixForSub() const {
  SymbolType symbolTypeForSub = SymbolType::Others;
  if (symbolType_ == SymbolType::CN_MainBoard ||                //
      symbolType_ == SymbolType::CN_SecondBoard ||              //
      symbolType_ == SymbolType::CN_StartupBoard ||             //
      symbolType_ == SymbolType::CN_Index ||                    //
      symbolType_ == SymbolType::CN_TechBoard ||                //
      symbolType_ == SymbolType::CN_StateBond ||                //
      symbolType_ == SymbolType::CN_EnterpriseBond ||           //
      symbolType_ == SymbolType::CN_CompanyBond ||              //
      symbolType_ == SymbolType::CN_ConvertableBond ||          //
      symbolType_ == SymbolType::CN_NationalBondReverseRepo ||  //
      symbolType_ == SymbolType::CN_ETF_SingleMarketStock ||    //
      symbolType_ == SymbolType::CN_ETF_InterMarketStock ||     //
      symbolType_ == SymbolType::CN_ETF_CrossBorderStock ||     //
      symbolType_ == SymbolType::CN_ETF_SingleMarketBond ||     //
      symbolType_ == SymbolType::CN_ETF_Gold ||                 //
      symbolType_ == SymbolType::CN_StructuredFundChild ||      //
      symbolType_ == SymbolType::CN_SZSE_RecreationFund ||      //
      symbolType_ == SymbolType::CN_StockOption ||              //
      symbolType_ == SymbolType::CN_ETF_Option ||               //
      symbolType_ == SymbolType::CN_Allotment ||                //
      symbolType_ == SymbolType::CN_MoneyMonetaryFundSHCR ||    //
      symbolType_ == SymbolType::CN_MonetaryFundSHTR ||         //
      symbolType_ == SymbolType::CN_MonetaryFundSZ) {
    symbolTypeForSub = SymbolType::Spot;

  } else if (symbolType_ == SymbolType::CN_Futures ||         //
             symbolType_ == SymbolType::CN_FuturesOptions ||  //
             symbolType_ == SymbolType::CN_FuturesComb ||     //
             symbolType_ == SymbolType::CN_SpotFutures ||     //
             symbolType_ == SymbolType::CN_FuturesEFP ||      //
             symbolType_ == SymbolType::CN_SpotOptions ||     //
             symbolType_ == SymbolType::CN_FuturesTAS ||      //
             symbolType_ == SymbolType::CN_FuturesMI) {
    symbolTypeForSub = SymbolType::Futures;

  } else {
    symbolTypeForSub = symbolType_;
  }

  const auto ret =
      fmt::format("{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_TOPIC,
                  GetMarketName(marketCode_), SEP_OF_TOPIC,
                  magic_enum::enum_name(symbolTypeForSub), SEP_OF_TOPIC,
                  symbolCode_, SEP_OF_TOPIC);
  return ret;
}

bool PosInfo::oneMoreFeeCurrencyThanInput(const PosInfoSPtr& posInfo) const {
  bool keyWithoutFeeCurrencyNotEqual =  // add stgGrpId
      (productGrpId_ != posInfo->productGrpId_ ||
       productId_ != posInfo->productId_ || userId_ != posInfo->userId_ ||
       acctGrpId_ != posInfo->acctGrpId_ || acctId_ != posInfo->acctId_ ||
       trdAcctId_ != posInfo->trdAcctId_ || stgGrpId_ != posInfo->stgGrpId_ ||
       stgId_ != posInfo->stgId_ || stgInstId_ != posInfo->stgInstId_ ||
       algoId_ != posInfo->algoId_ || marketCode_ != posInfo->marketCode_ ||
       symbolType_ != posInfo->symbolType_ ||
       strcmp(symbolCode_, posInfo->symbolCode_) != 0 ||
       side_ != posInfo->side_ || posSide_ != posInfo->posSide_ ||
       parValue_ != posInfo->parValue_);

  if (keyWithoutFeeCurrencyNotEqual || feeCurrency_[0] == '\0') {
    return false;
  } else {
    return true;
  }
}

bool PosInfo::isEqual(const PosInfoSPtr& posInfo) {
  if (pnlUnReal_ != posInfo->pnlUnReal_) return false;
  if (pnlReal_ != posInfo->pnlReal_) return false;
  if (totalBidSize_ != posInfo->totalBidSize_) return false;
  if (totalAskSize_ != posInfo->totalAskSize_) return false;
  if (preTotalBidSize_ != posInfo->preTotalBidSize_) return false;
  if (preTotalAskSize_ != posInfo->preTotalAskSize_) return false;
  if (totalOpenSize_ != posInfo->totalOpenSize_) return false;
  if (preTotalOpenSize_ != posInfo->preTotalOpenSize_) return false;
  if (fee_ != posInfo->fee_) return false;
  if (pos_ != posInfo->pos_) return false;
  if (prePos_ != posInfo->prePos_) return false;
  if (avgOpenPrice_ != posInfo->avgOpenPrice_) return false;
  if (preAvgOpenPrice_ != posInfo->preAvgOpenPrice_) return false;
  return true;
}

// clang-format off
std::string PosInfo::getSqlOfReplace() const {
const auto sql = fmt::format(
"INSERT INTO {} ("
  "`productGrpId`,"
  "`productId`,"
  "`userId`,"
  "`acctGrpId`,"
  "`acctId`,"
  "`trdAcctId`,"
  "`stgGrpId`,"  // add stgGrpId
  "`stgId`,"
  "`stgInstId`,"
  "`algoId`,"
  "`marketCode`,"
  "`symbolType`,"
  "`symbolCode`,"
  "`side`,"
  "`posSide`,"
  "`parValue`,"
  "`feeCurrency`,"
  "`fee`,"
  "`pos`,"
  "`prePos`,"
  "`avgOpenPrice`,"
  "`preAvgOpenPrice`,"
  "`pnlReal`,"
  "`totalBidSize`,"
  "`totalAskSize`,"
  "`preTotalBidSize`,"
  "`preTotalAskSize`,"
  "`totalOpenSize`,"
  "`preTotalOpenSize`,"
  "`lastNoUsedToCalcPos`"
")"
"VALUES"
"("
  " {} ,"  // productGrpId
  " {} ,"  // productId
  " {} ,"  // userId
  " {} ,"  // acctGrpId
  " {} ,"  // acctId
  " {} ,"  // trdAcctId
  " {} ,"  // stgGrpId add stgGrpId
  " {} ,"  // stgId
  " {} ,"  // stgInstId
  " {} ,"  // algoId
  "'{}',"  // marketCode
  "'{}',"  // symbolType
  "'{}',"  // symbolCode
  "'{}',"  // side
  "'{}',"  // posSide
  " {} ,"  // parValue
  "'{}',"  // feeCurrency
  "'{}',"  // fee
  "'{}',"  // pos
  "'{}',"  // prePos
  "'{}',"  // avgOpenPrice
  "'{}',"  // preAvgOpenPrice
  "'{}',"  // pnlReal
  "'{}',"  // totalBidSize
  "'{}',"  // totalAskSize
  "'{}',"  // preTotalBidSize
  "'{}',"  // preTotalAskSize
  "'{}',"  // totalOpenSize
  "'{}',"  // preTotalOpenSize
  " {}  "  // lastNoUsedToCalcPos
") "
  "ON DUPLICATE KEY UPDATE "
  "feeCurrency = '{}',"
  "fee = {},"
  "pos = {},"
  "prePos = {},"
  "avgOpenPrice = {},"
  "preAvgOpenPrice = {},"
  "pnlReal = {},"
  "totalBidSize = {},"
  "totalAskSize = {},"
  "preTotalBidSize = {},"
  "preTotalAskSize = {},"
  "totalOpenSize = {},"
  "preTotalOpenSize = {},"
  "lastNoUsedToCalcPos = {};" 
  ,
  TBLPosInfo::TableName,
  productGrpId_,
  productId_,
  userId_,
  acctGrpId_,
  acctId_,
  trdAcctId_,
  stgGrpId_,  // add stgGrpId
  stgId_,
  stgInstId_,
  algoId_,
  GetMarketName(marketCode_),
  magic_enum::enum_name(symbolType_),
  symbolCode_,
  magic_enum::enum_name(side_),
  magic_enum::enum_name(posSide_),
  parValue_,
  feeCurrency_,
  fee_,
  pos_,
  prePos_,
  avgOpenPrice_,
  preAvgOpenPrice_,
  pnlReal_,
  totalBidSize_,
  totalAskSize_,
  preTotalBidSize_,
  preTotalAskSize_,
  totalOpenSize_,
  preTotalOpenSize_,
  lastNoUsedToCalcPos_, 
  feeCurrency_,
  fee_,
  pos_,
  prePos_,
  avgOpenPrice_,
  preAvgOpenPrice_,
  pnlReal_,
  totalBidSize_,
  totalAskSize_,
  preTotalBidSize_,
  preTotalAskSize_,
  totalOpenSize_,
  preTotalOpenSize_,
  lastNoUsedToCalcPos_ 
);
return sql;
} ;

// clang-format off
std::string PosInfo::getSqlOfUpdatePnl() const {
const auto sql = fmt::format(
"INSERT INTO {} ("
  "`productGrpId`,"
  "`productId`,"
  "`userId`,"
  "`acctGrpId`,"
  "`acctId`,"
  "`trdAcctId`,"
  "`stgGrpId`,"  // add stgGrpId
  "`stgId`,"
  "`stgInstId`,"
  "`algoId`,"
  "`marketCode`,"
  "`symbolType`,"
  "`symbolCode`,"
  "`side`,"
  "`posSide`,"
  "`parValue`,"
  "`pnlUnReal`,"
  "`updateTime`"
")"
"VALUES"
"("
  " {} ,"  // productGrpId
  " {} ,"  // productId
  " {} ,"  // userId
  " {} ,"  // acctGrpId
  " {} ,"  // acctId
  " {} ,"  // trdAcctId
  " {} ,"  // stgGrpId add stgGrpId
  " {} ,"  // stgId
  " {} ,"  // stgInstId
  " {} ,"  // algoId
  "'{}',"  // marketCode
  "'{}',"  // symbolType
  "'{}',"  // symbolCode
  "'{}',"  // side
  "'{}',"  // posSide
  " {} ,"  // parValue
  "'{}',"  // pnlUnReal
  "'{}' "  // updateTime
")  "
  "ON DUPLICATE KEY UPDATE "
  "pnlUnReal = {},"
  "updateTime = '{}';"
  ,
  TBLPosInfo::TableName,
  productGrpId_,
  productId_,
  userId_,
  acctGrpId_,
  acctId_,
  trdAcctId_,
  stgGrpId_,  // add stgGrpId
  stgId_,
  stgInstId_,
  algoId_,
  GetMarketName(marketCode_),
  magic_enum::enum_name(symbolType_),
  symbolCode_,
  magic_enum::enum_name(side_),
  magic_enum::enum_name(posSide_),
  parValue_,
  pnlUnReal_,
  ConvertTsToDBTime(updateTime_),
  pnlUnReal_,
  ConvertTsToDBTime(updateTime_)
);
return sql;
} ;

std::string PosInfo::getSqlOfInsert() const {
const auto sql = fmt::format(
"INSERT INTO {} ("
  "`productGrpId`,"
  "`productId`,"
  "`userId`,"
  "`acctGrpId`,"
  "`acctId`,"
  "`trdAcctId`,"
  "`stgGrpId`,"  // add stgGrpId
  "`stgId`,"
  "`stgInstId`,"
  "`algoId`,"
  "`marketCode`,"
  "`symbolType`,"
  "`symbolCode`,"
  "`side`,"
  "`posSide`,"
  "`parValue`,"
  "`feeCurrency`,"
  "`fee`,"
  "`pos`,"
  "`prePos`,"
  "`avgOpenPrice`,"
  "`preAvgOpenPrice`,"
  "`pnlUnReal`,"
  "`pnlReal`,"
  "`totalBidSize`,"
  "`totalAskSize`,"
  "`preTotalBidSize`,"
  "`preTotalAskSize`,"
  "`totalOpenSize`,"
  "`preTotalOpenSize`,"
  "`lastNoUsedToCalcPos`,"
  "`updateTime`"
")"
"VALUES"
"("
  " {} ,"  // productGrpId
  " {} ,"  // productId
  " {} ,"  // userId
  " {} ,"  // acctGrpId
  " {} ,"  // acctId
  " {} ,"  // trdAcctId
  " {} ,"  // stgGrpId add stgGrpId
  " {} ,"  // stgId
  " {} ,"  // stgInstId
  " {} ,"  // algoId
  "'{}',"  // marketCode
  "'{}',"  // symbolType
  "'{}',"  // symbolCode
  "'{}',"  // side
  "'{}',"  // posSide
  " {} ,"  // parValue
  "'{}',"  // feeCurrency
  "'{}',"  // fee
  "'{}',"  // pos
  "'{}',"  // prePos
  "'{}',"  // avgOpenPrice
  "'{}',"  // preAvgOpenPrice
  "'{}',"  // pnlUnReal
  "'{}',"  // pnlReal
  "'{}',"  // totalBidSize
  "'{}',"  // totalAskSize
  "'{}',"  // preTotalBidSize
  "'{}',"  // preTotalAskSize
  "'{}',"  // totalOpenSize
  "'{}',"  // preTotalOpenSize
  " {} ,"  // lastNoUsedToCalcPos
  "'{}' "  // updateTime
"); ",
  TBLPosInfo::TableName,
  productGrpId_,
  productId_,
  userId_,
  acctGrpId_,
  acctId_,
  trdAcctId_,
  stgGrpId_,  // add stgGrpId
  stgId_,
  stgInstId_,
  algoId_,
  GetMarketName(marketCode_),
  magic_enum::enum_name(symbolType_),
  symbolCode_,
  magic_enum::enum_name(side_),
  magic_enum::enum_name(posSide_),
  parValue_,
  feeCurrency_,
  fee_,
  pos_,
  prePos_,
  avgOpenPrice_,
  preAvgOpenPrice_,
  pnlUnReal_,
  pnlReal_,
  totalBidSize_,
  totalAskSize_,
  preTotalBidSize_,
  preTotalAskSize_,
  totalOpenSize_,
  preTotalOpenSize_,
  lastNoUsedToCalcPos_,
  ConvertTsToDBTime(updateTime_)
);
return sql;
} ;

std::string PosInfo::getSqlOfUpdate() const {
const auto sql = fmt::format(
"UPDATE {} SET "
  "`fee`                = {}, "
  "`pos`                = {}, "
  "`prePos`             = {}, "
  "`avgOpenPrice`       = {}, "
  "`preAvgOpenPrice`    = {}, "
  "`pnlUnreal`          = {}, "
  "`pnlReal`            = {}, "
  "`totalBidSize`       = {}, "
  "`totalAskSize`       = {}, "
  "`preTotalBidSize`    = {}, "
  "`preTotalAskSize`    = {}, "
  "`totalOpenSize`      = {}, "
  "`preTotalOpenSize`   = {}, "
  "`lastNoUsedToCalcPos`= {},"
  "`updateTime`         ='{}' "
"WHERE `productGrpId` = {}  "
  "AND `productId`    = {}  "
  "AND `userId`       = {}  "
  "AND `acctGrpId`    = {}  "
  "AND `acctId`       = {}  "
  "AND `trdAcctId`    = {}  "
  "AND `stgGrpId`     = {}  "  // add stgGrpId
  "AND `stgId`        = {}  "
  "AND `stgInstId`    = {}  "
  "AND `algoId`       = {}  "
  "AND `marketCode`   ='{}' "
  "AND `symbolType`   ='{}' "
  "AND `symbolCode`   ='{}' "
  "AND `side`         ='{}' "
  "AND `posSide`      ='{}' "
  "AND `parValue`     = {}  "
  "AND `feeCurrency`  ='{}';",
  TBLPosInfo::TableName,
  fee_,
  pos_,
  prePos_,
  avgOpenPrice_,
  preAvgOpenPrice_,
  pnlUnReal_,
  pnlReal_,
  totalBidSize_,
  totalAskSize_,
  preTotalBidSize_,
  preTotalAskSize_,
  totalOpenSize_,
  preTotalOpenSize_,
  lastNoUsedToCalcPos_,
  ConvertTsToDBTime(updateTime_),
  productGrpId_,
  productId_,
  userId_,
  acctGrpId_,
  acctId_,
  trdAcctId_,
  stgGrpId_,  // add stgGrpId
  stgId_,
  stgInstId_,
  algoId_,
  GetMarketName(marketCode_),
  magic_enum::enum_name(symbolType_),
  symbolCode_,
  magic_enum::enum_name(side_),
  magic_enum::enum_name(posSide_),
  parValue_,
  feeCurrency_
);
return sql;
}

std::string PosInfo::getSqlOfDelete() const{
const auto sql = fmt::format(
  "DELETE FROM {} "
  "WHERE `productGrpId` = {}  "
  "  AND `productId`    = {}  "
  "  AND `userId`       = {}  "
  "  AND `acctGrpId`    = {}  "
  "  AND `acctId`       = {}  "
  "  AND `trdAcctId`    = {}  "
  "  AND `stgGrpId`     = {}  "  // add stgGrpId
  "  AND `stgId`        = {}  "
  "  AND `stgInstId`    = {}  "
  "  AND `algoId`       = {}  "
  "  AND `marketCode`   ='{}' "
  "  AND `symbolType`   ='{}' "
  "  AND `symbolCode`   ='{}' "
  "  AND `side`         ='{}' "
  "  AND `posSide`      ='{}' "
  "  AND `parValue`     ='{}' "
  "  AND `feeCurrency`  ='{}';",
  TBLPosInfo::TableName,
  productGrpId_,
  productId_,
  userId_,
  acctGrpId_,
  acctId_,
  trdAcctId_,
  stgGrpId_,  // add stgGrpId
  stgId_,
  stgInstId_,
  algoId_,
  GetMarketName(marketCode_),
  magic_enum::enum_name(symbolType_),
  symbolCode_,
  magic_enum::enum_name(side_),
  magic_enum::enum_name(posSide_),
  parValue_,
  feeCurrency_
);
return sql;
} ;
// clang-format on

PosInfoSPtr MakePosInfo(const db::posInfo::RecordSPtr& recPosInfo) {
  auto posInfo = std::make_shared<PosInfo>();

  posInfo->productGrpId_ = recPosInfo->productGrpId;
  posInfo->productId_ = recPosInfo->productId;
  posInfo->userId_ = recPosInfo->userId;
  posInfo->acctGrpId_ = recPosInfo->acctGrpId;
  posInfo->acctId_ = recPosInfo->acctId;
  posInfo->trdAcctId_ = recPosInfo->trdAcctId;
  posInfo->stgGrpId_ = recPosInfo->stgGrpId;  // add stgGrpId
  posInfo->stgId_ = recPosInfo->stgId;
  posInfo->stgInstId_ = recPosInfo->stgInstId;
  posInfo->algoId_ = recPosInfo->algoId;

  posInfo->marketCode_ = GetMarketCode(recPosInfo->marketCode);
  posInfo->symbolType_ =
      magic_enum::enum_cast<SymbolType>(recPosInfo->symbolType).value();
  strncpy(posInfo->symbolCode_, recPosInfo->symbolCode.c_str(),
          sizeof(posInfo->symbolCode_) - 1);

  posInfo->side_ = magic_enum::enum_cast<Side>(recPosInfo->side).value();
  posInfo->posSide_ =
      magic_enum::enum_cast<PosSide>(recPosInfo->posSide).value();

  posInfo->parValue_ = recPosInfo->parValue;
  strncpy(posInfo->feeCurrency_, recPosInfo->feeCurrency.c_str(),
          sizeof(posInfo->feeCurrency_) - 1);
  posInfo->fee_ = CONV(Decimal, recPosInfo->fee);
  posInfo->pos_ = CONV(Decimal, recPosInfo->pos);
  posInfo->prePos_ = CONV(Decimal, recPosInfo->prePos);
  posInfo->avgOpenPrice_ = CONV(Decimal, recPosInfo->avgOpenPrice);
  posInfo->preAvgOpenPrice_ = CONV(Decimal, recPosInfo->preAvgOpenPrice);
  posInfo->pnlUnReal_ = CONV(Decimal, recPosInfo->pnlUnReal);
  posInfo->pnlReal_ = CONV(Decimal, recPosInfo->pnlReal);
  posInfo->totalBidSize_ = CONV(Decimal, recPosInfo->totalBidSize);
  posInfo->totalAskSize_ = CONV(Decimal, recPosInfo->totalAskSize);
  posInfo->preTotalBidSize_ = CONV(Decimal, recPosInfo->preTotalBidSize);
  posInfo->preTotalAskSize_ = CONV(Decimal, recPosInfo->preTotalAskSize);
  posInfo->totalOpenSize_ = CONV(Decimal, recPosInfo->totalOpenSize);
  posInfo->preTotalOpenSize_ = CONV(Decimal, recPosInfo->preTotalOpenSize);
  posInfo->updateTime_ = ConvertDBTimeToTS(recPosInfo->updateTime);

  const auto key = posInfo->getKey();
  posInfo->keyHash_ = XXH3_64bits(key.data(), key.size());

  return posInfo;
}

PosInfoSPtr MakePosInfoOfContract(const OrderInfoSPtr& orderInfo) {
  auto posInfo = std::make_shared<PosInfo>();

  posInfo->productGrpId_ = orderInfo->productGrpId_;
  posInfo->productId_ = orderInfo->productId_;
  posInfo->userId_ = orderInfo->userId_;
  posInfo->acctGrpId_ = orderInfo->acctGrpId_;
  posInfo->acctId_ = orderInfo->acctId_;
  posInfo->trdAcctId_ = orderInfo->trdAcctId_;
  posInfo->stgGrpId_ = orderInfo->stgGrpId_;  // add stgGrpId
  posInfo->stgId_ = orderInfo->stgId_;
  posInfo->stgInstId_ = orderInfo->stgInstId_;
  posInfo->algoId_ = orderInfo->algoId_;

  posInfo->marketCode_ = orderInfo->marketCode_;
  posInfo->symbolType_ = orderInfo->symbolType_;
  strncpy(posInfo->symbolCode_, orderInfo->symbolCode_,
          sizeof(posInfo->symbolCode_) - 1);

  posInfo->side_ = orderInfo->side_;
  posInfo->posSide_ = orderInfo->posSide_;

  posInfo->parValue_ = orderInfo->parValue_;

  strncpy(posInfo->feeCurrency_, orderInfo->feeCurrency_,
          sizeof(posInfo->feeCurrency_) - 1);
  posInfo->fee_ = orderInfo->getFeeOfLastTrade();

  posInfo->pos_ = orderInfo->lastDealSize_;
  posInfo->prePos_ = 0;

  posInfo->avgOpenPrice_ = orderInfo->lastDealPrice_;
  posInfo->preAvgOpenPrice_ = 0;

  posInfo->pnlUnReal_ = 0;
  posInfo->pnlReal_ = 0;
  posInfo->totalBidSize_ = 0;
  posInfo->totalAskSize_ = 0;
  posInfo->preTotalBidSize_ = 0;
  posInfo->preTotalAskSize_ = 0;
  posInfo->totalOpenSize_ = 0;
  posInfo->preTotalOpenSize_ = 0;

  posInfo->updateTime_ = GetTotalUSSince1970();

  const auto key = posInfo->getKey();
  posInfo->keyHash_ = XXH3_64bits(key.data(), key.size());

  return posInfo;
}

PosInfoSPtr MakePosInfoOfContractWithKeyFields(const OrderInfoSPtr& orderInfo,
                                               Side side) {
  auto posInfo = std::make_shared<PosInfo>();

  posInfo->productGrpId_ = orderInfo->productGrpId_;
  posInfo->productId_ = orderInfo->productId_;
  posInfo->userId_ = orderInfo->userId_;
  posInfo->acctGrpId_ = orderInfo->acctGrpId_;
  posInfo->acctId_ = orderInfo->acctId_;
  posInfo->trdAcctId_ = orderInfo->trdAcctId_;
  posInfo->stgGrpId_ = orderInfo->stgGrpId_;  // add stgGrpId
  posInfo->stgId_ = orderInfo->stgId_;
  posInfo->stgInstId_ = orderInfo->stgInstId_;
  posInfo->algoId_ = orderInfo->algoId_;

  posInfo->marketCode_ = orderInfo->marketCode_;
  posInfo->symbolType_ = orderInfo->symbolType_;
  strncpy(posInfo->symbolCode_, orderInfo->symbolCode_,
          sizeof(posInfo->symbolCode_) - 1);

  posInfo->side_ = side;
  posInfo->posSide_ = orderInfo->posSide_;

  posInfo->parValue_ = orderInfo->parValue_;
  strncpy(posInfo->feeCurrency_, orderInfo->feeCurrency_,
          sizeof(posInfo->feeCurrency_) - 1);

  posInfo->updateTime_ = GetTotalUSSince1970();

  const auto key = posInfo->getKey();
  posInfo->keyHash_ = XXH3_64bits(key.data(), key.size());

  return posInfo;
}

bool isEqual(const Key2PosInfoGroupSPtr& lhs, const Key2PosInfoGroupSPtr& rhs) {
  if (lhs->size() != rhs->size()) {
    return false;
  }
  auto lhsIter = std::begin(*lhs);
  auto rhsIter = std::begin(*rhs);
  while (true) {
    if (lhsIter == std::end(*lhs)) break;
    if (lhsIter->first != rhsIter->first) return false;
    ++lhsIter;
    ++rhsIter;
  }

  for (const auto& lhsRec : *lhs) {
    const auto& lhsPosInfo = lhsRec.second;
    const auto& rhsPosInfo = (*rhs)[lhsRec.first];
    if (lhsPosInfo->isEqual(rhsPosInfo) == false) {
      return false;
    }
  }

  return true;
}

Key2PosInfoGroup MakePosUpdateOfAcctId(
    const PosUpdateOfAcctIdForPubSPtr& posUpdateOfAcctIdForPub) {
  Key2PosInfoGroup ret;
  auto posInfoAddr = posUpdateOfAcctIdForPub->posInfoGroup_;
  for (std::uint16_t i = 0; i < posUpdateOfAcctIdForPub->num_; ++i) {
    const auto posInfoRecv = reinterpret_cast<const PosInfo*>(posInfoAddr);
    const auto posInfo = std::make_shared<PosInfo>(*posInfoRecv);
    ret.emplace(posInfo->getKey(), posInfo);
    posInfoAddr += sizeof(PosInfo);
  }
  return ret;
}

Key2PosInfoGroup MakePosUpdateOfStgId(
    const PosUpdateOfStgIdForPubSPtr& posUpdateOfStgIdForPub) {
  Key2PosInfoGroup ret;
  auto posInfoAddr = posUpdateOfStgIdForPub->posInfoGroup_;
  for (std::uint16_t i = 0; i < posUpdateOfStgIdForPub->num_; ++i) {
    const auto posInfoRecv = reinterpret_cast<const PosInfo*>(posInfoAddr);
    const auto posInfo = std::make_shared<PosInfo>(*posInfoRecv);
    ret.emplace(posInfo->getKey(), posInfo);
    posInfoAddr += sizeof(PosInfo);
  }
  return ret;
}

Key2PosInfoGroup MakePosUpdateOfStgInstId(
    const PosUpdateOfStgInstIdForPubSPtr& posUpdateOfStgInstIdForPub) {
  Key2PosInfoGroup ret;
  auto posInfoAddr = posUpdateOfStgInstIdForPub->posInfoGroup_;
  for (std::uint16_t i = 0; i < posUpdateOfStgInstIdForPub->num_; ++i) {
    const auto posInfoRecv = reinterpret_cast<const PosInfo*>(posInfoAddr);
    const auto posInfo = std::make_shared<PosInfo>(*posInfoRecv);
    ret.emplace(posInfo->getKey(), posInfo);
    posInfoAddr += sizeof(PosInfo);
  }
  return ret;
}

Key2PosInfoGroup MakePosUpdateOfAll(
    const PosUpdateOfAllForPubSPtr& posUpdateOfAllForPub) {
  Key2PosInfoGroup ret;
  auto posInfoAddr = posUpdateOfAllForPub->posInfoGroup_;
  for (std::uint16_t i = 0; i < posUpdateOfAllForPub->num_; ++i) {
    const auto posInfoRecv = reinterpret_cast<const PosInfo*>(posInfoAddr);
    const auto posInfo = std::make_shared<PosInfo>(*posInfoRecv);
    ret.emplace(posInfo->getKey(), posInfo);
    posInfoAddr += sizeof(PosInfo);
  }
  return ret;
}

//! 必须确保lhs和rhs的symbolType是一样的
Decimal CalcAvgOpenPrice(const PosInfoSPtr& lhs, const PosInfoSPtr& rhs) {
  assert(lhs->symbolType_ == rhs->symbolType_ &&
         "lhs->symbolType_ == rhs->symbolType_");

  Decimal avgOpenPrice = 0;

  if (lhs->symbolType_ == SymbolType::CN_MainBoard ||
      lhs->symbolType_ == SymbolType::CN_SecondBoard ||
      lhs->symbolType_ == SymbolType::CN_StartupBoard ||
      lhs->symbolType_ == SymbolType::CN_TechBoard ||
      lhs->symbolType_ == SymbolType::CN_Futures ||
      lhs->symbolType_ == SymbolType::Spot ||
      lhs->symbolType_ == SymbolType::Perp ||
      lhs->symbolType_ == SymbolType::Futures) {
    const auto totalPos = lhs->pos_ + rhs->pos_;
    if (!DEC::ZERO(totalPos)) {
      const auto totalAmt =
          lhs->pos_ * lhs->avgOpenPrice_ + rhs->pos_ * rhs->avgOpenPrice_;
      avgOpenPrice = totalAmt / totalPos;
    }

  } else if (lhs->symbolType_ == SymbolType::CPerp ||
             lhs->symbolType_ == SymbolType::CFutures) {
    const auto totalPos = lhs->pos_ + rhs->pos_;
    //! 两个posInfo的avgOpenPrice_都为0
    if (DEC::ZERO(lhs->avgOpenPrice_) && DEC::ZERO(rhs->avgOpenPrice_)) {
      avgOpenPrice = 0;
    } else {
      //! 两个posInfo中的一个avgOpenPrice_为0
      if (DEC::ZERO(lhs->avgOpenPrice_)) {
        avgOpenPrice = totalPos / (rhs->pos_ / rhs->avgOpenPrice_);

        //! 两个posInfo中的一个avgOpenPrice_为0
      } else if (DEC::ZERO(rhs->avgOpenPrice_)) {
        avgOpenPrice = totalPos / (lhs->pos_ / lhs->avgOpenPrice_);

        //! 两个posInfo中的avgOpenPrice_都不为0
      } else {
        avgOpenPrice = totalPos / (lhs->pos_ / lhs->avgOpenPrice_ +
                                   rhs->pos_ / rhs->avgOpenPrice_);
      }
    }

  } else {
    LOG_W("Unhandled symbolType {}.", magic_enum::enum_name(lhs->symbolType_));
  }

  return avgOpenPrice;
}

void MergePosInfoHasNoFeeCurrency(PosInfoGroup& posInfoGroup) {
  //! 查找可用于合并手续费为空的仓位的手续费的记录
  auto getPosInfoUsedToMerge = [](const auto& posInfoGroup,
                                  const auto& posInfoHasNoFeeCurrency) {
    PosInfoSPtr ret = nullptr;
    for (const auto& posInfo : posInfoGroup) {
      //! 如果当前仓位信息仅仅比入参多出feeCurrency字段
      if (posInfo->oneMoreFeeCurrencyThanInput(posInfoHasNoFeeCurrency)) {
        //! 如果返回值为空或者说当前记录的updateTime_更加新，那么更新
        if (ret == nullptr || posInfo->updateTime_ > ret->updateTime_) {
          ret = posInfo;
        }
      }
    }
    return ret;
  };

  auto calcFeeOfEstimate = [](const auto& posInfo,
                              const auto& posInfoHasNoFeeCurrency) {
    Decimal feeOfEstimated = 0;
    const auto totalDealSize = posInfo->totalBidSize_ - posInfo->totalAskSize_;
    const auto totalDealSizeOfPosInfoHasNoFeeCurrency =
        posInfoHasNoFeeCurrency->totalBidSize_ -
        posInfoHasNoFeeCurrency->totalAskSize_;
    if (!DEC::ZERO(totalDealSize)) {
      feeOfEstimated = totalDealSizeOfPosInfoHasNoFeeCurrency / totalDealSize *
                       posInfo->fee_;
    }
    return feeOfEstimated;
  };

  auto mergePosInfo = [&](auto& posInfo, const auto& posInfoHasNoFeeCurrency) {
    const auto feeOfEstimated =
        calcFeeOfEstimate(posInfo, posInfoHasNoFeeCurrency);
    posInfo->fee_ += feeOfEstimated;
    //! 更改pos_前先计算avgOpenPrice_，不然pos_发生变化会影响avgOpenPrice_的计算。
    posInfo->avgOpenPrice_ = CalcAvgOpenPrice(posInfo, posInfoHasNoFeeCurrency);
    posInfo->pos_ += posInfoHasNoFeeCurrency->pos_;
    posInfo->prePos_ += posInfoHasNoFeeCurrency->prePos_;
    posInfo->pnlUnReal_ += posInfoHasNoFeeCurrency->pnlUnReal_;
    posInfo->pnlReal_ += posInfoHasNoFeeCurrency->pnlReal_;
    posInfo->totalBidSize_ += posInfoHasNoFeeCurrency->totalBidSize_;
    posInfo->totalAskSize_ += posInfoHasNoFeeCurrency->totalAskSize_;
    posInfo->totalOpenSize_ += posInfoHasNoFeeCurrency->totalOpenSize_;
  };

  //! 分离出feeCurrency为空的仓位集合
  PosInfoGroup posInfoHasNoFeeCurrencyGroup;
  std::ext::erase_if(posInfoGroup, [&](const auto& posInfo) {
    if (posInfo->feeCurrency_[0] == '\0') {
      posInfoHasNoFeeCurrencyGroup.emplace_back(posInfo);
      return true;
    } else {
      return false;
    }
  });

  //! 遍历feeCurrency为空的仓位的集合，将其合并到feeCurrency不为空的仓位上
  for (const auto& posInfoHasNoFeeCurrency : posInfoHasNoFeeCurrencyGroup) {
    auto posInfo = getPosInfoUsedToMerge(posInfoGroup, posInfoHasNoFeeCurrency);
    if (posInfo != nullptr) {
      mergePosInfo(posInfo, posInfoHasNoFeeCurrency);
    } else {
      posInfoGroup.emplace_back(posInfo);
    }
  }
}

}  // namespace bq
