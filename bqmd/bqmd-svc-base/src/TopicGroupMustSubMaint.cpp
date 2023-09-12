/*!
 * \file TopicGroupMustSubMaint.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TopicGroupMustSubMaint.hpp"

#include "Config.hpp"
#include "MDSvc.hpp"
#include "SubAndUnSubSvc.hpp"
#include "def/BQConst.hpp"
#include "def/Const.hpp"
#include "util/BQMDUtil.hpp"
#include "util/Logger.hpp"
#include "util/Scheduler.hpp"
#include "util/StdExt.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/Util.hpp"

namespace bq::md::svc {

int TopicGroupMustSubMaint::start() {
  //! 创建定时器
  const auto milliSecIntervalOfTopicGroupMustSubMaint =
      CONFIG["milliSecIntervalOfTopicGroupMustSubMaint"].as<std::uint32_t>();
  schedulerTopicGroupMustSubMaint_ = std::make_shared<Scheduler>(
      "TOPIC_GROUP_MUST_MAINT", [this]() { execTopicGroupMustSubMaint(); },
      milliSecIntervalOfTopicGroupMustSubMaint);

  //! 先执行定时器过程
  execTopicGroupMustSubMaint();

  //! 启动定时器
  auto ret = schedulerTopicGroupMustSubMaint_->start();
  if (ret != 0) {
    LOG_E("Start topic group must sub maintainment failed.");
    return ret;
  }

  LOG_D("Start topic group must sub maintainment success.");
  return 0;
}

//! 按照现在的逻辑，如果发起一个不存在的品种的订阅，那么默认订阅就是成功的，后台
//! 一直没有收到此品种的行情，那么过一段时间后会重新发起订阅，直到订阅方撤销这个
//! 订阅为止。
//!
int TopicGroupMustSubMaint::execTopicGroupMustSubMaint() {
  LOG_T("Begin to exec topic group must sub maintainment.");

  //! 从配置获取topicGroupMustSubInAdvance并更新缓存中的
  //! cacheOfTopicGroupMustSubInAdvance_
  auto [retOfMustSubInAdvance, topicGroupMustSubInAdvance, topicGroupMustSave] =
      Config::get_mutable_instance().refreshTopicGroupMustSubAndSave();
  if (retOfMustSubInAdvance != 0) {
    LOG_W("Exec topic group must sub maintainment failed.");
    return retOfMustSubInAdvance;
  }

  //! 从配置获取topicGroupInBlackList取并更新缓存中的
  //! cacheOfTopicGroupInBlackList_
  auto [retOfInBlackList, topicGroupInBlackList] =
      Config::get_mutable_instance().refreshTopicGroupInBlackList();
  if (retOfInBlackList != 0) {
    LOG_W("Exec topic group must sub maintainment failed.");
    return retOfInBlackList;
  }

  //! 将订阅者发起的订阅列表放入topicGroupMustSubOfAll
  TopicGroup topicGroupMustSubOfAll = getTopicOfSubscriber();

  //! 将配置中的订阅列表放入topicGroupMustSubOfAll
  topicGroupMustSubOfAll =
      addTopicGroupMustSub(topicGroupMustSubOfAll, *topicGroupMustSubInAdvance);

  //! 将黑名单中的订阅列表从topicGroupMustSubOfAll中移除
  topicGroupMustSubOfAll =
      removeTopicInBlackList(topicGroupMustSubOfAll, *topicGroupInBlackList);

  //! 订单簿里的topic包含深度信息，这样会导致重复订阅，因此这里需要合并仅仅是深
  //! 度不同的订单簿的订阅
  topicGroupMustSubOfAll =
      mergeSameTypeTopicToAvoidDupSub(topicGroupMustSubOfAll);

  //! 将那些长时间没有收到行情的topic移除以便重新发起订阅
  removeTopicThatHaveNotRecvMDForALongTimeForReSub();

  //! 将topicGroupMustSubOfAll和topicGroupAlreadySub_比较获取需要订阅和取消订阅
  //! 的topic
  auto topicGroupNeedMaint = getTopicGroupNeedMaint(topicGroupMustSubOfAll);

  //! 如果存在需要订阅或者取消顶订阅的topic
  if (topicGroupNeedMaint->needSubOrUnSub()) {
    LOG_I("Find topic need to be sub or unsub. {}",
          topicGroupNeedMaint->toStr());
    //!
    //! 下面两行代码如果把顺序倒过来会有问题，代码倒过来的情况下，假设执行了
    //! handle之后此处线程卡住，在SubAndUnSubSvc服务处理完订阅，正确设置
    //! topicGroupAlreadySub_之后，然后执行updateTopicGroupAlreadySub，这样
    //! topicGroupAlreadySub_就被覆盖了。
    //!
    updateTopicGroupAlreadySub(topicGroupMustSubOfAll);
    //! 将订阅主题丢给SubAndUnSubSvc处理
    mdSvc_->getSubAndUnSubSvc()->getTaskDispatcher()->dispatch(
        topicGroupNeedMaint);
  }

  return 0;
}

void TopicGroupMustSubMaint::removeTopicForSubAgain(const std::string& topic) {
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    topicGroupAlreadySub_.erase(topic);
  }
}

void TopicGroupMustSubMaint::addTopicForUnSubAgain(const std::string& topic) {
  const auto now = GetTotalMSSince1970();
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    topicGroupAlreadySub_.emplace(topic, now);
  }
}

void TopicGroupMustSubMaint::addTopicOfSubscriber(const std::string& topic) {
  LOG_D("Add topic of subscriber. {}", topic);
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupOfSubscriber_);
    topicGroupOfSubscriber_.emplace(topic);
  }
}

void TopicGroupMustSubMaint::removeTopicOfSubscriber(const std::string& topic) {
  LOG_D("Remove topic of subscriber. {}", topic);
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupOfSubscriber_);
    topicGroupOfSubscriber_.erase(topic);
  }
}

TopicGroup TopicGroupMustSubMaint::getTopicOfSubscriber() const {
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupOfSubscriber_);
    return topicGroupOfSubscriber_;
  }
}

TopicGroup TopicGroupMustSubMaint::addTopicGroupMustSub(
    const TopicGroup& origTopicGroup, const TopicGroup& topicGroupMustSub) {
  TopicGroup ret;
  std::set_union(std::begin(origTopicGroup), std::end(origTopicGroup),
                 std::begin(topicGroupMustSub), std::end(topicGroupMustSub),
                 std::inserter(ret, std::begin(ret)));
  return ret;
}

TopicGroup TopicGroupMustSubMaint::removeTopicInBlackList(
    const TopicGroup& origTopicGroup, const TopicGroup& topicGroupInBlackList) {
  auto topicExistsInBlackList = [](const auto& topic,
                                   const auto& topicGroupInBlackList) {
    for (auto topicInBlackList : topicGroupInBlackList) {
      boost::erase_all(topicInBlackList, "*");
      if (boost::contains(topic, topicInBlackList)) {
        return true;
      }
    }
    return false;
  };

  TopicGroup ret;
  for (const auto& topic : origTopicGroup) {
    if (topicExistsInBlackList(topic, topicGroupInBlackList)) {
      LOG_T("Find topic {} in blacklist.", topic);
    } else {
      ret.emplace(topic);
    }
  }

  return ret;
}

TopicGroup TopicGroupMustSubMaint::mergeSameTypeTopicToAvoidDupSub(
    const TopicGroup& origTopicGroup) {
  TopicGroup ret;
  for (const auto& topic : origTopicGroup) {
    const auto t = RemoveDepthInTopicOfBooks(topic);
    ret.emplace(t);
  }
  return ret;
}

void TopicGroupMustSubMaint::
    removeTopicThatHaveNotRecvMDForALongTimeForReSub() {
  //! 从配置获取每个topic的重新订阅的超时阈值
  const auto [ret, topic2ThresholdOfReSubGroup] =
      loadTopic2ThresholdOfReSubGroup();
  if (ret != 0) {
    LOG_W(
        "Remove topic that have not recv market data for a long time failed.");
    return;
  }

  //! 获取默认的阈值defaultThreshold
  std::uint32_t defaultThreshold = 60000;
  const auto iter = topic2ThresholdOfReSubGroup.find("default");
  if (iter != std::end(topic2ThresholdOfReSubGroup)) {
    defaultThreshold = iter->second;
  }

  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    //! 遍历已经订阅的topic列表
    for (auto iter = std::begin(topicGroupAlreadySub_);
         iter != std::end(topicGroupAlreadySub_);) {
      const auto topic = iter->first;
      const auto now = GetTotalMSSince1970();
      const auto activeTime = iter->second;
      //! 当前topic已经有timeDurOfNotRecvMD毫秒没有收到行情
      const auto timeDurOfNotRecvMD = now - activeTime;

      //! 获取当前topic的超时重订阈值thresholdOfReSub
      auto thresholdOfReSub = defaultThreshold;
      if (const auto iter = topic2ThresholdOfReSubGroup.find(topic);
          iter != std::end(topic2ThresholdOfReSubGroup)) {
        thresholdOfReSub = iter->second;
      }

      if (timeDurOfNotRecvMD > thresholdOfReSub) {
        iter = topicGroupAlreadySub_.erase(iter);
        LOG_W(
            "Resub {} required because of time dur of not recv MD {}ms greater "
            "than threshold of resub {}ms.",
            topic, timeDurOfNotRecvMD, thresholdOfReSub);
      } else {
        ++iter;
        LOG_T("===== {} {} {}", topic, thresholdOfReSub, timeDurOfNotRecvMD);
      }
    }
  }
}

std::tuple<int, std::map<std::string, std::uint32_t>>
TopicGroupMustSubMaint::loadTopic2ThresholdOfReSubGroup() {
  std::map<std::string, std::uint32_t> topic2ThresholdOfReSubGroup;
  try {
    const auto prefix = fmt::format("{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                                    SEP_OF_TOPIC, mdSvc_->getMarketCode(),
                                    SEP_OF_TOPIC, mdSvc_->getSymbolType());

    const auto configFilename =
        CONFIG["thresholdOfReSubWithoutRecvMD"].as<std::string>();
    const auto [ret, config] = InitConfig(configFilename);
    if (ret != 0) {
      LOG_W("Load threshold of resub group failed.");
      return {ret, std::map<std::string, std::uint32_t>()};
    }

    const auto& thresholdOfReSub = config["thresholdOfReSub"];
    for (const auto& rec : thresholdOfReSub) {
      const auto name = rec["name"].as<std::string>();
      std::string topic = name;
      if (topic != "default") {
        topic = fmt::format("{}{}{}", prefix, SEP_OF_TOPIC, name);
      }
      const auto timeDur = rec["timeDur"].as<std::uint32_t>();
      topic2ThresholdOfReSubGroup.emplace(topic, timeDur);
    }

  } catch (const std::exception& e) {
    LOG_W("Load threshold of resub group failed. [{}]", e.what());
    return {0, std::map<std::string, std::uint32_t>()};
  }

  return {0, topic2ThresholdOfReSubGroup};
}

TopicGroupNeedMaintSPtr TopicGroupMustSubMaint::getTopicGroupNeedMaint(
    const TopicGroup& topicGroupMustSubOfAll) const {
  auto ret = std::make_shared<TopicGroupNeedMaint>();
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    for (const auto& topic : topicGroupMustSubOfAll) {
      if (topicGroupAlreadySub_.find(topic) ==
          std::end(topicGroupAlreadySub_)) {
        ret->topicGroupNeedSub_.emplace(topic);
      }
    }
    for (const auto& rec : topicGroupAlreadySub_) {
      const auto topic = rec.first;
      if (topicGroupMustSubOfAll.find(topic) ==
          std::end(topicGroupMustSubOfAll)) {
        ret->topicGroupNeedUnSub_.emplace(topic);
      }
    }
  }
  return ret;
}

void TopicGroupMustSubMaint::updateTopicGroupAlreadySub(
    const TopicGroup& topicGroupMustSubOfAll) {
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    for (const auto& topic : topicGroupMustSubOfAll) {
      if (topicGroupAlreadySub_.find(topic) ==
          std::end(topicGroupAlreadySub_)) {
        const auto now = GetTotalMSSince1970();
        topicGroupAlreadySub_.emplace(topic, now);
      }
    }
    for (auto iter = std::begin(topicGroupAlreadySub_);
         iter != std::end(topicGroupAlreadySub_);) {
      const auto topic = iter->first;
      if (topicGroupMustSubOfAll.find(topic) ==
          std::end(topicGroupMustSubOfAll)) {
        iter = topicGroupAlreadySub_.erase(iter);
      } else {
        ++iter;
      }
    }
  }
}

void TopicGroupMustSubMaint::clearTopicGroupAlreadySub() {
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    topicGroupAlreadySub_.clear();
  }
}

void TopicGroupMustSubMaint::updateTopicActiveTime(const std::string& topic,
                                                   std::uint64_t value) {
  {
    std::lock_guard<std::mutex> guard(mtxTopicGroupAlreadySub_);
    const auto iter = topicGroupAlreadySub_.find(topic);
    if (iter != std::end(topicGroupAlreadySub_)) {
      iter->second = value;
    } else {
      LOG_I(
          "Can not find topic {} in topic group already sub "
          "when update topic active time.",
          topic);
    }
  }
}

void TopicGroupMustSubMaint::stop() {
  schedulerTopicGroupMustSubMaint_->stop();
  LOG_D("Stop topic group must sub maintainment finished. ");
}

}  // namespace bq::md::svc
