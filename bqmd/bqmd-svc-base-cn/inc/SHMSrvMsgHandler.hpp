/*!
 * \file SHMSrvMsgHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc {

class MDSvcOfCN;

class SHMSrvMsgHandler;
using SHMSrvMsgHandlerSPtr = std::shared_ptr<SHMSrvMsgHandler>;

class SHMSrvMsgHandler {
 public:
  SHMSrvMsgHandler(const SHMSrvMsgHandler&) = delete;
  SHMSrvMsgHandler& operator=(const SHMSrvMsgHandler&) = delete;
  SHMSrvMsgHandler(const SHMSrvMsgHandler&&) = delete;
  SHMSrvMsgHandler& operator=(const SHMSrvMsgHandler&&) = delete;
  SHMSrvMsgHandler(MDSvcOfCN* mdSvc);

  void handleReq(const void* shmBuf, std::size_t shmBufLen);

 private:
  void handleSyncSubInfo(const void* shmBuf, std::size_t shmBufLen);

 protected:
  MDSvcOfCN* mdSvc_{nullptr};
};

}  // namespace bq::md::svc
