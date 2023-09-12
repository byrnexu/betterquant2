/*!
 * \file TestMain.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "AlgoUtil.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Pch.hpp"

using namespace bq;
using namespace bq::algo;

void printResult(Decimal totalSize, std::uint32_t splitNum, Decimal prec,
                 const std::vector<Decimal>& result) {
  std::string content;
  for (auto elem : result) {
    content.append(std::to_string(elem)).append(",");
  }
  if (!content.empty()) content.pop_back();
  std::cout << fmt::format("{}/{} {} [{}]", totalSize, splitNum, prec, content)
            << std::endl;
}

std::string toStr(const std::vector<Decimal>& result) {
  std::string content;
  for (auto elem : result) {
    content.append(std::to_string(elem)).append(",");
  }
  if (!content.empty()) content.pop_back();
  return fmt::format("[{}]", content);
}

// 定义测试用例
TEST(SplitTotalSizeTest, Test1) {
  // 调用被测试函数
  {
    auto result = splitTotalSize(1000, 3, 100);
    EXPECT_EQ(toStr(std::get<1>(result)), "[300.000000,300.000000,400.000000]");
  }

  {
    auto result = splitTotalSize(1000, 2, 100);
    EXPECT_EQ(toStr(std::get<1>(result)), "[500.000000,500.000000]");
  }

  {
    auto result = splitTotalSize(10.0, 3, 1.0);
    EXPECT_EQ(toStr(std::get<1>(result)), "[3.000000,3.000000,4.000000]");
  }

  {
    auto result = splitTotalSize(10.0, 3, 0.1);
    EXPECT_EQ(toStr(std::get<1>(result)), "[3.300000,3.300000,3.400000]");
  }

  {
    auto result = splitTotalSize(0.5, 3, 1);
    EXPECT_EQ(toStr(std::get<1>(result)), "[0.000000,0.000000,0.500000]");
  }

  {
    auto result = splitTotalSize(1.1, 3, 1);
    EXPECT_EQ(toStr(std::get<1>(result)), "[0.000000,0.000000,1.100000]");
  }

  {
    auto result = splitTotalSize(0.03, 3, 0.01);
    EXPECT_EQ(toStr(std::get<1>(result)), "[0.010000,0.010000,0.010000]");
  }

  {
    auto result = splitTotalSize(0.5, 3, 0.1);
    EXPECT_EQ(toStr(std::get<1>(result)), "[0.200000,0.200000,0.100000]");
  }
}

TEST(GetPriceOfUrgencyTest, TestPriceOfMaker) {
  auto bid1ask1 = std::make_shared<Bid1Ask1>();
  bid1ask1->askPrice_ = 10.0;
  bid1ask1->askSize_ = 100;
  bid1ask1->bidPrice_ = 9.0;
  bid1ask1->bidSize_ = 200;

  Decimal tickerSize = 0.1;
  std::uint32_t tickerOffset = 1;

  Side side1 = Side::Bid;
  Urgency urgency1 = Urgency::PriceOfMaker;
  Decimal expectedPrice1 = 9.0 - tickerOffset * tickerSize;
  EXPECT_DOUBLE_EQ(expectedPrice1, GetPriceOfUrgency(bid1ask1, side1, urgency1,
                                                     tickerOffset, tickerSize));

  Side side2 = Side::Ask;
  Urgency urgency2 = Urgency::PriceOfMaker;
  Decimal expectedPrice2 = 10.0 + tickerOffset * tickerSize;
  EXPECT_DOUBLE_EQ(expectedPrice2, GetPriceOfUrgency(bid1ask1, side2, urgency2,
                                                     tickerOffset, tickerSize));
}

TEST(GetPriceOfUrgencyTest, TestPriceOfMakerWith1Ticker) {
  auto bid1ask1 = std::make_shared<Bid1Ask1>();
  bid1ask1->askPrice_ = 10.0;
  bid1ask1->askSize_ = 100;
  bid1ask1->bidPrice_ = 9.9;
  bid1ask1->bidSize_ = 200;

  Decimal tickerSize = 0.1;
  std::uint32_t tickerOffset = 1;

  Side side1 = Side::Bid;
  Urgency urgency1 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice1 = 10.0;  // spread is approximately equal to tickerSize
  EXPECT_DOUBLE_EQ(expectedPrice1, GetPriceOfUrgency(bid1ask1, side1, urgency1,
                                                     tickerOffset, tickerSize));

  Side side2 = Side::Ask;
  Urgency urgency2 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice2 = 9.9;  // spread is approximately equal to tickerSize
  EXPECT_DOUBLE_EQ(expectedPrice2, GetPriceOfUrgency(bid1ask1, side2, urgency2,
                                                     tickerOffset, tickerSize));

  bid1ask1->askPrice_ = 10.0;
  bid1ask1->bidPrice_ = 9.7;

  Side side3 = Side::Bid;
  Urgency urgency3 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice3 = 9.8;
  EXPECT_DOUBLE_EQ(expectedPrice3, GetPriceOfUrgency(bid1ask1, side3, urgency3,
                                                     tickerOffset, tickerSize));

  Side side4 = Side::Ask;
  Urgency urgency4 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice4 = 9.9;
  EXPECT_DOUBLE_EQ(expectedPrice4, GetPriceOfUrgency(bid1ask1, side4, urgency4,
                                                     tickerOffset, tickerSize));

  bid1ask1->askPrice_ = 10.0;
  bid1ask1->bidPrice_ = 9.8;

  Side side5 = Side::Bid;
  Urgency urgency5 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice5 = 9.9;
  EXPECT_DOUBLE_EQ(expectedPrice5, GetPriceOfUrgency(bid1ask1, side5, urgency5,
                                                     tickerOffset, tickerSize));

  Side side6 = Side::Ask;
  Urgency urgency6 = Urgency::PriceOfMakerWith1Ticker;
  Decimal expectedPrice6 = 9.9;
  EXPECT_DOUBLE_EQ(expectedPrice6, GetPriceOfUrgency(bid1ask1, side6, urgency6,
                                                     tickerOffset, tickerSize));
}

TEST(GetPriceOfUrgencyTest, TestPriceOfMidPoint) {
  auto bid1ask1 = std::make_shared<Bid1Ask1>();
  bid1ask1->askPrice_ = 10.0;
  bid1ask1->askSize_ = 100;
  bid1ask1->bidPrice_ = 9.3;
  bid1ask1->bidSize_ = 200;

  Decimal tickerSize = 0.1;
  std::uint32_t tickerOffset = 1;

  Side side1 = Side::Bid;
  Urgency urgency1 = Urgency::PriceOfMidPoint;
  Decimal expectedPrice1 = 9.6;
  EXPECT_DOUBLE_EQ(expectedPrice1, GetPriceOfUrgency(bid1ask1, side1, urgency1,
                                                     tickerOffset, tickerSize));

  Side side2 = Side::Ask;
  Urgency urgency2 = Urgency::PriceOfMidPoint;
  Decimal expectedPrice2 = 9.7;
  EXPECT_DOUBLE_EQ(expectedPrice2, GetPriceOfUrgency(bid1ask1, side2, urgency2,
                                                     tickerOffset, tickerSize));
}

TEST(GetPriceOfUrgencyTest, TestPriceOfTaker) {
  auto bid1ask1 = std::make_shared<Bid1Ask1>();
  bid1ask1->askPrice_ = 10.0;
  bid1ask1->askSize_ = 100;
  bid1ask1->bidPrice_ = 9.0;
  bid1ask1->bidSize_ = 200;

  Decimal tickerSize = 0.1;
  std::uint32_t tickerOffset = 1;

  Side side1 = Side::Bid;
  Urgency urgency1 = Urgency::PriceOfTaker;
  Decimal expectedPrice1 = 10.0 + tickerOffset * tickerSize;
  EXPECT_DOUBLE_EQ(expectedPrice1, GetPriceOfUrgency(bid1ask1, side1, urgency1,
                                                     tickerOffset, tickerSize));

  Side side2 = Side::Ask;
  Urgency urgency2 = Urgency::PriceOfTaker;
  Decimal expectedPrice2 = 9.0 - tickerOffset * tickerSize;
  EXPECT_DOUBLE_EQ(expectedPrice2, GetPriceOfUrgency(bid1ask1, side2, urgency2,
                                                     tickerOffset, tickerSize));
}

class global_event : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new global_event);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
