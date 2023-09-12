/*!
 * \file bq_v1_user_info.cpp
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
// http://localhost/v1/user/info?token=o24jiIAaxillaIHM
/*
{
	"statusCode": 0,
	"statusMsg": "Success",
	"recordSetGroup": [
		[{
			"id": 3,
			"userId": 1,
			"username": "admin",
			"password": "123456",
			"role": "admin,trader",
			"createTime": "2023-03-10 15:29:23.000000",
			"updateTime": "2023-04-01 06:44:43.002073"
		}]
	],
	"req": {
		"restful": "/v1/user/info?token=3vp6CTV67ZpISWT"
	}
}
*/
// clang-format on
class Info : public drogon::HttpController<Info> {
 public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(Info::info, "/v1/user/info?token={token}", Get, Options);
  METHOD_LIST_END

  void info(const HttpRequestPtr &req,
            std::function<void(const HttpResponsePtr &)> &&callback,
            std::string &&token) const;
};

}  // namespace user
}  // namespace v1
}  // namespace bq
