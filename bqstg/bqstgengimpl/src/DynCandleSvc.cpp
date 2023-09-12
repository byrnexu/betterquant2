/*!
 * \file DynCandleSvc.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/14
 *
 * \brief
 */

#include "DynCandleSvc.hpp"

#include "AlgoMgr.hpp"
#include "SHMIPCTask.hpp"
#include "StgEngImpl.hpp"
#include "def/MarketDataIF.hpp"
#include "util/BQUtil.hpp"
#include "util/Literal.hpp"
#include "util/StdExt.hpp"
#include "util/String.hpp"
#include "util/SubMgr.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::stg {

DynCandleSvc::DynCandleSvc(StgEngImpl* stgEngImpl) : stgEngImpl_(stgEngImpl) {}

int DynCandleSvc::init() {
  const auto dynCandleSvcParamInStrFmt = SetParam(
      DEFAULT_TASK_DISPATCHER_PARAM,
      stgEngImpl_->getConfig()["dynCandleSvcParam"].as<std::string>(
          "moduleName=dynCandleSvc; numOfUnprocessedTaskAlert=1000; "
          "taskRandAllocThreadPoolSize=0; taskSpecificThreadPoolSize=1"));
  const auto [ret, dynCandleSvcParam] =
      MakeTaskDispatcherParam(dynCandleSvcParamInStrFmt);
  if (ret != 0) {
    stgEngImpl_->logError("Init failed. {}", {dynCandleSvcParamInStrFmt},
                          stgEngImpl_->getDftStgInstInfo());
    return ret;
  }

  taskDispatcher_ = std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
      dynCandleSvcParam, nullptr,
      [](auto& asyncTask, auto taskSpecificThreadPoolSize) {
        return ThreadNo(0);
      },
      [this](auto& asyncTask) { handleAsyncTask(asyncTask); });

  getTaskDispatcher()->init();

  return ret;
}

void DynCandleSvc::start() { getTaskDispatcher()->start(); }
void DynCandleSvc::stop() { getTaskDispatcher()->stop(); }

void DynCandleSvc::handle(const void* shmBuf, std::size_t shmBufLen) {
  //! 这里收到的是所有的品种的LastPrice，需要过滤
  const auto lastPrice = static_cast<const LastPrice*>(shmBuf);

  //! 保护性代码
  if (lastPrice->lastPrice_ == 0 || lastPrice->lastPrice_ == DBL_MAX) {
    return;
  }

  const auto symbolInfo = std::make_shared<SymbolInfo>(
      lastPrice->mdHeader_.marketCode_, lastPrice->mdHeader_.symbolCode_);
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSymbolGroupUsedToGenCandle_);
    const auto iter = symbolGroupUsedToGenCandle_.find(symbolInfo);
    if (iter == std::end(symbolGroupUsedToGenCandle_)) {
      //! 如果该代码不在订阅列表中，那么退出
      return;
    }
  }

  stgEngImpl_->logTrace("Use last price of {} to gen dyn candle.",
                        {symbolInfo->toStr()},
                        stgEngImpl_->getDftStgInstInfo());

  auto asyncTask = std::make_shared<SHMIPCAsyncTask>(
      std::make_shared<SHMIPCTask>(shmBuf, shmBufLen, CopyIPCData::True),
      std::any());
  getTaskDispatcher()->dispatch(asyncTask);
}

int DynCandleSvc::sub(StgInstId subscriber, const std::string& topic) {
  const auto [statusCode, topicOfLastPrice] = makeTopicOfLastPrice(topic);
  if (statusCode != 0) {
    return statusCode;
  }

  bool isSub = true;

  //! 根据订阅或者取消订阅的代码重新生成需要生成Candle的代码列表
  if (const auto statusCode = resetSymbolGroupUsedToGenCandle(topic, isSub);
      statusCode != 0) {
    return statusCode;
  }

  //! 根据订阅或者取消订阅的代码生成订阅者信息
  const auto [sCodeOfReset, optSubscriberNum] =
      resetDynCandleKey2Subscriber(subscriber, topic, isSub);
  if (sCodeOfReset != 0) {
    return sCodeOfReset;
  }

  //! 根据订阅或者取消的代码生成缓存的Candle
  const auto sCodeOfInit =
      initDynCandleKey2Candle(topic, isSub, optSubscriberNum.value());
  if (sCodeOfInit != 0) {
    return sCodeOfInit;
  }

  const auto ret = stgEngImpl_->getSubMgr()->sub(subscriber, topicOfLastPrice);
  const auto s = fmt::format("{}-{}", stgEngImpl_->getAppName(), subscriber);
  return ret;
}

int DynCandleSvc::unSub(StgInstId subscriber, const std::string& topic) {
  const auto [statusCode, topicOfLastPrice] = makeTopicOfLastPrice(topic);
  if (statusCode != 0) {
    return statusCode;
  }

  bool isSub = false;

  //! 根据订阅或者取消的代码生成需要重新生成Candle的代码列表
  if (const auto statusCode = resetSymbolGroupUsedToGenCandle(topic, isSub);
      statusCode != 0) {
    return statusCode;
  }

  //! 根据订阅或者取消的代码生成订阅者信息
  const auto [sCodeOfReset, optSubscriberNum] =
      resetDynCandleKey2Subscriber(subscriber, topic, isSub);
  if (sCodeOfReset != 0) {
    return sCodeOfReset;
  }

  //! 根据订阅或者取消的代码生成缓存的Candle
  const auto sCodeOfInit =
      initDynCandleKey2Candle(topic, isSub, optSubscriberNum.value());
  if (sCodeOfInit != 0) {
    return sCodeOfInit;
  }

  const auto ret =
      stgEngImpl_->getSubMgr()->unSub(subscriber, topicOfLastPrice);
  const auto s = fmt::format("{}-{}", stgEngImpl_->getAppName(), subscriber);
  return ret;
}

int DynCandleSvc::resetSymbolGroupUsedToGenCandle(const std::string& topic,
                                                  bool isSub) {
  //! 从 topic 获取包含 startTs interval marketCode symbolCode 的 dynCandleKey
  const auto [statusCode, dynCandleKey] = makeDynCandleKey(topic);
  if (statusCode != 0) return statusCode;
  if (isSub) {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSymbolGroupUsedToGenCandle_);
    symbolGroupUsedToGenCandle_.emplace(std::make_shared<SymbolInfo>(
        dynCandleKey->marketCode_, dynCandleKey->symbolCode_));
  } else {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSymbolGroupUsedToGenCandle_);
    symbolGroupUsedToGenCandle_.erase(std::make_shared<SymbolInfo>(
        dynCandleKey->marketCode_, dynCandleKey->symbolCode_));
  }
  return 0;
}

//! 根据DynCandle的topic获取需要订阅的LastPrice的topic
std::tuple<int, std::string> DynCandleSvc::makeTopicOfLastPrice(
    const std::string& topic) {
  //! "shm://MD.SSE.Spot/603123/DynCandle/tsStart/1684029376/interval/3"
  std::vector<std::string> fieldGroup;
  const auto internalTopic = convertTopic(topic);
  boost::algorithm::split(fieldGroup, internalTopic,
                          boost::is_any_of(SEP_OF_TOPIC));
  if (fieldGroup.size() != 9) {
    stgEngImpl_->logWarn("Invalid field number of topic {}.", {topic},
                         stgEngImpl_->getDftStgInstInfo());
    return {SCODE_STG_INVALID_TOPIC, ""};
  }

  auto topicOfLastPrice =
      std::accumulate(fieldGroup.begin(), fieldGroup.begin() + 4, std::string(),
                      [](std::string& s, const std::string& str) {
                        return s.empty() ? str : s + SEP_OF_TOPIC + str;
                      });
  topicOfLastPrice.append(SEP_OF_TOPIC)
      .append(magic_enum::enum_name(MDType::LastPrice));

  return {0, topicOfLastPrice};
}

//! 从 topic 获取包含 startTs interval marketCode symbolCode 的 dynCandleKey
std::tuple<int, DynCandleKeySPtr> DynCandleSvc::makeDynCandleKey(
    const std::string& topic) {
  //! "shm://MD.SSE.Spot/603123/DynCandle/tsStart/1684029376/interval/3"
  std::vector<std::string> fieldGroup;
  const auto internalTopic = convertTopic(topic);
  boost::algorithm::split(fieldGroup, internalTopic,
                          boost::is_any_of(SEP_OF_TOPIC));
  if (fieldGroup.size() != 9) {
    stgEngImpl_->logWarn("Invalid field number of topic {}.", {topic},
                         stgEngImpl_->getDftStgInstInfo());
    return {SCODE_STG_INVALID_TOPIC, nullptr};
  }

  const auto optTsStart = CONV_OPT(std::uint64_t, fieldGroup[6]);
  if (optTsStart == boost::none) {
    stgEngImpl_->logWarn("Invalid field number of topic {}.", {topic},
                         stgEngImpl_->getDftStgInstInfo());
    return {SCODE_STG_INVALID_TOPIC, nullptr};
  }

  const auto optInterval = CONV_OPT(std::uint32_t, fieldGroup[8]);
  if (optInterval == boost::none) {
    stgEngImpl_->logWarn("Invalid field number of topic {}.", {topic},
                         stgEngImpl_->getDftStgInstInfo());
    return {SCODE_STG_INVALID_TOPIC, nullptr};
  }

  const auto optMarketCode = magic_enum::enum_cast<MarketCode>(fieldGroup[1]);
  if (optMarketCode == std::nullopt) {
    stgEngImpl_->logWarn("Invalid field number of topic {}.", {topic},
                         stgEngImpl_->getDftStgInstInfo());
    return {SCODE_STG_INVALID_TOPIC, nullptr};
  }

  const auto symbolCode = fieldGroup[3];

  const auto dynCandleKey =
      std::make_shared<DynCandleKey>(optTsStart.value(), optInterval.value(),
                                     optMarketCode.value(), symbolCode);

  return {0, dynCandleKey};
}

//! topic = "shm://MD.SSE.Spot/603123/DynCandle/tsStart/1684029376/interval/3"
std::tuple<int, std::optional<int>> DynCandleSvc::resetDynCandleKey2Subscriber(
    StgInstId subscriber, const std::string& topic, bool isSub) {
  int subscriberNum = 0;

  auto removeIfSubscriberIsDuplicated = [](auto& dynCandleKey2Subscriber,
                                           const auto& dynCandleKey,
                                           const auto& subscriber) {
    auto& subscribers = dynCandleKey2Subscriber[dynCandleKey];
    subscribers.emplace_back(subscriber);
    std::sort(subscribers.begin(), subscribers.end());
    const auto newEnd = std::unique(subscribers.begin(), subscribers.end());
    subscribers.erase(newEnd, subscribers.end());
  };

  //! 从 topic 获取包含 startTs interval marketCode symbolCode 的 dynCandleKey
  const auto [statusCode, dynCandleKey] = makeDynCandleKey(topic);
  if (statusCode != 0) return {statusCode, std::optional<int>()};

  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxDynCandleKey2Subscriber_);

    if (isSub) {
      //! 如果是订阅，那么新增订阅者
      dynCandleKey2Subscriber_[dynCandleKey].emplace_back(subscriber);
      removeIfSubscriberIsDuplicated(dynCandleKey2Subscriber_, dynCandleKey,
                                     subscriber);

      if (dynCandleKey2Subscriber_.size() % 100 == 0) {
        stgEngImpl_->logWarn("Size of dynCandleKey2Subscriber is {}.",
                             {std::to_string(dynCandleKey2Subscriber_.size())},
                             stgEngImpl_->getDftStgInstInfo());
      }

      if (dynCandleKey2Subscriber_[dynCandleKey].size() % 100 == 0) {
        stgEngImpl_->logWarn("Size of subscriber of {} is {}.",
                             {dynCandleKey->toStr(),
                              std::to_string(dynCandleKey2Subscriber_.size())},
                             stgEngImpl_->getDftStgInstInfo());
      }

    } else {
      //! 如果是取消订阅，那么移除订阅者
      const auto iter = dynCandleKey2Subscriber_.find(dynCandleKey);
      if (iter != std::end(dynCandleKey2Subscriber_)) {
        auto& subscribers = iter->second;
        std::ext::erase_if(subscribers, [subscriber](const auto& elem) {
          return elem == subscriber;
        });
      }
    }

    subscriberNum = dynCandleKey2Subscriber_[dynCandleKey].size();
  }

  return {0, std::make_optional<int>(subscriberNum)};
}

int DynCandleSvc::initDynCandleKey2Candle(const std::string& topic, bool isSub,
                                          int subscriberNum) {
  //! "shm://MD.SSE.Spot/603123/DynCandle/tsStart/1684029376/interval/3"
  const auto [statusCode, dynCandleKey] = makeDynCandleKey(topic);
  if (statusCode != 0) return statusCode;

  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxDynCandleKey2Candle_);
    if (isSub) {
      const auto iter = dynCandleKey2Candle_.find(dynCandleKey);
      if (iter != std::end(dynCandleKey2Candle_)) {
        return 0;
      }

      auto candle = std::make_shared<Candle>();
      const auto internalTopic = convertTopic(topic);
      candle->shmHeader_.topicHash_ =
          XXH3_64bits(internalTopic.c_str(), internalTopic.size());
      candle->shmHeader_.msgId_ = MSG_ID_ON_MD_DYN_CANDLE;
      candle->mdHeader_.exchTs_ = 0;
      candle->mdHeader_.localTs_ = 0;
      strncpy(candle->mdHeader_.symbolCode_, dynCandleKey->symbolCode_.c_str(),
              sizeof(candle->mdHeader_.symbolCode_));
      candle->mdHeader_.mdType_ = MDType::DynCandle;
      candle->startTs_ = dynCandleKey->startTs_;
      candle->interval_ = dynCandleKey->interval_;
      candle->startTsOfCandle_ = 0;

      dynCandleKey2Candle_.emplace(dynCandleKey, candle);
      if (dynCandleKey2Candle_.size() % 100 == 0) {
        stgEngImpl_->logWarn("Size of dynCandleKey2Candle is {}.",
                             {std::to_string(dynCandleKey2Candle_.size())},
                             stgEngImpl_->getDftStgInstInfo());
      }
    } else {
      //! 如果没有订阅者了才移除，后面不再更新dynCandleKey对应的Candle
      if (subscriberNum == 0) {
        dynCandleKey2Candle_.erase(dynCandleKey);
      }
    }
  }

  return 0;
}

void DynCandleSvc::handleAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask) {
  //! 获取 lastPrice
  const auto lastPrice = MakeMsgSPtrByTask<LastPrice>(asyncTask->task_);

#ifdef _NDEBUG
  const auto exchTsOfLastPrice = lastPrice->mdHeader_.exchTs_ / 1000000;
#else
  //! DEBUG 模式下，修改时间戳，方便测试
  const auto exchTsOfLastPrice = GetTotalSecSince1970();
#endif

  //! 浅拷贝一份dynCandleKey2Candle
  decltype(dynCandleKey2Candle_) dynCandleKey2CandleShallowCopy;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxDynCandleKey2Candle_);
    for (auto iter = std::begin(dynCandleKey2Candle_);
         iter != std::end(dynCandleKey2Candle_); ++iter) {
      const auto& dynCandleKey = iter->first;
      if (dynCandleKey->symbolCode_ != lastPrice->mdHeader_.symbolCode_ ||
          dynCandleKey->marketCode_ != lastPrice->mdHeader_.marketCode_) {
        continue;
      }
      dynCandleKey2CandleShallowCopy.emplace(*iter);
    }
  }

  //! 遍历 dynCandleKey2Candle 处理其中的 Candle
  for (auto iter = std::begin(dynCandleKey2CandleShallowCopy);
       iter != std::end(dynCandleKey2CandleShallowCopy); ++iter) {
    const auto& dynCandleKey = iter->first;

    auto& candle = iter->second;
    //! 计算lastPrice中exchTs对应的candle起始时间点
    const auto startTsOfCandleCalcByExchTs_ =
        exchTsOfLastPrice / candle->interval_ * candle->interval_;

    if (candle->startTsOfCandle_ != 0) {
      //! 开始处理LastPrice，用其中的数据更新Candle或者生成下一个Candle
      if (startTsOfCandleCalcByExchTs_ == candle->startTsOfCandle_) {
        //! 如果没有切换到下一Candle
        if (DEC::GT(lastPrice->lastPrice_, candle->high_)) {
          candle->high_ = lastPrice->lastPrice_;
        }
        if (DEC::GT(candle->low_, lastPrice->lastPrice_)) {
          candle->low_ = lastPrice->lastPrice_;
        }
        candle->close_ = lastPrice->lastPrice_;
        candle->vol_ += lastPrice->lastSize_;
        candle->amt_ += lastPrice->lastSize_ * lastPrice->lastPrice_;

        stgEngImpl_->logTrace("Get candle. {}", {candle->toStr()},
                              stgEngImpl_->getDftStgInstInfo());

      } else if (startTsOfCandleCalcByExchTs_ > candle->startTsOfCandle_) {
        //! 如果已经切换到下一Candle
        candle->startTsOfCandle_ = startTsOfCandleCalcByExchTs_;
        candle->open_ = lastPrice->lastPrice_;
        candle->high_ = lastPrice->lastPrice_;
        candle->low_ = lastPrice->lastPrice_;
        candle->close_ = lastPrice->lastPrice_;
        candle->vol_ = lastPrice->lastSize_;
        candle->amt_ = lastPrice->lastSize_ * lastPrice->lastPrice_;

        stgEngImpl_->logDebug("Switch to next candle. {}", {candle->toStr()},
                              stgEngImpl_->getDftStgInstInfo());

      } else {
        //! 时间戳变小，乱序行情
        stgEngImpl_->logWarn(
            "Candle ts {} of {} becomes smaller, "
            "the LastPrice quotes received may be out of order.",
            {std::to_string(exchTsOfLastPrice), dynCandleKey->toStr()},
            stgEngImpl_->getDftStgInstInfo());
        continue;
      }

    } else {
      //! candle->startTsOfCandle_为0说明从未用LastPrice初始化Candle
      if (exchTsOfLastPrice >= candle->startTs_) {
        //! 只需要判断当前LastPrice中的时间戳已经过了candle->startTs_，因为收到的
        //! lastPrice_的exchTs不一定能被candle->interval_整除，所以这里不需要判断
        //! lastPrice_的exchTs是否能够被candle->interval_整除
        candle->startTsOfCandle_ = startTsOfCandleCalcByExchTs_;
        candle->open_ = lastPrice->lastPrice_;
        candle->high_ = lastPrice->lastPrice_;
        candle->low_ = lastPrice->lastPrice_;
        candle->close_ = lastPrice->lastPrice_;
        candle->vol_ = lastPrice->lastSize_;
        candle->amt_ = lastPrice->lastSize_ * lastPrice->lastPrice_;
        stgEngImpl_->logInfo("Init candle of {} by exchTs of lastPrice {}.",
                             {dynCandleKey->toStr(), lastPrice->toStr()},
                             stgEngImpl_->getDftStgInstInfo(),
                             NotifyToTerminal::False);
      } else {
        //! 如果当前lastPrice时间戳还没到candle的起始时间点
        const auto timeDurFromFirstCandle =
            candle->startTs_ - exchTsOfLastPrice;
        stgEngImpl_->logDebug(
            "Time duration from the first "
            "candle of {} is {} seconds. [startTs = {}]",
            {dynCandleKey->toStr(), std::to_string(timeDurFromFirstCandle),
             boost::posix_time::to_iso_string(
                 boost::posix_time::from_time_t(candle->startTs_))},
            stgEngImpl_->getDftStgInstInfo());
        continue;
      }
    }

    //! 将dynCandle发送给所有订阅者
    sendCandleToSubscriber(dynCandleKey, candle);
  }
}

void DynCandleSvc::sendCandleToSubscriber(const DynCandleKeySPtr& dynCandleKey,
                                          const CandleSPtr& candle) {
  auto shmIPCTask = std::make_shared<SHMIPCTask>(  //
      candle.get(), sizeof(Candle), CopyIPCData::True);

  //! 发送一份行情给算法交易引擎
  stgEngImpl_->getAlgoMgr()->handle(shmIPCTask);

  //! 获取dynCandleKey的所有订阅者
  std::vector<StgInstId> subscribers;
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxDynCandleKey2Subscriber_);
    auto iter = dynCandleKey2Subscriber_.find(dynCandleKey);
    if (iter == std::end(dynCandleKey2Subscriber_)) {
      return;
    }
    subscribers.assign(std::begin(iter->second), std::end(iter->second));
  }

  //! 将dynCandle发送给所有订阅者
  for (auto subscriber : subscribers) {
    auto asyncTask = std::make_shared<SHMIPCAsyncTask>(shmIPCTask, subscriber);
    stgEngImpl_->getStgInstTaskDispatcher()->dispatch(asyncTask);
  }
}

}  // namespace bq::stg
