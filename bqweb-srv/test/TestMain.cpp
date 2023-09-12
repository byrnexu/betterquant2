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

#include "util/Pch.hpp"

class global_event : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

std::string MakeCommonHttpBody(int statusCode, const std::string& statusMsg,
                               std::string rspBody, std::string reqBody) {
  auto ret = fmt::format(R"({{"statusCode":{},"statusMsg":"{}"}})", statusCode,
                         statusMsg);
  if (!rspBody.empty()) {
    ret.pop_back();
    rspBody[0] = ',';
    ret.append(rspBody);
  }

  if (!reqBody.empty()) {
    ret[ret.size() - 1] = ',';
    ret.append(R"("req":)");
    ret.append(reqBody);
    ret.append("}");
  }

  return ret;
}

// clang-format off
TEST(MakeCommonHttpBodyTest, AllEmpty) {
  const auto result = MakeCommonHttpBody(200, "OK", "", "");
  const std::string expected = R"({"statusCode":200,"statusMsg":"OK"})";
  ASSERT_EQ(result, expected);
}

TEST(MakeCommonHttpBodyTest, RspBodyEmpty) {
  const auto result = MakeCommonHttpBody(400, "Bad Request", "", R"({"name": "John"})");
  const std::string expected = R"({"statusCode":400,"statusMsg":"Bad Request","req":{"name": "John"}})";
  ASSERT_EQ(result, expected);
}

TEST(MakeCommonHttpBodyTest, ReqBodyEmpty) {
  const auto result = MakeCommonHttpBody(500, "Internal Server Error", R"({"error": "Something went wrong"})", "");
  const std::string expected = R"({"statusCode":500,"statusMsg":"Internal Server Error","error": "Something went wrong"})";
  ASSERT_EQ(result, expected);
}

TEST(MakeCommonHttpBodyTest, NoneEmpty) {
  const auto result = MakeCommonHttpBody(201, "Created", R"({"id": 123})", R"({"name": "John"})");
  const std::string expected = R"({"statusCode":201,"statusMsg":"Created","id": 123,"req":{"name": "John"}})";
  ASSERT_EQ(result, expected);
}
// clang-format on

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new global_event);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
