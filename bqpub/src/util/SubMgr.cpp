/*!
 * \file SubMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/SubMgr.hpp"

#include "SHMCli.hpp"
#include "SHMIPCConst.hpp"
#include "def/StatusCode.hpp"
#include "util/BQUtil.hpp"
#include "util/Logger.hpp"
#include "util/StdExt.hpp"

namespace bq {

SubMgr::SubMgr(const std::string& appNameOfSubscriber,
               const DataRecvCallback& dataRecvCallback)
    : appNameOfSubscriber_(appNameOfSubscriber),
      dataRecvCallback_(dataRecvCallback) {}

SubMgr::~SubMgr() {}

void SubMgr::start() {}

void SubMgr::stop() {
  Addr2SHMCliGroup addr2SHMCliGroupCopy;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAddr2SHMCliGroup_);
    addr2SHMCliGroupCopy.insert(std::begin(addr2SHMCliGroup_),
                                std::end(addr2SHMCliGroup_));
  }
  for (const auto& rec : addr2SHMCliGroupCopy) {
    auto& shmCli = rec.second;
    shmCli->stop();
    LOG_I("Channel {} stopped.", rec.first);
  }
}

int SubMgr::sub(ClientChannel subscriber, const std::string& topic) {
  const auto internalTopic = convertTopic(topic);
  const auto topicHash =
      XXH3_64bits(internalTopic.data(), internalTopic.size());

  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2SubscriberGroup_);
    const auto iter = topicHash2SubscriberGroup_.find(topicHash);
    if (iter == std::end(topicHash2SubscriberGroup_)) {
      //! 没有任何用户订阅过
      SubscriberGroup subscriberGroup;
      subscriberGroup.emplace_back(subscriber);
      topicHash2SubscriberGroup_[topicHash] = subscriberGroup;
      LOG_I("SUB {} [{}]", topic, internalTopic);
    } else {
      auto& subscriberGroup = iter->second;
      auto iterSubscriber = std::find(std::begin(subscriberGroup),
                                      std::end(subscriberGroup), subscriber);
      if (iterSubscriber == std::end(subscriberGroup)) {
        //! 当前用户没有订阅过
        subscriberGroup.emplace_back(subscriber);
        LOG_I("SUB {} [{}]", topic, internalTopic);
      } else {
        return SCODE_BQPUB_TOPIC_ALREADY_SUB;
      }
    }
  }

  const auto ret = startSHMCliIfNotExists(internalTopic);
  return ret;
}

int SubMgr::startSHMCliIfNotExists(const std::string& topic) {
  const auto [ret, addr] = GetAddrFromTopic(appNameOfSubscriber_, topic);
  if (ret != 0) {
    return ret;
  }
  initSHMCli(addr);
  return 0;
}

void SubMgr::initSHMCli(const std::string& addr) {
  SHMCliSPtr shmCli;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAddr2SHMCliGroup_);
    const auto iter = addr2SHMCliGroup_.find(addr);
    if (iter != std::end(addr2SHMCliGroup_)) {
      return;
    }
    shmCli = std::make_shared<SHMCli>(addr, dataRecvCallback_);
    //! 订阅行情服务端的 PUB_CHANNEL
    shmCli->setClientChannel(PUB_CHANNEL);
    addr2SHMCliGroup_.emplace(addr, shmCli);
  }
  shmCli->start();
  LOG_I("Channel {} started.", addr);
}

int SubMgr::unSub(ClientChannel subscriber, const std::string& topic) {
  const auto internalTopic = convertTopic(topic);
  const auto topicHash =
      XXH3_64bits(internalTopic.data(), internalTopic.size());
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2SubscriberGroup_);
    const auto iter = topicHash2SubscriberGroup_.find(topicHash);
    if (iter == std::end(topicHash2SubscriberGroup_)) {
      //! 一个策略实例下多个算法单退出，会重复取消订阅，是正常现象
      LOG_I("Subscriber {} not sub topic {}. [topicHash = {}]", subscriber,
            internalTopic, topicHash);
      return SCODE_BQPUB_TOPIC_NOT_SUB;
    }

    auto& subscriberGroup = iter->second;
    auto iterSubscriber = std::find(std::begin(subscriberGroup),
                                    std::end(subscriberGroup), subscriber);
    if (iterSubscriber == std::end(subscriberGroup)) {
      //! 一个策略实例下多个算法单退出，会重复取消订阅，是正常现象
      LOG_I("Subscriber {} not sub topic {}. [topicHash = {}]", subscriber,
            internalTopic, topicHash);
      return SCODE_BQPUB_TOPIC_NOT_SUB;
    }
    subscriberGroup.erase(iterSubscriber);
    LOG_I("UNSUB {} [{}]", topic, internalTopic);
  }
  return 0;
}

void SubMgr::unSubAllTopic(ClientChannel subscriber) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2SubscriberGroup_);
    for (auto iter = std::begin(topicHash2SubscriberGroup_);
         iter != std::end(topicHash2SubscriberGroup_); ++iter) {
      auto& subscriberGroup = iter->second;
      std::ext::erase_if(subscriberGroup, [&](auto curSubscriber) {
        if (curSubscriber == subscriber) {
          LOG_I("Remove subscriber {} in subscriber group of topic {}.",
                subscriber, iter->first);
          return true;
        }
        return false;
      });
    }
  }
}

SubscriberGroup SubMgr::getSubscriberGroupByTopicHash(
    TopicHash topicHash) const {
  SubscriberGroup ret;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTopicHash2SubscriberGroup_);
    const auto iter = topicHash2SubscriberGroup_.find(topicHash);
    if (iter != std::end(topicHash2SubscriberGroup_)) {
      const auto& subscriberGroup = iter->second;
      ret.assign(std::begin(subscriberGroup), std::end(subscriberGroup));
    }
  }
  return ret;
}

Addr2SHMCliGroup SubMgr::getSHMCliGroup() const {
  Addr2SHMCliGroup ret;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAddr2SHMCliGroup_);
    for (const auto& addr2SHMClI : addr2SHMCliGroup_) {
      const auto& shmCli = addr2SHMClI.second;
      if (shmCli->isReady()) {
        ret.emplace(addr2SHMClI);
      }
    }
  }
  return ret;
}

}  // namespace bq
