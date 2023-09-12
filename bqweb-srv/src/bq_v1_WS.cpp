/*!
 * \file bq_v1_ws.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#include "bq_v1_WS.hpp"

#include "UserId2WSConnGroup.hpp"
#include "WebSrv.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"

namespace bq {

void WSSrv::handleNewMessage(const WebSocketConnectionPtr &wsConnPtr,
                             std::string &&message,
                             const WebSocketMessageType &messageType) {
  if (messageType == WebSocketMessageType::Ping ||
      messageType == WebSocketMessageType::Pong ||
      messageType == WebSocketMessageType::Close) {
    LOG_D("Recv message of {}", magic_enum::enum_name(messageType));
    return;
  }

  LOG_I("WS recv {} from {}", message, wsConnPtr->peerAddr().toIpPort());

  Doc doc;
  if (doc.Parse(message.data()).HasParseError()) {
    LOG_W("WS recv invalid msg from client. {} {}",
          magic_enum::enum_name(messageType), message);
    return;
  }

  if (!doc.HasMember(MSG_NAME) || !doc[MSG_NAME].IsString()) {
    LOG_W("WS recv invalid msg from client. {}", message);
    return;
  }

  const std::string msgName = doc[MSG_NAME].GetString();
  if (msgName == MSG_REGISTER) {
    handleMsgRegister(wsConnPtr, std::move(message), messageType, doc);
  } else if (msgName == MSG_SUB) {
    handleMsgSub(wsConnPtr, std::move(message), messageType, doc);
  } else if (msgName == MSG_UN_SUB) {
    handleMsgUnSub(wsConnPtr, std::move(message), messageType, doc);
  } else {
    LOG_W("Unable to process msg {}.", msgName);
  }
}

void WSSrv::handleMsgRegister(const WebSocketConnectionPtr &wsConnPtr,
                              std::string &&message,
                              const WebSocketMessageType &messageType,
                              const Doc &doc) {
  if (!doc.HasMember("userId") || !doc["userId"].IsUint()) {
    LOG_W("WS recv invalid msg from client.", message);
    return;
  }
  const auto userId = doc["userId"].GetUint();
  WebSrv::get_mutable_instance().getUserId2WSConnGroup()->setUserId2WSConn(
      userId, wsConnPtr);
}

void WSSrv::handleMsgSub(const WebSocketConnectionPtr &wsConnPtr,
                         std::string &&message,
                         const WebSocketMessageType &messageType,
                         const Doc &doc) {
  if (!doc.HasMember("userId") || !doc["userId"].IsUint()) {
    LOG_W("WS recv invalid msg from client.", message);
    return;
  }

  if (!doc.HasMember("topic") || !doc["topic"].IsString()) {
    LOG_W("WS recv invalid msg from client.", message);
    return;
  }

  const auto userId = doc["userId"].GetUint();
  const auto topic = doc["topic"].GetString();
  WebSrv::get_mutable_instance().sub(userId, topic);
}

void WSSrv::handleMsgUnSub(const WebSocketConnectionPtr &wsConnPtr,
                           std::string &&message,
                           const WebSocketMessageType &messageType,
                           const Doc &doc) {
  if (!doc.HasMember("userId") || !doc["userId"].IsUint()) {
    LOG_W("WS recv invalid msg from client.", message);
    return;
  }

  if (!doc.HasMember("topic") || !doc["topic"].IsString()) {
    LOG_W("WS recv invalid msg from client.", message);
    return;
  }

  const auto userId = doc["userId"].GetUint();
  const auto topic = doc["topic"].GetString();
  WebSrv::get_mutable_instance().unSub(userId, topic);
}

void WSSrv::handleNewConnection(const HttpRequestPtr &req,
                                const WebSocketConnectionPtr &wsConnPtr) {
  LOG_I("WS new connection from {}.", wsConnPtr->peerAddr().toIpPort());
}

void WSSrv::handleConnectionClosed(const WebSocketConnectionPtr &wsConnPtr) {
  LOG_I("WS connection from {} closed.", wsConnPtr->peerAddr().toIpPort());
  const auto userId =
      WebSrv::get_mutable_instance().getUserId2WSConnGroup()->removeWSConn(
          wsConnPtr);
  WebSrv::get_mutable_instance().unSubAllTopic(userId);
}

}  // namespace bq
