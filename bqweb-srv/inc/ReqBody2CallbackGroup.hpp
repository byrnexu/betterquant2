/*!
 * \file ReqBody2CallbackGroup.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/16 *
 * \brief
 */

#pragma once

#include <drogon/HttpController.h>
#include <drogon/WebSocketController.h>

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

using namespace drogon;

namespace bq {

class WebSrv;

class ReqBody2CallbackGroup;
using ReqBody2CallbackGroupSPtr = std::shared_ptr<ReqBody2CallbackGroup>;

class ReqBody2CallbackGroup {
 public:
  ReqBody2CallbackGroup(const ReqBody2CallbackGroup&) = delete;
  ReqBody2CallbackGroup& operator=(const ReqBody2CallbackGroup&) = delete;
  ReqBody2CallbackGroup(const ReqBody2CallbackGroup&&) = delete;
  ReqBody2CallbackGroup& operator=(const ReqBody2CallbackGroup&&) = delete;

  explicit ReqBody2CallbackGroup(WebSrv* webSrv);

  std::function<void(const HttpResponsePtr&)> getCallback(
      const std::string& reqBody);
  void cacheCallback(
      const std::string& reqBody,
      const std::function<void(const HttpResponsePtr&)>& callback);

 private:
  WebSrv* webSrv_{nullptr};

  std::map<std::string, std::function<void(const HttpResponsePtr&)>>
      reqBody2CallbackGroup_;
  mutable std::ext::spin_mutex mtxReqBody2CallbackGroup_;
};

}  // namespace bq
