/*!
 * \file TBLFeeInfo.hpp
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

namespace bq::db::feeInfo {

struct FieldGroupOfKey {
  TrdAcctId trdAcctId;
  std::string side;
  std::string posDirection;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;

  JSER(FieldGroupOfKey, trdAcctId, side, posDirection, marketCode, symbolType,
       symbolCode)
};

struct FieldGroupOfVal {
  AcctId acctId;
  std::string feeType;
  std::string commission;
  std::string minCommission;
  std::string stampDuty;
  std::string minStampDuty;
  std::string transFee;
  std::string minTransFee;
  std::string updateTime;
  int isDel;

  JSER(FieldGroupOfVal, acctId, feeType, commission, minCommission, stampDuty,
       minStampDuty, transFee, minTransFee, updateTime, isDel)
};

struct FieldGroupOfAll {
  AcctId acctId;
  TrdAcctId trdAcctId;
  std::string side;
  std::string posDirection;
  std::string marketCode;
  std::string symbolType;
  std::string symbolCode;

  std::string feeType;
  std::string commission;
  std::string minCommission;
  std::string stampDuty;
  std::string minStampDuty;
  std::string transFee;
  std::string minTransFee;
  std::string updateTime;
  int isDel;

  JSER(FieldGroupOfAll, acctId, trdAcctId, side, posDirection, marketCode,
       symbolType, symbolCode, feeType, commission, minCommission, stampDuty,
       minStampDuty, transFee, minTransFee, updateTime, isDel)
};

struct TableSchema {
  inline const static std::string TableName = "`BetterQuant`.`baseFeeInfo`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::feeInfo

using TBLFeeInfo = bq::db::feeInfo::TableSchema;
