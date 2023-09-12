/*!
 * \file AlgoUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/17
 *
 * \brief
 */

#include "AlgoUtil.hpp"

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"

namespace bq::algo {

std::tuple<int, std::vector<Decimal>> splitTotalSize(Decimal totalSize,
                                                     uint32_t splitNum,
                                                     Decimal prec) {
  std::vector<Decimal> result;

  if (DEC::LE(totalSize, 0.0)) {
    LOG_W("[ALGO] Invalid totalSize {} in algo params. ", totalSize);
    return {SCODE_ALGO_INVALID_TOTAL_SIZE, result};
  }

  if (splitNum == 0) {
    LOG_W("[ALGO] Invalid splitNum {} in algo params. ", splitNum);
    return {SCODE_ALGO_INVALID_SPLIT_NUM, result};
  }

  if (DEC::LE(prec, 0.0)) {
    LOG_W("[ALGO] Invalid prec {} in algo params. ", prec);
    return {SCODE_ALGO_INVALID_PREC, result};
  }

  //! 计算每份的基本大小
  Decimal baseSize = totalSize / splitNum;

  //! 计算每份应该增加或减少的大小
  Decimal remainder = std::fmod(baseSize, prec);
  Decimal adjustment = remainder > prec / 2.0 ? prec - remainder : -remainder;

  //! 调整每份的大小并加入结果向量
  for (uint32_t i = 0; i < splitNum; ++i) {
    Decimal adjustedSize = baseSize + adjustment;
    result.push_back(adjustedSize);
    totalSize -= adjustedSize;
  }

  //! 在最后一份中添加剩余的大小
  result.back() += totalSize;

  //! 如果分割出来含有0的记录，那么说明入参有问题
  const auto iter = std::find_if(std::begin(result), std::end(result),
                                 [](auto elem) { return DEC::ZERO(elem); });
  if (iter != std::end(result)) {
    return {SCODE_ALGO_INVALID_ALGO_PARAM, result};
  }

  return {0, result};
}

Decimal GetPriceOfUrgency(const Bid1Ask1SPtr& bid1ask1, Side side,
                          Urgency urgency, std::uint32_t tickerOffset,
                          Decimal precOfOrderPrice) {
  switch (urgency) {
    case Urgency::PriceOfMaker:
      if (side == Side::Bid) {
        return bid1ask1->bidPrice_ - tickerOffset * precOfOrderPrice;
      } else {
        return bid1ask1->askPrice_ + tickerOffset * precOfOrderPrice;
      }

    case Urgency::PriceOfMakerWith1Ticker: {
      const auto spread = bid1ask1->askPrice_ - bid1ask1->bidPrice_;
      if (DEC::EQ(spread, precOfOrderPrice)) {
        if (side == Side::Bid) {
          return bid1ask1->askPrice_;
        } else {
          return bid1ask1->bidPrice_;
        }
      } else {
        if (side == Side::Bid) {
          return bid1ask1->bidPrice_ + precOfOrderPrice;
        } else {
          return bid1ask1->askPrice_ - precOfOrderPrice;
        }
      }
    } break;

    case Urgency::PriceOfMidPoint: {
      const auto spread = bid1ask1->askPrice_ - bid1ask1->bidPrice_;
      if (DEC::EQ(spread, precOfOrderPrice)) {
        if (side == Side::Bid) {
          return bid1ask1->askPrice_;
        } else {
          return bid1ask1->bidPrice_;
        }
      } else {
        const auto priceOfMidPoint =
            (bid1ask1->bidPrice_ + bid1ask1->askPrice_) / 2;
        if (side == Side::Bid) {
          return std::floor(priceOfMidPoint / precOfOrderPrice) *
                 precOfOrderPrice;
        } else {
          return std::ceil(priceOfMidPoint / precOfOrderPrice) *
                 precOfOrderPrice;
        }
      }
    } break;

    case Urgency::PriceOfTaker:
      if (side == Side::Bid) {
        return bid1ask1->askPrice_ + tickerOffset * precOfOrderPrice;
      } else {
        return bid1ask1->bidPrice_ - tickerOffset * precOfOrderPrice;
      }

    default:
      break;
  }

  return 0;
}

std::tuple<Urgency, std::uint32_t> UpgradeUrgency(Urgency curUrgency,
                                                  std::uint32_t tickerOffset) {
  switch (curUrgency) {
    case Urgency::PriceOfMaker:
      if (tickerOffset == 0) {
        return {Urgency::PriceOfMakerWith1Ticker, 0};
      } else {
        tickerOffset -= 1;
        return {Urgency::PriceOfMaker, tickerOffset};
      }

    case Urgency::PriceOfMakerWith1Ticker:
      return {Urgency::PriceOfMidPoint, 0};

    case Urgency::PriceOfMidPoint:
      return {Urgency::PriceOfTaker, 0};

    case Urgency::PriceOfTaker:
      tickerOffset += 1;
      return {Urgency::PriceOfTaker, tickerOffset};

    default:
      break;
  }
  return {Urgency::PriceOfMaker, 0};
}

}  // namespace bq::algo
