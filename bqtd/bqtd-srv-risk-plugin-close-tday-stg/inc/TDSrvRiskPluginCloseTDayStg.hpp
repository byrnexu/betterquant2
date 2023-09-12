/*!
 * \file TDSrvRiskPluginCloseTDayStg.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "TDSrvRiskPlugin.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class OpenedContractGroup;
using OpenedContractGroupSPtr = std::shared_ptr<OpenedContractGroup>;
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPluginCloseTDayStg
    : public TDSrvRiskPlugin {
 public:
  TDSrvRiskPluginCloseTDayStg(const TDSrvRiskPluginCloseTDayStg&) = delete;
  TDSrvRiskPluginCloseTDayStg& operator=(const TDSrvRiskPluginCloseTDayStg&) =
      delete;
  TDSrvRiskPluginCloseTDayStg(const TDSrvRiskPluginCloseTDayStg&&) = delete;
  TDSrvRiskPluginCloseTDayStg& operator=(const TDSrvRiskPluginCloseTDayStg&&) =
      delete;

  explicit TDSrvRiskPluginCloseTDayStg(TDSrv* tdSrv);

 private:
  boost::dll::fs::path getLocation() const final;

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
  void initOpenedContractGroup(std::uint32_t combNo, std::uint32_t threadNo);

 private:
  OpenedContractGroupSPtr openedContractGroup_{nullptr};
};

}  // namespace bq::td::srv

inline bq::td::srv::TDSrvRiskPluginCloseTDayStg* Create(
    bq::td::srv::TDSrv* tdSrv) {
  return new bq::td::srv::TDSrvRiskPluginCloseTDayStg(tdSrv);
}

BOOST_DLL_ALIAS_SECTIONED(Create, CreatePlugin, PlugIn)
