/*!
 * \file MDStorageSvc.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#pragma once

#include "MDSvcOfCNDef.hpp"
#include "def/BQMDDef.hpp"
#include "def/ConstIF.hpp"
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

struct RawMD;
using RawMDSPtr = std::shared_ptr<RawMD>;
using RawMDAsyncTask = AsyncTask<RawMDSPtr>;
using RawMDAsyncTaskSPtr = std::shared_ptr<RawMDAsyncTask>;

struct MDHeader;
}  // namespace bq

namespace bq::md {
struct RawMDAsyncTaskArg;
using RawMDAsyncTaskArgSPtr = std::shared_ptr<RawMDAsyncTaskArg>;
}  // namespace bq::md

namespace bq::md::svc {

class MDSvcOfCN;

using Topic2AsyncTaskGroup =
    std::map<std::string, std::vector<RawMDAsyncTaskSPtr>>;
using Topic2AsyncTaskGroupSPtr = std::shared_ptr<Topic2AsyncTaskGroup>;

class MDStorageSvc {
 public:
  explicit MDStorageSvc(MDSvcOfCN const* mdSvc);
  ~MDStorageSvc() {}

 public:
  int init();

 public:
  void start();
  void stop();

 public:
  void dispatch(RawMDAsyncTaskSPtr& asyncTask);

 private:
  void handle(RawMDAsyncTaskSPtr& asyncTask);

  void handleMDTickers(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDTrades(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDOrders(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDBooks(RawMDAsyncTaskSPtr& asyncTask);

 private:
  void checkAndUpdateTsToAvoidDulplication(RawMDAsyncTaskSPtr& asyncTask,
                                           MDHeader* mdHeader);

  Topic2AsyncTaskGroupSPtr getTopic2AsyncTaskGroupWrittenToTDEng(
      const RawMDAsyncTaskSPtr& asyncTask);

 public:
  void flushMDToTDEng();

 private:
  void flushMDToTDEng(const Topic2AsyncTaskGroupSPtr& topic2AsyncTaskGroup);
  std::string makeSql(const Topic2AsyncTaskGroupSPtr& topic2AsyncTaskGroup);

 private:
  MDSvcOfCN const* mdSvc_{nullptr};

  Topic2AsyncTaskGroupSPtr topic2AsyncTaskGroup_{nullptr};
  mutable std::mutex mtxTopic2AsyncTaskGroup_;

  Topic2LastTsGroupSPtr topic2LastExchTsGroup_{nullptr};
  Topic2LastTsGroupSPtr topic2LastLocalTsGroup_{nullptr};
  mutable std::mutex mtxTopic2LastTsGroup_;

  TaskDispatcherSPtr<RawMDSPtr, BlockType::Block> taskDispatcher_{nullptr};
};

}  // namespace bq::md::svc
