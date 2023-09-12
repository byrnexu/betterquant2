/*!
 * \file SHMSrvMsgHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "SHMSrvMsgHandler.hpp"

#include "CommonIPCData.hpp"
#include "MDSvcOfCN.hpp"
#include "SHMIPC.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/TopicMgr.hpp"

namespace bq::md::svc {

SHMSrvMsgHandler::SHMSrvMsgHandler(MDSvcOfCN* mdSvc) : mdSvc_(mdSvc) {}

void SHMSrvMsgHandler::handleReq(const void* shmBuf, std::size_t shmBufLen) {
  const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
  switch (shmHeader->msgId_) {
    case MSG_ID_SYNC_SUB_INFO:
      handleSyncSubInfo(shmBuf, shmBufLen);
      break;
    default:
      break;
  }
}

/*
{
 "subscriber2TopicGroup": [{
    "subscriber": "1",
    "topicGroup": ["MD@SSE@Spot@603123@Orders", "MD@SSE@Spot@603123@Tickers"]
 }, {
    "subscriber": "2",
    "topicGroup": ["MD@SSE@Spot@603123@Orders", "MD@SSE@Spot@603123@Tickers"]
}]
}
*/
void SHMSrvMsgHandler::handleSyncSubInfo(const void* shmBuf,
                                         std::size_t shmBufLen) {
  //! 接受订阅请求，同步TopicMgr中的订阅信息
  const auto commonIPCData = static_cast<const CommonIPCData*>(shmBuf);
  const auto subscriber2TopicGroupInJsonFmt = std::string(commonIPCData->data_);
  mdSvc_->getTopicMgr()->updateForSrv(subscriber2TopicGroupInJsonFmt);
}

}  // namespace bq::md::svc
