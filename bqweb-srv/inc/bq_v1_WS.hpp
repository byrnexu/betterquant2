/*!
 * \file bq_v1_ws.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#pragma once

#include <drogon/WebSocketController.h>

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/StdExt.hpp"

using namespace drogon;

namespace bq {

class WSSrv : public drogon::WebSocketController<WSSrv> {
 public:
  void handleNewMessage(const WebSocketConnectionPtr &, std::string &&,
                        const WebSocketMessageType &) final;

  void handleNewConnection(const HttpRequestPtr &,
                           const WebSocketConnectionPtr &) final;

  void handleConnectionClosed(const WebSocketConnectionPtr &) final;

  WS_PATH_LIST_BEGIN
  WS_PATH_ADD("/ws", Get, Post, Options);
  WS_PATH_LIST_END

 private:
  void handleMsgRegister(const WebSocketConnectionPtr &wsConnPtr,
                         std::string &&message,
                         const WebSocketMessageType &messageType,
                         const Doc &doc);

  void handleMsgSub(const WebSocketConnectionPtr &wsConnPtr,
                    std::string &&message,
                    const WebSocketMessageType &messageType, const Doc &doc);

  void handleMsgUnSub(const WebSocketConnectionPtr &wsConnPtr,
                      std::string &&message,
                      const WebSocketMessageType &messageType, const Doc &doc);
};

}  // namespace bq
