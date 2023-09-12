/*!
 * \file TopicMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/26
 *
 * \brief
 */

#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

using Subscriber2TopicGroup = std::map<std::string, std::set<std::string>>;
using Topic2SubscriberGroup = std::map<std::string, std::set<std::string>>;
using Subscriber2ActiveTimeGroup = std::map<std::string, std::uint64_t>;

class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

using CBHandleSubInfo = std::function<void(const std::any&)>;

enum class TopicMgrRole { Cli, Mid, Srv };

class TopicMgr {
 public:
  TopicMgr(const TopicMgr&) = delete;
  TopicMgr& operator=(const TopicMgr&) = delete;
  TopicMgr(const TopicMgr&&) = delete;
  TopicMgr& operator=(const TopicMgr&&) = delete;

  explicit TopicMgr(TopicMgrRole role,
                    const CBHandleSubInfo& cbHandleSubInfo = nullptr);
  ~TopicMgr();

 public:
  void start();
  void stop();

  //! //////////
  //! 客户端接口
  //! //////////
 public:
  //! step 1. 仅仅用来维护subscriber2TopicGroup_
  int add(const std::string& subscriber, const std::string& topic);

  //! step 1. 仅仅用来维护subscriber2TopicGroup_
  int remove(const std::string& subscriber, const std::string& topic);

  //! step 2. 将Cli的订阅信息同步到Mid
  void handleSubInfoForCli();

  //! //////////
  //! 中间层接口
  //! //////////
 public:
  //! step 1. 更新中间层的subscriber2TopicGroup_
  void updateForMid(const std::string& subscriber2TopicGroupInJsonFmt);

  //! step 2 将Mid的订阅信息同步到Mid或者Srv
  void handleSubInfoForMid();

  //! //////////
  //! 服务端接口
  //! //////////
 public:
  //! step 1. 更新服务端的subscriber2TopicGroup_
  void updateForSrv(const std::string& subscriber2TopicGroupInJsonFmt);

  //! step 2. 更新本地订阅信息并比较获取需要订阅和取消订阅的topic
  void handleSubInfoForSrv();

  //! //////////////////////
  //! 各个层面用到的公共接口
  //! //////////////////////
 public:
  void handleSubInfo();

  //! 移除长时间没有同步的subscriber及其topicGroup
  Subscriber2TopicGroup removeExpiredSubscriber();

  //! 将subscriber2TopicGroup_转换为json用于同步
  std::string toJson(const Subscriber2TopicGroup& subscriber2TopicGroup);

  //! 将json转换为subscriber2TopicGroup_用于更新服务端订阅情况
  Subscriber2TopicGroup fromJson(
      const std::string& subscriber2TopicGroupInJsonFmt);

 public:
  Subscriber2TopicGroup getSubscriber2TopicGroup() const {
    {
      std::lock_guard<std::mutex> guard(mtxTopicMgr);
      return subscriber2TopicGroup_;
    }
  }

 private:
  TopicMgrRole role_;

  CBHandleSubInfo cbHandleSubInfo_{nullptr};
  SchedulerSPtr scheduler_{nullptr};

  Subscriber2TopicGroup subscriber2TopicGroup_;
  Subscriber2ActiveTimeGroup subscriber2ActiveTimeGroup_;
  Topic2SubscriberGroup oldTopic2SubscriberGroup_;
  mutable std::mutex mtxTopicMgr;
};

}  // namespace bq
