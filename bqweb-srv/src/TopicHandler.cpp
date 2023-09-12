/*!
 * \file TopicHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/14
 *
 * \brief
 */

#include "TopicHandler.hpp"

#include "CommonIPCData.hpp"
#include "ReqBody2CallbackGroup.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCTask.hpp"
#include "SHMIPCTopicName.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "UserId2WSConnGroup.hpp"
#include "WebSrv.hpp"
#include "WebSrvUtil.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/PosInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SyncTask.hpp"
#include "def/WebSrvMsgName.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"
#include "util/StdExt.hpp"
#include "util/SubMgr.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/Util.hpp"

namespace bq {

TopicHandler::TopicHandler(WebSrv* webSrv) : webSrv_(webSrv) {}

void TopicHandler::handleData(const void* shmBuf, std::size_t shmBufLen) {
  const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
  switch (shmHeader->msgId_) {
    case MSG_ID_ON_MD_TRADES:
      handleMsgIdTrades(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_ORDERS:
      handleMsgIdOrders(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_TICKERS:
      handleMsgIdTickers(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_CANDLE:
      handleMsgIdCandle(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_BOOKS:
      handleMsgIdBooks(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_BID1_ASK1:
      handleMsgIdMDBid1Ask1(shmBuf, shmBufLen);
      break;
    case MSG_ID_ON_MD_LAST_PRICE:
      handleMsgIdMDLastPrice(shmBuf, shmBufLen);
      break;
    case MSG_ID_POS_UPDATE_OF_ALL:
    case MSG_ID_POS_SNAPSHOT_OF_ALL:
      handleMsgIdPosSnapshot(shmBuf, shmBufLen);
      break;
    case MSG_ID_POS_UPDATE_OF_ACCT_ID:
    case MSG_ID_POS_SNAPSHOT_OF_ACCT_ID:
    case MSG_ID_POS_UPDATE_OF_STG_ID:
    case MSG_ID_POS_SNAPSHOT_OF_STG_ID:
    case MSG_ID_POS_UPDATE_OF_STG_INST_ID:
    case MSG_ID_POS_SNAPSHOT_OF_STG_INST_ID:
      //! 订阅了"RISK@PubChannel@Trade@PosInfo@All"之后也会收到这些消息
      break;
    default:
      LOG_W("Unable to process msgId {} - {}.", shmHeader->msgId_,
            GetMsgName(shmHeader->msgId_));
      break;
  }
}

void TopicHandler::handleMsgIdTrades(const void* shmBuf,
                                     std::size_t shmBufLen) {
  const auto md = static_cast<const Trades*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv trades. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdOrders(const void* shmBuf,
                                     std::size_t shmBufLen) {
  const auto md = static_cast<const Orders*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv orders. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdTickers(const void* shmBuf,
                                      std::size_t shmBufLen) {
  const auto md = static_cast<const Tickers*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv tickers. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdCandle(const void* shmBuf,
                                     std::size_t shmBufLen) {
  const auto md = static_cast<const Candle*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv candle. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdBooks(const void* shmBuf, std::size_t shmBufLen) {
  const auto md = static_cast<const Books*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv books. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdMDBid1Ask1(const void* shmBuf,
                                         std::size_t shmBufLen) {
  const auto md = static_cast<const Bid1Ask1*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv bid1ask1. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdMDLastPrice(const void* shmBuf,
                                          std::size_t shmBufLen) {
  const auto md = static_cast<const LastPrice*>(shmBuf);
  auto marketData = md->toJson();
  LOG_T("Recv lastprice. {}", marketData);
  pushMarketData(md->shmHeader_.topicHash_, std::move(marketData));
}

void TopicHandler::handleMsgIdPosSnapshot(const void* shmBuf,
                                          std::size_t shmBufLen) {
  //! 根据订阅得到的消息构建 posSnapshot
  auto buf = malloc(shmBufLen);
  memcpy(buf, shmBuf, shmBufLen);

  const auto posUpdateOfAllForPub =
      MakeMsgSPtrByTask<PosUpdateOfAllForPub>(buf);
  const auto posUpdateOfAll = MakePosUpdateOfAll(posUpdateOfAllForPub);
  const auto posSnapshotOfAll =
      std::make_shared<PosSnapshot>(std::move(posUpdateOfAll), nullptr);
  webSrv_->setPosSnapshotOfAll(posSnapshotOfAll);
}

void TopicHandler::pushMarketData(std::uint64_t topicHash,
                                  std::string&& marketData) {
  if (marketData.empty()) return;

  std::string rsp = fmt::format(R"({{"statusCode":0,"statusMsg":"","{}":"{}")",
                                MSG_NAME, MSG_MARKET_DATA);
  marketData[0] = ',';
  rsp.append(marketData);

  const auto subscriberGroup =
      webSrv_->getSubMgr()->getSubscriberGroupByTopicHash(topicHash);
  for (auto userId : subscriberGroup) {
    const auto wsConn = webSrv_->getUserId2WSConnGroup()->getWSConn(userId);
    if (wsConn) {
      wsConn->send(rsp);
      LOG_T("Push topic to user {}. {}", userId, rsp);
    } else {
      LOG_W(
          "Push topic to user {} failed "
          "because of ws connection not exists. {}",
          userId, rsp);
    }
  }
}

}  // namespace bq
