/*!
 * \file ReqParserOfBinance.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "ReqParserOfBinance.hpp"

#include "Config.hpp"
#include "MDSvc.hpp"
#include "MDSvcOfBinanceUtil.hpp"
#include "util/Json.hpp"
#include "util/String.hpp"

namespace bq::md::svc::binance {

/*
 * {
 *  "method": "SUBSCRIBE",
 *   "params": [
 *    "btcusdt@aggTrade",
 *    "btcusdt@depth"
 *   ],
 *   "id": 1
 * }
 *
 * MD@Binance@Spot@BTC-USDT@Candle@detail
 */
std::vector<std::string> ReqParserOfBinance::doGetTopicGroupForSubOrUnSubAgain(
    const std::string& req) {
  std::vector<std::string> ret;
  Doc doc;
  if (doc.Parse(req.data()).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(), req);
    return ret;
  }

  //! 生成topic前缀，格式类似MD@Binance@Spot
  const auto& marketCode = mdSvc_->getMarketCode();
  const auto& symbolType = mdSvc_->getSymbolType();
  const auto prefix =
      fmt::format("{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_TOPIC,
                  marketCode, SEP_OF_TOPIC, symbolType);

  //! 遍历请求中的params列表
  for (std::size_t i = 0; i < doc["params"].Size(); ++i) {
    const std::string params = doc["params"][i].GetString();
    //! 切分params，格式类似"btcusdt@depth"，得到exchSymbolCode和exchMDType
    const auto [splitRet, exchSymbolCode, exchMDType] =
        SplitStrIntoTwoParts(params, SEP_OF_EXCH_PARAMS);

    //! 根据exchSymbolCode查询获取symbolCode
    const auto [retOfGetSym, symbolCode] =
        mdSvc_->getTBLMonitorOfSymbolInfo()->getSymbolCode(
            marketCode, symbolType, exchSymbolCode);
    if (retOfGetSym != 0) {
      LOG_W("Get topic group for sub or unsub again failed. {}", req);
      continue;
    }

    //! 根据exchMDType获取mdType
    const auto [retOfGetMDType, mdType] = GetMDType(exchMDType);
    if (retOfGetMDType != 0) {
      LOG_W("Get topic group for sub or unsub again failed. {}", req);
      continue;
    }

    //! 加上topic前缀得到topic，格式类似MD@Binance@BTC-USDT@trades
    auto topic = fmt::format("{}{}{}{}{}", prefix, SEP_OF_TOPIC, symbolCode,
                             SEP_OF_TOPIC, mdType);
    if (mdType == magic_enum::enum_name(MDType::Candle)) {
      topic = topic + SEP_OF_TOPIC + SUFFIX_OF_CANDLE_DETAIL;
    }
    ret.emplace_back(topic);
  }

  return ret;
}

}  // namespace bq::md::svc::binance
