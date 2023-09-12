/*!
 * \file AlgoMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 */
#pragma once

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {
struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

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

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct OrderInfo;
}  // namespace bq

namespace bq::stg {
class StgEngImpl;
}  // namespace bq::stg

namespace bq::algo {

class AlgoOrder;
using AlgoOrderSPtr = std::shared_ptr<AlgoOrder>;

class AlgoMgr {
  using AlgoOrderGroup = std::vector<AlgoOrderSPtr>;
  using TopicHash2AlgoOrderGroup =
      ankerl::unordered_dense::map<std::uint64_t, AlgoOrderGroup>;
  using AlgoId2AlgoOrderGroup =
      ankerl::unordered_dense::map<AlgoId, AlgoOrderSPtr>;

 public:
  AlgoMgr(const AlgoMgr&) = delete;
  AlgoMgr& operator=(const AlgoMgr&) = delete;
  AlgoMgr(const AlgoMgr&&) = delete;
  AlgoMgr& operator=(const AlgoMgr&&) = delete;

  explicit AlgoMgr(stg::StgEngImpl* stgEngImpl);

 public:
  int init();

 private:
  int initTaskDispatacher();

 public:
  void start();
  void stop();

 public:
  std::tuple<int, AlgoId> algoOrder(const StgInstInfoSPtr& stgInstInfo,
                                    const std::string& algoType,
                                    const std::string& algoName,
                                    std::uint32_t lifeTime,
                                    const std::string& algoParamsInJsonFmt);

  int cancelAlgoOrder(
      AlgoId algoId,  //
      TryToCancelAllOrders tryToCancelAllOrders = TryToCancelAllOrders::True);

  std::string getProgressOfAlgoOrder(AlgoId algoId);

 public:
  void handle(SHMIPCTaskSPtr& shmIPCTask);

 private:
  void handleAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void onAsyncTask(SHMIPCAsyncTaskSPtr& asyncTask);
  AlgoOrderGroup getAlgoOrderGroupOfSubTopic(std::uint64_t topicHash);

 private:
  void onTimer();

 private:
  std::tuple<int, AlgoOrderSPtr> releaseAlgoOrderInAlgoMgr(AlgoId algoId);

 public:
  stg::StgEngImpl* getStgEng() { return stgEngImpl_; }

 private:
  stg::StgEngImpl* stgEngImpl_{nullptr};
  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::NoBlock> taskDispatcher_{
      nullptr};
  std::uint32_t usIntervalOfTaskHandler_{1};

  AlgoId2AlgoOrderGroup algoId2AlgoOrderGroup_;
  mutable std::ext::spin_mutex mtxAlgoId2AlgoOrderGroup_;

  TopicHash2AlgoOrderGroup topicHash2AlgoOrderGroup_;
  mutable std::ext::spin_mutex mtxTopicHash2AlgoOrderGroup_;
};

}  // namespace bq::algo
