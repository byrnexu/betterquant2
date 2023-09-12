/*!
 * \file UserId2WSConnGroup.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/16 *
 * \brief
 */

#include "UserId2WSConnGroup.hpp"

#include "util/Logger.hpp"

namespace bq {

UserId2WSConnGroup::UserId2WSConnGroup(WebSrv* webSrv) : webSrv_(webSrv) {}

WebSocketConnectionPtr UserId2WSConnGroup::getWSConn(UserId userId) {
  WebSocketConnectionPtr ret = nullptr;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxUserId2WSConnGroup_);
    const auto iter = userId2WSConnGroup_.find(userId);
    if (iter != std::end(userId2WSConnGroup_)) {
      LOG_T("Get connection {} of userId {} in userId to wsConn group.",
            iter->second->peerAddr().toIpPort(), userId);
      ret = iter->second;
      return ret;
    } else {
      LOG_W("Get connection of userId {} in userId to wsConn group failed.",
            userId);
      return ret;
    }
  }
}

void UserId2WSConnGroup::setUserId2WSConn(
    UserId userId, const WebSocketConnectionPtr& wsConn) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxUserId2WSConnGroup_);
    LOG_I("Set connection {} of userId {} in userId to wsConn group.",
          wsConn->peerAddr().toIpPort(), userId);
    userId2WSConnGroup_[userId] = wsConn;
  }
}

UserId UserId2WSConnGroup::removeWSConn(const WebSocketConnectionPtr& wsConn) {
  UserId userId = 0;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxUserId2WSConnGroup_);
    for (auto iter = std::begin(userId2WSConnGroup_);
         iter != std::end(userId2WSConnGroup_); ++iter) {
      if (iter->second->peerAddr().toIpPort() ==
          wsConn->peerAddr().toIpPort()) {
        LOG_I("Remove connection {} of userId {} in userId to wsConn group.",
              wsConn->peerAddr().toIpPort(), iter->first);
        userId = iter->first;
        userId2WSConnGroup_.erase(iter);
        return userId;
      }
    }
  }
  return userId;
}

}  // namespace bq
