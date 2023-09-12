/*!
 * \file TDSrvRiskPluginSelfTradeCtrl.hpp
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

struct PendingOrder {
  // hash of acctId marketCode symbolType symbolCode
  std::uint64_t hashOfSym_{0};
  Decimal maxBidPrice_{std::numeric_limits<Decimal>::min()};
  Decimal minAskPrice_{std::numeric_limits<Decimal>::max()};
  std::uint8_t selfTradeTriggerTimes_{0};
  std::string toStr() const {
    const auto ret = fmt::format(
        "hash: {}; maxBidPrice: {}; minAskPrice: {}; selfTradeTriggerTimes: {}",
        hashOfSym_, maxBidPrice_, minAskPrice_, selfTradeTriggerTimes_);
    return ret;
  }
};
using DelType =
    bip::deleter<PendingOrder, bip::managed_shared_memory::segment_manager>;
using PendingOrderSHMSPtr = bip::shared_ptr<PendingOrder, VoidAlloc, DelType>;

class TDSrv;

class BOOST_SYMBOL_VISIBLE TDSrvRiskPluginSelfTradeCtrl
    : public TDSrvRiskPlugin {
  inline const static char* NAME_OF_PENDING_ORDER_GROUP = "PendingOrderGroup";

  struct TagHashOfSym {};
  struct KeyHashOfSym
      : boost::multi_index::composite_key<
            PendingOrder,
            MIDX_MEMBER(PendingOrder, std::uint64_t, hashOfSym_)> {};
  using MIdxHashOfSym = boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagHashOfSym>, KeyHashOfSym,
      boost::multi_index::composite_key_result_less<KeyHashOfSym::result_type>>;

  using PendingOrderGroup = boost::multi_index::multi_index_container<
      PendingOrderSHMSPtr, boost::multi_index::indexed_by<MIdxHashOfSym>,
      bip::managed_shared_memory::allocator<PendingOrderSHMSPtr>::type>;

 public:
  TDSrvRiskPluginSelfTradeCtrl(const TDSrvRiskPluginSelfTradeCtrl&) = delete;
  TDSrvRiskPluginSelfTradeCtrl& operator=(const TDSrvRiskPluginSelfTradeCtrl&) =
      delete;
  TDSrvRiskPluginSelfTradeCtrl(const TDSrvRiskPluginSelfTradeCtrl&&) = delete;
  TDSrvRiskPluginSelfTradeCtrl& operator=(
      const TDSrvRiskPluginSelfTradeCtrl&&) = delete;

  explicit TDSrvRiskPluginSelfTradeCtrl(TDSrv* tdSrv);

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

  void cachePendingOrders(const OrderInfoSPtr& order, std::uint32_t combNo,
                          std::uint32_t threadNo, const std::string& sym,
                          std::uint64_t hashOfSym);

  std::tuple<int, std::string> handleSymHasPendingOrders(
      const OrderInfoSPtr& order, std::uint32_t combNo, std::uint32_t threadNo,
      const std::string& sym,
      const bip::shared_ptr<PendingOrder, VoidAlloc, DelType>& pendingOrder);

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
  void findOrCtorPendingOrderGroup(std::uint32_t combNo,
                                   std::uint32_t threadNo);

 private:
  PendingOrderGroup* pendingOrderGroup_{nullptr};
  std::uint32_t maxSelfTradeTriggerTimesAllowed_{0};
};

}  // namespace bq::td::srv

inline bq::td::srv::TDSrvRiskPluginSelfTradeCtrl* Create(
    bq::td::srv::TDSrv* tdSrv) {
  return new bq::td::srv::TDSrvRiskPluginSelfTradeCtrl(tdSrv);
}

BOOST_DLL_ALIAS_SECTIONED(Create, CreatePlugin, PlugIn)
