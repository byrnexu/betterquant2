/*!
 * \file SyncTask.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

enum class SyncToRiskMgr { True = 1, False = 2 };

struct SyncTask {
  SyncTask(MsgId msgId, const std::any& task, SyncToRiskMgr syncToRiskMgr,
           SyncToDB syncToDB)
      : msgId_(msgId),
        task_(task),
        syncToRiskMgr_(syncToRiskMgr),
        syncToDB_(syncToDB) {}
  MsgId msgId_;
  std::any task_;
  SyncToRiskMgr syncToRiskMgr_{SyncToRiskMgr::True};
  SyncToDB syncToDB_{SyncToDB::False};
};
using SyncTaskSPtr = std::shared_ptr<SyncTask>;

using SyncTaskGroup = std::vector<SyncTaskSPtr>;
using SyncTaskGroupSPtr = std::shared_ptr<SyncTaskGroup>;

}  // namespace bq
