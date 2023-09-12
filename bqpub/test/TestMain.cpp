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

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/PosInfo.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/SymbolCode.hpp"
#include "util/BQUtil.hpp"
#include "util/PosSnapshotImpl.hpp"
#include "util/TopicMgr.hpp"

using namespace bq;

class global_event : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST(testSimedTDInfo, testSimedTDInfoConvert) {
  {
    const auto simedTDInfoInJsonFmt =
        R"({"m":"simedTD","s":100,"t":"0,0.1,T;0,0.9,M"})";
    const auto [statusCode, simedTDInfo] =
        MakeSimedTDInfo(simedTDInfoInJsonFmt);
    const auto simedTDInfoInJsonFmtAfterConvert =
        ConvertSimedTDInfoToJsonFmt(simedTDInfo);
    EXPECT_TRUE(simedTDInfoInJsonFmt == simedTDInfoInJsonFmtAfterConvert);
  }

  {
    const auto simedTDInfoInJsonFmt = R"({"s":100,"t":"0,0.1,T;0,0.9,M"})";
    const auto j = MakeSimedTDInfoInJsonFmt(
        OrderStatus::Filled,
        {{Slippage(0), FilledPer(0.1), LiquidityDirection::Taker},
         {Slippage(0), FilledPer(0.9), LiquidityDirection::Maker}});
    EXPECT_TRUE(simedTDInfoInJsonFmt == j);
  }

  {
    const auto simedTDInfoInJsonFmt = R"({"m":"simedTD","s":110,"t":""})";
    const auto [statusCode, simedTDInfo] =
        MakeSimedTDInfo(simedTDInfoInJsonFmt);
    const auto simedTDInfoInJsonFmtAfterConvert =
        ConvertSimedTDInfoToJsonFmt(simedTDInfo);
    EXPECT_TRUE(simedTDInfoInJsonFmt == simedTDInfoInJsonFmtAfterConvert);
  }

  {
    const auto simedTDInfoInJsonFmt = R"({"s":110,"t":""})";
    const auto j = MakeSimedTDInfoInJsonFmt(OrderStatus::Failed, {});
    EXPECT_TRUE(simedTDInfoInJsonFmt == j);
  }
}

std::string PosGroup2Str(const PosInfoGroup& posInfoGroup) {
  std::string ret;
  for (const auto& rec : posInfoGroup) {
    ret = ret + rec->toStr() + "\n";
  }
  if (!ret.empty()) ret.pop_back();
  return ret;
}

TEST(testPosInfo, testMergePosInfoOfSpot) {
  auto posInfo1 = std::make_shared<PosInfo>();
  posInfo1->productId_ = 0;
  posInfo1->userId_ = 1;
  posInfo1->acctId_ = 2;
  posInfo1->stgId_ = 3;
  posInfo1->stgInstId_ = 4;
  posInfo1->algoId_ = 5;
  posInfo1->marketCode_ = MarketCode::Binance;
  posInfo1->symbolType_ = SymbolType::Spot;
  strncpy(posInfo1->symbolCode_, "BTC-USDT", sizeof(posInfo1->symbolCode_) - 1);
  posInfo1->side_ = Side::Bid;
  posInfo1->posSide_ = PosSide::Both;
  posInfo1->parValue_ = 100;
  strncpy(posInfo1->feeCurrency_, "USDT", sizeof(posInfo1->feeCurrency_) - 1);

  posInfo1->fee_ = 10;
  posInfo1->pos_ = 100;
  posInfo1->prePos_ = 0;
  posInfo1->avgOpenPrice_ = 20000;
  posInfo1->pnlUnReal_ = 30;
  posInfo1->pnlReal_ = 50;
  posInfo1->totalBidSize_ = 50;
  posInfo1->totalAskSize_ = -50;

  auto posInfo2 = std::make_shared<PosInfo>(*posInfo1);
  posInfo2->avgOpenPrice_ = 40000;
  posInfo2->feeCurrency_[0] = '\0';
  posInfo2->totalBidSize_ = 100;
  posInfo2->totalAskSize_ = -100;

  auto posInfo3 = std::make_shared<PosInfo>();
  posInfo3->productId_ = 0;
  posInfo3->userId_ = 1;
  posInfo3->acctId_ = 2;
  posInfo3->stgId_ = 3;
  posInfo3->stgInstId_ = 4;
  posInfo3->algoId_ = 5;
  posInfo3->marketCode_ = MarketCode::Binance;
  posInfo3->symbolType_ = SymbolType::Spot;
  strncpy(posInfo3->symbolCode_, "BTC-USDT", sizeof(posInfo3->symbolCode_) - 1);
  posInfo3->side_ = Side::Ask;
  posInfo3->posSide_ = PosSide::Both;
  posInfo3->parValue_ = 100;
  strncpy(posInfo3->feeCurrency_, "USDT", sizeof(posInfo3->feeCurrency_) - 1);

  posInfo3->fee_ = 10;
  posInfo3->pos_ = -100;
  posInfo3->prePos_ = 0;
  posInfo3->avgOpenPrice_ = 20000;
  posInfo3->pnlUnReal_ = 30;
  posInfo3->pnlReal_ = 50;
  posInfo3->totalBidSize_ = 50;
  posInfo3->totalAskSize_ = -50;

  auto posInfo4 = std::make_shared<PosInfo>(*posInfo3);
  posInfo4->avgOpenPrice_ = 40000;
  posInfo4->feeCurrency_[0] = '\0';
  posInfo4->totalBidSize_ = 100;
  posInfo4->totalAskSize_ = -100;

  PosInfoGroup posInfoGroup;
  posInfoGroup.emplace_back(posInfo1);
  posInfoGroup.emplace_back(posInfo2);
  posInfoGroup.emplace_back(posInfo3);
  posInfoGroup.emplace_back(posInfo4);

  MergePosInfoHasNoFeeCurrency(posInfoGroup);

  EXPECT_TRUE(posInfoGroup.size() == 2);
  EXPECT_TRUE(
      posInfoGroup[0]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/Spot/BTC-USDT/Bid/Both/100/USDT fee=30; pos=200; prePos=0; avgOpenPrice=30000; pnlUnReal=60; pnlReal=100)");
  EXPECT_TRUE(
      posInfoGroup[1]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/Spot/BTC-USDT/Ask/Both/100/USDT fee=30; pos=-200; prePos=0; avgOpenPrice=30000; pnlUnReal=60; pnlReal=100)");
}

TEST(testPosInfo, testMergePosInfoOfUBaseContract) {
  auto posInfo1 = std::make_shared<PosInfo>();
  posInfo1->productId_ = 0;
  posInfo1->userId_ = 1;
  posInfo1->acctId_ = 2;
  posInfo1->stgId_ = 3;
  posInfo1->stgInstId_ = 4;
  posInfo1->algoId_ = 5;
  posInfo1->marketCode_ = MarketCode::Binance;
  posInfo1->symbolType_ = SymbolType::Perp;
  strncpy(posInfo1->symbolCode_, "BTC-USDT", sizeof(posInfo1->symbolCode_) - 1);
  posInfo1->side_ = Side::Bid;
  posInfo1->posSide_ = PosSide::Both;
  posInfo1->parValue_ = 100;
  strncpy(posInfo1->feeCurrency_, "USDT", sizeof(posInfo1->feeCurrency_) - 1);

  posInfo1->fee_ = 10;
  posInfo1->pos_ = 100;
  posInfo1->prePos_ = 0;
  posInfo1->avgOpenPrice_ = 20000;
  posInfo1->pnlUnReal_ = 30;
  posInfo1->pnlReal_ = 50;
  posInfo1->totalBidSize_ = 50;
  posInfo1->totalAskSize_ = -50;

  auto posInfo2 = std::make_shared<PosInfo>(*posInfo1);
  posInfo2->avgOpenPrice_ = 40000;
  posInfo2->feeCurrency_[0] = '\0';
  posInfo2->totalBidSize_ = 100;
  posInfo2->totalAskSize_ = -100;

  auto posInfo3 = std::make_shared<PosInfo>();
  posInfo3->productId_ = 0;
  posInfo3->userId_ = 1;
  posInfo3->acctId_ = 2;
  posInfo3->stgId_ = 3;
  posInfo3->stgInstId_ = 4;
  posInfo3->algoId_ = 5;
  posInfo3->marketCode_ = MarketCode::Binance;
  posInfo3->symbolType_ = SymbolType::Perp;
  strncpy(posInfo3->symbolCode_, "BTC-USDT", sizeof(posInfo3->symbolCode_) - 1);
  posInfo3->side_ = Side::Ask;
  posInfo3->posSide_ = PosSide::Both;
  posInfo3->parValue_ = 100;
  strncpy(posInfo3->feeCurrency_, "USDT", sizeof(posInfo3->feeCurrency_) - 1);

  posInfo3->fee_ = 10;
  posInfo3->pos_ = -100;
  posInfo3->prePos_ = 0;
  posInfo3->avgOpenPrice_ = 20000;
  posInfo3->pnlUnReal_ = 30;
  posInfo3->pnlReal_ = 50;
  posInfo3->totalBidSize_ = 50;
  posInfo3->totalAskSize_ = -50;

  auto posInfo4 = std::make_shared<PosInfo>(*posInfo3);
  posInfo4->avgOpenPrice_ = 40000;
  posInfo4->feeCurrency_[0] = '\0';
  posInfo4->totalBidSize_ = 100;
  posInfo4->totalAskSize_ = -100;

  PosInfoGroup posInfoGroup;
  posInfoGroup.emplace_back(posInfo1);
  posInfoGroup.emplace_back(posInfo2);
  posInfoGroup.emplace_back(posInfo3);
  posInfoGroup.emplace_back(posInfo4);

  MergePosInfoHasNoFeeCurrency(posInfoGroup);

  EXPECT_TRUE(posInfoGroup.size() == 2);
  EXPECT_TRUE(
      posInfoGroup[0]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/Perp/BTC-USDT/Bid/Both/100/USDT fee=30; pos=200; prePos=0; avgOpenPrice=30000; pnlUnReal=60; pnlReal=100)");
  EXPECT_TRUE(
      posInfoGroup[1]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/Perp/BTC-USDT/Ask/Both/100/USDT fee=30; pos=-200; prePos=0; avgOpenPrice=30000; pnlUnReal=60; pnlReal=100)");
}

TEST(testPosInfo, testMergePosInfoOfCBaseContract) {
  auto posInfo1 = std::make_shared<PosInfo>();
  posInfo1->productId_ = 0;
  posInfo1->userId_ = 1;
  posInfo1->acctId_ = 2;
  posInfo1->stgId_ = 3;
  posInfo1->stgInstId_ = 4;
  posInfo1->algoId_ = 5;
  posInfo1->marketCode_ = MarketCode::Binance;
  posInfo1->symbolType_ = SymbolType::CPerp;
  strncpy(posInfo1->symbolCode_, "BTC-USDT", sizeof(posInfo1->symbolCode_) - 1);
  posInfo1->side_ = Side::Bid;
  posInfo1->posSide_ = PosSide::Both;
  posInfo1->parValue_ = 100;
  strncpy(posInfo1->feeCurrency_, "USDT", sizeof(posInfo1->feeCurrency_) - 1);

  posInfo1->fee_ = 10;
  posInfo1->pos_ = 100;
  posInfo1->prePos_ = 0;
  posInfo1->avgOpenPrice_ = 20000;
  posInfo1->pnlUnReal_ = 30;
  posInfo1->pnlReal_ = 50;
  posInfo1->totalBidSize_ = 50;
  posInfo1->totalAskSize_ = -50;

  auto posInfo2 = std::make_shared<PosInfo>(*posInfo1);
  posInfo2->avgOpenPrice_ = 40000;
  posInfo2->feeCurrency_[0] = '\0';
  posInfo2->totalBidSize_ = 100;
  posInfo2->totalAskSize_ = -100;

  auto posInfo3 = std::make_shared<PosInfo>();
  posInfo3->productId_ = 0;
  posInfo3->userId_ = 1;
  posInfo3->acctId_ = 2;
  posInfo3->stgId_ = 3;
  posInfo3->stgInstId_ = 4;
  posInfo3->algoId_ = 5;
  posInfo3->marketCode_ = MarketCode::Binance;
  posInfo3->symbolType_ = SymbolType::CPerp;
  strncpy(posInfo3->symbolCode_, "BTC-USDT", sizeof(posInfo3->symbolCode_) - 1);
  posInfo3->side_ = Side::Ask;
  posInfo3->posSide_ = PosSide::Both;
  posInfo3->parValue_ = 100;
  strncpy(posInfo3->feeCurrency_, "USDT", sizeof(posInfo3->feeCurrency_) - 1);

  posInfo3->fee_ = 10;
  posInfo3->pos_ = -100;
  posInfo3->prePos_ = 0;
  posInfo3->avgOpenPrice_ = 20000;
  posInfo3->pnlUnReal_ = 30;
  posInfo3->pnlReal_ = 50;
  posInfo3->totalBidSize_ = 50;
  posInfo3->totalAskSize_ = -50;

  auto posInfo4 = std::make_shared<PosInfo>(*posInfo3);
  posInfo4->avgOpenPrice_ = 40000;
  posInfo4->feeCurrency_[0] = '\0';
  posInfo4->totalBidSize_ = 100;
  posInfo4->totalAskSize_ = -100;

  PosInfoGroup posInfoGroup;
  posInfoGroup.emplace_back(posInfo1);
  posInfoGroup.emplace_back(posInfo2);
  posInfoGroup.emplace_back(posInfo3);
  posInfoGroup.emplace_back(posInfo4);

  MergePosInfoHasNoFeeCurrency(posInfoGroup);

  EXPECT_TRUE(posInfoGroup.size() == 2);
  EXPECT_TRUE(
      posInfoGroup[0]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/CPerp/BTC-USDT/Bid/Both/100/USDT fee=30; pos=200; prePos=0; avgOpenPrice=26666.666666666668; pnlUnReal=60; pnlReal=100)");
  EXPECT_TRUE(
      posInfoGroup[1]->toStr() ==
      R"(0/0/1/0/2/0/0/3/4/5/Binance/CPerp/BTC-USDT/Ask/Both/100/USDT fee=30; pos=-200; prePos=0; avgOpenPrice=26666.666666666668; pnlUnReal=60; pnlReal=100)");
}

TEST(testPosSnapshotImpl, testPosSnapshotImplGroupBy) {
  auto posInfo1 = std::make_shared<PosInfo>();
  posInfo1->userId_ = 1;
  posInfo1->acctId_ = 2;
  posInfo1->stgId_ = 3;
  posInfo1->stgInstId_ = 1211;

  auto posInfo2 = std::make_shared<PosInfo>(*posInfo1);
  posInfo2->userId_ = 2;
  posInfo2->acctId_ = 1;
  posInfo1->stgInstId_ = 2122;

  auto posInfo3 = std::make_shared<PosInfo>(*posInfo1);
  posInfo3->userId_ = 1;
  posInfo3->acctId_ = 2;
  posInfo1->stgInstId_ = 1213;

  auto posInfo4 = std::make_shared<PosInfo>(*posInfo1);
  posInfo4->userId_ = 2;
  posInfo4->acctId_ = 1;
  posInfo1->stgInstId_ = 2124;

  std::map<std::string, PosInfoSPtr> posInfoDetail;
  posInfoDetail.emplace(posInfo1->getKey(), posInfo1);
  posInfoDetail.emplace(posInfo2->getKey(), posInfo2);
  posInfoDetail.emplace(posInfo3->getKey(), posInfo3);
  posInfoDetail.emplace(posInfo4->getKey(), posInfo4);

  auto posSnapshotImpl =
      std::make_shared<PosSnapshotImpl>(posInfoDetail, nullptr);
  auto [ret, posInfoGroup] =
      posSnapshotImpl->queryPosInfoGroupBy("acctId&userId");
  EXPECT_TRUE((*posInfoGroup)["acctId=1&userId=2"]->size() == 2);
  EXPECT_TRUE((*posInfoGroup)["acctId=2&userId=1"]->size() == 2);
}

TEST(MatchPrefixTest, ValidMatch) {
  EXPECT_TRUE(matchPrefix("IF2309", "IF*"));
  EXPECT_TRUE(matchPrefix("IC2309", "IC*"));
}

TEST(MatchPrefixTest, InvalidMatch) {
  EXPECT_FALSE(matchPrefix("IFI2309", "IF*"));
  EXPECT_FALSE(matchPrefix("IF2309", "IC*"));
  EXPECT_FALSE(matchPrefix("I2309", "IC*"));
}

TEST(MatchPrefixTest, InvalidInput) { EXPECT_FALSE(matchPrefix("", "IF*")); }

TEST(testTopicMgr, testTopicMgr) {}

TEST(is_any_value_of, IntValues) {
  static_assert(std::ext::is_any_value_of<int, 1, 2, 3>(2),
                "Failed: is_any_value_of");
  static_assert(!std::ext::is_any_value_of<int, 1, 2, 3>(4),
                "Failed: is_any_value_of");
}

TEST(IsCNMarketOfFutures2Test, ValidFuturesMarkets) {
  ASSERT_TRUE(IsCNMarketOfFutures(MarketCode::SHFE));
  ASSERT_TRUE(IsCNMarketOfFutures(MarketCode::CZCE));
  ASSERT_TRUE(IsCNMarketOfFutures(MarketCode::DCE));
  ASSERT_TRUE(IsCNMarketOfFutures(MarketCode::CFFEX));
  ASSERT_TRUE(IsCNMarketOfFutures(MarketCode::INE));
}

TEST(IsCNMarketOfFutures2Test, InvalidFuturesMarkets) {
  ASSERT_FALSE(IsCNMarketOfFutures(MarketCode::SSE));
  ASSERT_FALSE(IsCNMarketOfFutures(MarketCode::SZSE));
  ASSERT_FALSE(IsCNMarketOfFutures(MarketCode::Others));
}

TEST(IsCNMarketOfSpots2Test, ValidSpotMarkets) {
  ASSERT_TRUE(IsCNMarketOfSpots(MarketCode::SSE));
  ASSERT_TRUE(IsCNMarketOfSpots(MarketCode::SZSE));
}

TEST(IsCNMarketOfSpots2Test, InvalidSpotMarkets) {
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::SHFE));
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::CZCE));
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::DCE));
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::CFFEX));
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::INE));
  ASSERT_FALSE(IsCNMarketOfSpots(MarketCode::Others));
}

TEST(IsCNMarket2Test, ValidMarkets) {
  ASSERT_TRUE(IsCNMarket(MarketCode::SSE));
  ASSERT_TRUE(IsCNMarket(MarketCode::SZSE));
  ASSERT_TRUE(IsCNMarket(MarketCode::SHFE));
  ASSERT_TRUE(IsCNMarket(MarketCode::CZCE));
  ASSERT_TRUE(IsCNMarket(MarketCode::DCE));
  ASSERT_TRUE(IsCNMarket(MarketCode::CFFEX));
  ASSERT_TRUE(IsCNMarket(MarketCode::INE));
}

TEST(IsCNMarket2Test, InvalidMarkets) {
  ASSERT_FALSE(IsCNMarket(MarketCode::Others));
}

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new global_event);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
