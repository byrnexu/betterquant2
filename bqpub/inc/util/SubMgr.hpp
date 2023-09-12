/*!
 * \file SubMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMIPCDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

using SubscriberGroup = std::vector<ClientChannel>;
using TopicHash2SubscriberGroup =
    absl::node_hash_map<TopicHash, SubscriberGroup>;

using Addr2SHMCliGroup = std::map<std::string, SHMCliSPtr>;
using SHMCliGroup = std::vector<SHMCliSPtr>;

class SubMgr {
 public:
  SubMgr(const SubMgr&) = delete;
  SubMgr& operator=(const SubMgr&) = delete;
  SubMgr(const SubMgr&&) = delete;
  SubMgr& operator=(const SubMgr&&) = delete;

  SubMgr(const std::string& appNameOfSubscriber,
         const DataRecvCallback& dataRecvCallback);
  ~SubMgr();

 public:
  //! 因为WebSrv停止时异常，所以用stop方法手动停止SubMgr
  void start();
  void stop();

 public:
  int sub(ClientChannel subscriber, const std::string& topic);
  int unSub(ClientChannel subscriber, const std::string& topic);

  void unSubAllTopic(ClientChannel subscriber);

 private:
  int startSHMCliIfNotExists(const std::string& topic);

 public:
  SubscriberGroup getSubscriberGroupByTopicHash(TopicHash topicHash) const;
  Addr2SHMCliGroup getSHMCliGroup() const;

 private:
  void initSHMCli(const std::string& addr);

 private:
  std::string appNameOfSubscriber_;
  DataRecvCallback dataRecvCallback_{nullptr};

  TopicHash2SubscriberGroup topicHash2SubscriberGroup_;
  mutable std::ext::spin_mutex mtxTopicHash2SubscriberGroup_;

  Addr2SHMCliGroup addr2SHMCliGroup_;
  mutable std::ext::spin_mutex mtxAddr2SHMCliGroup_;
};

}  // namespace bq
