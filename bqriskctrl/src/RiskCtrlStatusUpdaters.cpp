/*!
 * \file RiskCtrlStatusUpdaters.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/04
 *
 * \brief
 */

#include "RiskCtrlStatusUpdaters.hpp"

#include "util/Logger.hpp"

namespace bq {

void RiskCtrlStatusUpdaters::stash(const RiskCtrlStatusUpdater& updater) {
  riskCtrlStatusUpdaterBundle_.emplace_back(updater);
}

void RiskCtrlStatusUpdaters::batchExecute() {
  for (const auto& updater : riskCtrlStatusUpdaterBundle_) {
    updater();
  }
  riskCtrlStatusUpdaterBundle_.clear();
}

void RiskCtrlStatusUpdaters::batchRollback() {
  riskCtrlStatusUpdaterBundle_.clear();
}

}  // namespace bq
