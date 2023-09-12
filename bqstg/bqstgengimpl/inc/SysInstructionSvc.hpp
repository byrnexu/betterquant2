/*!
 * \file SysInstructionSvc.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/06
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"

namespace bq {

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

}  // namespace bq

namespace bq::stg {

class StgEngImpl;

class SysInstructionSvc {
 public:
  SysInstructionSvc(const SysInstructionSvc&) = delete;
  SysInstructionSvc& operator=(const SysInstructionSvc&) = delete;
  SysInstructionSvc(const SysInstructionSvc&&) = delete;
  SysInstructionSvc& operator=(const SysInstructionSvc&&) = delete;

  explicit SysInstructionSvc(StgEngImpl* stgEng);

 public:
  bool handle(const StgInstInfoSPtr& stgInstInfo,
              const std::string& instruction);

 private:
  void handleInstrOfCancelAllOrders(const StgInstInfoSPtr& stgInstInfo,
                                    const std::string& instruction,
                                    const Doc& doc);

 private:
  StgEngImpl* stgEng_{nullptr};
};

}  // namespace bq::stg
