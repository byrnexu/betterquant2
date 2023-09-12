/*!
 * \file AlgoSmartOrder.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 */
#pragma once

#include "AlgoOrder.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace msm = boost::msm;
namespace mpl = boost::mpl;

namespace bq {
struct TrdSymParams;
using TrdSymParamsSPtr = std::shared_ptr<TrdSymParams>;

struct Bid1Ask1;
using Bid1Ask1SPtr = std::shared_ptr<Bid1Ask1>;
struct LastPrice;
using LastPriceSPtr = std::shared_ptr<LastPrice>;

struct OrderInfo;
}  // namespace bq

namespace bq::algo {

class AlgoMgr;

struct SmartOrderParams;
using SmartOrderParamsSPtr = std::shared_ptr<SmartOrderParams>;

struct OrderStateMachineImpl;
using OrderStateMachine = msm::back::state_machine<OrderStateMachineImpl>;
using OrderStateMachineSPtr = std::shared_ptr<OrderStateMachine>;
using OrderStateMachineGroup = std::deque<OrderStateMachineSPtr>;

class SmartOrder : public AlgoOrder {
 public:
  SmartOrder(const SmartOrder&) = delete;
  SmartOrder& operator=(const SmartOrder&) = delete;
  SmartOrder(const SmartOrder&&) = delete;
  SmartOrder& operator=(const SmartOrder&&) = delete;

  explicit SmartOrder(AlgoMgr* algoMgr, const StgInstInfoSPtr& stgInstInfo,
                      const std::string& algoType, const std::string& algoName,
                      std::uint32_t lifetime);

 private:
  int afterInit(const std::string& algoParamsInJsonFmt) final;
  int initAlgoParams(const std::string& algoParamsInJsonFmt);
  int initTrdSymParams();

 private:
  std::tuple<int, TopicGroup> doGetTopicGroupNeedSub() final;

 private:
  bool algoOrderIsFinished() final;

 private:
  void doOnTimer() final;
  bool tryToSendOrder();

  std::string notifyProgressOfAlgoOrder() final;

 private:
  void doOnOrderRet(const OrderInfo* orderInfo) final;

 private:
  void doOnCancelOrderRet(const OrderInfo* orderInfo) final;

 private:
  void doOnBid1Ask1(const Bid1Ask1* bid1ask1) final;
  void doOnLastPrice(const LastPrice* lastPrice) final;

 public:
  const SmartOrderParamsSPtr& getSmartOrderParams() {
    return smartOrderParams_;
  }
  const TrdSymParamsSPtr& getTrdSymParams() { return trdSymParams_; }

 private:
  SmartOrderParamsSPtr smartOrderParams_{nullptr};
  TrdSymParamsSPtr trdSymParams_{nullptr};

  //! 行情因为每个订单都要用到，有连续性，不必在每个订单中生成
  Bid1Ask1SPtr bid1ask1_{nullptr};
  LastPriceSPtr lastPrice_{nullptr};

  Decimal priceOfLowerBound_{0};
  Decimal priceOfUpperBound_{0};

  int statusCode_{0};
  OrderId orderId_{0};
  OrderStatus orderStatus_{OrderStatus::Created};
  bool alreadySendOrder_{false};

  Decimal dealSize_{0};
  Decimal avgDealPrice_{0};
};

}  // namespace bq::algo
