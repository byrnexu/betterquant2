/*!
 * \file bq_v1_Logout.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/25
 *
 * \brief
 */

#pragma once

#include <drogon/HttpController.h>

#include "def/BQDef.hpp"

using namespace drogon;

namespace bq {
namespace v1 {
namespace user {

class Logout : public drogon::HttpController<Logout> {
 public:
  METHOD_LIST_BEGIN
  // http://localhost/v1/user/logout
  ADD_METHOD_TO(Logout::logout, "/v1/user/logout", Post, Options);
  METHOD_LIST_END

  void logout(const HttpRequestPtr &req,
              std::function<void(const HttpResponsePtr &)> &&callback) const;
};

}  // namespace user
}  // namespace v1
}  // namespace bq
