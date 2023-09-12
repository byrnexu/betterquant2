/*!
 * \file UserId2WSConnGroup.hpp
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

class UserId2WSConnGroup;
using UserId2WSConnGroupSPtr = std::shared_ptr<UserId2WSConnGroup>;

class UserId2WSConnGroup {
 public:
  UserId2WSConnGroup(const UserId2WSConnGroup&) = delete;
  UserId2WSConnGroup& operator=(const UserId2WSConnGroup&) = delete;
  UserId2WSConnGroup(const UserId2WSConnGroup&&) = delete;
  UserId2WSConnGroup& operator=(const UserId2WSConnGroup&&) = delete;

  UserId2WSConnGroup(WebSrv* webSrv);

  WebSocketConnectionPtr getWSConn(UserId userId);
  void setUserId2WSConn(UserId userId, const WebSocketConnectionPtr& wsConn);
  UserId removeWSConn(const WebSocketConnectionPtr& wsConn);

 private:
  WebSrv* webSrv_{nullptr};

  std::map<UserId, WebSocketConnectionPtr> userId2WSConnGroup_;
  std::ext::spin_mutex mtxUserId2WSConnGroup_;
};

}  // namespace bq
