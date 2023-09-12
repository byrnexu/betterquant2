/*!
 * \file SysInstructionSvc.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/06
 *
 * \brief
 */

#include "SysInstructionSvc.hpp"

#include "StgEngConst.hpp"
#include "StgEngImpl.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Logger.hpp"
#include "util/LoggerUtil.hpp"

namespace bq::stg {

SysInstructionSvc::SysInstructionSvc(StgEngImpl* stgEng) : stgEng_(stgEng) {}

bool SysInstructionSvc::handle(const StgInstInfoSPtr& stgInstInfo,
                               const std::string& instruction) {
  Doc doc;
  if (doc.Parse(instruction.data()).HasParseError()) {
    stgEng_->logWarn("Recv invalid manual invention instruction. {}",
                     {instruction}, stgInstInfo);
    return true;
  }

  if (doc.HasMember("category")) {
    if (doc.HasMember("instructionName")) {
    } else {
      stgEng_->logWarn("Recv invalid manual invention instruction. {}",
                       {instruction}, stgInstInfo);
      return true;
    }
  } else {
    return false;
  }

  const std::string instructionName = doc["instructionName"].GetString();
  stgEng_->logInfo("Recv sys instruction {}. {}",
                   {instructionName, instruction}, stgInstInfo);

  if (instructionName == InstrCancelAllOrders) {
    handleInstrOfCancelAllOrders(stgInstInfo, instruction, doc);
  }

  return true;
}

void SysInstructionSvc::handleInstrOfCancelAllOrders(
    const StgInstInfoSPtr& stgInstInfo, const std::string& instruction,
    const Doc& doc) {
  stgEng_->cancelAllOrderOfStgInst(stgInstInfo->stgInstId_);
}

}  // namespace bq::stg
