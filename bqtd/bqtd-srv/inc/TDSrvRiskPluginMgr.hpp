/*!
 * \file TDSrvRiskPluginMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "Config.hpp"
#include "def/BQConst.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::srv {

using No2LibPath = std::map<std::size_t, boost::dll::fs::path>;

class TDSrv;

class TDSrvRiskPlugin;
using TDSrvRiskPluginSPtr = std::shared_ptr<TDSrvRiskPlugin>;

class SafeTDSrvRiskPlugin {
 public:
  void set(const TDSrvRiskPluginSPtr& value) {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTDSrvRiskPlugin_);
    tdSrvRiskPlugin_ = value;
  }

  TDSrvRiskPluginSPtr get() const {
    std::lock_guard<std::ext::spin_mutex> guard(mtxTDSrvRiskPlugin_);
    return tdSrvRiskPlugin_;
  }

 private:
  TDSrvRiskPluginSPtr tdSrvRiskPlugin_{nullptr};
  mutable std::ext::spin_mutex mtxTDSrvRiskPlugin_;
};

class TDSrvRiskPluginMgr {
 public:
  TDSrvRiskPluginMgr(const TDSrvRiskPluginMgr&) = delete;
  TDSrvRiskPluginMgr& operator=(const TDSrvRiskPluginMgr&) = delete;
  TDSrvRiskPluginMgr(const TDSrvRiskPluginMgr&&) = delete;
  TDSrvRiskPluginMgr& operator=(const TDSrvRiskPluginMgr&&) = delete;

  explicit TDSrvRiskPluginMgr(TDSrv* tdSrv, std::uint32_t no);

 public:
  int load();

 private:
  std::tuple<int, No2LibPath> initNo2LibPath();
  int getNo(const boost::filesystem::path& file);
  void initPlugIn(const No2LibPath& no2LibPath);

 private:
  TDSrvRiskPluginSPtr createPlugin(std::size_t no,
                                   const boost::filesystem::path& libPath);

 public:
  void onThreadStart(std::uint32_t combNo, std::uint32_t threadNo);
  void onThreadExit(std::uint32_t combNo, std::uint32_t threadNo);

 public:
  int onRiskCtrlConfChg(const std::string& conf, std::uint32_t combNo,
                        std::uint32_t threadNo);

  std::tuple<int, std::string> onOrder(const OrderInfoSPtr& order,
                                       std::uint32_t combNo,
                                       std::uint32_t threadNo);

  std::tuple<int, std::string> onCancelOrder(const OrderInfoSPtr& order,
                                             std::uint32_t combNo,
                                             std::uint32_t threadNo);

  std::tuple<int, std::string> onOrderRet(const OrderInfoSPtr& order,
                                          std::uint32_t combNo,
                                          std::uint32_t threadNo);

  std::tuple<int, std::string> onCancelOrderRet(const OrderInfoSPtr& order,
                                                std::uint32_t combNo,
                                                std::uint32_t threadNo);

 private:
  TDSrv* tdSrv_{nullptr};
  std::uint32_t no_{0};
  std::array<SafeTDSrvRiskPlugin, MAX_TD_SRV_RISK_PLUGIN_NUM>
      safeTDSrvRiskPluginGroup_;
};

}  // namespace bq::td::srv
