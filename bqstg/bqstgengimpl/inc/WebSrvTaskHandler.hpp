/*!
 * \file WebSrvTaskHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/12
 *
 * \brief
 */

#pragma once

#include "SHMIPCMsgId.hpp"
#include "util/Pch.hpp"

namespace bq::stg {

class StgEngImpl;

class WebSrvTaskHandler {
 public:
  WebSrvTaskHandler(const WebSrvTaskHandler&) = delete;
  WebSrvTaskHandler& operator=(const WebSrvTaskHandler&) = delete;
  WebSrvTaskHandler(const WebSrvTaskHandler&&) = delete;
  WebSrvTaskHandler& operator=(const WebSrvTaskHandler&&) = delete;

  explicit WebSrvTaskHandler(StgEngImpl* stgEng);

 public:
  void handleTask(const void* buf, std::size_t bufLen);

 private:
  void handleStgManualIntervention(const void* buf, std::size_t bufLen);
  void handleStgManualOrder(const void* buf, std::size_t bufLen);
  void handleStgManualCancelOrder(const void* buf, std::size_t bufLen);

 private:
  void sendRspTOWebSrv(MsgId msgId, int statusCode, const std::string& reqBody);

 private:
  StgEngImpl* stgEng_{nullptr};
};

}  // namespace bq::stg
