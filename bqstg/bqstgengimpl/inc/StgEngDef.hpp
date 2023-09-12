/*!
 * \file StgEngDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "SHMHeader.hpp"
#include "SHMIPCMsgId.hpp"
#include "def/BQDefIF.hpp"

namespace bq {

using StgOrdMgr = OrdMgr<MIdxOrderIdOfOM, MIdxMarketCodeExchOrderIdOfOM,
                         MIdxStgInstIdOfOM, MIdxAlgoIdOfOM>;
using StgOrdMgrSPtr = std::shared_ptr<StgOrdMgr>;

using StgPosMgr = PosMgr<MIdxMainOfPM, MIdxStgInstIdOfPM>;
using StgPosMgrSPtr = std::shared_ptr<StgPosMgr>;

}  // namespace bq

namespace bq::stg {

struct StgSignal {
  StgSignal(MsgId msgId) : shmHeader_(msgId) {}
  SHMHeader shmHeader_;
};

struct TaskOfFixedTime;
using TaskOfFixedTimeSPtr = std::shared_ptr<TaskOfFixedTime>;

struct TaskOfFixedTime {
  TaskOfFixedTime() = default;
  TaskOfFixedTime(TaskOfFixedTime&) = default;
  TaskOfFixedTime& operator=(TaskOfFixedTime&) = default;
  TaskOfFixedTime(StgInstId stgInstId, const std::string& timerName,
                  const std::string& execTime, std::uint32_t timeZone)
      : stgInstId_(stgInstId),
        timerName_(timerName),
        execTime_(execTime),
        timeZone_(timeZone) {}

  StgInstId stgInstId_;
  std::string timerName_;
  std::string execTime_;
  std::uint32_t timeZone_;
  bool triggered_{false};
};

}  // namespace bq::stg
