/*!
 * \file TDSrvRiskPluginTrdSymbolList.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "TDSrvRiskPlugin.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq::td::srv {

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPluginTrdSymbolList
    : public TDSrvRiskPlugin {
 public:
  TDSrvRiskPluginTrdSymbolList(const TDSrvRiskPluginTrdSymbolList&) = delete;
  TDSrvRiskPluginTrdSymbolList& operator=(const TDSrvRiskPluginTrdSymbolList&) =
      delete;
  TDSrvRiskPluginTrdSymbolList(const TDSrvRiskPluginTrdSymbolList&&) = delete;
  TDSrvRiskPluginTrdSymbolList& operator=(
      const TDSrvRiskPluginTrdSymbolList&&) = delete;

  explicit TDSrvRiskPluginTrdSymbolList(TDSrv* tdSrv);

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
};

}  // namespace bq::td::srv

inline bq::td::srv::TDSrvRiskPluginTrdSymbolList* Create(
    bq::td::srv::TDSrv* tdSrv) {
  return new bq::td::srv::TDSrvRiskPluginTrdSymbolList(tdSrv);
}

BOOST_DLL_ALIAS_SECTIONED(Create, CreatePlugin, PlugIn)
