/*!
 * \file RiskCtrlDataMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

class RiskCtrlDataMgr {
 public:
  RiskCtrlDataMgr(const RiskCtrlDataMgr&) = delete;
  RiskCtrlDataMgr& operator=(const RiskCtrlDataMgr&) = delete;
  RiskCtrlDataMgr(RiskCtrlDataMgr&&) = delete;
  RiskCtrlDataMgr& operator=(RiskCtrlDataMgr&&) = delete;

  explicit RiskCtrlDataMgr(const std::string& step, std::uint32_t threadNo,
                           const std::string& fieldGroupUsedToGenHash) noexcept;

 public:
  std::tuple<int, std::string> update(const Doc& doc);

 private:
  virtual std::tuple<int, std::string> handleTableChgTypeOfAdd(
      const Doc& doc) = 0;
  virtual std::tuple<int, std::string> handleTableChgTypeOfDel(
      const Doc& doc) = 0;
  virtual std::tuple<int, std::string> handleTableChgTypeOfChg(
      const Doc& doc) = 0;

 protected:
  std::string step_;
  std::uint32_t threadNo_;
  std::string fieldGroupUsedToGenHash_;
};

}  // namespace bq
