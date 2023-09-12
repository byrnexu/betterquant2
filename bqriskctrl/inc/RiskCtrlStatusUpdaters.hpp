/*!
 * \file RiskCtrlStatusUpdaters.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/04
 *
 * \brief
 */

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

using RiskCtrlStatusUpdater = std::function<void()>;
using RiskCtrlStatusUpdaterBundle = std::vector<RiskCtrlStatusUpdater>;

class RiskCtrlStatusUpdaters {
 public:
  RiskCtrlStatusUpdaters(const RiskCtrlStatusUpdaters&) = delete;
  RiskCtrlStatusUpdaters& operator=(const RiskCtrlStatusUpdaters&) = delete;
  RiskCtrlStatusUpdaters(const RiskCtrlStatusUpdaters&&) = delete;
  RiskCtrlStatusUpdaters& operator=(const RiskCtrlStatusUpdaters&&) = delete;

  RiskCtrlStatusUpdaters() = default;

 public:
  void stash(const RiskCtrlStatusUpdater& updater);

 public:
  void batchExecute();
  void batchRollback();

 private:
  RiskCtrlStatusUpdaterBundle riskCtrlStatusUpdaterBundle_;
};

}  // namespace bq
