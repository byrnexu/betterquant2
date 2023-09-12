/*!
 * \file DynCandleSvc.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/14
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

template <typename Task>
struct AsyncTask;
template <typename Task>
using AsyncTaskSPtr = std::shared_ptr<AsyncTask<Task>>;

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;

struct Candle;
using CandleSPtr = std::shared_ptr<Candle>;

}  // namespace bq

namespace bq::stg {

struct SymbolInfo {
  SymbolInfo(MarketCode marketCode, const std::string& symbolCode)
      : marketCode_(marketCode), symbolCode_(symbolCode) {}

  MarketCode marketCode_;
  std::string symbolCode_;

  std::string toStr() const {
    return fmt::format("{}/{}", magic_enum::enum_name(marketCode_),
                       symbolCode_);
  }
};
using SymbolInfoSPtr = std::shared_ptr<SymbolInfo>;

struct SymbolInfoComp {
  bool operator()(const SymbolInfoSPtr& lhs, const SymbolInfoSPtr& rhs) const {
    if (lhs->marketCode_ != rhs->marketCode_) {
      return lhs->marketCode_ < rhs->marketCode_;
    }
    return lhs->symbolCode_ < rhs->symbolCode_;
  }
};

struct DynCandleKey {
  DynCandleKey(std::uint64_t startTs, std::uint32_t interval,
               MarketCode marketCode, const std::string& symbolCode)
      : startTs_(startTs),
        interval_(interval),
        marketCode_(marketCode),
        symbolCode_(symbolCode) {}

  std::uint64_t startTs_{0};
  std::uint32_t interval_{60};
  MarketCode marketCode_;
  std::string symbolCode_;

  std::string toStr() const {
    return fmt::format("{}/{}/{}/{}", magic_enum::enum_name(marketCode_),
                       symbolCode_, startTs_, interval_);
  }
};
using DynCandleKeySPtr = std::shared_ptr<DynCandleKey>;

struct DynCandleKeyComp {
  bool operator()(const DynCandleKeySPtr& lhs,
                  const DynCandleKeySPtr& rhs) const {
    if (lhs->startTs_ != rhs->startTs_) {
      return lhs->startTs_ < rhs->startTs_;
    }
    if (lhs->interval_ != rhs->interval_) {
      return lhs->interval_ < rhs->interval_;
    }
    if (lhs->marketCode_ != rhs->marketCode_) {
      return lhs->marketCode_ < rhs->marketCode_;
    }
    return lhs->symbolCode_ < rhs->symbolCode_;
  }
};

class StgEngImpl;

class DynCandleSvc {
 public:
  DynCandleSvc(StgEngImpl* stgEngImpl);

 public:
  int init();
  void start();
  void stop();

 public:
  int sub(StgInstId subscriber, const std::string& topic);
  int unSub(StgInstId subscriber, const std::string& topic);

 private:
  int resetSymbolGroupUsedToGenCandle(const std::string& topic, bool isSub);
  std::tuple<int, std::string> makeTopicOfLastPrice(const std::string& topic);
  std::tuple<int, DynCandleKeySPtr> makeDynCandleKey(const std::string& topic);
  std::tuple<int, std::optional<int>> resetDynCandleKey2Subscriber(
      StgInstId subscriber, const std::string& topic, bool isSub);
  int initDynCandleKey2Candle(const std::string& topic, bool isSub,
                              int subscriberNum);

 public:
  void handle(const void* shmBuf, std::size_t shmBufLen);

 private:
  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> getTaskDispatcher()
      const {
    return taskDispatcher_;
  }

 private:
  void handleAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask);
  void sendCandleToSubscriber(const DynCandleKeySPtr& dynCandleKey,
                              const CandleSPtr& candle);

 protected:
  StgEngImpl* stgEngImpl_{nullptr};
  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> taskDispatcher_{nullptr};

  //! 用于生成动态k线代码列表
  std::set<SymbolInfoSPtr, SymbolInfoComp> symbolGroupUsedToGenCandle_;
  std::ext::spin_mutex mtxSymbolGroupUsedToGenCandle_;

  //! 动态k线类型和动态k线
  std::map<DynCandleKeySPtr, CandleSPtr> dynCandleKey2Candle_;
  std::ext::spin_mutex mtxDynCandleKey2Candle_;

  //! 动态k线类型和订阅者列表
  std::map<DynCandleKeySPtr, std::vector<StgInstId>, DynCandleKeyComp>
      dynCandleKey2Subscriber_;
  std::ext::spin_mutex mtxDynCandleKey2Subscriber_;
};

}  // namespace bq::stg
