/*!
 * \file DataStruOfTD.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "def/DataStruOfTD.hpp"

#include "db/TBLOrderInfo.hpp"
#include "db/TBLStgInstInfo.hpp"
#include "db/TBLTradeInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfStg.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/Logger.hpp"
#include "util/OpenedContractGroup.hpp"
#include "util/Random.hpp"
#include "util/String.hpp"

namespace bq {

//!
//! 同步订单也就是回报可能需要填写的字段，以下字段要么不填，要么有效值
//!
//! ---------------
//! orderStatus_  |
//! ---------------
//! dealSize_     |
//! ---------------
//! avgDealPrice_ |
//! ---------------
//! fee_          |
//! ---------------
//! lastDealSize_ |
//! ---------------
//! lastDealPrice_|
//! ---------------
//! lastTradeId_  |
//! ---------------
//! lastDealTime_ |
//! ---------------
//! feeCurrency_  | 静态字段
//! ---------------
//! exchOrderId_  | 静态字段 索引字段
//! ---------------
//! statusCode_   | 静态字段
//! ---------------
//!
//! 订单状态变得更新，成交数量变大，一些字段从没有值变成有值，这几种情况下要更新
//! OrdMgr并将isTheOrderInfoUpdated = true，然后将更新后的订单推送给tdSrv
//!
//! 这个函数的主要功能就是更新OrdMgr，并将更新后的订单推送的tdSrv，包括以下几种情况
//!
//! 1. 静态字段发生变化，那么更新静态字段推送
//! 2. 委托状态变得更新且原始订单的委托状态并未完结，那么更新委托状态推送
//! 3. 成交数量dealSize_变大且合法（小于orderSize_），那么更新成交数量金额等信息
//!
//! 注意tdSrv可能还是会收到两笔除了静态字段以外其他字段一模一样的全成的订单信息，
//! 因此存储过程tradeInfo入库的时候，对于全成也做了dealSize是否变大的判断，如果
//! dealSize没有变大，那么说明收到了重复的全成。
//!
std::tuple<IsSomeFieldOfOrderUpdated, IsTheKeyFieldOfOrderUpdated>
OrderInfo::updateByOrderInfoFromExch(
    const OrderInfoSPtr& newOrderInfo, std::uint64_t noUsedToCalcPos,
    const FeeInfoCacheSPtr& feeInfoCache,
    const OpenedContractGroupSPtr& openedContractGroup) {
  IsSomeFieldOfOrderUpdated isTheOrderInfoUpdated =
      IsSomeFieldOfOrderUpdated::False;
  IsTheKeyFieldOfOrderUpdated isTheKeyFieldOfOrderUpdated =
      IsTheKeyFieldOfOrderUpdated::False;

  //!
  //! 处理同步未完结订单和websocket委托回报得到orderInfoFromExch之后马上进入这里，
  //! 因此在此处统一将dealSize_和lastDealSize_的正负值设定正确。
  //!
  if (side_ == Side::Bid) {
    if (newOrderInfo->dealSize_ < 0) newOrderInfo->dealSize_ *= -1;
    if (newOrderInfo->lastDealSize_ < 0) newOrderInfo->lastDealSize_ *= -1;
  } else {  // Side::Ask
    if (newOrderInfo->dealSize_ > 0) newOrderInfo->dealSize_ *= -1;
    if (newOrderInfo->lastDealSize_ > 0) newOrderInfo->lastDealSize_ *= -1;
  }

  //! 直接赋值，如果成交有变化，自然会入库
  noUsedToCalcPos_ = noUsedToCalcPos;

  //！acctId 发生变化说明有借仓逻辑发生，这段代码通常在策略引擎中触发
  if (newOrderInfo->acctId_ != 0) {
    if (acctId_ != newOrderInfo->acctId_) {
      LOG_I("Account {} borrow pos from account {}. {}", acctId_,
            newOrderInfo->acctId_, newOrderInfo->toShortStr());
      acctId_ = newOrderInfo->acctId_;
    }
  }

  //！trdAcctId 发生变化说明有借仓逻辑发生，这段代码通常在策略引擎中触发
  if (newOrderInfo->trdAcctId_ != 0) {
    if (trdAcctId_ != newOrderInfo->trdAcctId_) {
      LOG_I("Trade account {} borrow pos from trade account {}. {}", trdAcctId_,
            newOrderInfo->trdAcctId_, newOrderInfo->toShortStr());
      trdAcctId_ = newOrderInfo->trdAcctId_;
    }
  }

  //! 状态变得更加新
  if (newOrderInfo->orderStatus_ > orderStatus_) {
    if (notClosed()) {
      //! 如果OrdMgr中的订单未完结且当前订单状态变得更加新，那么更新订单状态
      orderStatus_ = newOrderInfo->orderStatus_;
      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
    } else {
      //! 订单从一个终态变成另一个终态，不合理
      LOG_W("Orders change from one final state {} to another. {}",
            magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
    }
  } else if (newOrderInfo->orderStatus_ == orderStatus_) {
    //! 如果订单的状态没有变化
    if (OrderStatus::PartialFilled == newOrderInfo->orderStatus_ &&
        OrderStatus::PartialFilled == orderStatus_) {
      //! 这里先不将isTheOrderInfoUpdated置为True，必须等isValidOrderInfo为
      //! true的时候，也就是dealSize_是合法的情况下
    } else {
      //! 订单状态没有变化且不是部分成交（同步订单的时候可能会有此提示，因此日志
      //! 等级改为info）
      LOG_D("The order status {} not changed. {}",
            magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
    }
  } else {
    //! 状态变得更旧，成交回报的时候创建了只有成交信息的回报的时候会进入这里
    LOG_I("The order status {} has become older. {}",
          magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
  }

  //! 判断成交数量是否合法，也就是成交数量是否变大且小于等于委托数量
  bool isValidOrderInfo = false;
  if (DEC::GT(std::fabs(newOrderInfo->dealSize_), std::fabs(dealSize_))) {
    //! 如果成交数量变大
    if (DEC::LE(std::fabs(newOrderInfo->dealSize_), orderSize_)) {
      //! 如果成交数量小于等于委托数量那么是合理的
      isValidOrderInfo = true;
    } else {
      //! 如果成交数量大于委托数量那么非法
      LOG_W("Deal size greater {} than order size {}. {}",
            std::fabs(newOrderInfo->dealSize_), orderSize_,
            newOrderInfo->toShortStr());
    }
  } else {
    //! 如果成交数量没有变化或者变小，那么收到了非法的回报（同步订单的时候可能
    //! 会有此提示，因此日志等级改为info）
    LOG_D("New deal size {} less than or equal to original deal size {}. {}",
          std::fabs(newOrderInfo->dealSize_), std::fabs(dealSize_),
          newOrderInfo->toShortStr());
  }

  //! dealSize_变大的情况，也就是dealSize_不为0
  if (isValidOrderInfo) {
    //! 只有成交均价也不为0的情况下，其他数量价格字段才有意义一起更新
    if (!DEC::ZERO(newOrderInfo->avgDealPrice_)) {
      const auto oldDealSize = dealSize_;
      const auto newDealSize = newOrderInfo->dealSize_;
      const auto oldAvgDealPrice = avgDealPrice_;
      const auto newAvgDealPrice = newOrderInfo->avgDealPrice_;

      dealSize_ = newDealSize;
      avgDealPrice_ = newAvgDealPrice;

      if (!DEC::ZERO(newOrderInfo->lastDealSize_)) {
        lastDealSize_ = newOrderInfo->lastDealSize_;
      } else {
        lastDealSize_ = newDealSize - oldDealSize;
      }

      if (!DEC::ZERO(newOrderInfo->lastDealPrice_)) {
        lastDealPrice_ = newOrderInfo->lastDealPrice_;
      } else {
        const auto lastDealAmt =
            newAvgDealPrice * newDealSize - oldAvgDealPrice * oldDealSize;
        if (!DEC::ZERO(lastDealSize_)) {
          lastDealPrice_ = lastDealAmt / lastDealSize_;
        } else {
          LOG_W("Last deal size {} is equal to 0 when calc last deal price. {}",
                lastDealSize_, newOrderInfo->toShortStr());
        }
      }

      //! fee_由dealSize_决定，dealSize_发生变化的时候必须重新赋值或者计算
      if (!DEC::ZERO(newOrderInfo->fee_)) {
        fee_ = newOrderInfo->fee_;
      } else {
        if (feeInfoCache != nullptr) {
          const auto dealAmt = std::fabs(newDealSize * newAvgDealPrice);
          fee_ = calcFee(feeInfoCache, openedContractGroup, dealAmt);
        } else {
          //! 原始订单中的fee_不为0才能按照比例计算出最新的fee
          if (!DEC::ZERO(fee_) && !DEC::ZERO(oldDealSize)) {
            fee_ = newDealSize / oldDealSize * fee_;
          }
        }
      }

      strncpy(lastTradeId_, newOrderInfo->lastTradeId_,
              sizeof(lastTradeId_) - 1);
      if (newOrderInfo->lastDealTime_ != 0) {
        lastDealTime_ = newOrderInfo->lastDealTime_;
      }

      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
    }
  }

  //! 处理成交回报
  if (DEC::ZERO(newOrderInfo->dealSize_) &&
      DEC::ZERO(newOrderInfo->avgDealPrice_)) {
    if (!DEC::ZERO(newOrderInfo->lastDealSize_) &&
        !DEC::ZERO(newOrderInfo->lastDealPrice_)) {
      const auto oldDealSize = dealSize_;

      const auto dealAmt =
          dealSize_ * avgDealPrice_ +
          newOrderInfo->lastDealSize_ * newOrderInfo->lastDealPrice_;
      dealSize_ += newOrderInfo->lastDealSize_;
      avgDealPrice_ = dealAmt / dealSize_;

      lastDealSize_ = newOrderInfo->lastDealSize_;
      lastDealPrice_ = newOrderInfo->lastDealPrice_;

      if (!DEC::ZERO(newOrderInfo->fee_)) {
        fee_ = newOrderInfo->fee_;
      } else {
        if (feeInfoCache != nullptr) {
          fee_ = calcFee(feeInfoCache, openedContractGroup, dealAmt);
        } else {
          //! 原始订单中的fee_不为0才能按照比例计算出最新的fee
          if (!DEC::ZERO(fee_) && !DEC::ZERO(oldDealSize)) {
            fee_ = dealSize_ / oldDealSize * fee_;
          }
        }
      }

      strncpy(lastTradeId_, newOrderInfo->lastTradeId_,
              sizeof(lastTradeId_) - 1);
      if (newOrderInfo->lastDealTime_ != 0) {
        lastDealTime_ = newOrderInfo->lastDealTime_;
      }

      if (DEC::EQ(std::fabs(dealSize_), orderSize_)) {
        orderStatus_ = OrderStatus::Filled;
        LOG_T(
            "Set order status to {} because of dealSize {} "
            "equal to orderSize {}. oldOrderInfo: {}; newOrderInfo: ",
            magic_enum::enum_name(orderStatus_), std::fabs(dealSize_),
            orderSize_, toShortStr(), newOrderInfo->toShortStr());

      } else if (DEC::LT(std::fabs(dealSize_), orderSize_)) {
        orderStatus_ = OrderStatus::PartialFilled;
        LOG_T(
            "Set order status to {} because of dealSize {} "
            "less than orderSize {}. oldOrderInfo: {}; newOrderInfo: {}",
            magic_enum::enum_name(orderStatus_), std::fabs(dealSize_),
            orderSize_, toShortStr(), newOrderInfo->toShortStr());

      } else {
        orderStatus_ = OrderStatus::Filled;
        LOG_W(
            "Find deal size {} is greater than order size {} "
            "when handle rtn trade. {}",
            std::fabs(dealSize_), orderSize_, newOrderInfo->toShortStr());
      }

      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
    }
  }

  //! 以下更新静态字段
  if (newOrderInfo->exchOrderId_[0] != '\0') {
    if (exchOrderId_[0] == '\0') {
      strcpy(exchOrderId_, newOrderInfo->exchOrderId_);
      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
      hashOfExchOrderId_ = XXH3_64bits(exchOrderId_, strlen(exchOrderId_));
      isTheKeyFieldOfOrderUpdated = IsTheKeyFieldOfOrderUpdated::True;
    } else {
      //! 新回报里的exchOrderId_发生了变化，正常情况下不可能出现
    }
  } else {
    //! 新回报里的exchOrderId_为0，有可能出现
  }

  if (newOrderInfo->feeCurrency_[0] != '\0') {
    if (feeCurrency_[0] == '\0') {
      strncpy(feeCurrency_, newOrderInfo->feeCurrency_,
              sizeof(feeCurrency_) - 1);
      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
    }
  }

  if (newOrderInfo->statusCode_ != 0) {
    if (statusCode_ == 0) {
      statusCode_ = newOrderInfo->statusCode_;
      isTheOrderInfoUpdated = IsSomeFieldOfOrderUpdated::True;
    }
  }

  if (openedContractGroup) {
    openedContractGroup->saveOpenedContract<LockFunc::True>(this);
  }

  return {isTheOrderInfoUpdated, isTheKeyFieldOfOrderUpdated};
}

//!
//! 这个函数负责更新OrdMgr中的订单信息以及决定该订单是否参与仓位计算
//!
std::tuple<IsTheOrderCanBeUsedCalcPos, IsTheKeyFieldOfOrderUpdated>
OrderInfo::updateByOrderInfoFromTDGW(const OrderInfoSPtr& newOrderInfo) {
  IsTheOrderCanBeUsedCalcPos isTheOrderCanBeUsedCalcPos =
      IsTheOrderCanBeUsedCalcPos::False;
  IsTheKeyFieldOfOrderUpdated isTheKeyFieldOfOrderUpdated =
      IsTheKeyFieldOfOrderUpdated::False;

  //! 理论上不需要再判断，因为交易网关的逻辑是检查到交易所给的回报数据合法后，再
  //! 覆盖OrdMgr中的数据，然后再把OrdMgr中的数据拿出来发给tdSrv后期考虑删除不必
  //! 要的判断。
  if (newOrderInfo->orderStatus_ > orderStatus_) {
    if (notClosed()) {
      //! 如果OrdMgr中的订单未完结且当前订单状态变得更加新，那么更新订单状态
      orderStatus_ = newOrderInfo->orderStatus_;
    } else {
      //! 从一个终态变成另一个终态，不合理
      LOG_W("Orders change from one final state {} to another. {}",
            magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
    }
  } else if (newOrderInfo->orderStatus_ == orderStatus_) {
    //! 如果订单的状态没有变化
    if (OrderStatus::PartialFilled == newOrderInfo->orderStatus_ &&
        OrderStatus::PartialFilled == orderStatus_) {
      //! 部成到部成的变化是否需要参与仓位计算在后面判断
    } else {
      //! 订单状态没有变化且不是部分成交
      LOG_W("The order status has not changed and it is not PartialFilled. {}",
            newOrderInfo->toShortStr());
    }
  } else {
    //! 状态变得更旧，不合理
    LOG_W("The order status {} has become older. {} {}",
          magic_enum::enum_name(orderStatus_), toShortStr(),
          newOrderInfo->toShortStr());
  }

  bool isValidOrderInfo = false;
  if (DEC::GT(std::fabs(newOrderInfo->dealSize_), std::fabs(dealSize_))) {
    //! 如果成交数量变大
    if (DEC::LE(std::fabs(newOrderInfo->dealSize_), orderSize_)) {
      //! 如果成交数量小于等于委托数量那么是合理的，是否参与仓位计算需要在下面
      //! 进一步判断。
      isValidOrderInfo = true;
    } else {
      //! 理论上不会出现，因为交易网关的逻辑是检查到交易所给的回报数据合法后，
      //! 再覆盖OrdMgr中的数据，然后再把OrdMgr中的数据拿出来发给tdSrv
      LOG_W("Deal size {} greater than order size {}. {}",
            std::fabs(newOrderInfo->dealSize_), orderSize_,
            newOrderInfo->toShortStr());
    }
  } else {
    //! 理论上不会出现
    LOG_T("New deal size {} less than or equal to original deal size {}. {}",
          std::fabs(newOrderInfo->dealSize_), std::fabs(dealSize_),
          newOrderInfo->toShortStr());
  }

  //! dealSize_变大的情况，也就是dealSize_不为0
  if (isValidOrderInfo) {
    //! 如果newOrderInfo->avgDealPrice_不为0，那么updateByOrderInfoFromExch确保
    //! 了下面的值有意义，可以直接赋值。
    if (!DEC::ZERO(newOrderInfo->avgDealPrice_)) {
      dealSize_ = newOrderInfo->dealSize_;
      avgDealPrice_ = newOrderInfo->avgDealPrice_;

      lastDealSize_ = newOrderInfo->lastDealSize_;
      lastDealPrice_ = newOrderInfo->lastDealPrice_;
      strncpy(lastTradeId_, newOrderInfo->lastTradeId_,
              sizeof(lastTradeId_) - 1);
      lastDealTime_ = newOrderInfo->lastDealTime_;

      fee_ = newOrderInfo->fee_;
      isTheOrderCanBeUsedCalcPos = IsTheOrderCanBeUsedCalcPos::True;
    }
  }

  //! 以下更新静态字段
  if (newOrderInfo->exchOrderId_[0] != '\0') {
    if (exchOrderId_[0] == '\0') {
      strcpy(exchOrderId_, newOrderInfo->exchOrderId_);
      hashOfExchOrderId_ = XXH3_64bits(exchOrderId_, strlen(exchOrderId_));
      isTheKeyFieldOfOrderUpdated = IsTheKeyFieldOfOrderUpdated::True;
    } else {
      //! 新回报里的exchOrderId_发生了变化，正常情况下不可能出现
    }
  } else {
    //! 新回报里的exchOrderId_为0，有可能出现
  }

  if (newOrderInfo->feeCurrency_[0] != '\0') {
    if (feeCurrency_[0] == '\0') {
      strncpy(feeCurrency_, newOrderInfo->feeCurrency_,
              sizeof(feeCurrency_) - 1);
    }
  }

  if (newOrderInfo->statusCode_ != 0) {
    if (statusCode_ == 0) {
      statusCode_ = newOrderInfo->statusCode_;
    }
  }

  return {isTheOrderCanBeUsedCalcPos, isTheKeyFieldOfOrderUpdated};
}

//! newOrderInfo中必须有orderId、orderStatus、dealSize这三个字段，或者marketCode、
//! exchOrderId、orderStatus、dealSize
bool OrderInfo::compAndCheckIfUpdate(const OrderInfoSPtr& newOrderInfo) {
  //! 状态变得更加新
  if (newOrderInfo->orderStatus_ > orderStatus_) {
    if (notClosed()) {
      //! 如果OrdMgr中的订单未完结且当前订单状态变得更加新，那么更新订单状态
      return true;
    } else {
      //! 订单从一个终态变成另一个终态，不合理
      LOG_W("Orders change from one final state {} to another. {}",
            magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
    }
  } else if (newOrderInfo->orderStatus_ == orderStatus_) {
    //! 如果订单的状态没有变化
    if (OrderStatus::PartialFilled == newOrderInfo->orderStatus_ &&
        OrderStatus::PartialFilled == orderStatus_) {
      //! 如果是部分成交，但是成交量变大
      if (DEC::GT(newOrderInfo->dealSize_, dealSize_)) {
        return true;
      }
    } else {
      //! 订单状态没有变化且不是部分成交（同步订单的时候可能会有此提示，因此日志
      //! 等级改为info）
      LOG_I("The order status {} not changed. {}",
            magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
    }
  } else {
    //! 状态变得更旧，成交回报的时候创建了只有成交信息的回报的时候会进入这里
    LOG_I("The order status {} has become older. {}",
          magic_enum::enum_name(orderStatus_), newOrderInfo->toShortStr());
  }

  if (feeCurrency_[0] == '\0') {
    if (newOrderInfo->feeCurrency_[0] != '\0') {
      return true;
    }
  }

  if (exchOrderId_[0] == '\0') {
    if (newOrderInfo->exchOrderId_[0] != '\0') {
      return true;
    }
  }

  return false;
}

// clang-format off
std::string OrderInfo::toShortStr() const {
  const auto ret  = fmt::format(
    "["
    "{} "
    "stgInstId: {} "
    "orderId: {} "
    "exchOrderId: {} "
    "{} "
    "{} "
    "{} "
    "{} "
    "{} "
    "{} "
    "{} "
    "price: {} "
    "size: {} "
    "avgDealPrice: {} "
    "dealSize: {} "
    "fee: {} "
    "lastDealPrice: {} "
    "lastDealSize: {} "
    "orderStatus: {} "
    "statusCode: {} "
    "statusMsg: {} "
    "]",
    shmHeader_.toStr(),
    stgInstId_,
    orderId_,
    exchOrderId_,
    GetMarketName(marketCode_),
    magic_enum::enum_name(symbolType_),
    symbolCode_,
    magic_enum::enum_name(side_),
    magic_enum::enum_name(posDirection_),
    magic_enum::enum_name(posSide_),
    magic_enum::enum_name(closeTDayStg_),
    orderPrice_,
    orderSize_,
    avgDealPrice_,
    dealSize_,
    fee_,
    lastDealPrice_,
    lastDealSize_,
    magic_enum::enum_name(orderStatus_),
    statusCode_,
    GetStatusMsg(statusCode_)
    );
  return ret;
}
// clang-format on

std::string OrderInfo::toJson() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();

  writer.Key("productGrpId");
  writer.Uint(productGrpId_);
  writer.Key("productId");
  writer.Uint(productId_);
  writer.Key("userId");
  writer.Uint(userId_);
  writer.Key("acctGrpId");
  writer.Uint(acctGrpId_);
  writer.Key("acctId");
  writer.Uint(acctId_);
  writer.Key("trdAcctId");
  writer.Uint(trdAcctId_);

  writer.Key("stgGrpId");  // add stgGrpId
  writer.Uint(stgGrpId_);  // add stgGrpId
  writer.Key("stgId");
  writer.Uint(stgId_);
  writer.Key("stgInstId");
  writer.Uint(stgInstId_);
  writer.Key("algoId");
  writer.Uint64(algoId_);

  writer.Key("orderId");
  writer.Uint64(orderId_);
  writer.Key("exchOrderId");
  writer.String(exchOrderId_);
  writer.Key("parentOrderId");
  writer.Uint64(parentOrderId_);

  writer.Key("marketCode");
  writer.String(GetMarketName(marketCode_).c_str());
  writer.Key("symbolType");
  writer.String(ENUM_TO_CSTR(symbolType_));
  writer.Key("symbolCode");
  writer.String(symbolCode_);
  writer.Key("exchSymbolCode");
  writer.Key(exchSymbolCode_);

  writer.Key("side");
  writer.Key(ENUM_TO_CSTR(side_));
  writer.Key("posDirection");
  writer.Key(ENUM_TO_CSTR(posDirection_));
  writer.Key("posSide");
  writer.Key(ENUM_TO_CSTR(posSide_));

  writer.Key("orderPrice");
  writer.Double(orderPrice_);
  writer.Key("orderSize");
  writer.Double(orderSize_);

  writer.Key("parValue");
  writer.Uint(parValue_);

  writer.Key("orderType");
  writer.Key(ENUM_TO_CSTR(orderType_));
  writer.Key("orderTypeExtra");
  writer.Key(ENUM_TO_CSTR(orderTypeExtra_));
  writer.Key("closeTDayStg");
  writer.Key(ENUM_TO_CSTR(closeTDayStg_));

  writer.Key("orderTime");
  writer.Uint64(orderTime_);

  writer.Key("fee");
  writer.Double(fee_);
  writer.Key("feeCurrency");
  writer.String(feeCurrency_);

  writer.Key("dealSize");
  writer.Double(dealSize_);
  writer.Key("avgDealPrice");
  writer.Double(avgDealPrice_);

  writer.Key("lastTradeId");
  writer.String(lastTradeId_);
  writer.Key("lastDealPrice");
  writer.Double(lastDealPrice_);
  writer.Key("lastDealSize");
  writer.Double(lastDealSize_);
  writer.Key("lastDealTime");
  writer.Uint64(lastDealTime_);

  writer.Key("orderStatus");
  writer.Key(ENUM_TO_CSTR(orderStatus_));

  writer.Key("statusCode");
  writer.Int(statusCode_);

  writer.EndObject();

  return strBuf.GetString();
}

std::string OrderInfo::getSymbolInfo() const {
  const auto ret = fmt::format("{}-{}-{}", GetMarketName(marketCode_),
                               magic_enum::enum_name(symbolType_), symbolCode_);
  return ret;
}

// clang-format off
std::string OrderInfo::getSqlOfReplace() const {
  const auto sql = fmt::format(
    "REPLACE INTO {} ("
    "`productGrpId`,"
    "`productId`,"
    "`userId`,"
    "`acctGrpId`,"
    "`acctId`,"
    "`trdAcctId`,"
    "`stgGrpId`,"  // add stgGrpId
    "`stgId`,"
    "`stgInstId`,"
    "`algoId`,"
    "`orderId`,"
    "`exchOrderId`,"
    "`parentOrderId`,"
    "`marketCode`,"
    "`symbolType`,"
    "`symbolCode`,"
    "`exchSymbolCode`,"
    "`side`,"
    "`posDirection`,"
    "`posSide`,"
    "`orderPrice`,"
    "`orderSize`,"
    "`parValue`,"
    "`orderType`,"
    "`orderTypeExtra`,"
    "`closeTDayStg`,"
    "`orderTime`,"
    "`fee`,"
    "`feeCurrency`,"
    "`dealSize`,"
    "`avgDealPrice`,"
    "`lastTradeId`,"
    "`lastDealPrice`,"
    "`lastDealSize`,"
    "`lastDealTime`,"
    "`orderStatus`,"
    "`statusCode`,"
    "`statusMsg`"
    ")"
  "VALUES"
  "("
    " {}, "  // productGrpId 
    " {}, "  // productId 
    " {}, "  // userId
    " {}, "  // acctGrpId
    " {}, "  // acctId
    " {}, "  // trdAcctId
    " {}, "  // stgGrpId add stgGrpId
    " {}, "  // stgId
    " {}, "  // stgInstId
    " {}, "  // algoId
    "'{}',"  // orderId
    "'{}',"  // exchOrderId
    "'{}',"  // parentOrderId
    "'{}',"  // marketCode
    "'{}',"  // symbolType
    "'{}',"  // symbolCode
    "'{}',"  // exchSymbolCode
    "'{}',"  // side
    "'{}',"  // posDirection
    "'{}',"  // posSide
    "'{}',"  // orderPrice
    "'{}',"  // orderSize
    "'{}',"  // parValue
    "'{}',"  // orderType
    "'{}',"  // orderTypeExtra
    "'{}',"  // closeTDayStg
    "'{}',"  // orderTime
    "'{}',"  // fee
    "'{}',"  // feeCurrency
    "'{}',"  // dealSize
    "'{}',"  // avgDealPrice
    "'{}',"  // lastTradeId
    "'{}',"  // lastDealPrice
    "'{}',"  // lastDealSize
    "'{}',"  // lastDealTime
    " {}, "  // orderStatus
    " {}, "  // statusCode
    "'{}' "  // statusMsg
  "); ",
    TBLOrderInfo::TableName,
    productGrpId_,
    productId_,
    userId_,
    acctGrpId_,
    acctId_,
    trdAcctId_,
    stgGrpId_,  // add stgGrpId
    stgId_,
    stgInstId_,
    algoId_,
    orderId_,
    exchOrderId_,
    parentOrderId_,
    GetMarketName(marketCode_),
    magic_enum::enum_name(symbolType_),
    symbolCode_,
    exchSymbolCode_,
    magic_enum::enum_name(side_),
    magic_enum::enum_name(posDirection_),
    magic_enum::enum_name(posSide_),
    orderPrice_,
    orderSize_,
    parValue_,
    magic_enum::enum_name(orderType_),
    magic_enum::enum_name(orderTypeExtra_),
    magic_enum::enum_name(closeTDayStg_),
    ConvertTsToDBTime(orderTime_),
    fee_,
    feeCurrency_,
    dealSize_,
    avgDealPrice_,
    lastTradeId_,
    lastDealPrice_,
    lastDealSize_,
    ConvertTsToDBTime(lastDealTime_),
    magic_enum::enum_integer(orderStatus_),
    statusCode_,
    GetStatusMsg(statusCode_)
  );
  return sql;
}
// clang-format on

// clang-format off
std::string OrderInfo::getSqlOfUSPOrderInfoUpdate() const {
  const auto sql = fmt::format(
  "call `uspOrderInfoUpdate` "
  "("
    " {}, "  // productGrpId 
    " {}, "  // productId 
    " {}, "  // userId
    " {}, "  // acctGrpId
    " {}, "  // acctId
    " {}, "  // trdAcctId
    " {}, "  // stgGrpId add stgGrpId
    " {}, "  // stgId
    " {}, "  // stgInstId
    " {}, "  // algoId
    "'{}',"  // orderId
    "'{}',"  // exchOrderId
    "'{}',"  // parentOrderId
    "'{}',"  // marketCode
    "'{}',"  // symbolType
    "'{}',"  // symbolCode
    "'{}',"  // exchSymbolCode
    "'{}',"  // side
    "'{}',"  // posDirection
    "'{}',"  // posSide
    "'{}',"  // orderPrice
    "'{}',"  // orderSize
    "'{}',"  // parValue
    "'{}',"  // orderType
    "'{}',"  // orderTypeExtra
    "'{}',"  // closeTDayStg
    "'{}',"  // orderTime
    "'{}',"  // fee
    "'{}',"  // feeCurrency
    "'{}',"  // dealSize
    "'{}',"  // avgDealPrice
    "'{}',"  // lastTradeId
    "'{}',"  // lastDealPrice
    "'{}',"  // lastDealSize
    "'{}',"  // lastDealTime
    " {}, "  // orderStatus
    " {}, "  // noUsedToCalcPos
    " {}, "  // statusCode
    "'{}',"  // statusMsg
    "'{}' "  // extData
  "); ",
    productGrpId_,
    productId_,
    userId_,
    acctGrpId_,
    acctId_,
    trdAcctId_,
    stgGrpId_,  // add stgGrpId
    stgId_,
    stgInstId_,
    algoId_,
    orderId_,
    exchOrderId_,
    parentOrderId_,
    GetMarketName(marketCode_),
    magic_enum::enum_name(symbolType_),
    symbolCode_,
    exchSymbolCode_,
    magic_enum::enum_name(side_),
    magic_enum::enum_name(posDirection_),
    magic_enum::enum_name(posSide_),
    orderPrice_,
    orderSize_,
    parValue_,
    magic_enum::enum_name(orderType_),
    magic_enum::enum_name(orderTypeExtra_),
    magic_enum::enum_name(closeTDayStg_),
    ConvertTsToDBTime(orderTime_),
    fee_,
    feeCurrency_,
    dealSize_,
    avgDealPrice_,
    lastTradeId_,
    lastDealPrice_,
    lastDealSize_,
    ConvertTsToDBTime(lastDealTime_),
    magic_enum::enum_integer(orderStatus_),
    noUsedToCalcPos_,
    statusCode_,
    GetStatusMsg(statusCode_),
    ""
  );
  return sql;
}
// clang-format on

Decimal OrderInfo::getFeeOfLastTrade() const {
  if (DEC::ZERO(dealSize_)) {
    return 0;
  }
  const auto ret = (lastDealSize_ / dealSize_) * fee_;
  return ret;
}

std::string OrderInfo::getPosKey() const {
  const auto ret = fmt::format(  // add stgGrpId
      "{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}", productGrpId_,
      productId_, userId_, acctGrpId_, acctId_, trdAcctId_, stgGrpId_, stgId_,
      stgInstId_, algoId_, GetMarketName(marketCode_),
      magic_enum::enum_name(symbolType_), symbolCode_,
      magic_enum::enum_name(side_), magic_enum::enum_name(posSide_), parValue_,
      feeCurrency_);
  return ret;
}

std::string OrderInfo::getPosKeyOfBid() const {
  const auto ret = fmt::format(  // add stgGrpId
      "{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}", productGrpId_,
      productId_, userId_, acctGrpId_, acctId_, trdAcctId_, stgGrpId_, stgId_,
      stgInstId_, algoId_, GetMarketName(marketCode_),
      magic_enum::enum_name(symbolType_), symbolCode_,
      magic_enum::enum_name(Side::Bid), magic_enum::enum_name(posSide_),
      parValue_, feeCurrency_);
  return ret;
}

std::string OrderInfo::getPosKeyOfAsk() const {
  const auto ret = fmt::format(  // add stgGrpId
      "{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}/{}", productGrpId_,
      productId_, userId_, acctGrpId_, acctId_, trdAcctId_, stgGrpId_, stgId_,
      stgInstId_, algoId_, GetMarketName(marketCode_),
      magic_enum::enum_name(symbolType_), symbolCode_,
      magic_enum::enum_name(Side::Ask), magic_enum::enum_name(posSide_),
      parValue_, feeCurrency_);
  return ret;
}

Decimal OrderInfo::calcFee(const FeeInfoCacheSPtr& feeInfoCache,
                           const OpenedContractGroupSPtr& openedContractGroup,
                           Decimal dealAmt) {
  Decimal parValue = parValue_;
  PosDirection posDirection = PosDirection::Close;
  if (marketCode_ == MarketCode::SSE || marketCode_ == MarketCode::SZSE) {
    parValue = 1;
    if (side_ == Side::Bid) {
      //! feeInfo表中现货是 Bid + Open
      posDirection = PosDirection::Open;
    } else {
      //! feeInfo表中现货是 Ask + Close
      posDirection = PosDirection::Close;
    }

  } else if (marketCode_ == MarketCode::SHFE ||
             marketCode_ == MarketCode::INE) {
    //! 上期和原油所 posDirection 和原始订单中的 posDirection_ 一致
    posDirection = posDirection_;
    if (posDirection_ == PosDirection::CloseYDay) {
      posDirection = PosDirection::Close;
    }

  } else if (marketCode_ == MarketCode::CZCE ||
             marketCode_ == MarketCode::DCE ||
             marketCode_ == MarketCode::CFFEX) {
    //! 郑商所、大商所、中金所
    if (posDirection_ == PosDirection::Open) {
      //! 如果是开仓那么都按照开仓的手续费计算
      posDirection = posDirection_;
    } else {
      //! 如果是平仓
      posDirection = PosDirection::Close;
      if (openedContractGroup) {
        auto openedTDay =
            openedContractGroup->haveOpenedTDay<LockFunc::True>(this);
        if (openedTDay == OpenedTDay::True) {
          posDirection = PosDirection::CloseTDay;
        }
      }
    }
  } else {
    LOG_W("MarketCode {} that cannot calc fees. {}", GetMarketName(marketCode_),
          this->toShortStr());
    return 0;
  }

  //! 根据上面的逻辑，posDirection 只能是 Open Close CloseTDay
  const auto ret =
      feeInfoCache->calcFee(this, posDirection, dealAmt * parValue);
  return ret;
}

OrderInfoSPtr MakeOrderInfo(const db::orderInfo::RecordSPtr& recOrderInfo) {
  auto orderInfo = std::make_shared<OrderInfo>();

  orderInfo->productGrpId_ = recOrderInfo->productGrpId;
  orderInfo->productId_ = recOrderInfo->productId;
  orderInfo->userId_ = recOrderInfo->userId;
  orderInfo->acctGrpId_ = recOrderInfo->acctGrpId;
  orderInfo->acctId_ = recOrderInfo->acctId;
  orderInfo->trdAcctId_ = recOrderInfo->trdAcctId;
  orderInfo->stgGrpId_ = recOrderInfo->stgGrpId;  // add stgGrpId
  orderInfo->stgId_ = recOrderInfo->stgId;
  orderInfo->stgInstId_ = recOrderInfo->stgInstId;
  orderInfo->algoId_ = recOrderInfo->algoId;

  if (!recOrderInfo->orderId.empty()) {
    orderInfo->orderId_ = CONV(OrderId, recOrderInfo->orderId);
  }
  if (!recOrderInfo->exchOrderId.empty()) {
    strncpy(orderInfo->exchOrderId_, recOrderInfo->exchOrderId.c_str(),
            MAX_EXCH_ORDER_ID_LEN - 1);
  }
  if (!recOrderInfo->parentOrderId.empty()) {
    orderInfo->parentOrderId_ = CONV(OrderId, recOrderInfo->parentOrderId);
  }

  orderInfo->marketCode_ = GetMarketCode(recOrderInfo->marketCode);
  orderInfo->symbolType_ =
      magic_enum::enum_cast<SymbolType>(recOrderInfo->symbolType).value();
  strncpy(orderInfo->symbolCode_, recOrderInfo->symbolCode.c_str(),
          sizeof(orderInfo->symbolCode_) - 1);
  strncpy(orderInfo->exchSymbolCode_, recOrderInfo->exchSymbolCode.c_str(),
          sizeof(orderInfo->exchSymbolCode_) - 1);

  orderInfo->side_ = magic_enum::enum_cast<Side>(recOrderInfo->side).value();
  orderInfo->posDirection_ =
      magic_enum::enum_cast<PosDirection>(recOrderInfo->posDirection).value();
  orderInfo->posSide_ =
      magic_enum::enum_cast<PosSide>(recOrderInfo->posSide).value();

  orderInfo->orderPrice_ = CONV(Decimal, recOrderInfo->orderPrice);
  orderInfo->orderSize_ = CONV(Decimal, recOrderInfo->orderSize);

  orderInfo->parValue_ = recOrderInfo->parValue;

  orderInfo->orderType_ =
      magic_enum::enum_cast<OrderType>(recOrderInfo->orderType).value();
  orderInfo->orderTypeExtra_ =
      magic_enum::enum_cast<OrderTypeExtra>(recOrderInfo->orderTypeExtra)
          .value();
  orderInfo->closeTDayStg_ =
      magic_enum::enum_cast<CloseTDayStg>(recOrderInfo->closeTDayStg).value();

  orderInfo->orderTime_ = ConvertDBTimeToTS(recOrderInfo->orderTime);

  orderInfo->fee_ = CONV(Decimal, recOrderInfo->fee);
  strncpy(orderInfo->feeCurrency_, recOrderInfo->feeCurrency.c_str(),
          sizeof(orderInfo->feeCurrency_) - 1);

  orderInfo->dealSize_ = CONV(Decimal, recOrderInfo->dealSize);
  orderInfo->avgDealPrice_ = CONV(Decimal, recOrderInfo->avgDealPrice);

  strncpy(orderInfo->lastTradeId_, recOrderInfo->lastTradeId.c_str(),
          sizeof(orderInfo->lastTradeId_) - 1);
  orderInfo->lastDealPrice_ = CONV(Decimal, recOrderInfo->lastDealPrice);
  orderInfo->lastDealSize_ = CONV(Decimal, recOrderInfo->lastDealSize);
  orderInfo->lastDealTime_ = ConvertDBTimeToTS(recOrderInfo->lastDealTime);

  orderInfo->orderStatus_ =
      magic_enum::enum_cast<OrderStatus>(recOrderInfo->orderStatus).value();

  orderInfo->noUsedToCalcPos_ = recOrderInfo->noUsedToCalcPos;
  orderInfo->statusCode_ = recOrderInfo->statusCode;

  return orderInfo;
}

OrderInfoSPtr MakeOrderInfo(const db::tradeInfo::RecordSPtr& recTradeInfo) {
  auto orderInfo = std::make_shared<OrderInfo>();

  orderInfo->productGrpId_ = recTradeInfo->productGrpId;
  orderInfo->productId_ = recTradeInfo->productId;
  orderInfo->userId_ = recTradeInfo->userId;
  orderInfo->acctGrpId_ = recTradeInfo->acctGrpId;
  orderInfo->acctId_ = recTradeInfo->acctId;
  orderInfo->trdAcctId_ = recTradeInfo->trdAcctId;
  orderInfo->stgGrpId_ = recTradeInfo->stgGrpId;  // add stgGrpId
  orderInfo->stgId_ = recTradeInfo->stgId;
  orderInfo->stgInstId_ = recTradeInfo->stgInstId;
  orderInfo->algoId_ = recTradeInfo->algoId;

  if (!recTradeInfo->orderId.empty()) {
    orderInfo->orderId_ = CONV(OrderId, recTradeInfo->orderId);
  }
  if (!recTradeInfo->exchOrderId.empty()) {
    strncpy(orderInfo->exchOrderId_, recTradeInfo->exchOrderId.c_str(),
            MAX_EXCH_ORDER_ID_LEN - 1);
  }
  if (!recTradeInfo->parentOrderId.empty()) {
    orderInfo->parentOrderId_ = CONV(OrderId, recTradeInfo->parentOrderId);
  }

  orderInfo->marketCode_ = GetMarketCode(recTradeInfo->marketCode);
  orderInfo->symbolType_ =
      magic_enum::enum_cast<SymbolType>(recTradeInfo->symbolType).value();
  strncpy(orderInfo->symbolCode_, recTradeInfo->symbolCode.c_str(),
          sizeof(orderInfo->symbolCode_) - 1);
  strncpy(orderInfo->exchSymbolCode_, recTradeInfo->exchSymbolCode.c_str(),
          sizeof(orderInfo->exchSymbolCode_) - 1);

  orderInfo->side_ = magic_enum::enum_cast<Side>(recTradeInfo->side).value();
  orderInfo->posDirection_ =
      magic_enum::enum_cast<PosDirection>(recTradeInfo->posDirection).value();
  orderInfo->posSide_ =
      magic_enum::enum_cast<PosSide>(recTradeInfo->posSide).value();

  orderInfo->orderPrice_ = CONV(Decimal, recTradeInfo->orderPrice);
  orderInfo->orderSize_ = CONV(Decimal, recTradeInfo->orderSize);

  orderInfo->parValue_ = recTradeInfo->parValue;

  orderInfo->orderType_ =
      magic_enum::enum_cast<OrderType>(recTradeInfo->orderType).value();
  orderInfo->orderTypeExtra_ =
      magic_enum::enum_cast<OrderTypeExtra>(recTradeInfo->orderTypeExtra)
          .value();
  orderInfo->closeTDayStg_ =
      magic_enum::enum_cast<CloseTDayStg>(recTradeInfo->closeTDayStg).value();

  orderInfo->orderTime_ = ConvertDBTimeToTS(recTradeInfo->orderTime);

  orderInfo->fee_ = CONV(Decimal, recTradeInfo->fee);
  strncpy(orderInfo->feeCurrency_, recTradeInfo->feeCurrency.c_str(),
          sizeof(orderInfo->feeCurrency_) - 1);

  orderInfo->dealSize_ = CONV(Decimal, recTradeInfo->dealSize);
  orderInfo->avgDealPrice_ = CONV(Decimal, recTradeInfo->avgDealPrice);

  strncpy(orderInfo->lastTradeId_, recTradeInfo->lastTradeId.c_str(),
          sizeof(orderInfo->lastTradeId_) - 1);
  orderInfo->lastDealPrice_ = CONV(Decimal, recTradeInfo->lastDealPrice);
  orderInfo->lastDealSize_ = CONV(Decimal, recTradeInfo->lastDealSize);
  orderInfo->lastDealTime_ = ConvertDBTimeToTS(recTradeInfo->lastDealTime);

  orderInfo->orderStatus_ =
      magic_enum::enum_cast<OrderStatus>(recTradeInfo->orderStatus).value();

  orderInfo->noUsedToCalcPos_ = recTradeInfo->noUsedToCalcPos;
  orderInfo->statusCode_ = recTradeInfo->statusCode;

  return orderInfo;
}

OrderInfoSPtr MakeOrderInfo(const StgInstInfoSPtr& stgInstInfo,
                            ProductGrpId productGrpId, StgGrpId stgGrpId,
                            AcctGrpId acctGrpId, AcctId acctId,
                            TrdAcctId trdAcctId, MarketCode marketCode,
                            const std::string& symbolCode, Side side,
                            PosDirection posDirection, Decimal orderPrice,
                            Decimal orderSize, CloseTDayStg closeTDayStg,
                            AlgoId algoId, const std::string& simedTDInfo) {
  const auto now = GetTotalUSSince1970();

  auto orderInfoPtr = static_cast<OrderInfo*>(
      calloc(1, sizeof(OrderInfo) + simedTDInfo.size() + 1));
  std::shared_ptr<OrderInfo> orderInfo(orderInfoPtr);

  orderInfo->orderTime_ = now;

  orderInfo->productGrpId_ = productGrpId;
  orderInfo->stgGrpId_ = stgGrpId;
  orderInfo->acctGrpId_ = acctGrpId;

  orderInfo->productId_ = stgInstInfo->productId_;
  orderInfo->userId_ = stgInstInfo->userId_;
  orderInfo->acctId_ = acctId;
  orderInfo->trdAcctId_ = trdAcctId;

  orderInfo->stgId_ = stgInstInfo->stgId_;
  orderInfo->stgInstId_ = stgInstInfo->stgInstId_;
  orderInfo->algoId_ = algoId;

  orderInfo->marketCode_ = marketCode;
  orderInfo->symbolType_ = SymbolType::Others;
  strncpy(orderInfo->symbolCode_, symbolCode.c_str(),
          sizeof(orderInfo->symbolCode_) - 1);

  orderInfo->side_ = side;
  orderInfo->posDirection_ = posDirection;
  orderInfo->posSide_ = PosSide::Both;

  orderInfo->orderPrice_ = orderPrice;
  orderInfo->orderSize_ = orderSize;

  orderInfo->orderType_ = OrderType::Limit;
  orderInfo->orderTypeExtra_ = OrderTypeExtra::Normal;
  orderInfo->closeTDayStg_ = closeTDayStg;

  orderInfo->lastDealTime_ = UNDEFINED_FIELD_MIN_TS;
  orderInfo->orderStatus_ = OrderStatus::Created;

  orderInfo->extDataLen_ = simedTDInfo.size() + 1;
  strcpy(orderInfo->extData_, simedTDInfo.c_str());

  return orderInfo;
}

std::uint64_t OrderInfo::getHashOfSymbolInfo() const {
  auto symbolInfo =
      fmt::format("{}-{}-{}", magic_enum::enum_name(marketCode_),
                  magic_enum::enum_name(symbolType_), symbolCode_);
  const auto hash = XXH3_64bits(symbolInfo.data(), symbolInfo.size());
  return hash;
}

bool OrderInfo::isRealOrder() const {
  if (extDataLen_ == 0) {
    return true;
  }

  Doc doc;
  if (doc.Parse(extData_).HasParseError()) {
    return true;
  }

  const auto extMsgId = doc["m"].GetString();
  if (std::strcmp(extMsgId, EXT_MSG_ID_SIMED_TD.c_str()) == 0) {
    return false;
  }

  return true;
}

}  // namespace bq
