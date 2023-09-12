/*!
 * \file Pnl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "def/Pnl.hpp"

#include "db/TBLHisPnl.hpp"
#include "def/SymbolCodeIF.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Pch.hpp"

namespace bq {

std::string Pnl::toStr() const {
  const auto delay = GetTotalMSSince1970() - updateTime_ / 1000;
  const auto ret = fmt::format(
      "{} totalPnl: {} {}; pnlUnReal: {} {}; pnlReal: {} {}; fee: {} {}; "
      "delay: {}ms; statusCode: {}",
      queryCond_, ToPrettyStr(getTotalPnl()), quoteCurrencyForCalc_,
      ToPrettyStr(pnlUnReal_), quoteCurrencyForCalc_, ToPrettyStr(pnlReal_),
      quoteCurrencyForCalc_, ToPrettyStr((-1 * fee_)), quoteCurrencyForCalc_,
      delay, statusCode_);
  return ret;
}

Decimal Pnl::getTotalPnl() const { return pnlUnReal_ + pnlReal_ - fee_; }

std::uint64_t Pnl::delay() const {
  const auto realDelay = GetTotalSecSince1970() - updateTime_ / 1000000;
  return realDelay;
}

bool Pnl::isValid(std::uint32_t secDelayOfPrice) const {
  return delay() < secDelayOfPrice && statusCode_ == 0;
}

// clang-format off
std::string Pnl::getSqlOfInsert() const {
  const auto now = GetTotalUSSince1970();
  std::uint64_t delay;
  if (now >= updateTime_){
    delay = (now - updateTime_) / 1000;
  } else {
    delay = 0;
  }

  const auto sql = fmt::format(
  "INSERT INTO {} ("
    "`queryCond`,"
    "`quoteCurrency`,"
    "`totalPnl`,"
    "`pnlUnReal`,"
    "`pnlReal`,"
    "`fee`,"
    "`delay`,"
    "`statusCode`"
  ")"
  "VALUES"
  "("
    "'{}',"  // queryCond
    "'{}',"  // quoteCurrency
    " {} ,"  // totalPnl
    " {} ,"  // pnlUnReal
    " {} ,"  // pnlReal
    " {} ,"  // fee
    " {} ,"  // delay
    " {}  "  // statusCode
  "); ",
    TBLHisPnl::TableName,
    queryCond_,
    quoteCurrencyForCalc_,
    getTotalPnl(),
    pnlUnReal_,
    pnlReal_,
    fee_,
    delay,
    statusCode_ 
  );

  return sql;
} ;
// clang-format on

std::string Pnl::toJson() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();
  writer.Key("queryCond");
  writer.String(queryCond_.c_str());
  writer.Key("pnlUnReal");
  writer.Double(pnlUnReal_);
  writer.Key("pnlReal");
  writer.Double(pnlReal_);
  writer.Key("fee");
  writer.Double(fee_);
  writer.Key("updateTime");
  writer.Uint64(updateTime_);
  writer.Key("quoteCurrencyForCalc");
  writer.String(quoteCurrencyForCalc_.c_str());
  writer.Key("statusCode");
  writer.Int(statusCode_);
  writer.EndObject();

  return strBuf.GetString();
}

void Pnl::fromJson(const std::string& jsonStr) {
  Doc doc;
  doc.Parse(jsonStr.data());
  queryCond_ = doc["queryCond"].GetString();
  pnlUnReal_ = doc["pnlUnReal"].GetDouble();
  pnlReal_ = doc["pnlReal"].GetDouble();
  fee_ = doc["fee"].GetDouble();
  updateTime_ = doc["updateTime"].GetDouble();
  quoteCurrencyForCalc_ = doc["quoteCurrencyForCalc"].GetDouble();
  statusCode_ = doc["statusCode"].GetInt();
}

}  // namespace bq
