/*!
 * \file DataStruOfMD.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "def/DataStruOfMD.hpp"

#include "def/Def.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq {

std::string MDHeader::toStr() const {
  const auto ret = fmt::format(
      "exchTs: {}; localTs_: {}; {} {} {} {}", ConvertTsToPtime(exchTs_),
      ConvertTsToPtime(localTs_), GetMarketName(marketCode_),
      magic_enum::enum_name(symbolType_), symbolCode_,
      magic_enum::enum_name(mdType_));
  return ret;
}

std::string MDHeader::getTopicPrefix() const {
  const auto ret = fmt::format("{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                               SEP_OF_TOPIC, GetMarketName(marketCode_),
                               SEP_OF_TOPIC, magic_enum::enum_name(symbolType_),
                               SEP_OF_TOPIC, symbolCode_, SEP_OF_TOPIC);
  return ret;
}

std::string MDHeader::toJson() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();

  writer.Key("exchTs");
  writer.Uint64(exchTs_);

  writer.Key("localTs");
  writer.Uint64(localTs_);

  writer.Key("marketCode");
  writer.String(GetMarketName(marketCode_).data());

  writer.Key("symbolType");
  writer.String(ENUM_TO_STR(symbolType_).data());

  writer.Key("symbolCode");
  writer.String(symbolCode_);

  writer.Key("mdType");
  writer.String(ENUM_TO_STR(mdType_).data());

  writer.EndObject();

  const auto ret = strBuf.GetString();
  return ret;
}

std::string Trades::toStr() const {
  const auto ret = fmt::format(
      "{} {} tradeTime: {}; tradeNo: {}; price: {}; size: {}; side: {}; "
      "bidOrderId: {}; askOrderId: {}; tradingDay : {}; extDataLen: {}",
      shmHeader_.toStr(), mdHeader_.toStr(), tradeTime_, tradeNo_, price_,
      size_, magic_enum::enum_name(side_), bidOrderId_, askOrderId_,
      tradingDay_, extDataLen_);
  return ret;
}

std::string Trades::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string Trades::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();
  writer.Key("tradeTime");
  writer.Uint64(tradeTime_);
  writer.Key("tradeNo");
  writer.String(tradeNo_);
  writer.Key("price");
  writer.Double(price_);
  writer.Key("size");
  writer.Double(size_);
  writer.Key("side");
  writer.String(ENUM_TO_STR(side_).data());
  writer.Key("bidOrderId");
  writer.String(bidOrderId_);
  writer.Key("askOrderId");
  writer.String(askOrderId_);
  writer.Key("tradingDay");
  writer.String(tradingDay_);
  writer.EndObject();

  return strBuf.GetString();
}

std::string Trades::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string Trades::getTDEngSqlPrefix() const {
  //! tableName = Trades_Spot_SSE_600600
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string Trades::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                              // symbolCode
  );
  // clang-format on
  return ret;
}

std::string Trades::getTDEngSqlRawTagsPart(const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string Trades::getTDEngSqlValuesPart() const {
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, {}, '{}', {}, {}, {}, '{}', '{}', '{}') ", 
    mdHeader_.exchTs_,               // exchTs_
    mdHeader_.localTs_,              // localTs_
    tradeTime_,                      // tradeTime_
    tradeNo_,                        // tradeNo_
    price_,                          // price_ 
    size_,                           // size_ 
    magic_enum::enum_integer(side_), // side_ 
    bidOrderId_,                     // bidOrderId
    askOrderId_,                     // askOrderId
    tradingDay_                      // tradingDay
  );
  // clang-format on
  return ret;
}

std::string Trades::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string Trades::getTDEngSqlRawValuesPart(void* data,
                                             std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string Orders::toStr() const {
  const auto ret = fmt::format(
      "{} {} orderTime: {}; orderNo: {}; price: {}; size: {}; side: {}; "
      "tradingDay : {}; extDataLen: {}",
      shmHeader_.toStr(), mdHeader_.toStr(), orderTime_, orderNo_, price_,
      size_, magic_enum::enum_name(side_), tradingDay_, extDataLen_);
  return ret;
}

std::string Orders::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string Orders::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();
  writer.Key("orderTime");
  writer.Uint64(orderTime_);
  writer.Key("orderNo");
  writer.String(orderNo_);
  writer.Key("price");
  writer.Double(price_);
  writer.Key("size");
  writer.Double(size_);
  writer.Key("side");
  writer.String(ENUM_TO_STR(side_).data());
  writer.Key("tradingDay");
  writer.String(tradingDay_);
  writer.EndObject();

  return strBuf.GetString();
}

std::string Orders::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string Orders::getTDEngSqlPrefix() const {
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string Orders::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_), // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                            // symbolCode
  );
  // clang-format on
  return ret;
}

std::string Orders::getTDEngSqlRawTagsPart(const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string Orders::getTDEngSqlValuesPart() const {
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, {}, '{}', {}, {}, {}, '{}') ", 
    mdHeader_.exchTs_,               // exchTs_
    mdHeader_.localTs_,              // localTs_
    orderTime_,                      // tradeTime_
    orderNo_,                        // tradeNo_
    price_,                          // price_ 
    size_,                           // size_ 
    magic_enum::enum_integer(side_), // side_ 
    tradingDay_                      // tradingDay
  );
  // clang-format on
  return ret;
}

std::string Orders::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string Orders::getTDEngSqlRawValuesPart(void* data,
                                             std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string Books::toStr() const {
  const auto ret = fmt::format(
      "{} {} lastPrice: {} totalVol: {} totalAmt: "
      "{} tradesCount: "
      "{} asks bids tradingDay: {} extDataLen: {}",
      shmHeader_.toStr(), mdHeader_.toStr(), lastPrice_, totalVol_, totalAmt_,
      tradesCount_, tradingDay_, extDataLen_);
  return ret;
}

std::string Books::toJson(std::uint32_t level) const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data(level));
  return ret;
}

std::string Books::data(std::uint32_t level) const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();

  writer.Key("lastPrice");
  writer.Double(lastPrice_);

  writer.Key("totalVol");
  writer.Double(totalVol_);

  writer.Key("totalAmt");
  writer.Double(totalAmt_);

  writer.Key("tradesCount");
  writer.Uint64(tradesCount_);

  writer.Key("tradingDay");
  writer.String(tradingDay_);

  writer.Key("asks");
  writer.StartArray();
  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (i >= level) break;
    if (DEC::ZERO(asks_[i].price_) && DEC::ZERO(asks_[i].size_) &&
        asks_[i].orderNum_ == 0) {
      break;
    }
    writer.StartObject();
    writer.Key("price");
    writer.Double(asks_[i].price_);
    writer.Key("size");
    writer.Double(asks_[i].size_);
    writer.Key("orderNum");
    writer.Uint(asks_[i].orderNum_);
    writer.EndObject();
  }
  writer.EndArray();

  writer.Key("bids");
  writer.StartArray();
  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (i >= level) break;
    if (DEC::ZERO(bids_[i].price_) && DEC::ZERO(bids_[i].size_) &&
        bids_[i].orderNum_ == 0) {
      break;
    }
    writer.StartObject();
    writer.Key("price");
    writer.Double(bids_[i].price_);
    writer.Key("size");
    writer.Double(bids_[i].size_);
    writer.Key("orderNum");
    writer.Uint(bids_[i].orderNum_);
    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();

  return strBuf.GetString();
}

std::string Books::dataOfUnifiedFmt(std::uint32_t level) const {
  const auto ret = fmt::format(R"({{"mdHeader":{},"data":{}}})",
                               mdHeader_.toJson(), data(level));
  return ret;
}

std::string Books::getTDEngSqlPrefix() const {
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string Books::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                              // symbolCode
  );
  // clang-format on
  return ret;
}

std::string Books::getTDEngSqlRawTagsPart(const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string Books::getTDEngSqlValuesPart() const {
  const auto fmtStr = "{}{}{}{}{}{}";

  std::string asks;
  for (std::uint32_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (DEC::ZERO(asks_[i].size_) && DEC::ZERO(asks_[i].price_) &&
        DEC::ZERO(asks_[i].orderNum_))
      break;
    asks += fmt::format(fmtStr, asks_[i].price_, SEP_OF_DEPTH_FIELDS,
                        asks_[i].size_, SEP_OF_DEPTH_FIELDS, asks_[i].orderNum_,
                        SEP_OF_DEPTH_REC);
  }
  if (!asks.empty()) asks.pop_back();

  std::string bids;
  for (std::uint32_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (DEC::ZERO(bids_[i].size_) && DEC::ZERO(bids_[i].price_) &&
        DEC::ZERO(asks_[i].orderNum_))
      break;
    bids += fmt::format(fmtStr, bids_[i].price_, SEP_OF_DEPTH_FIELDS,
                        bids_[i].size_, SEP_OF_DEPTH_FIELDS, bids_[i].orderNum_,
                        SEP_OF_DEPTH_REC);
  }
  if (!bids.empty()) bids.pop_back();

  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, {}, {}, {}, {}, '{}', '{}', '{}') ", 
    mdHeader_.exchTs_,
    mdHeader_.localTs_,
    lastPrice_,   
    totalVol_,     
    totalAmt_,       
    tradesCount_,    
    tradingDay_,      
    asks,              
    bids                
  );
  // clang-format on

  return ret;
}

std::string Books::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string Books::getTDEngSqlRawValuesPart(void* data,
                                            std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string Bid1Ask1::toStr() const {
  const auto ret =
      fmt::format("{} {} askPrice: {}; askSize: {}; bidPrice: {}; bidSize: {}",
                  shmHeader_.toStr(), mdHeader_.toStr(), askPrice_, askSize_,
                  bidPrice_, bidSize_);
  return ret;
}

std::string Bid1Ask1::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string Bid1Ask1::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();

  writer.Key("askPrice");
  writer.Double(askPrice_);
  writer.Key("askSize");
  writer.Double(askSize_);
  writer.Key("bidPrice");
  writer.Double(bidPrice_);
  writer.Key("bidSize");
  writer.Double(bidSize_);

  writer.Key("tradingDay");
  writer.String(tradingDay_);
  writer.EndObject();

  return strBuf.GetString();
}

std::string Bid1Ask1::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string Bid1Ask1::getTDEngSqlPrefix() const {
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string Bid1Ask1::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                              // symbolCode
  );
  // clang-format on
  return ret;
}

std::string Bid1Ask1::getTDEngSqlRawTagsPart(const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string Bid1Ask1::getTDEngSqlValuesPart() const {
  const auto fmtStr = "{}{}{}{}{}{}";

  // clang-format off
  const auto ret = fmt::format(
    "VALUES({}, {}, {}, {}, {}, {}, '{}') ", 
    mdHeader_.exchTs_,               
    mdHeader_.localTs_,             
    askPrice_,
    askSize_,
    bidPrice_,
    bidSize_,
    tradingDay_                    
  );
  // clang-format on

  return ret;
}

std::string Bid1Ask1::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string Bid1Ask1::getTDEngSqlRawValuesPart(void* data,
                                               std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string LastPrice::toStr() const {
  const auto ret =
      fmt::format("{} {} lastPrice: {}; lastSize: {};", shmHeader_.toStr(),
                  mdHeader_.toStr(), lastPrice_, lastSize_);
  return ret;
}

std::string LastPrice::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string LastPrice::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();

  writer.Key("lastPrice");
  writer.Double(lastPrice_);
  writer.Key("lastSize");
  writer.Double(lastSize_);

  writer.Key("tradingDay");
  writer.String(tradingDay_);
  writer.EndObject();

  return strBuf.GetString();
}

std::string LastPrice::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string LastPrice::getTDEngSqlPrefix() const {
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string LastPrice::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                              // symbolCode
  );
  // clang-format on
  return ret;
}

std::string LastPrice::getTDEngSqlRawTagsPart(
    const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string LastPrice::getTDEngSqlValuesPart() const {
  // clang-format off
  const auto ret = fmt::format(
    "VALUES({}, {}, {}, {}, '{}') ", 
    mdHeader_.exchTs_,               
    mdHeader_.localTs_,             
    lastPrice_,
    lastSize_,
    tradingDay_                    
  );
  // clang-format on

  return ret;
}

std::string LastPrice::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string LastPrice::getTDEngSqlRawValuesPart(void* data,
                                                std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string Tickers::toStr() const {
  const auto ret = fmt::format(
      "{} {} lastPrice: {}; lastSize: {}; askPrice: {}; "
      "askSize: {}; bidPrice: {}; bidSize: {}; open: {}; "
      "high: {}; low: {}; vol: {}; amt: {}; extDataLen: {}",
      shmHeader_.toStr(), mdHeader_.toStr(), lastPrice_, lastSize_, askPrice_,
      askSize_, bidPrice_, bidSize_, open_, high_, low_, vol_, amt_,
      extDataLen_);
  return ret;
}

std::string Tickers::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string Tickers::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();

  writer.Key("open");
  writer.Double(open_);
  writer.Key("high");
  writer.Double(high_);
  writer.Key("low");
  writer.Double(low_);

  writer.Key("lastPrice");
  writer.Double(lastPrice_);
  writer.Key("lastSize");
  writer.Double(lastSize_);

  writer.Key("upperLimitPrice");
  writer.Double(upperLimitPrice_);
  writer.Key("lowerLimitPrice");
  writer.Double(lowerLimitPrice_);

  writer.Key("preClosePrice");
  writer.Double(preClosePrice_);
  writer.Key("preSettlementPrice");
  writer.Double(preSettlementPrice_);

  writer.Key("closePrice_");
  writer.Double(closePrice_);
  writer.Key("settlementPrice");
  writer.Double(settlementPrice_);

  writer.Key("preOpenInterest");
  writer.Double(preOpenInterest_);
  writer.Key("openInterest");
  writer.Double(openInterest_);

  writer.Key("vol");
  writer.Double(vol_);
  writer.Key("amt");
  writer.Double(amt_);

  writer.Key("askPrice");
  writer.Double(askPrice_);
  writer.Key("askSize");
  writer.Double(askSize_);
  writer.Key("bidPrice");
  writer.Double(bidPrice_);
  writer.Key("bidSize");
  writer.Double(bidSize_);

  writer.Key("tradingDay");
  writer.String(tradingDay_);

  writer.Key("asks");
  writer.StartArray();
  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (DEC::ZERO(asks_[i].price_) && DEC::ZERO(asks_[i].size_) &&
        asks_[i].orderNum_ == 0) {
      break;
    }
    writer.StartObject();
    writer.Key("price");
    writer.Double(asks_[i].price_);
    writer.Key("size");
    writer.Double(asks_[i].size_);
    writer.Key("orderNum");
    writer.Uint(asks_[i].orderNum_);
    writer.EndObject();
  }
  writer.EndArray();

  writer.Key("bids");
  writer.StartArray();
  for (std::size_t i = 0; i < MAX_DEPTH_LEVEL; ++i) {
    if (DEC::ZERO(bids_[i].price_) && DEC::ZERO(bids_[i].size_) &&
        bids_[i].orderNum_ == 0) {
      break;
    }
    writer.StartObject();
    writer.Key("price");
    writer.Double(bids_[i].price_);
    writer.Key("size");
    writer.Double(bids_[i].size_);
    writer.Key("orderNum");
    writer.Uint(bids_[i].orderNum_);
    writer.EndObject();
  }
  writer.EndArray();
  writer.EndObject();

  return strBuf.GetString();
}

std::string Tickers::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string Tickers::getTDEngSqlPrefix() const {
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_);
  const auto stableName = magic_enum::enum_name(mdHeader_.mdType_);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,  // dbname
                               tableName,           // tableName
                               TBENG_DBNAME_OF_MD,  // dbname
                               stableName           // stableName
  );
  // clang-on
  return ret;
}

std::string Tickers::getTDEngSqlTagsPart() const {
  // clang-format off
  const auto ret = fmt::format("TAGS({}, {}, '{}') ", 
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_                              // symbolCode
  );
  // clang-format on
  return ret;
}

std::string Tickers::getTDEngSqlRawTagsPart(const std::string& apiName) const {
  // clang-format off
  const auto ret = fmt::format("TAGS('{}', {}, {}, '{}', {}) ", 
    apiName,
    magic_enum::enum_integer(mdHeader_.symbolType_),   // symbolType
    static_cast<std::uint16_t>(mdHeader_.marketCode_), // marketCode
    mdHeader_.symbolCode_,                             // symbolCode
    magic_enum::enum_integer(mdHeader_.mdType_)        // mdType
  );
  // clang-format on
  return ret;
}

std::string Tickers::getTDEngSqlValuesPart() const {
  const auto fmtStr = "{}{}{}{}{}{}";

  std::string asks;
  for (std::uint32_t i = 0; i < MAX_DEPTH_LEVEL_IN_TICKER; ++i) {
    asks += fmt::format(fmtStr, asks_[i].price_, SEP_OF_DEPTH_FIELDS,
                        asks_[i].size_, SEP_OF_DEPTH_FIELDS, asks_[i].orderNum_,
                        SEP_OF_DEPTH_REC);
  }
  if (!asks.empty()) asks.pop_back();

  std::string bids;
  for (std::uint32_t i = 0; i < MAX_DEPTH_LEVEL_IN_TICKER; ++i) {
    bids += fmt::format(fmtStr, bids_[i].price_, SEP_OF_DEPTH_FIELDS,
                        bids_[i].size_, SEP_OF_DEPTH_FIELDS, bids_[i].orderNum_,
                        SEP_OF_DEPTH_REC);
  }
  if (!bids.empty()) bids.pop_back();

  // clang-format off
  const auto ret = fmt::format(
    "VALUES({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, '{}', '{}', '{}') ", 
    mdHeader_.exchTs_,               
    mdHeader_.localTs_,             
    open_,
    high_,
    low_,
    lastPrice_,
    lastSize_,
    upperLimitPrice_,
    lowerLimitPrice_,
    preClosePrice_,
    preSettlementPrice_,
    closePrice_,
    settlementPrice_,
    preOpenInterest_,
    openInterest_,
    askPrice_,
    askSize_,
    bidPrice_,
    bidSize_,
    vol_,
    amt_,
    tradingDay_,                    
    asks,
    bids
  );
  // clang-format on

  return ret;
}

std::string Tickers::getTDEngSqlRawPrefix() const {
  //! tableName = Trades_Spot_SSE_600600_OrigData
  const auto tableName = fmt::format(
      "{}{}{}{}{}{}{}{}{}", magic_enum::enum_name(mdHeader_.mdType_),
      SEP_OF_TDENG_TABLE_NAME, magic_enum::enum_name(mdHeader_.symbolType_),
      SEP_OF_TDENG_TABLE_NAME, GetMarketName(mdHeader_.marketCode_),
      SEP_OF_TDENG_TABLE_NAME, mdHeader_.symbolCode_, SEP_OF_TDENG_TABLE_NAME,
      TBENG_ORIG_DATA_TABLE_NAME_SUFFIX);
  // clang-off
  const auto ret = fmt::format("{}.{} USING {}.{} ",
                               TBENG_DBNAME_OF_MD,          // dbname
                               tableName,                   // tableName
                               TBENG_DBNAME_OF_MD,          // dbname
                               TBENG_TABLE_NAME_OF_ORIG_MD  // stableName
  );
  // clang-on
  return ret;
}

std::string Tickers::getTDEngSqlRawValuesPart(void* data,
                                              std::size_t dataLen) const {
  std::string str(static_cast<const char*>(data), dataLen);
  // clang-format off
  const auto ret = fmt::format("VALUES({}, {}, '{}', '{}') ", 
    mdHeader_.localTs_,              // localTs
    mdHeader_.exchTs_,               // exchTs
    tradingDay_,                     // tradingDay
    Base64Encode(str)
  );
  // clang-format on
  return ret;
}

std::string Candle::toStr() const {
  const auto ret = fmt::format(
      "{} {} startTs:{}; interval:{}; startTsOfCandle: {}; open: {}; high: {}; "
      "low: {}; close: {}; vol: {}; amt: {}; extDataLen: {}",
      shmHeader_.toStr(), mdHeader_.toStr(), startTs_, interval_,
      startTsOfCandle_, open_, high_, low_, close_, vol_, amt_, extDataLen_);
  return ret;
}

std::string Candle::toJson() const {
  const auto ret = MakeMarketData(shmHeader_, mdHeader_, data());
  return ret;
}

std::string Candle::data() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();
  writer.Key("startTs");
  writer.Uint64(startTs_);
  writer.Key("interval");
  writer.Uint(interval_);
  writer.Key("startTsOfCandle");
  writer.Uint64(startTsOfCandle_);
  writer.Key("open");
  writer.Double(open_);
  writer.Key("high");
  writer.Double(high_);
  writer.Key("low");
  writer.Double(low_);
  writer.Key("close");
  writer.Double(close_);
  writer.Key("vol");
  writer.Double(vol_);
  writer.Key("amt");
  writer.Double(amt_);
  writer.EndObject();

  return strBuf.GetString();
}

std::string Candle::dataOfUnifiedFmt() const {
  const auto ret =
      fmt::format(R"({{"mdHeader":{},"data":{}}})", mdHeader_.toJson(), data());
  return ret;
}

std::string Candle::getTDEngSqlPrefix() const { return ""; }

std::string Candle::getTDEngSqlTagsPart() const { return ""; }

std::string Candle::getTDEngSqlValuesPart() const { return ""; }

std::string MakeMarketData(const SHMHeader& shmHeader, const MDHeader& mdHeader,
                           const std::string& data) {
  std::string ret;
  ret.append(R"({"shmHeader":)")
      .append(shmHeader.toJson())
      .append(",")
      .append(R"("mdHeader":)")
      .append(mdHeader.toJson())
      .append(",")
      .append(R"("data":)")
      .append(data)
      .append("}");
  return ret;
}

//! 以下代码目前用于回放行情的时候生成行情包头信息
void initMDHeader(SHMHeader& shmHeader, MDHeader& mdHeader, const Doc& doc) {
  mdHeader.exchTs_ = doc["mdHeader"]["exchTs"].GetUint64();
  mdHeader.localTs_ = doc["mdHeader"]["localTs"].GetUint64();

  const auto marketCode = doc["mdHeader"]["marketCode"].GetString();
  mdHeader.marketCode_ = GetMarketCode(marketCode);

  const auto symbolType = doc["mdHeader"]["symbolType"].GetString();
  mdHeader.symbolType_ = magic_enum::enum_cast<SymbolType>(symbolType).value();

  const auto symbolCode = doc["mdHeader"]["symbolCode"].GetString();
  strncpy(mdHeader.symbolCode_, symbolCode, sizeof(mdHeader.symbolCode_) - 1);

  const auto mdType = doc["mdHeader"]["mdType"].GetString();
  mdHeader.mdType_ = magic_enum::enum_cast<MDType>(mdType).value();

  shmHeader.msgId_ = GetMsgIdByMDType(mdHeader.mdType_);

  std::string topic;
  TopicHash topicHash;
  if (mdHeader.mdType_ == MDType::Books) {
    std::tie(topic, topicHash) =
        MakeTopicInfo(marketCode, symbolType, symbolCode, mdHeader.mdType_,
                      Int2StrInCompileTime<MAX_DEPTH_LEVEL>::type::value);

  } else if (mdHeader.mdType_ == MDType::Candle) {
    std::tie(topic, topicHash) =
        MakeTopicInfo(marketCode, symbolType, symbolCode, mdHeader.mdType_,
                      SUFFIX_OF_CANDLE_DETAIL);

  } else {
    std::tie(topic, topicHash) =
        MakeTopicInfo(marketCode, symbolType, symbolCode, mdHeader.mdType_);
  }
  shmHeader.topicHash_ = topicHash;
  strncpy(shmHeader.topic_, topic.c_str(), sizeof(shmHeader.topic_) - 1);
}

/*
{
  "mdHeader": {
    "exchTs": 1669338603492000,
    "localTs": 1669338603626138,
    "marketCode": "Binance",
    "symbolType": "Spot",
    "symbolCode": "BTC-USDT",
    "mdType": "Trades"
  },
  "data": {
    "tradeTime": 1669338603491000,
    "tradeNo": "1920957568-2242251309-22422513",
    "price": 16529.42,
    "size": 0.00312,
    "side": "Bid"
  }
}
*/
std::tuple<int, Trades> MakeTrades(const std::string& jsonStr) {
  Trades ret;

  Doc doc;
  if (doc.Parse(jsonStr.data()).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), jsonStr);
    return {-1, ret};
  }

  initMDHeader(ret.shmHeader_, ret.mdHeader_, doc);

  ret.tradeTime_ = doc["data"]["tradeTime"].GetUint64();
  strncpy(ret.tradeNo_, doc["data"]["tradeNo"].GetString(),
          sizeof(ret.tradeNo_) - 1);
  ret.price_ = doc["data"]["price"].GetDouble();
  ret.size_ = doc["data"]["size"].GetDouble();
  const auto side = doc["data"]["side"].GetString();
  ret.side_ = magic_enum::enum_cast<Side>(side).value();

  return {0, ret};
}

/*
{
  "mdHeader": {
    "exchTs": 1669338601386000,
    "localTs": 1669338601888897,
    "marketCode": "Binance",
    "symbolType": "Spot",
    "symbolCode": "BTC-USDT",
    "mdType": "Books"
  },
  "data": {
    "asks": [{
      "price": 16527.85,
      "size": 0.0379,
      "orderNum": 0
    }, {
      "price": 16527.87,
      "size": 0.01513,
      "orderNum": 0
    }],
    "bids": [{
      "price": 16527.41,
      "size": 0.00065,
      "orderNum": 0
    }, {
      "price": 16527.4,
      "size": 0.006,
      "orderNum": 0
    }]
  }
}
*/
std::tuple<int, Books> MakeBooks(const std::string& jsonStr) {
  Books ret;

  Doc doc;
  if (doc.Parse(jsonStr.data()).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), jsonStr);
    return {-1, ret};
  }

  initMDHeader(ret.shmHeader_, ret.mdHeader_, doc);

  for (std::size_t i = 0; i < doc["data"]["asks"].Size(); ++i) {
    if (i == MAX_DEPTH_LEVEL) break;
    ret.asks_[i].price_ = doc["data"]["asks"][i]["price"].GetDouble();
    ret.asks_[i].size_ = doc["data"]["asks"][i]["size"].GetDouble();
    ret.asks_[i].orderNum_ = doc["data"]["asks"][i]["orderNum"].GetInt();
  }

  for (std::size_t i = 0; i < doc["data"]["bids"].Size(); ++i) {
    if (i == MAX_DEPTH_LEVEL) break;
    ret.bids_[i].price_ = doc["data"]["bids"][i]["price"].GetDouble();
    ret.bids_[i].size_ = doc["data"]["bids"][i]["size"].GetDouble();
    ret.bids_[i].orderNum_ = doc["data"]["bids"][i]["orderNum"].GetInt();
  }

  return {0, ret};
}

/*
{
  "mdHeader": {
    "exchTs": 1669339079941000,
    "localTs": 1669339080121968,
    "marketCode": "Binance",
    "symbolType": "Spot",
    "symbolCode": "BTC-USDT",
    "mdType": "Candle"
  },
  "data": {
    "open": 16541.34,
    "high": 16543.28,
    "low": 16538.99,
    "close": 16542.51,
    "vol": 41.77032,
    "amt": 690941.9306651
  }
}
*/
std::tuple<int, Candle> MakeCandle(const std::string& jsonStr) {
  Candle ret;

  Doc doc;
  if (doc.Parse(jsonStr.data()).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), jsonStr);
    return {-1, ret};
  }

  initMDHeader(ret.shmHeader_, ret.mdHeader_, doc);

  ret.startTs_ = doc["data"]["startTs"].GetUint64();
  ret.interval_ = doc["data"]["interval"].GetUint();
  ret.startTsOfCandle_ = doc["data"]["startTsOfCandle"].GetUint64();
  ret.open_ = doc["data"]["open"].GetDouble();
  ret.high_ = doc["data"]["high"].GetDouble();
  ret.low_ = doc["data"]["low"].GetDouble();
  ret.close_ = doc["data"]["close"].GetDouble();
  ret.vol_ = doc["data"]["vol"].GetDouble();
  ret.amt_ = doc["data"]["amt"].GetDouble();

  return {0, ret};
}

/*
{
  "mdHeader": {
    "exchTs": 1669338602578000,
    "localTs": 1669338602832317,
    "marketCode": "Binance",
    "symbolType": "Spot",
    "symbolCode": "BTC-USDT",
    "mdType": "Tickers"
  },
  "data": {
    "lastPrice": 16528.88,
    "lastSize": 0.0,
    "askPrice": 0.0,
    "askSize": 0.0,
    "bidPrice": 0.0,
    "bidSize": 0.0,
    "open": 16558.8,
    "high": 16812.63,
    "low": 16458.05,
    "vol": 208577.12905,
    "amt": 3464772623.884822
  }
}
*/
std::tuple<int, Tickers> MakeTickers(const std::string& jsonStr) {
  Tickers ret;

  Doc doc;
  if (doc.Parse(jsonStr.data()).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), jsonStr);
    return {-1, ret};
  }

  initMDHeader(ret.shmHeader_, ret.mdHeader_, doc);

  ret.lastPrice_ = doc["data"]["lastPrice"].GetDouble();
  ret.lastSize_ = doc["data"]["lastSize"].GetDouble();
  ret.askPrice_ = doc["data"]["askPrice"].GetDouble();
  ret.askSize_ = doc["data"]["askSize"].GetDouble();
  ret.bidPrice_ = doc["data"]["bidPrice"].GetDouble();
  ret.bidSize_ = doc["data"]["bidSize"].GetDouble();
  ret.open_ = doc["data"]["open"].GetDouble();
  ret.high_ = doc["data"]["high"].GetDouble();
  ret.low_ = doc["data"]["low"].GetDouble();
  ret.vol_ = doc["data"]["vol"].GetDouble();
  ret.amt_ = doc["data"]["amt"].GetDouble();

  return {0, ret};
}

MsgId GetMsgIdByMDType(MDType mdType) {
  switch (mdType) {
    case MDType::Trades:
      return MSG_ID_ON_MD_TRADES;
    case MDType::Orders:
      return MSG_ID_ON_MD_ORDERS;
    case MDType::Books:
      return MSG_ID_ON_MD_BOOKS;
    case MDType::Tickers:
      return MSG_ID_ON_MD_TICKERS;
    case MDType::Candle:
      return MSG_ID_ON_MD_CANDLE;
    default:
      return 0;
  }
  return 0;
}

}  // namespace bq
