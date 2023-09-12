/*!
 * \file bq_v1_Login.cpp
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

// clang-format off
// curl -i -k -H "Content-type: application/json" -X POST -d '{"username":"admin", "password":"123456", "sql":"SELECT * FROM baseUserInfo"}' "http://localhost/v1/user/login"
//
/*
{
	"statusCode": 0,
	"stausMsg": "",
	"userId": 1,
	"username": "admin",
	"password": "123456",
	"token": "GB7gmcjLDYLkAPC",
	"req": {
		"username": "admin",
		"password": "123456"
	}
}
*/
// clang-format on
class Login : public drogon::HttpController<Login> {
 public:
  METHOD_LIST_BEGIN
  // http://localhost/v1/user/login
  ADD_METHOD_TO(Login::login, "/v1/user/login", Post, Options);
  METHOD_LIST_END

  void login(const HttpRequestPtr &req,
             std::function<void(const HttpResponsePtr &)> &&callback) const;
};

}  // namespace user
}  // namespace v1
}  // namespace bq
