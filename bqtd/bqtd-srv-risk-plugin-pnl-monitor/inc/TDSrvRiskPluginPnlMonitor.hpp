/*!
 * \file TDSrvRiskPluginPnlMonitor.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "TDSrvRiskPlugin.hpp"
#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPluginPnlMonitor : public TDSrvRiskPlugin {
 public:
  TDSrvRiskPluginPnlMonitor(const TDSrvRiskPluginPnlMonitor&) = delete;
  TDSrvRiskPluginPnlMonitor& operator=(const TDSrvRiskPluginPnlMonitor&) =
      delete;
  TDSrvRiskPluginPnlMonitor(const TDSrvRiskPluginPnlMonitor&&) = delete;
  TDSrvRiskPluginPnlMonitor& operator=(const TDSrvRiskPluginPnlMonitor&&) =
      delete;

  explicit TDSrvRiskPluginPnlMonitor(TDSrv* tdSrv);

 private:
  int doLoad() final;

 private:
  boost::dll::fs::path getLocation() const final;

 private:
  int doOnRiskCtrlConfChg(const std::string& step, const Doc& doc,
                          const std::string& conf, std::uint32_t combNo,
                          std::uint32_t threadNo) final;

 private:
  std::tuple<int, std::string> doOnOrder(const OrderInfoSPtr& order,
                                         std::uint32_t combNo,
                                         std::uint32_t threadNo) final;

  std::tuple<int, std::string> doOnCancelOrder(const OrderInfoSPtr& order,
                                               std::uint32_t combNo,
                                               std::uint32_t threadNo) final;

 private:
  std::tuple<int, std::string> doOnOrderRet(const OrderInfoSPtr& order,
                                            std::uint32_t combNo,
                                            std::uint32_t threadNo) final;

  std::tuple<int, std::string> doOnCancelOrderRet(const OrderInfoSPtr& order,
                                                  std::uint32_t combNo,
                                                  std::uint32_t threadNo) final;

 private:
  std::uint32_t secDelayOfPrice_;
};

}  // namespace bq::td::srv

inline bq::td::srv::TDSrvRiskPluginPnlMonitor* Create(
    bq::td::srv::TDSrv* tdSrv) {
  return new bq::td::srv::TDSrvRiskPluginPnlMonitor(tdSrv);
}

BOOST_DLL_ALIAS_SECTIONED(Create, CreatePlugin, PlugIn)
