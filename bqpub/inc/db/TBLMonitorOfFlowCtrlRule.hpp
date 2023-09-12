/*!
 * \file TBLMonitorOfFlowCtrlRule.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "db/TBLFlowCtrlRule.hpp"
#include "db/TBLMonitor.hpp"
#include "def/BQDef.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"

namespace bq::db {

using RecFlowCtrlRuleGroup =
    std::map<std::string, bq::db::flowCtrlRule::RecordSPtr>;

class TBLMonitorOfFlowCtrlRule : public TBLMonitor<TBLFlowCtrlRule> {
 public:
  using TBLMonitor<TBLFlowCtrlRule>::TBLMonitor;

 public:
  const RecFlowCtrlRuleGroup& getFlowCtrlRuleGroup() const {
    return flowCtrlRuleGroup_;
  }

  RecFlowCtrlRuleGroup getFlowCtrlRuleGroup(const std::string& step) const {
    RecFlowCtrlRuleGroup ret;
    for (const auto& rec : flowCtrlRuleGroup_) {
      const auto& flowCtrlRule = rec.second;
      if (flowCtrlRule->step == step) {
        ret.emplace(rec);
      }
    }
    return ret;
  }

 private:
  void initNecessaryDataStructures(
      const TBLRecSetSPtr<TBLFlowCtrlRule>& tblRecOfAll) final {
    {
      for (const auto& tblRec : *tblRecOfAll) {
        const auto rec = tblRec.second->getRecWithAllFields();
        const auto ret = flowCtrlRuleGroup_.emplace(rec->getKey(), rec);
        if (ret.second == false) {
          LOG_W(
              "Add rec to flow ctrl rule group failed, "
              "maybe key duplicated. key = {}",
              rec->getKey());
        }
      }
    }
  }

 private:
  RecFlowCtrlRuleGroup flowCtrlRuleGroup_;
};

}  // namespace bq::db
