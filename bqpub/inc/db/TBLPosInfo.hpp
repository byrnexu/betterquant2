/*!
 * \file TBLPosInfo.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::posInfo {

struct FieldGroupOfKey {
  ProductGrpId productGrpId;
  ProductId productId;
  UserId userId;
  AcctGrpId acctGrpId;
  AcctId acctId;
  TrdAcctId trdAcctId;
  StgGrpId stgGrpId;
  StgId stgId;  // add stgGrpId
  StgInstId stgInstId;
  AlgoId algoId;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string side;
  std::string posSide;
  std::uint32_t parValue;
  std::string feeCurrency;

  JSER(FieldGroupOfKey, productGrpId, productId, userId, acctGrpId, acctId,
       trdAcctId, stgGrpId, stgId, stgInstId, algoId, marketCode, symbolType,
       symbolCode, side, posSide, parValue, feeCurrency)  // add stgGrpId
};

struct FieldGroupOfVal {
  std::string fee;
  std::string pos;
  std::string prePos;
  std::string avgOpenPrice;
  std::string preAvgOpenPrice;
  std::string pnlUnReal;
  std::string pnlReal;
  std::string totalBidSize;
  std::string totalAskSize;
  std::string preTotalBidSize;
  std::string preTotalAskSize;
  std::string totalOpenSize;
  std::string preTotalOpenSize;
  std::string updateTime;

  JSER(FieldGroupOfVal, fee, pos, prePos, avgOpenPrice, preAvgOpenPrice,
       pnlUnReal, pnlReal, totalBidSize, totalAskSize, preTotalBidSize,
       totalOpenSize, preTotalOpenSize, preTotalAskSize, updateTime)
};

struct FieldGroupOfAll {
  ProductGrpId productGrpId;
  ProductId productId;
  UserId userId;
  AcctGrpId acctGrpId;
  AcctId acctId;
  TrdAcctId trdAcctId;
  StgGrpId stgGrpId;  // add stgGrpId
  StgId stgId;
  StgInstId stgInstId;
  AlgoId algoId;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;
  std::string side;
  std::string posSide;
  std::uint32_t parValue;
  std::string feeCurrency;

  std::string fee;
  std::string pos;
  std::string prePos;
  std::string avgOpenPrice;
  std::string preAvgOpenPrice;
  std::string pnlUnReal;
  std::string pnlReal;
  std::string totalBidSize;
  std::string totalAskSize;
  std::string preTotalBidSize;
  std::string preTotalAskSize;
  std::string totalOpenSize;
  std::string preTotalOpenSize;
  std::string updateTime;

  JSER(FieldGroupOfAll, productGrpId, productId, userId, acctGrpId, acctId,
       trdAcctId, stgGrpId, stgId, stgInstId, algoId, marketCode, symbolType,
       symbolCode, side, posSide, parValue, feeCurrency, fee, pos, prePos,
       avgOpenPrice, preAvgOpenPrice, pnlUnReal, pnlReal, totalBidSize,
       totalAskSize, preTotalBidSize, preTotalAskSize, totalOpenSize,
       preTotalOpenSize,
       updateTime)  // add stgGrpId
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`trdPosInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::posInfo

using TBLPosInfo = bq::db::posInfo::TableSchema;
