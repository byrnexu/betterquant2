/*!
 * \file WebSrv.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/20
 *
 * \brief
 */

#pragma once

#include <drogon/WebSocketController.h>

#include "SHMIPCDef.hpp"
#include "db/DBEngDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/PnlIF.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/SvcBase.hpp"

using namespace drogon;

namespace bq::tdeng {
class TDEngConnpool;
using TDEngConnpoolSPtr = std::shared_ptr<TDEngConnpool>;
}  // namespace bq::tdeng

namespace bq {

class StgMgr;
using StgMgrSPtr = std::shared_ptr<StgMgr>;

class TopicHandler;
using TopicHandlerSPtr = std::shared_ptr<TopicHandler>;

class SubMgr;
using SubMgrSPtr = std::shared_ptr<SubMgr>;

class PosSnapshot;
using PosSnapshotSPtr = std::shared_ptr<PosSnapshot>;

class TopicMgr;
using TopicMgrSPtr = std::shared_ptr<TopicMgr>;

class ReqBody2CallbackGroup;
using ReqBody2CallbackGroupSPtr = std::shared_ptr<ReqBody2CallbackGroup>;

class UserId2WSConnGroup;
using UserId2WSConnGroupSPtr = std::shared_ptr<UserId2WSConnGroup>;

class StgEngTaskHandler;
using StgEngTaskHandlerSPtr = std::shared_ptr<StgEngTaskHandler>;

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

class SessionTable;
using SessionTableSPtr = std::shared_ptr<SessionTable>;

class Scheduler;
using SchedulerSPtr = std::shared_ptr<Scheduler>;

struct ScheduleTask;
using ScheduleTaskSPtr = std::shared_ptr<ScheduleTask>;
using ScheduleTaskBundle = std::vector<ScheduleTaskSPtr>;
using ScheduleTaskBundleSPtr = std::shared_ptr<ScheduleTaskBundle>;

class CacheOfDBRet;
using CacheOfDBRetSPtr = std::shared_ptr<CacheOfDBRet>;

class WebSrv : public SvcBase, public boost::serialization::singleton<WebSrv> {
 public:
  using SvcBase::SvcBase;

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  int initDBEng();
  int initTDEng();
  void initSubMgr();
  void initTopicMgr();
  int initStgEngTaskDispatcher();
  void initSHMSrv();
  void initScheduleTaskBundle();

 public:
  int doRun() final;

 private:
  void startDrogon();

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 private:
  void stopDrogon();

 public:
  int sub(UserId userId, const std::string& topic);
  int unSub(UserId userId, const std::string& topic);
  void unSubAllTopic(UserId userId);

 public:
  db::DBEngSPtr& getDBEng() { return dbEng_; }
  tdeng::TDEngConnpoolSPtr& getTDEngConnpool() { return tdEngConnpool_; }

  StgMgrSPtr& getStgMgr() { return stgMgr_; }
  SubMgrSPtr& getSubMgr() { return subMgr_; }

  void setPosSnapshotOfAll(const PosSnapshotSPtr& value);
  PosSnapshotSPtr getPosSnapshotOfAll() const;

  std::tuple<int, Key2PnlGroupSPtr> queryPnlGroupBy(
      const std::string& groupCond);

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block>&
  getStgEngTaskDispatcher() {
    return stgEngTaskDispatcher_;
  }

  SHMSrvSPtr& getSHMSrvOfStgEng() { return shmSrvOfStgEng_; }
  SessionTableSPtr& getSessionTable() { return sessionTable_; };

  ReqBody2CallbackGroupSPtr& getReqBody2CallbackGroup() {
    return reqBody2CallbackGroup_;
  }

  UserId2WSConnGroupSPtr& getUserId2WSConnGroup() {
    return userId2WSConnGroup_;
  }

  CacheOfDBRetSPtr& getCacheOfDBRet() { return cacheOfDBRet_; }

 private:
  db::DBEngSPtr dbEng_{nullptr};
  tdeng::TDEngConnpoolSPtr tdEngConnpool_{nullptr};

  StgMgrSPtr stgMgr_{nullptr};

  TopicHandlerSPtr topicHandler_{nullptr};
  TopicMgrSPtr topicMgr_{nullptr};
  SubMgrSPtr subMgr_{nullptr};

  PosSnapshotSPtr posSnapshot_{nullptr};
  mutable std::ext::spin_mutex mtxPosSnapshot_;

  ReqBody2CallbackGroupSPtr reqBody2CallbackGroup_{nullptr};
  UserId2WSConnGroupSPtr userId2WSConnGroup_{nullptr};
  CacheOfDBRetSPtr cacheOfDBRet_{nullptr};

  StgEngTaskHandlerSPtr stgEngTaskHandler_{nullptr};

  TaskDispatcherSPtr<SHMIPCTaskSPtr, BlockType::Block> stgEngTaskDispatcher_{
      nullptr};
  SHMSrvSPtr shmSrvOfStgEng_{nullptr};

  std::shared_ptr<std::thread> threadDrogon_;
  SessionTableSPtr sessionTable_;

  ScheduleTaskBundleSPtr scheduleTaskBundle_{nullptr};
  SchedulerSPtr scheduleTaskBundleExecutor_{nullptr};
};

}  // namespace bq
