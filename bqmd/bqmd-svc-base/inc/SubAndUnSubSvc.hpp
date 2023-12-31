/*!
 * \file SubAndUnSubSvc.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQMDDef.hpp"
#include "util/Pch.hpp"
#include "def/ConstIF.hpp"

namespace bq {

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

template <typename Task>
struct AsyncTask;
template <typename Task>
using AsyncTaskSPtr = std::shared_ptr<AsyncTask<Task>>;
}  // namespace bq

namespace bq::md::svc {

struct TopicGroupNeedMaint;
using TopicGroupNeedMaintSPtr = std::shared_ptr<TopicGroupNeedMaint>;

using SubAndUnSubAsyncTask = AsyncTask<TopicGroupNeedMaintSPtr>;
using SubAndUnSubAsyncTaskSPtr = std::shared_ptr<SubAndUnSubAsyncTask>;

using WSReqGroup = std::vector<std::string>;

class MDSvc;

class SubAndUnSubSvc {
 public:
  SubAndUnSubSvc(MDSvc const* mdSvc);

 public:
  int init();
  void start();
  void stop();

 public:
  TaskDispatcherSPtr<TopicGroupNeedMaintSPtr, BlockType::Block> getTaskDispatcher() const {
    return taskDispatcher_;
  }

 private:
  void handleAsyncTask(SubAndUnSubAsyncTaskSPtr& asyncTask);
  void handleTaskOfSubAndUnSub(SubAndUnSubAsyncTaskSPtr& asyncTask);
  void doSubOrUnSub(const WSReqGroup& reqGroup, TopicOP topicOP);
  void updateTopicGroupForSubOrUnSubAgain(const std::string& req,
                                          TopicOP topicOP);

  void handleTaskOfUpdateActiveTime(SubAndUnSubAsyncTaskSPtr& asyncTask);

 private:
  virtual std::tuple<WSReqGroup, WSReqGroup> convertTopicToWSReq(
      TopicGroupNeedMaintSPtr& topicGroupNeedMaint) = 0;

 protected:
  MDSvc const* mdSvc_{nullptr};
  TaskDispatcherSPtr<TopicGroupNeedMaintSPtr, BlockType::Block> taskDispatcher_{nullptr};
};

}  // namespace bq::md::svc
