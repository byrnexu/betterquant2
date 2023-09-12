/*!
 * \file RawMD.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/22
 *
 * \brief
 */

#include "def/RawMD.hpp"

namespace bq {

//! MD@SH@Spot@600600@Tickers
void InitTopicInfo(RawMDSPtr& rawMD) {
  rawMD->topic_ = fmt::format(
      "{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_TOPIC,
      GetMarketName(rawMD->marketCode_), SEP_OF_TOPIC,
      magic_enum::enum_name(rawMD->symbolType_), SEP_OF_TOPIC,
      rawMD->symbolCode_, SEP_OF_TOPIC, magic_enum::enum_name(rawMD->mdType_));
  rawMD->topicHash_ = XXH3_64bits(rawMD->topic_.data(), rawMD->topic_.size());
}

}  // namespace bq
