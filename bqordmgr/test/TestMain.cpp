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

#include "OrdMgr.hpp"

using namespace bq;

class global_event : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST(test, test1) {
  auto ordMgr1 = std::make_shared<OrdMgr<MIdxOrderIdOfOM>>();
  auto ordMgr2 = std::make_shared<
      OrdMgr<MIdxOrderIdOfOM, MIdxMarketCodeExchOrderIdOfOM>>();
}

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new global_event);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
