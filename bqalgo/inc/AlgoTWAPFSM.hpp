/*!
 * \file AlgoTWAPFSM.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/17
 *
 * \brief
 */
#pragma once

#include "AlgoConst.hpp"
#include "AlgoDef.hpp"
#include "AlgoTWAP.hpp"
#include "AlgoTWAPDef.hpp"
#include "AlgoUtil.hpp"
#include "TrdSymParams.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"

namespace msm = boost::msm;
namespace msmf = boost::msm::front;
namespace mpl = boost::mpl;

namespace bq::algo {

enum class NextActionOfAlgo { HandleCurOrder, HandleNextOrder, TerminateAlgo };

struct EventOfOrderSuccess {};
struct EventOfOrderFailed {};
struct EventOfCheckIfCancelOrder {};
struct EventOfCancelOrderFailed {};
struct EventOfCancelOrderRetFailed {};
struct EventOfOrderRetFilled {};
struct EventOfOrderRetFailed {};
struct EventOfOrderRetCanceled {};
struct EventOfOrderSizeLTTickerSize {};
struct EventOfCheckIfRestoreOrder {};

/*
 * 触发顺序
 *
 * on_entry fsm
 * on_entry StateOfNewOrder
 * Enter    GuardOfNewOrder
 * on_exit  StateOfNewOrder
 * Enter    ActionOfNewOrder
 * on_entry StateOfOrderInProc
 *
 */
class OrderStateMachineImpl
    : public msm::front::state_machine_def<OrderStateMachineImpl> {
 private:
  //! 新订单
  struct StateOfNewOrder : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event& evt, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfNewOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      fsm.startTime_ = GetTotalMSSince1970();
      fsm.curOrderSize_ = fsm.orderSize_;
      fsm.curUrgency_ = fsm.twap_->getTWAPParams()->initialUrgency_;
      fsm.curTickerOffset_ = fsm.twap_->getTWAPParams()->tickerOffset_;
      fsm.order(fsm);
    }

    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfNewOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 正在下单
  struct StateOfOrderInProc : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfOrderInProc. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }

    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfOrderInProc. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  struct GuardOfCancelOrder {
    template <class Event, class Fsm, class SourceState, class TargetState>
    bool operator()(const Event&, Fsm& fsm, SourceState&, TargetState&) const {
      std::uint64_t checkTimepoint = 0;
      if (fsm.lastCancelOrderTime_ == 0) {
        //! 说明是首次撤单，下次撤单间隔从fsm.lastOrderTime_开始计算
        checkTimepoint = fsm.lastOrderTime_;
      } else {
        //! 说明是重复撤单(撤单出错会有这个情况)，下次撤单间隔从fsm.lastCancelOrderTime_开始计算
        checkTimepoint = fsm.lastCancelOrderTime_;
      }

      const auto now = GetTotalMSSince1970();
      const auto timeDur = now - checkTimepoint;
      if (timeDur <=
          fsm.twap_->getTWAPParams()->minMSIntervalOfOrderAndCancelOrder_) {
        //! 如果间隔小于minMSIntervalOfOrderAndCancelOrder_那么不撤单
        return false;
      }

      if (!fsm.twap_->getBid1Ask1()) {
        //! 这通常是由于lastPrice超出报单价格区间引起的，那么就在这里一直等待bid1ask1
        const auto timeDurOfSec = timeDur / 1000;
        static std::decay_t<decltype(timeDurOfSec)> timeDurOfSecOfPrint = 0;
        if (timeDurOfSec % 5 == 0) {
          if (timeDurOfSec != timeDurOfSecOfPrint) {
            //! timeDurOfSecOfPrint确保每5秒只打印一次
            fsm.twap_->getAlgoMgr()->getStgEng()->logWarn(
                "[ALGO] Bid1Ask1 is null when try to order. [{}] {}",
                {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
          } else {
            timeDurOfSecOfPrint = timeDurOfSec;
          }
        }
        return false;
      }

      const auto priceOffset =
          fsm.curUrgency_ == Urgency::PriceOfMaker
              ? fsm.twap_->getTrdSymParams()->precOfOrderPrice_ *
                    fsm.curTickerOffset_
              : 0;

      const auto priceForComp =
          fsm.twap_->getTWAPParams()->side_ == Side::Bid
              ? fsm.twap_->getBid1Ask1()->bidPrice_ - priceOffset
              : fsm.twap_->getBid1Ask1()->askPrice_ + priceOffset;

      if (fsm.twap_->getTWAPParams()->side_ == Side::Bid) {
        //! 如果是买入且下单价格已经小于买一减去偏移
        if (DEC::LT(fsm.orderPrice_, priceForComp)) {
          fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
              "[ALGO] Past the cancel order guard because of orderPice {} less "
              "than bidPrice {} - priceOfOffset {} . [{}] {}",
              {std::to_string(fsm.orderPrice_),
               std::to_string(fsm.twap_->getBid1Ask1()->bidPrice_),
               std::to_string(priceOffset), fsm.twap_->toStr(), fsm.toStr()},
              fsm.twap_->getStgInstInfo());
          return true;
        }

      } else {
        //! 如果是卖出且下单价格已经大于卖一加上偏移
        if (DEC::GT(fsm.orderPrice_, priceForComp)) {
          fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
              "[ALGO] Past the cancel order guard because of orderPice {} "
              "greater than askPrice {} + priceOfOffset {}. [{}] {}",
              {std::to_string(fsm.orderPrice_),
               std::to_string(fsm.twap_->getBid1Ask1()->askPrice_),
               std::to_string(priceOffset), fsm.twap_->toStr(), fsm.toStr()},
              fsm.twap_->getStgInstInfo());
          return true;
        }
      }

      return false;
    }
  };

  struct ActionOfCancelOrder {
    template <class Event, class Fsm, class SourceState, class TargetState>
    void operator()(const Event&, Fsm& fsm, SourceState&, TargetState&) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering ActionOfCancelOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      //! 重置最后一次撤单时间
      fsm.lastCancelOrderTime_ = GetTotalMSSince1970();
      const auto statusCode =
          fsm.twap_->getAlgoMgr()->getStgEng()->cancelOrder(fsm.orderId_);
      if (statusCode != 0) {
        fsm.twap_->getAlgoMgr()->getStgEng()->logWarn(
            "[ALGO] Send cancel order failed. [{}] {} [{} - {}]",
            {fsm.twap_->toStr(), fsm.toStr(), std::to_string(statusCode),
             GetStatusMsg(statusCode)},
            fsm.twap_->getStgInstInfo());
        //! 撤单失败，回到StateOfOrderInProc
        fsm.process_event(EventOfCancelOrderFailed{});
      } else {
        fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
            "[ALGO] Send cancel order {} success. [{}] {}",
            {std::to_string(fsm.orderId_), fsm.twap_->toStr(), fsm.toStr()},
            fsm.twap_->getStgInstInfo());
      }
    }
  };

  //! 正在撤单
  struct StateOfCancelOrderInProc : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfCancelOrderInProc. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfCancelOrderInProc. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 已经撤单
  struct StateOfOrderCanceled : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      ++fsm.canceledTimes_;
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfOrderCanceled, "
          "the order has been canceled {} times. [{}] {}",
          {std::to_string(fsm.canceledTimes_), fsm.twap_->toStr(), fsm.toStr()},
          fsm.twap_->getStgInstInfo());
    }
    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfOrderCanceled. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 尝试重新报单
  struct StateOfRetryOrder : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfRetryOrder . [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      const auto threshold =
          fsm.twap_->getTWAPParams()->cancelTimesOfUpgradeUrgency_;
      //! 如果撤单次数达到cancelTimesOfUpgradeUrgency的整数倍，升级urgency
      if (fsm.canceledTimes_ % threshold == 0) {
        std::tie(fsm.curUrgency_, fsm.curTickerOffset_) =
            UpgradeUrgency(fsm.curUrgency_, fsm.curTickerOffset_);
        fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
            "[ALGO] Upgrade urgency to {}:{}. [{}] {}",
            {ENUM_TO_STR(fsm.curUrgency_), std::to_string(fsm.curTickerOffset_),
             fsm.twap_->toStr(), fsm.toStr()},
            fsm.twap_->getStgInstInfo());
      }
      //! 如果在ActionOfRetryOrder里写下单代码的话，transition_table不直观
      fsm.order(fsm);
    }

    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfRetryOrder . [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 报单出错，重新报单
  struct StateOfRestoreOrder : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfRestoreOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }

    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfRestoreOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 订单成功完结
  struct StateOfSuccess : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfSuccess. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      fsm.nextActionOfAlgo_ = NextActionOfAlgo::HandleNextOrder;
    }
    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfSuccess. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  //! 订单出错
  struct StateOfError : public msm::front::state<> {
    template <class Event, class Fsm>
    void on_entry(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering StateOfError. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
    template <class Event, class Fsm>
    void on_exit(const Event&, Fsm& fsm) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Exiting StateOfError. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
    }
  };

  struct GuardOfRestoreOrder {
    template <class Event, class Fsm, class SourceState, class TargetState>
    bool operator()(const Event&, Fsm& fsm, SourceState&, TargetState&) const {
      if (fsm.twap_->needRetoreOrder(fsm.statusCode_)) {
        const auto now = GetTotalMSSince1970();
        if (now - fsm.lastOrderTime_ > fsm.twap_->getMSIntervalOfRetryOrder()) {
          fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
              "[ALGO] Retry order after order failed "
              "because statusCode {} in white list. [{}] {}",
              {std::to_string(fsm.statusCode_), fsm.twap_->toStr(),
               fsm.toStr()},
              fsm.twap_->getStgInstInfo());
          return true;
        } else {
          return false;
        }
      } else {
        fsm.nextActionOfAlgo_ = NextActionOfAlgo::TerminateAlgo;
        return false;
      }
      return true;
    }
  };

  struct ActionOfRestoreOrder {
    template <class Event, class Fsm, class SourceState, class TargetState>
    void operator()(const Event&, Fsm& fsm, SourceState&, TargetState&) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Entering ActionOfRetoreOrder. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      // 这里已经是StateOfRestore状态了，所以 StateOfRestoreOrder =>
      // StateOfOrderInProc 起作用
      fsm.order(fsm);
    }
  };

 public:
  OrderStateMachineImpl(TWAP* twap, std::size_t no, Decimal orderSize)
      : twap_(twap), no_(no), orderSize_(orderSize) {}

  using initial_state = StateOfNewOrder;

  // clang-format off
  struct transition_table
      : public boost::mpl::vector<
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        //       + Start                  + Event                       + Target                   + Action             + Guard             +
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfNewOrder         , EventOfOrderSuccess         , StateOfOrderInProc      >,
        msmf::Row<StateOfNewOrder         , EventOfOrderFailed          , StateOfError            >,
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfOrderInProc      , EventOfCheckIfCancelOrder   , StateOfCancelOrderInProc , ActionOfCancelOrder, GuardOfCancelOrder>,
        msmf::Row<StateOfOrderInProc      , EventOfCancelOrderFailed    , StateOfOrderInProc      >,
        msmf::Row<StateOfOrderInProc      , EventOfOrderRetFilled       , StateOfSuccess          >,
        msmf::Row<StateOfOrderInProc      , EventOfOrderRetFailed       , StateOfError            >,
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfCancelOrderInProc, EventOfCancelOrderRetFailed , StateOfOrderInProc      >,
        msmf::Row<StateOfCancelOrderInProc, EventOfOrderRetFilled       , StateOfSuccess          >,
        msmf::Row<StateOfCancelOrderInProc, EventOfOrderRetFailed       , StateOfError            >,
        msmf::Row<StateOfCancelOrderInProc, EventOfOrderRetCanceled     , StateOfRetryOrder       >,  // 人工撤单会触发此事件从而导致 StateOfOrderInProc => StateOfRetryOrder
        msmf::Row<StateOfCancelOrderInProc, EventOfOrderSizeLTTickerSize, StateOfSuccess          >,  // 人工撤单会触发此事件从而导致 StateOfOrderInProc => StateOfSuccess
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfRetryOrder       , EventOfOrderSuccess         , StateOfOrderInProc      >,
        msmf::Row<StateOfRetryOrder       , EventOfOrderFailed          , StateOfError            >,
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfError            , EventOfCheckIfRestoreOrder  , StateOfRestoreOrder     , ActionOfRestoreOrder, GuardOfRestoreOrder>,
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        msmf::Row<StateOfRestoreOrder     , EventOfOrderSuccess         , StateOfOrderInProc      >,
        msmf::Row<StateOfRestoreOrder     , EventOfOrderFailed          , StateOfError            > 
        // ------+------------------------+-----------------------------+--------------------------+--------------------+-------------------+ 
        > {};
  // clang-format on

  //! 定义订单状态机的初始行为
  template <class Event, class FSM>
  void on_entry(Event const&, FSM& fsm) {
    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Entering FSM. [{}] {}", {fsm.twap_->toStr(), fsm.toStr()},
        fsm.twap_->getStgInstInfo());
  }

  //! 定义订单状态机的退出行为
  template <class Event, class FSM>
  void on_exit(Event const&, FSM& fsm) {
    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Exiting FSM. [{}] {}", {fsm.twap_->toStr(), fsm.toStr()},
        fsm.twap_->getStgInstInfo());
  }

  template <class FSM, class Event>
  void no_transition(Event const& e, FSM& fsm, int state) {}

  //! 状态机业务代码
 public:
  template <class Fsm>
  void order(Fsm& fsm) {
    //! 重置最后一次下单时间
    fsm.lastOrderTime_ = GetTotalMSSince1970();

    if (!fsm.twap_->getBid1Ask1()) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logWarn(
          "[ALGO] Bid1Ask1 is null when try to order. [{}] {}",
          {fsm.twap_->toStr(), fsm.toStr()}, fsm.twap_->getStgInstInfo());
      //! 如果发现价格为空(由于超出报单价格区间引起)，那么通过EventOfOrderSuccess切换到下一订单
      fsm.process_event(EventOfOrderSuccess{});
      return;
    }

    //! 根据Urgency计算下单价格
    const auto orderPrice = GetPriceOfUrgency(
        fsm.twap_->getBid1Ask1(), fsm.twap_->getTWAPParams()->side_,
        fsm.curUrgency_, fsm.curTickerOffset_,
        fsm.twap_->getTrdSymParams()->precOfOrderPrice_);

    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Calc order price {} by cur urgency. [{}] {}",
        {std::to_string(orderPrice), fsm.twap_->toStr(), fsm.toStr()},
        fsm.twap_->getStgInstInfo());

    //! 开始报单
    OrderId orderId;
    std::tie(fsm.statusCode_, orderId) =
        fsm.twap_->getAlgoMgr()->getStgEng()->order(
            fsm.twap_->getStgInstInfo(),                //
            fsm.twap_->getTWAPParams()->marketCode_,    //
            fsm.twap_->getTWAPParams()->symbolCode_,    //
            fsm.twap_->getTWAPParams()->side_,          //
            fsm.twap_->getTWAPParams()->posDirection_,  //
            orderPrice,                                 //
            fsm.curOrderSize_,                          //
            fsm.twap_->getTWAPParams()->trdAcctId_,     //
            fsm.twap_->getTWAPParams()->closeTDayStg_,  //
            fsm.twap_->getAlgoId());                    //

    //! 下单成功保存下单价格和订单号
    if (fsm.statusCode_ == 0) {
      fsm.orderId_ = orderId;
      fsm.orderPrice_ = orderPrice;
      fsm.process_event(EventOfOrderSuccess{});
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Send order {} success. [{}] {}",
          {std::to_string(orderId), std::to_string(orderPrice),
           fsm.twap_->toStr(), fsm.toStr()},
          fsm.twap_->getStgInstInfo());
    } else {
      fsm.twap_->getAlgoMgr()->getStgEng()->logWarn(
          "[ALGO] Send order failed. [{}] {} [{} - {}]",
          {std::to_string(orderPrice), fsm.twap_->toStr(), fsm.toStr(),
           std::to_string(fsm.statusCode_), GetStatusMsg(fsm.statusCode_)},
          fsm.twap_->getStgInstInfo());
      fsm.process_event(EventOfOrderFailed{});
    }

    return;
  }

  template <class FSM>
  void onTimer(FSM& fsm) {
    const auto stateOfNewOrder = fsm.template get_state<StateOfNewOrder&>();
    if (fsm.startTime_ == 0) {
      //! 说明订单尚未开始
      return;
    }

    // if state = StateOfError => StateOfRestoreOrder
    fsm.process_event(EventOfCheckIfRestoreOrder{});

    // if state = StateOfOrderInProc => StateOfCancelOrderInProc
    fsm.process_event(EventOfCheckIfCancelOrder{});
  }

  template <class FSM>
  void onOrderRet(FSM& fsm, const OrderInfo* orderInfo) {
    //! 防护代码，确保是当前订单的下单回报
    if (orderInfo->orderId_ != fsm.orderId_) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Recv order ret of another order {}. [{}] {}",
          {std::to_string(orderInfo->orderId_), fsm.twap_->toStr(),
           fsm.toStr()},
          fsm.twap_->getStgInstInfo());
      return;
    }

    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Order status of order {} is {}. [progress: {}/{}] [{}] {} {}",
        {std::to_string(orderInfo->orderId_),
         ENUM_TO_STR(orderInfo->orderStatus_),
         std::to_string(orderInfo->dealSize_),
         std::to_string(orderInfo->orderSize_), fsm.twap_->toStr(), fsm.toStr(),
         orderInfo->toShortStr()},
        fsm.twap_->getStgInstInfo());

    switch (orderInfo->orderStatus_) {
      case OrderStatus::Filled:
        handleOrderFilled(fsm, orderInfo);
        break;
      case OrderStatus::Canceled:
      case OrderStatus::PartialFilledCanceled:
        handleOrderCanceled(fsm, orderInfo);
        break;
      case OrderStatus::Failed:
        handleOrderFailed(fsm, orderInfo);
        break;
      default:
        break;
    }
  }

  template <class FSM>
  void handleOrderFilled(FSM& fsm, const OrderInfo* orderInfo) {
    recalcDealInfo(fsm, orderInfo);

    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Update deal size of order {} to {}. [{}] {} {}",
        {std::to_string(orderInfo->orderId_), std::to_string(fsm.dealSize_),
         fsm.twap_->toStr(), fsm.toStr(), orderInfo->toShortStr()},
        fsm.twap_->getStgInstInfo());

    fsm.process_event(EventOfOrderRetFilled{});
  }

  template <class FSM>
  void handleOrderCanceled(FSM& fsm, const OrderInfo* orderInfo) {
    recalcDealInfo(fsm, orderInfo);

    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Update deal size of order {} to {}. [{}] {} {}",
        {std::to_string(orderInfo->orderId_), std::to_string(fsm.dealSize_),
         fsm.twap_->toStr(), fsm.toStr(), orderInfo->toShortStr()},
        fsm.twap_->getStgInstInfo());

    //! 开始计算curOrderSize_，也就是下次报单的数量
    const auto origOrdereSize = fsm.curOrderSize_;

    //! 这里必须减去orderInfo->dealSize_而不是ftm.dealSize_，因为假如当前状态机经
    //! 过反复报撤之后已经成交了很多导致ftm.dealSize_已经是一个很大的数字了，但是
    //! curOrderSize_已经几乎没有了，也就是说是一个很小的数字，这样减出来的结果明
    //! 显是不对的，所以未成交的数量应该是当前订单数减去当前订单成交数
    fsm.curOrderSize_ -= orderInfo->dealSize_;
    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Adjust order size "
        "from {} to {} after minus deal size {}. [{}] {}",
        {std::to_string(origOrdereSize), std::to_string(fsm.curOrderSize_),
         std::to_string(fsm.dealSize_), fsm.twap_->toStr(), fsm.toStr()},
        fsm.twap_->getStgInstInfo());

    //! 修正报单数量为precOfOrderVol的整数倍
    auto precOfOrderVol = fsm.twap_->getTrdSymParams()->precOfOrderVol_;
    if (IsCNMarketOfSpots(fsm.twap_->getTWAPParams()->marketCode_)) {
      precOfOrderVol = 100;
    }
    fsm.curOrderSize_ =
        std::floor(fsm.curOrderSize_ / precOfOrderVol) * precOfOrderVol;

    fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
        "[ALGO] Adjust order size to {} after "
        "set to an integer multiple of precOfOrderVol {}. [{}] {}",
        {std::to_string(fsm.curOrderSize_), std::to_string(precOfOrderVol),
         fsm.twap_->toStr(), fsm.toStr()},
        fsm.twap_->getStgInstInfo());

    if (DEC::ZERO(fsm.curOrderSize_)) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] The remaining order size {} "
          "less than 1 ticker size {}, switch to next order. [{}] {}",
          {std::to_string(fsm.curOrderSize_), std::to_string(precOfOrderVol),
           fsm.twap_->toStr(), fsm.toStr()},
          fsm.twap_->getStgInstInfo());
      //! 如果报单数量小于一个ticker那么结束当前订单，零股在TWAP里重新分配
      fsm.process_event(EventOfOrderSizeLTTickerSize{});
    } else {
      //! 否则进入StateOfOrderCanceled，尝试重新报单
      fsm.process_event(EventOfOrderRetCanceled{});
    }
  }

  template <class FSM>
  void recalcDealInfo(FSM& fsm, const OrderInfo* orderInfo) {
    //! 更新成交数量，在完结态累加确保只加一次，同时确保反复撤报也能正确计算dealSize
    const auto newDealSize = fsm.dealSize_ + std::fabs(orderInfo->dealSize_);
    if (!DEC::ZERO(newDealSize)) {
      fsm.avgDealPrice_ =
          (fsm.avgDealPrice_ * fsm.dealSize_ +
           orderInfo->avgDealPrice_ * std::fabs(orderInfo->dealSize_)) /
          newDealSize;
      fsm.dealSize_ = newDealSize;
    }
  }

  template <class FSM>
  void handleOrderFailed(FSM& fsm, const OrderInfo* orderInfo) {
    if (orderInfo->statusCode_ != 0) {
      fsm.statusCode_ = orderInfo->statusCode_;
    }
    fsm.process_event(EventOfOrderRetFailed{});
  }

  template <class FSM>
  void onCancelOrderRet(FSM& fsm, const OrderInfo* orderInfo) {
    //! 防护代码，确保是当前订单的撤单应答
    if (orderInfo->orderId_ != fsm.orderId_) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logInfo(
          "[ALGO] Recv cancel order ret of another order {}. [{}] {}",
          {std::to_string(orderInfo->orderId_), fsm.twap_->toStr(),
           fsm.toStr()},
          fsm.twap_->getStgInstInfo());
      return;
    }

    //! 撤单失败，继续StateOrderInProc状态
    if (orderInfo->statusCode_ != 0) {
      fsm.twap_->getAlgoMgr()->getStgEng()->logWarn(
          "[ALGO] Cancel order failed. [{}] {} [{} - {}]",
          {fsm.twap_->toStr(), fsm.toStr(),
           std::to_string(orderInfo->statusCode_),
           GetStatusMsg(orderInfo->statusCode_)},
          fsm.twap_->getStgInstInfo());
      //! 回到StateOrderInProc状态
      fsm.process_event(EventOfCancelOrderRetFailed{});
    }
  }

 public:
  TWAP* twap_{nullptr};

  const std::size_t no_{0};
  const Decimal orderSize_{0};

  std::uint64_t startTime_{0};

  std::uint64_t lastOrderTime_{0};
  std::uint64_t lastCancelOrderTime_{0};

  OrderId orderId_{0};
  Decimal orderPrice_{0};

  //! 因为部成部撤后再次报单不能用orderSize_所以有了这个变量用于计算每次报单数量
  Decimal curOrderSize_{0};

  //! 成交数量有了两个作用：
  //! 1. 用于部成部撤后重新下单的时候计算新的下单数量
  //! 2. 计算当前订单结束时因为反复撤报导致的未成交的零股
  Decimal dealSize_{0};
  //! 成交均价，通知算法单进度用到
  Decimal avgDealPrice_{0};

  Urgency curUrgency_;
  std::uint32_t curTickerOffset_{0};

  std::uint32_t canceledTimes_{0};

  int statusCode_{0};
  NextActionOfAlgo nextActionOfAlgo_{NextActionOfAlgo::HandleCurOrder};

  std::string toStr() const {
    return fmt::format("no: {}; orderId:{}; orderSize: {}; orderPrice: {}", no_,
                       orderId_, orderSize_, orderPrice_);
  }
};

using OrderStateMachine = msm::back::state_machine<OrderStateMachineImpl>;
using OrderStateMachineSPtr = std::shared_ptr<OrderStateMachine>;

using OrderStateMachineGroup = std::deque<OrderStateMachineSPtr>;

}  // namespace bq::algo
