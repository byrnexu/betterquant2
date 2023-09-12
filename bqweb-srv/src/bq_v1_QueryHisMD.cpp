/*!
 * \file bq_v1_QueryHisMD.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#include "bq_v1_QueryHisMD.hpp"

#include "CommonIPCData.hpp"
#include "Config.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMSrv.hpp"
#include "WebSrv.hpp"
#include "def/BQConst.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "taos.h"
#include "tdeng/TDEngConnpool.hpp"
#include "tdeng/TDEngUtil.hpp"
#include "util/BQMDHis.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"

using namespace bq::v1;

//! http://192.168.19.115/v1/QueryHisMD/between/SZSE/Spot/000610/Trades?tsBegin=1672295971000000&tsEnd=9668989747663000
void QueryHisMD::queryBetween2Ts(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string &&marketCode, std::string &&symbolType,
    std::string &&symbolCode, std::string &&mdType, std::uint64_t tsBegin,
    std::uint64_t tsEnd, std::string &&freq) {
  using namespace boost::gregorian;
  //! topic = MD@SSE@Spot@600600@Trades
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                  SEP_OF_TOPIC, marketCode,  //
                  SEP_OF_TOPIC, symbolType,  //
                  SEP_OF_TOPIC, symbolCode,  //
                  SEP_OF_TOPIC, mdType);
  if (tsBegin > tsEnd) {
    const auto statusMsg = fmt::format(
        "Load his market data between 2 ts failed because "
        "tsBegin {} greater than tsEnd {}. topic = {} ",
        tsBegin, tsEnd, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  //! 如果查询开始日期超过当前日期
  const auto dateBegin = GetDateFromTs(tsBegin / 1000000);
  const auto maxDataOfHisMD =
      day_clock::universal_day() + date_duration(MAX_DATE_OFFSET_OF_QUERY_MD);
  if (dateBegin > maxDataOfHisMD) {
    const auto statusMsg = fmt::format(
        "Load his market data between 2 ts failed because "
        "tsBegin {} greater than {}. topic = {}",
        tsBegin, to_iso_string(maxDataOfHisMD), topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  //! 如果查询结束日期晚于MIN_DATE_OF_HIS_MD
  const auto dateEnd = GetDateFromTs(tsEnd / 1000000);
  if (dateEnd < from_undelimited_string(MIN_DATE_OF_HIS_MD)) {
    const auto statusMsg = fmt::format(
        "Load his market data between 2 ts failed because "
        "tsEnd {} less than {}. topic = {}",
        tsEnd, MIN_DATE_OF_HIS_MD, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  LOG_I("Begin to load his market data between {} and {}.",
        ConvertTsToPtime(tsBegin), ConvertTsToPtime(tsEnd));

  const auto stableName = TBENG_DBNAME_OF_MD;
  //! tableName = Trades_Spot_SSE_600600 or Candle_Spot_SSE_600600_1m or
  //! Candle_Spot_Binance_BTC-USDT
  std::string tableName;
  tableName = fmt::format("{}{}{}{}{}{}{}", mdType, SEP_OF_TDENG_TABLE_NAME,
                          symbolType, SEP_OF_TDENG_TABLE_NAME, marketCode,
                          SEP_OF_TDENG_TABLE_NAME, symbolCode);
  if (mdType == magic_enum::enum_name(MDType::Candle) && !freq.empty()) {
    tableName.append(SEP_OF_TDENG_TABLE_NAME).append(freq);
  }
  const auto sql =
      fmt::format("SELECT * FROM {}.{} where exchTs >= {} AND exchTs < {};",
                  stableName, tableName, tsBegin, tsEnd);
  const auto maxNumOfRecReturned =
      CONFIG["maxNumOfRecReturned"].as<std::uint32_t>(10000);
  const auto [statusCode, statusMsg, recNum, recSet] =
      tdeng::QueryDataFromTDEng(
          WebSrv::get_mutable_instance().getTDEngConnpool(), sql,
          maxNumOfRecReturned);
  const auto rsp = makeHttpResponse(makeBody(statusCode, statusMsg, recSet));
  callback(rsp);
  return;
}

//! http://192.168.19.115/v1/QueryHisMD/before/SZSE/Spot/000610/Trades?ts=1672295971000000&num=100
void QueryHisMD::queryBeforeTs(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string &&marketCode, std::string &&symbolType,
    std::string &&symbolCode, std::string &&mdType, std::uint64_t ts, int num,
    std::string &&freq) const {
  using namespace boost::gregorian;
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                  SEP_OF_TOPIC, marketCode,  //
                  SEP_OF_TOPIC, symbolType,  //
                  SEP_OF_TOPIC, symbolCode,  //
                  SEP_OF_TOPIC, mdType);

  const auto maxNumOfRecReturned = CONFIG["maxNumOfRecReturned"].as<int>(10000);
  if (num > maxNumOfRecReturned) {
    const auto statusMsg = fmt::format(
        "Load his market data before ts failed because "
        "rec num of result {} greater than the query limit {}. topic = {}",
        num, maxNumOfRecReturned, topic);
    LOG_W(statusMsg);
    const auto rsp = makeHttpResponse(makeBody(
        SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT, statusMsg, ""));
    callback(rsp);
    return;
  }

  const auto dateBegin = GetDateFromTs(ts / 1000000);
  if (dateBegin < from_undelimited_string(MIN_DATE_OF_HIS_MD)) {
    const auto statusMsg = fmt::format(
        "Load his markete data before ts failed because "
        "ts {} less than {}. topic = {}",
        ts, MIN_DATE_OF_HIS_MD, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  if (dateBegin >
      day_clock::universal_day() + date_duration(MAX_DATE_OFFSET_OF_QUERY_MD)) {
    const auto statusMsg = fmt::format(
        "Load his market data before ts failed because "
        "ts {} greater than the day after tomorrow. topic = {}",
        ts, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  const auto dateEnd = from_undelimited_string(MIN_DATE_OF_HIS_MD);
  LOG_I("Begin to load {} numbers of of his market data before {}. topic = {}",
        num, ConvertTsToPtime(ts), topic);

  const auto stableName = TBENG_DBNAME_OF_MD;
  //! tableName = Trades_Spot_SSE_600600 or Candle_Spot_SSE_600600_1m
  std::string tableName;
  tableName = fmt::format("{}{}{}{}{}{}{}", mdType, SEP_OF_TDENG_TABLE_NAME,
                          symbolType, SEP_OF_TDENG_TABLE_NAME, marketCode,
                          SEP_OF_TDENG_TABLE_NAME, symbolCode);
  if (mdType == magic_enum::enum_name(MDType::Candle) && !freq.empty()) {
    tableName.append(SEP_OF_TDENG_TABLE_NAME).append(freq);
  }
  const auto sql =
      fmt::format("SELECT * FROM {}.{} where exchTs <= {} limit {};",
                  stableName, tableName, ts, num);
  const auto [statusCode, statusMsg, recNum, recSet] =
      tdeng::QueryDataFromTDEng(
          WebSrv::get_mutable_instance().getTDEngConnpool(), sql,
          maxNumOfRecReturned);
  if (recNum < num) {
    const auto rsp = makeHttpResponse(makeBody(
        SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES,
        GetStatusMsg(SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES), recSet));
    callback(rsp);
    return;
  }
  const auto rsp = makeHttpResponse(makeBody(statusCode, statusMsg, recSet));
  callback(rsp);
  return;
}

//! http://192.168.19.115/v1/QueryHisMD/after/SZSE/Spot/000610/Trades?ts=1672295971000000&num=100
void QueryHisMD::queryAfterTs(
    const HttpRequestPtr &req,
    std::function<void(const HttpResponsePtr &)> &&callback,
    std::string &&marketCode, std::string &&symbolType,
    std::string &&symbolCode, std::string &&mdType, std::uint64_t ts, int num,
    std::string &&freq) const {
  using namespace boost::gregorian;
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                  SEP_OF_TOPIC, marketCode,  //
                  SEP_OF_TOPIC, symbolType,  //
                  SEP_OF_TOPIC, symbolCode,  //
                  SEP_OF_TOPIC, mdType);

  const auto maxNumOfRecReturned = CONFIG["maxNumOfRecReturned"].as<int>(10000);
  if (num > maxNumOfRecReturned) {
    const auto statusMsg = fmt::format(
        "Load his market data after ts failed because "
        "rec num of result {} greater than the query limit {}. topic = {}",
        num, maxNumOfRecReturned, topic);
    LOG_W(statusMsg);
    const auto rsp = makeHttpResponse(makeBody(
        SCODE_HIS_MD_NUM_OF_RECORDS_GREATER_THAN_LIMIT, statusMsg, ""));
    callback(rsp);
    return;
  }

  const auto dateBegin = GetDateFromTs(ts / 1000000);
  if (dateBegin < from_undelimited_string(MIN_DATE_OF_HIS_MD)) {
    const auto statusMsg = fmt::format(
        "Load his market data after ts failed because "
        "ts {} less than {}. topic = {}",
        ts, MIN_DATE_OF_HIS_MD, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  const auto dateEnd =
      day_clock::universal_day() + date_duration(MAX_DATE_OFFSET_OF_QUERY_MD);
  if (dateBegin > dateEnd) {
    const auto statusMsg = fmt::format(
        "Load his market data after ts failed because "
        "ts {} greater than the day after tomorrow. topic = {}",
        ts, topic);
    LOG_W(statusMsg);
    const auto rsp =
        makeHttpResponse(makeBody(SCODE_HIS_MD_INVALID_TS, statusMsg, ""));
    callback(rsp);
    return;
  }

  LOG_I("Begin to load {} numbers of his market data after {}. topic = {}", num,
        ConvertTsToPtime(ts), topic);

  const auto stableName = TBENG_DBNAME_OF_MD;
  //! tableName = Trades_Spot_SSE_600600 or Candle_Spot_SSE_600600_1m
  std::string tableName;
  tableName = fmt::format("{}{}{}{}{}{}{}", mdType, SEP_OF_TDENG_TABLE_NAME,
                          symbolType, SEP_OF_TDENG_TABLE_NAME, marketCode,
                          SEP_OF_TDENG_TABLE_NAME, symbolCode);
  if (mdType == magic_enum::enum_name(MDType::Candle) && !freq.empty()) {
    tableName.append(SEP_OF_TDENG_TABLE_NAME).append(freq);
  }
  const auto sql =
      fmt::format("SELECT * FROM {}.{} where exchTs >= {} limit {};",
                  stableName, tableName, ts, num);
  const auto [statusCode, statusMsg, recNum, recSet] =
      tdeng::QueryDataFromTDEng(
          WebSrv::get_mutable_instance().getTDEngConnpool(), sql,
          maxNumOfRecReturned);
  if (recNum < num) {
    const auto rsp = makeHttpResponse(makeBody(
        SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES,
        GetStatusMsg(SCODE_HIS_MD_RECORDS_LESS_THAN_NUM_OF_QURIES), recSet));
    callback(rsp);
    return;
  }
  const auto rsp = makeHttpResponse(makeBody(statusCode, statusMsg, recSet));
  callback(rsp);
  return;
}

std::string QueryHisMD::makeBody(int statusCode, const std::string &statusMsg,
                                 std::string data) const {
  if (!data.empty()) {
    data[0] = ',';
  } else {
    data = ",\"data\":[]}";
  }
  std::string body = fmt::format("{{\"statusCode\":{},\"statusMsg\":\"{}\"{}",
                                 statusCode, statusMsg, data);
  return body;
}

HttpResponsePtr QueryHisMD::makeHttpResponse(const std::string &body) const {
  auto resp = HttpResponse::newHttpResponse();
  resp->setStatusCode(k200OK);
  resp->setContentTypeCode(CT_APPLICATION_JSON);
  resp->setBody(body);
  return resp;
}
