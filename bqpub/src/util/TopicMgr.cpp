/*!
 * \file TopicMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/26
 *
 * \brief
 */

#include "util/TopicMgr.hpp"

#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/Scheduler.hpp"

namespace bq {

TopicMgr::TopicMgr(TopicMgrRole role, const CBHandleSubInfo& cbHandleSubInfo) {
  role_ = role;
  cbHandleSubInfo_ = cbHandleSubInfo;
  scheduler_ = std::make_shared<Scheduler>(
      "TopicMgr", [this]() { handleSubInfo(); }, 1 * 100);
}

TopicMgr::~TopicMgr() {}

void TopicMgr::start() { scheduler_->start(); }
void TopicMgr::stop() { scheduler_->stop(); }

//! 此方法仅用于维护subscriber2TopicGroup_
int TopicMgr::add(const std::string& subscriber, const std::string& topic) {
  const auto internalTopic = convertTopic(topic);
  if (!boost::starts_with(internalTopic, TOPIC_PREFIX_OF_MARKET_DATA)) {
    return 0;
  }

  {
    std::lock_guard<std::mutex> guard(mtxTopicMgr);

    //! 先检查当前subscriber有没有订阅过topic
    const auto iter = subscriber2TopicGroup_.find(subscriber);

    if (iter == std::end(subscriber2TopicGroup_)) {
      //! 如果当前subscriber从来没有订阅过，那么给当前subscriber创建订阅列表并
      //! 增加该topic
      subscriber2TopicGroup_[subscriber].emplace(internalTopic);
      return 0;

    } else {
      //! 获取当前subscriber的订阅列表
      auto& topicGroup = iter->second;
      if (topicGroup.find(internalTopic) == std::end(topicGroup)) {
        //! 如果当前subscriber没有订阅该topic，那么给当前subscriber的订阅列表增
        //! 加该topic
        topicGroup.emplace(internalTopic);
        return 0;

      } else {
        //! 当前subscriber已经订阅过当前topic
        return SCODE_BQPUB_TOPIC_ALREADY_SUB;
      }
    }
  }

  return 0;
}

//! 此方法仅用于维护subscriber2TopicGroup_
int TopicMgr::remove(const std::string& subscriber, const std::string& topic) {
  const auto internalTopic = convertTopic(topic);
  if (!boost::starts_with(internalTopic, TOPIC_PREFIX_OF_MARKET_DATA)) {
    return 0;
  }

  {
    std::lock_guard<std::mutex> guard(mtxTopicMgr);

    //! 先检查当前subscriber有没有订阅过topic
    const auto iter = subscriber2TopicGroup_.find(subscriber);

    if (iter == std::end(subscriber2TopicGroup_)) {
      //! 当前subscriber没有订阅过任何topic
      return SCODE_BQPUB_TOPIC_NOT_SUB;

    } else {
      auto& topicGroup = iter->second;
      const auto iterOfTopicGroup = topicGroup.find(internalTopic);
      if (iterOfTopicGroup != std::end(topicGroup)) {
        //! 从当前subscriber的订阅列表中移除该topic
        topicGroup.erase(iterOfTopicGroup);

        //! 如果当前subscriber的订阅者列表为空，那么移除当前subscriber
        if (topicGroup.empty()) {
          subscriber2TopicGroup_.erase(iter);
        }

        return 0;

      } else {
        //! 当前subscriber没有订阅过该topic
        return SCODE_BQPUB_TOPIC_NOT_SUB;
      }
    }
  }
}

void TopicMgr::handleSubInfoForCli() {
  const auto subscriber2TopicGroup = getSubscriber2TopicGroup();
  const auto subscriber2TopicGroupInJsonFmt = toJson(subscriber2TopicGroup);
  if (cbHandleSubInfo_) {
    LOG_T("Sync {}", subscriber2TopicGroupInJsonFmt);
    cbHandleSubInfo_(subscriber2TopicGroupInJsonFmt);
  }
}

void TopicMgr::updateForMid(const std::string& subscriber2TopicGroupInJsonFmt) {
  //! 客户端过来的json格式的全量订阅信息
  const auto subscriber2TopicGroup = fromJson(subscriber2TopicGroupInJsonFmt);
  {
    std::lock_guard<std::mutex> guard(mtxTopicMgr);
    for (const auto& subscriber2Topic : subscriber2TopicGroup) {
      const auto subscriber = subscriber2Topic.first;
      //! 更新当前subscriber的topicGroup
      subscriber2TopicGroup_[subscriber] = subscriber2Topic.second;
      //! 更新当前subscriber的activeTime
      const auto now = GetTotalSecSince1970();
      subscriber2ActiveTimeGroup_[subscriber] = now;
    }
  }
}

void TopicMgr::handleSubInfoForMid() {
  //! 移除长时间没有同步的subscriber及其topicGroup，并返回subscriber2TopicGroup_
  Subscriber2TopicGroup subscriber2TopicGroup;
  {
    std::lock_guard<std::mutex> guard(mtxTopicMgr);
    subscriber2TopicGroup = removeExpiredSubscriber();
  }
  const auto subscriber2TopicGroupInJsonFmt = toJson(subscriber2TopicGroup);
  if (cbHandleSubInfo_) {
    LOG_I("Sync {}", subscriber2TopicGroupInJsonFmt);
    cbHandleSubInfo_(subscriber2TopicGroupInJsonFmt);
  }
}

void TopicMgr::updateForSrv(const std::string& subscriber2TopicGroupInJsonFmt) {
  updateForMid(subscriber2TopicGroupInJsonFmt);
}

void TopicMgr::handleSubInfoForSrv() {
  TopicGroup topicGroupNeedSub;
  TopicGroup topicGroupNeedUnSub;
  {
    std::lock_guard<std::mutex> guard(mtxTopicMgr);

    //! 移除长时间没有同步的subscriber及其topicGroup
    removeExpiredSubscriber();

    //! 根据最新的subscriber2TopicGroup_生成newTopic2SubscriberGroup
    Topic2SubscriberGroup newTopic2SubscriberGroup;
    for (const auto& subscriber2Topic : subscriber2TopicGroup_) {
      const auto& subscriber = subscriber2Topic.first;
      const auto& topicGroup = subscriber2Topic.second;
      for (const auto& topic : topicGroup) {
        newTopic2SubscriberGroup[topic].emplace(subscriber);
      }
    }

    //! 将当前的newTopic2SubscriberGroup和oldTopic2SubscriberGroup_比较，获取要订
    //! 阅和取消订阅的topic
    for (const auto& topic2Subscriber : newTopic2SubscriberGroup) {
      const auto& topic = topic2Subscriber.first;
      //! 新的topic列表中有但是旧的topic列表中没有，需要订阅
      if (oldTopic2SubscriberGroup_.find(topic) ==
          std::end(oldTopic2SubscriberGroup_)) {
        topicGroupNeedSub.emplace(topic);
      }
    }

    for (const auto& topic2Subscriber : oldTopic2SubscriberGroup_) {
      const auto& topic = topic2Subscriber.first;
      //! 旧的topic列表中有但是新的topic列表中没有，需要订阅
      if (newTopic2SubscriberGroup.find(topic) ==
          std::end(newTopic2SubscriberGroup)) {
        topicGroupNeedUnSub.emplace(topic);
      }
    }

    //! 更新oldTopic2SubscriberGroup_
    oldTopic2SubscriberGroup_ = newTopic2SubscriberGroup;
  }

  if (!topicGroupNeedSub.empty()) {
    LOG_I("Topic need sub: {}", boost::join(topicGroupNeedSub, ","));
  }

  if (!topicGroupNeedUnSub.empty()) {
    LOG_I("Topic need unsub: {}", boost::join(topicGroupNeedUnSub, ","));
  }

  if (cbHandleSubInfo_) {
    cbHandleSubInfo_(std::make_tuple(topicGroupNeedSub, topicGroupNeedUnSub));
  }
}

Subscriber2TopicGroup TopicMgr::removeExpiredSubscriber() {
  const std::uint32_t expiredTime = 10;
  for (const auto& subscriber2ActiveTime : subscriber2ActiveTimeGroup_) {
    const auto& subscriber = subscriber2ActiveTime.first;
    const auto now = GetTotalSecSince1970();
    const auto activeTime = subscriber2ActiveTime.second;
    if (now - activeTime > expiredTime) {
      subscriber2TopicGroup_.erase(subscriber);
    }
  }
  return subscriber2TopicGroup_;
}

void TopicMgr::handleSubInfo() {
  switch (role_) {
    case TopicMgrRole::Cli:
      LOG_T("Handle sub info for cli.");
      handleSubInfoForCli();
      break;
    case TopicMgrRole::Mid:
      LOG_T("Handle sub info for mid.");
      handleSubInfoForMid();
      break;
    case TopicMgrRole::Srv:
      LOG_T("Handle sub info for srv.");
      handleSubInfoForSrv();
      break;
  }
}

std::string TopicMgr::toJson(
    const Subscriber2TopicGroup& subscriber2TopicGroup) {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);

  writer.StartObject();
  writer.Key("subscriber2TopicGroup");
  writer.StartArray();

  for (const auto& subscriber2Topic : subscriber2TopicGroup) {
    const auto& subscriber = subscriber2Topic.first;
    const auto& topicGroup = subscriber2Topic.second;

    writer.StartObject();

    writer.Key("subscriber");
    writer.String(subscriber.c_str());

    writer.Key("topicGroup");
    writer.StartArray();
    for (const auto& topic : topicGroup) {
      writer.String(topic.c_str());
    }
    writer.EndArray();

    writer.EndObject();
  }

  writer.EndArray();
  writer.EndObject();

  const auto ret = strBuf.GetString();
  return ret;
}

Subscriber2TopicGroup TopicMgr::fromJson(
    const std::string& subscriber2TopicGroupInJsonFmt) {
  Doc doc;
  doc.Parse(subscriber2TopicGroupInJsonFmt.c_str());

  Subscriber2TopicGroup ret;
  for (std::size_t i = 0; i < doc["subscriber2TopicGroup"].Size(); ++i) {
    const auto& d = doc["subscriber2TopicGroup"][i];
    const auto subscriber = d["subscriber"].GetString();
    for (std::size_t j = 0; j < d["topicGroup"].Size(); ++j) {
      const auto topic = d["topicGroup"][j].GetString();
      ret[subscriber].emplace(topic);
    }
  }
  return ret;
}

}  // namespace bq
