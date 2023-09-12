/*!
 * \file OrderInfoIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMHeader.hpp"
#include "def/BQConstIF.hpp"
#include "def/BQDefIF.hpp"

namespace bq {

class FeeInfoCache;
using FeeInfoCacheSPtr = std::shared_ptr<FeeInfoCache>;

class OpenedContractGroup;
using OpenedContractGroupSPtr = std::shared_ptr<OpenedContractGroup>;

enum class IsSomeFieldOfOrderUpdated { True = 1, False = 2 };
enum class IsTheKeyFieldOfOrderUpdated { True = 1, False = 2 };
enum class IsTheOrderCanBeUsedCalcPos { True = 1, False = 2 };

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

struct OrderInfo {
  SHMHeader shmHeader_;

  /// 产品组合id
  ProductGrpId productGrpId_{0};
  /// 产品id
  ProductId productId_{0};
  /// 用户id
  UserId userId_{0};
  /// 内部账号组合id
  AcctGrpId acctGrpId_{0};
  /// 内部账号id
  AcctId acctId_{0};
  /// 交易账号id
  TrdAcctId trdAcctId_{0};

  /// 策略组合id
  StgGrpId stgGrpId_{0};  // add stgGrpId
  /// 策略id
  StgId stgId_{0};
  /// 子策略id
  StgInstId stgInstId_{0};
  /// 算法单id
  AlgoId algoId_{0};

  /// 内部订单id
  OrderId orderId_{0};
  /// 交易所订单id
  char exchOrderId_[MAX_EXCH_ORDER_ID_LEN];
  /// 父订单id
  OrderId parentOrderId_{0};

  /// 市场
  MarketCode marketCode_{MarketCode::Others};
  /// 代码类型
  SymbolType symbolType_{SymbolType::Others};
  /// 代码
  char symbolCode_[MAX_SYMBOL_CODE_LEN];
  /// 交易所代码
  char exchSymbolCode_[MAX_SYMBOL_CODE_LEN];

  /// 买卖
  Side side_{Side::Others};
  /// 开平
  PosDirection posDirection_{PosDirection::Others};
  /// 多空
  PosSide posSide_{PosSide::Both};

  /// 下单价格
  Decimal orderPrice_{0};
  /// 下单数量
  Decimal orderSize_{0};

  /// 面值 or 合约乘数
  std::int32_t parValue_{0};

  /// 订单类型，比如：限价单
  OrderType orderType_{OrderType::Limit};
  /// 高级限价单类型
  OrderTypeExtra orderTypeExtra_{OrderTypeExtra::Normal};
  /// 下单策略，默认不允许平今
  CloseTDayStg closeTDayStg_{CloseTDayStg::RejectCloseTDay};
  /// 下单时间
  std::uint64_t orderTime_{0};

  /// 手续费
  Decimal fee_{0};
  /// 手续费币种
  char feeCurrency_[MAX_CURRENCY_LEN];

  /// 成交数量
  Decimal dealSize_{0};
  /// 成交均价
  Decimal avgDealPrice_{0};

  /// 最后一笔成交id
  char lastTradeId_[MAX_TRADE_ID_LEN];
  /// 最后一笔成交价格
  Decimal lastDealPrice_{0};
  /// 最后一笔成交数量
  Decimal lastDealSize_{0};
  /// 最后一笔成交时间
  std::uint64_t lastDealTime_{UNDEFINED_FIELD_MIN_TS};

  /// 订单状态
  OrderStatus orderStatus_{OrderStatus::Created};

  /// 下单结果状态码
  std::int32_t statusCode_{0};

  /// 辅助变量，内部使用
  std::uint64_t noUsedToCalcPos_{0};
  /// 辅助变量，内部使用
  std::uint64_t hashOfSymbolCode_{0};
  /// 辅助变量，内部使用
  std::uint64_t hashOfExchOrderId_{0};
  /// 辅助变量，内部使用
  std::uint64_t closedTime_{0};

  std::uint32_t extDataLen_{0};
  char extData_[0];

  std::tuple<IsSomeFieldOfOrderUpdated, IsTheKeyFieldOfOrderUpdated>
  updateByOrderInfoFromExch(
      const OrderInfoSPtr& newOrderInfo, std::uint64_t noUsedToCalcPos,
      const FeeInfoCacheSPtr& feeInfoCache = nullptr,
      const OpenedContractGroupSPtr& openedContractGroup = nullptr);

  std::tuple<IsTheOrderCanBeUsedCalcPos, IsTheKeyFieldOfOrderUpdated>
  updateByOrderInfoFromTDGW(const OrderInfoSPtr& newOrderInfo);

  bool compAndCheckIfUpdate(const OrderInfoSPtr& newOrderInfo);

  bool closed() const { return orderStatus_ >= OrderStatus::Filled; }
  bool notClosed() const { return !closed(); }

  std::string toShortStr() const;
  std::string toJson() const;

  std::string getSymbolInfo() const;

  std::string getSqlOfReplace() const;
  std::string getSqlOfUSPOrderInfoUpdate() const;
  Decimal getFeeOfLastTrade() const;

  std::string getPosKey() const;
  std::string getPosKeyOfBid() const;
  std::string getPosKeyOfAsk() const;

  Decimal getUndealedSize() const {
    if (side_ == Side::Bid) {
      return orderSize_ - dealSize_;
    } else {
      return orderSize_ + dealSize_;
    }
  }

  std::uint64_t getHashOfSymbolInfo() const;

  bool isRealOrder() const;
  std::size_t size() const { return sizeof(OrderInfo) + extDataLen_; }

 private:
  Decimal calcFee(const FeeInfoCacheSPtr& feeInfoCache,
                  const OpenedContractGroupSPtr& openedContractGroup,
                  Decimal dealAmt);
};

inline OrderInfoSPtr MakeOrderInfo() { return std::make_shared<OrderInfo>(); }

}  // namespace bq
