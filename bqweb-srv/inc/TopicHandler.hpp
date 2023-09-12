/*!
 * \file TopicHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/14
 *
 * \brief
 */

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

class WebSrv;

class TopicHandler;
using TopicHandlerSPtr = std::shared_ptr<TopicHandler>;

class TopicHandler {
 public:
  TopicHandler(const TopicHandler&) = delete;
  TopicHandler& operator=(const TopicHandler&) = delete;
  TopicHandler(const TopicHandler&&) = delete;
  TopicHandler& operator=(const TopicHandler&&) = delete;

  explicit TopicHandler(WebSrv* webSrv);

 public:
  void handleData(const void* shmBuf, std::size_t shmBufLen);

 private:
  void handleMsgIdTrades(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdOrders(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdTickers(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdCandle(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdBooks(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdMDBid1Ask1(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdMDLastPrice(const void* shmBuf, std::size_t shmBufLen);
  void handleMsgIdPosSnapshot(const void* shmBuf, std::size_t shmBufLen);

 private:
  void pushMarketData(std::uint64_t topicHash, std::string&& marketData);

 private:
  WebSrv* webSrv_{nullptr};
};

}  // namespace bq
