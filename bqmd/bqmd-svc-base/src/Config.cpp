/*!
 * \file Config.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "Config.hpp"

#include "def/BQConst.hpp"
#include "util/Logger.hpp"

namespace bq::md::svc {

//!
//! TopicGroupMustSubMaint::execTopicGroupMustSubMaint定时刷新
//! cacheOfTopicGroupMustSubInAdvance_
//!
std::tuple<int, TopicGroupSPtr, TopicGroupSPtr>
Config::refreshTopicGroupMustSubAndSave() {
  //! 通过通用函数getTopicGroupMustSubAndSaveInConf获取配置信息
  auto [ret, topicGroupMustSubInAdvance, topicGroupMustSave] =
      getTopicGroupMustSubAndSaveInConf("topicGroupMustSubInAdvance");
  if (ret != 0) {
    LOG_W("Get topic group must save to disk failed.");
    return {-1, topicGroupMustSubInAdvance, topicGroupMustSave};
  }

  //! 如果topicGroupMustSubInAdvance没有变化，那么返回
  if (*topicGroupMustSubInAdvance != *topicGroupMustSubInAdvance_) {
    topicGroupMustSubInAdvance_ = topicGroupMustSubInAdvance;
    //! 如果topicGroupMustSubInAdvance有变化，那么更新topicGroupMustSubInAdvance_
    {
      std::lock_guard<std::mutex> guard(mtxCacheOfTopicGroupMustSubInAdvance_);
      //! 更新cacheOfTopicGroupMustSubInAdvance_
      cacheOfTopicGroupMustSubInAdvance_ =
          std::make_shared<TopicGroup>(*topicGroupMustSubInAdvance_);
    }
  }

  //! 如果topicGroupMustSave没有变化，那么返回
  if (*topicGroupMustSave != *topicGroupMustSave_) {
    topicGroupMustSave_ = topicGroupMustSave;
    //! 如果topicGroupMustSave有变化，那么更新topicGroupMustSave_
    {
      std::lock_guard<std::mutex> guard(mtxCacheOfTopicGroupMustSave_);
      //! 更新cacheOfTopicGroupMustSave_
      cacheOfTopicGroupMustSave_ =
          std::make_shared<TopicGroup>(*topicGroupMustSave_);
    }
  }

  return {0, topicGroupMustSubInAdvance, topicGroupMustSave};
}

//!
//! TopicGroupMustSubMaint::execTopicGroupMustSubMaint定时刷新
//! cacheOfTopicGroupInBlackList_
//!
std::tuple<int, TopicGroupSPtr> Config::refreshTopicGroupInBlackList() {
  //! 通过通用函数getTopicGroupInConf获取topicGroupInBlackList
  auto [ret, topicGroupInBlackList] =
      getTopicGroupInConf("topicGroupInBlackList");
  if (ret != 0) {
    LOG_W("Get topic group in black list failed.");
    return {-1, topicGroupInBlackList};
  }

  //! 如果topicGroupMustSubInAdvance没有变化，那么返回
  if (*topicGroupInBlackList == *topicGroupInBlackList_) {
    return {0, topicGroupInBlackList};
  }

  //! 如果topicGroupInBlackList 有变化，那么更新topicGroupInBlackList_
  topicGroupInBlackList_ = topicGroupInBlackList;
  {
    std::lock_guard<std::mutex> guard(mtxCacheOfTopicGroupInBlackList_);
    //! 更新cacheOfTopicGroupMustSubInAdvance_，此变量用于确定topic是否在黑名单里
    cacheOfTopicGroupInBlackList_ =
        std::make_shared<TopicGroup>(*topicGroupInBlackList_);
  }
  return {0, topicGroupInBlackList};
}

bool Config::topicMustSaveToDisk(const std::string& topic) const {
  {
    std::lock_guard<std::mutex> guard(mtxCacheOfTopicGroupMustSave_);
    const auto iter = cacheOfTopicGroupMustSave_->find(topic);
    return iter != std::end(*cacheOfTopicGroupMustSave_);
  }
}

bool Config::topicInBlackList(const std::string& topic) const {
  {
    std::lock_guard<std::mutex> guard(mtxCacheOfTopicGroupInBlackList_);
    const auto iter = cacheOfTopicGroupInBlackList_->find(topic);
    return iter != std::end(*cacheOfTopicGroupInBlackList_);
  }
}

std::tuple<int, TopicGroupSPtr, TopicGroupSPtr>
Config::getTopicGroupMustSubAndSaveInConf(const std::string& nodeName) const {
  auto emptyTopicGroup = std::make_shared<TopicGroup>();
  try {
    //! 从文件中加载配置
    const auto configFilename = node_[nodeName].as<std::string>();
    const auto [ret, config] = InitConfig(configFilename);
    if (ret != 0) {
      LOG_W("Get topic of {} failed.", nodeName);
      return {-1, emptyTopicGroup, emptyTopicGroup};
    }

    //! 获取topic中的marketCode+symbolType前缀
    const auto marketCode = node_["marketCode"].as<std::string>();
    const auto symbolType = node_["symbolType"].as<std::string>();
    const auto prefix =
        fmt::format("{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_TOPIC,
                    marketCode, SEP_OF_TOPIC, symbolType);

    auto topicGroupMustSubInAdvance = std::make_shared<TopicGroup>();
    auto topicGroupMustSave = std::make_shared<TopicGroup>();

    //! 遍历配置中的topic
    const auto& topicGroup = config["topicGroup"];
    for (auto iter = topicGroup.begin(); iter != topicGroup.end(); ++iter) {
      const auto name = (*iter)["name"].as<std::string>();
      const auto topic = fmt::format("{}{}{}", prefix, SEP_OF_TOPIC, name);
      topicGroupMustSubInAdvance->emplace(topic);
      if ((*iter)["saveToDisk"].as<bool>()) {
        topicGroupMustSave->emplace(topic);
      }
    }
    return {0, topicGroupMustSubInAdvance, topicGroupMustSave};

  } catch (const std::exception& e) {
    LOG_W("Get topic of {} failed. [{}]", nodeName, e.what());
    return {-1, emptyTopicGroup, emptyTopicGroup};
  }
}

std::tuple<int, TopicGroupSPtr> Config::getTopicGroupInConf(
    const std::string& nodeName) const {
  try {
    //! 从文件中加载配置
    const auto configFilename = node_[nodeName].as<std::string>();
    const auto [ret, config] = InitConfig(configFilename);
    if (ret != 0) {
      LOG_W("Get topic of {} failed.", nodeName);
      return {-1, std::make_shared<TopicGroup>()};
    }

    //! 获取topic中的marketCode+symbolType前缀
    const auto marketCode = node_["marketCode"].as<std::string>();
    const auto symbolType = node_["symbolType"].as<std::string>();
    const auto prefix =
        fmt::format("{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA, SEP_OF_TOPIC,
                    marketCode, SEP_OF_TOPIC, symbolType);

    auto topicGroupInConf = std::make_shared<TopicGroup>();
    //! 遍历配置中的topic
    const auto& topicGroup = config["topicGroup"];
    for (auto iter = topicGroup.begin(); iter != topicGroup.end(); ++iter) {
      const auto name = iter->as<std::string>();
      if (boost::contains(name, "*")) {
        //! 如果配置中的topic包含*，那么直接放入返回的topic集合
        topicGroupInConf->emplace(name);
      } else {
        //! 如果配置中的topic不包含*，那么加上前缀放入返回的topic集合
        const auto topic = fmt::format("{}{}{}", prefix, SEP_OF_TOPIC, name);
        topicGroupInConf->emplace(topic);
      }
    }
    return {0, topicGroupInConf};

  } catch (const std::exception& e) {
    LOG_W("Get topic of {} failed. [{}]", nodeName, e.what());
    return {-1, std::make_shared<TopicGroup>()};
  }
}

}  // namespace bq::md::svc
