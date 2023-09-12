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

#include "TrdSymbolListMgr.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"

using namespace bq;

class global_event : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

TEST(TrdSymbolListMgr, whiteListExistsAndNotIn) {
  auto mgr = std::make_shared<TrdSymbolListMgr>("global", 0, "");
}

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new global_event);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
