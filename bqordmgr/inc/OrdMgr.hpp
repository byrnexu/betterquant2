/*!
 * \file OrdMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "db/DBEng.hpp"
#include "db/TBLOrderInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/ConditionDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/OrderInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Datetime.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"
#include "util/PchBase.hpp"

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq {

enum class QryFromClosedOrder { True = 1, False };

class FeeInfoCache;
using FeeInfoCacheSPtr = std::shared_ptr<FeeInfoCache>;

class OpenedContractGroup;
using OpenedContractGroupSPtr = std::shared_ptr<OpenedContractGroup>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

//! OrderId索引
struct TagOrderIdOfOM {};
struct KeyOrderIdOfOM
    : boost::multi_index::composite_key<
          OrderInfo, MIDX_MEMBER(OrderInfo, OrderId, orderId_)> {};
using MIdxOrderIdOfOM = boost::multi_index::ordered_unique<
    boost::multi_index::tag<TagOrderIdOfOM>, KeyOrderIdOfOM,
    boost::multi_index::composite_key_result_less<
        KeyOrderIdOfOM ::result_type>>;

//! MarketCode和ExchOrderId联合索引
struct TagMarketCodeExchOrderIdOfOM {};
struct KeyMarketCodeExchOrderIdOfOM
    : boost::multi_index::composite_key<
          OrderInfo, MIDX_MEMBER(OrderInfo, MarketCode, marketCode_),
          MIDX_MEMBER(OrderInfo, std::uint64_t, hashOfExchOrderId_)> {};
using MIdxMarketCodeExchOrderIdOfOM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagMarketCodeExchOrderIdOfOM>,
    KeyMarketCodeExchOrderIdOfOM,
    boost::multi_index::composite_key_result_less<
        KeyMarketCodeExchOrderIdOfOM ::result_type>>;

//! acctId + marketCode + symbolType + symbolCode + posDirection 联合索引，
//! 用于查找某个品种是否有在途开仓单
struct TagAcctIdAndSymInfoOfOM {};
struct KeyAcctIdAndSymInfoOfOM
    : boost::multi_index::composite_key<
          OrderInfo, MIDX_MEMBER(OrderInfo, AcctId, acctId_),
          MIDX_MEMBER(OrderInfo, MarketCode, marketCode_),
          MIDX_MEMBER(OrderInfo, SymbolType, symbolType_),
          MIDX_MEMBER(OrderInfo, std::uint64_t, hashOfSymbolCode_),
          MIDX_MEMBER(OrderInfo, PosDirection, posDirection_)> {};
using MIdxAcctIdAndSymInfoOfOM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagAcctIdAndSymInfoOfOM>, KeyAcctIdAndSymInfoOfOM,
    boost::multi_index::composite_key_result_less<
        KeyAcctIdAndSymInfoOfOM ::result_type>>;

//! StgInstId 层面索引
struct TagStgInstIdOfOM {};
struct KeyStgInstIdOfOM
    : boost::multi_index::composite_key<
          OrderInfo, MIDX_MEMBER(OrderInfo, ProductId, productId_),
          MIDX_MEMBER(OrderInfo, UserId, userId_),
          MIDX_MEMBER(OrderInfo, StgId, stgId_),
          MIDX_MEMBER(OrderInfo, StgInstId, stgInstId_)> {};
using MIdxStgInstIdOfOM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagStgInstIdOfOM>, KeyStgInstIdOfOM,
    boost::multi_index::composite_key_result_less<
        KeyStgInstIdOfOM ::result_type>>;

//! ClosedTime索引
struct TagClosedTimeOfOM {};
struct KeyClosedTimeOfOM
    : boost::multi_index::composite_key<
          OrderInfo, MIDX_MEMBER(OrderInfo, std::uint64_t, closedTime_)> {};
using MIdxClosedTimeOfOM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagClosedTimeOfOM>, KeyClosedTimeOfOM,
    boost::multi_index::composite_key_result_less<
        KeyClosedTimeOfOM ::result_type>>;

//! AlgoId索引
struct TagAlgoIdOfOM {};
struct KeyAlgoIdOfOM : boost::multi_index::composite_key<
                           OrderInfo, MIDX_MEMBER(OrderInfo, AlgoId, algoId_)> {
};
using MIdxAlgoIdOfOM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagAlgoIdOfOM>, KeyAlgoIdOfOM,
    boost::multi_index::composite_key_result_less<KeyAlgoIdOfOM ::result_type>>;

template <typename... IndexTypes>
class OrdMgr {
  //! 用于存放在途订单的OrderInfoGroup
  using OrderInfoGroup = boost::multi_index::multi_index_container<
      OrderInfoSPtr, boost::multi_index::indexed_by<IndexTypes...>>;
  using OrderInfoGroupSPtr = std::shared_ptr<OrderInfoGroup>;

  //! 用于存放最近若干笔已完结订单的OrderInfoOfClosedGroup
  using OrderInfoOfClosedGroup = boost::multi_index::multi_index_container<
      OrderInfoSPtr,
      boost::multi_index::indexed_by<MIdxOrderIdOfOM,                //
                                     MIdxMarketCodeExchOrderIdOfOM,  //
                                     MIdxClosedTimeOfOM>>;
  using OrderInfoOfClosedGroupSPtr = std::shared_ptr<OrderInfoOfClosedGroup>;

 public:
  OrdMgr(const OrdMgr&) = delete;
  OrdMgr& operator=(const OrdMgr&) = delete;
  OrdMgr(const OrdMgr&&) = delete;
  OrdMgr& operator=(const OrdMgr&&) = delete;

  OrdMgr();

 public:
  int init(const YAML::Node& node, const db::DBEngSPtr& dbEng,
           const std::string& sql);

  void resetMaxSizeOfOrderInfoOfClosedGroup(std::size_t value) {
    if (value < 128) value = 128;
    maxSizeOfOrderInfoOfClosedGroup_ = value;
  }

 private:
  int initOrderInfoGroup(const std::string& sql);

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  int add(const OrderInfoSPtr& orderInfo);

  template <LockFunc lockFunc>
  int remove(OrderId orderId);

 public:
  template <LockFunc lockFunc>
  bool existsOpenPendingOrders(const OrderInfoSPtr& orderInfo);

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  std::tuple<int, OrderInfoSPtr> getOrderInfo(
      OrderId orderId,
      QryFromClosedOrder qryFromClosedOrder = QryFromClosedOrder::False,
      WriteLog writeLog = WriteLog::True);

  template <LockFunc lockFunc, DeepClone deepClone>
  std::tuple<int, OrderInfoSPtr> getOrderInfo(
      MarketCode marketCode, const std::string& exchOrderId,
      QryFromClosedOrder qryFromClosedOrder = QryFromClosedOrder::False,
      WriteLog writeLog = WriteLog::True);

  template <LockFunc lockFunc, DeepClone deepClone>
  std::tuple<int, OrderInfoSPtr> getOrderInfo(
      std::vector<MarketCode> marketCodeGroup, const std::string& exchOrderId,
      QryFromClosedOrder qryFromClosedOrder = QryFromClosedOrder::False);

  template <LockFunc lockFunc, DeepClone deepClone>
  std::vector<OrderInfoSPtr> getOrderInfoGroup(
      std::uint32_t secAgoTheOrderNeedToBeSynced) const;

  template <LockFunc lockFunc, DeepClone deepClone>
  std::vector<OrderInfoSPtr> getOrderInfoGroupOfStgInst(
      const StgInstInfoSPtr& stgInstInfo) const;

 public:
  template <LockFunc lockFunc>
  std::vector<OrderId> getOrderIdGroupOfStg(StgId stgId = 0);

  template <LockFunc lockFunc>
  std::vector<OrderId> getOrderIdGroupOfStgInst(StgInstId stgInstId);

  template <LockFunc lockFunc>
  std::vector<OrderId> getOrderIdGroupOfAlgo(AlgoId algoId);

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  std::vector<OrderInfoSPtr> getOrderInfoGroup(
      const ConditionTemplate& ConditionTemplate,
      const ConditionFieldGroup& conditionFieldGroup) const;

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  std::vector<OrderInfoSPtr> getOrderInfoGroup(
      AcctId acctId, MarketCode marketCode, SymbolType symbolType,
      const std::string& symbolCode) const;

 public:
  template <LockFunc lockFunc>
  Decimal getFrozenPos(const OrderInfoSPtr& orderInfo, AcctId acctId);

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  std::tuple<IsSomeFieldOfOrderUpdated, OrderInfoSPtr>
  updateByOrderInfoFromExch(
      const OrderInfoSPtr& orderInfoFromExch, std::uint64_t noUsedToCalcPos,
      const FeeInfoCacheSPtr& feeInfoCache = nullptr,
      const OpenedContractGroupSPtr& openedContractGroup = nullptr);

  template <LockFunc lockFunc>
  std::tuple<IsTheOrderCanBeUsedCalcPos, OrderInfoSPtr>
  updateByOrderInfoFromTDGW(const OrderInfoSPtr& orderInfoFromTDGW);

  template <LockFunc lockFunc>
  bool compAndCheckIfUpdate(const OrderInfoSPtr& orderInfoFromExch);

 public:
  YAML::Node& getNode() { return node_; }

 private:
  YAML::Node node_;

  db::DBEngSPtr dbEng_{nullptr};

  OrderInfoGroupSPtr orderInfoGroup_{nullptr};
  OrderInfoOfClosedGroupSPtr orderInfoOfClosedGroup_{nullptr};
  std::size_t maxSizeOfOrderInfoOfClosedGroup_{1024};
  mutable std::ext::spin_mutex mtxOrderInfoGroup_;
};

template <typename... IndexTypes>
using OrdMgrSPtr = std::shared_ptr<OrdMgr<IndexTypes...>>;

template <typename... IndexTypes>
OrdMgr<IndexTypes...>::OrdMgr()
    : orderInfoGroup_(std::make_shared<OrderInfoGroup>()),
      orderInfoOfClosedGroup_(std::make_shared<OrderInfoOfClosedGroup>()) {}

template <typename... IndexTypes>
int OrdMgr<IndexTypes...>::init(const YAML::Node& node,
                                const db::DBEngSPtr& dbEng,
                                const std::string& sql) {
  node_ = node;
  dbEng_ = dbEng;

  auto retOfInitOrd = initOrderInfoGroup(sql);
  if (retOfInitOrd != 0) {
    LOG_W("Init failed. [{}]", sql);
    return retOfInitOrd;
  }
  return 0;
}

template <typename... IndexTypes>
int OrdMgr<IndexTypes...>::initOrderInfoGroup(const std::string& sql) {
  auto [retOfMaker, tblRecSet] =
      db::TBLRecSetMaker<TBLOrderInfo>::ExecSql(dbEng_, sql);
  if (retOfMaker != 0) {
    LOG_W("Init order info group failed. {}", sql);
    return retOfMaker;
  }

  for (const auto& tblRec : *tblRecSet) {
    const auto recOrderInfo = tblRec.second->getRecWithAllFields();
    auto orderInfo = MakeOrderInfo(recOrderInfo);
    if (orderInfo->exchOrderId_[0] != '\0') {
      orderInfo->hashOfExchOrderId_ =
          XXH3_64bits(orderInfo->exchOrderId_, strlen(orderInfo->exchOrderId_));
    }
    if (orderInfo->symbolCode_[0] != '\0') {
      orderInfo->hashOfSymbolCode_ =
          XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));
    }
    orderInfoGroup_->emplace(orderInfo);
  }
  LOG_I("Init order info group success. [size = {}]", orderInfoGroup_->size());
  return 0;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
int OrdMgr<IndexTypes...>::add(const OrderInfoSPtr& orderInfo) {
  OrderInfoSPtr orderInfoClone;
  if constexpr (deepClone == DeepClone::True) {
    orderInfoClone = std::make_shared<OrderInfo>(*orderInfo);
  } else {
    orderInfoClone = orderInfo;
  }
  if (orderInfoClone->exchOrderId_[0] != '\0') {
    orderInfoClone->hashOfExchOrderId_ = XXH3_64bits(
        orderInfoClone->exchOrderId_, strlen(orderInfoClone->exchOrderId_));
  }
  decltype(std::declval<OrderInfoGroup>().emplace(orderInfo)) ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    ret = orderInfoGroup_->emplace(orderInfoClone);
  }
  if (!ret.second) {
    LOG_W(
        "Add order info to order info group failed, may be key duplicated. {}",
        orderInfo->toShortStr());
    return SCODE_ORD_MGR_ADD_ORDER_INFO_FAILED;
  }
  return 0;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
int OrdMgr<IndexTypes...>::remove(OrderId orderId) {
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagOrderIdOfOM>();
    const auto iter = idx.find(orderId);
    if (iter != std::end(idx)) {
      LOG_D("Remove order info in order info group. {}", (*iter)->toShortStr());
      idx.erase(iter);
      return 0;
    }
  }
  //! 同步订单应答先于报单http应答回来有两个废单回报就会出现此警告
  LOG_W(
      "Remove order info in order info group failed "
      "because of no order info of order id {} in order info group. ",
      orderId);

  return SCODE_ORD_MGR_REMOVE_ORDER_INFO_FAILED;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
bool OrdMgr<IndexTypes...>::existsOpenPendingOrders(
    const OrderInfoSPtr& orderInfo) {
  const auto hashOfSymbolCode =
      XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));
  const auto key = std::make_tuple(orderInfo->acctId_,      //
                                   orderInfo->marketCode_,  //
                                   orderInfo->symbolType_,  //
                                   hashOfSymbolCode,        //
                                   PosDirection::Open);
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagAcctIdAndSymInfoOfOM>();
    const auto iter = idx.find(key);
    if (iter != std::end(idx)) {
      LOG_W(
          "Exists pending open orders. \ncurrent order: {}\npending order: {}",
          orderInfo->toShortStr(), (*iter)->toShortStr());
      return true;
    }
  }
  return false;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::tuple<int, OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfo(
    OrderId orderId, QryFromClosedOrder qryFromClosedOrder, WriteLog writeLog) {
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagOrderIdOfOM>();
    const auto iter = idx.find(orderId);
    if (iter != std::end(idx)) {
      if constexpr (deepClone == DeepClone::True) {
        return {0, std::make_shared<OrderInfo>(**iter)};
      } else {
        return {0, *iter};
      }
    }

    if (qryFromClosedOrder == QryFromClosedOrder::True) {
      auto& idx = orderInfoOfClosedGroup_->template get<TagOrderIdOfOM>();
      const auto iter = idx.find(orderId);
      if (iter != std::end(idx)) {
        if constexpr (deepClone == DeepClone::True) {
          return {0, std::make_shared<OrderInfo>(**iter)};
        } else {
          return {0, *iter};
        }
      }
    }
  }

  if (writeLog == WriteLog::True) {
    LOG_W(
        "Get order info from order info group failed "
        "because of no order info of order id {} in order info group. ",
        orderId);
  }

  return {SCODE_ORD_MGR_CAN_NOT_FIND_ORDER, nullptr};
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::tuple<int, OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfo(
    MarketCode marketCode, const std::string& exchOrderId,
    QryFromClosedOrder qryFromClosedOrder, WriteLog writeLog) {
  if (exchOrderId.empty()) {
    LOG_W(
        "Get order info from order info group failed "
        "because of empty exchOrderId in query condition. ",
        GetMarketName(marketCode), exchOrderId);
    return {SCODE_ORD_MGR_CAN_NOT_FIND_ORDER, nullptr};
  }

  const auto hashOfExchOrderId =
      XXH3_64bits(exchOrderId.data(), exchOrderId.size());
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagMarketCodeExchOrderIdOfOM>();
    const auto iter = idx.find(std::make_tuple(marketCode, hashOfExchOrderId));
    if (iter != std::end(idx)) {
      if constexpr (deepClone == DeepClone::True) {
        return {0, std::make_shared<OrderInfo>(**iter)};
      } else {
        return {0, *iter};
      }
    }

    if (qryFromClosedOrder == QryFromClosedOrder::True) {
      auto& idx =
          orderInfoOfClosedGroup_->template get<TagMarketCodeExchOrderIdOfOM>();
      const auto iter =
          idx.find(std::make_tuple(marketCode, hashOfExchOrderId));
      if (iter != std::end(idx)) {
        if constexpr (deepClone == DeepClone::True) {
          return {0, std::make_shared<OrderInfo>(**iter)};
        } else {
          return {0, *iter};
        }
      }
    }
  }

  if (writeLog == WriteLog::True) {
    LOG_W(
        "Get order info from order info group failed "
        "because of no order info of {} - {} in order info group. ",
        GetMarketName(marketCode), exchOrderId);
  }

  return {SCODE_ORD_MGR_CAN_NOT_FIND_ORDER, nullptr};
}

//! 连续两次撤单第二次失败返回应答信息中需要包含原始订单信息，由于第一次撤单原始订
//! 单已完结，因此需要通过qryFromClosedOrder从orderInfoOfClosedGroup_获取此信息
template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::tuple<int, OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfo(
    std::vector<MarketCode> marketCodeGroup, const std::string& exchOrderId,
    QryFromClosedOrder qryFromClosedOrder) {
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    for (const auto marketCode : marketCodeGroup) {
      const auto [statusCode, orderInfo] =
          getOrderInfo<LockFunc::False, deepClone>(
              marketCode, exchOrderId, qryFromClosedOrder, WriteLog::False);
      if (statusCode == 0 && orderInfo) {
        return {statusCode, orderInfo};
      }
    }
  }
  LOG_W(
      "Get order info from order info group failed "
      "because of no order info of exchOrderId {} in order info group. ",
      exchOrderId);

  return {SCODE_ORD_MGR_CAN_NOT_FIND_ORDER, nullptr};
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::vector<OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfoGroup(
    std::uint32_t secAgoTheOrderNeedToBeSynced) const {
  const auto now = GetTotalUSSince1970();
  std::vector<OrderInfoSPtr> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    for (const auto& rec : *orderInfoGroup_) {
      const auto td = now - rec->orderTime_;
      if (td > secAgoTheOrderNeedToBeSynced * 1000 * 1000) {
        if constexpr (deepClone == DeepClone::True) {
          ret.emplace_back(std::make_shared<OrderInfo>(*rec));
        } else {
          ret.emplace_back(rec);
        }
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::vector<OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfoGroupOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) const {
  std::vector<OrderInfoSPtr> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagStgInstIdOfOM>();
    const auto key =
        std::make_tuple(stgInstInfo->productId_, stgInstInfo->userId_,
                        stgInstInfo->stgId_, stgInstInfo->stgInstId_);
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if constexpr (deepClone == DeepClone::True) {
        ret.emplace_back(std::make_shared<OrderInfo>(**iter));
      } else {
        ret.emplace_back(*iter);
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
std::vector<OrderId> OrdMgr<IndexTypes...>::getOrderIdGroupOfStg(StgId stgId) {
  std::vector<OrderId> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    for (const auto& orderInfo : *orderInfoGroup_) {
      if (stgId == 0) {
        ret.emplace_back(orderInfo->orderId_);
      } else {
        if (orderInfo->stgId_ == stgId) {
          ret.emplace_back(orderInfo->orderId_);
        }
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
std::vector<OrderId> OrdMgr<IndexTypes...>::getOrderIdGroupOfStgInst(
    StgInstId stgInstId) {
  std::vector<OrderId> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    for (const auto& orderInfo : *orderInfoGroup_) {
      if (orderInfo->stgInstId_ == stgInstId) {
        ret.emplace_back(orderInfo->orderId_);
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
std::vector<OrderId> OrdMgr<IndexTypes...>::getOrderIdGroupOfAlgo(
    AlgoId algoId) {
  std::vector<OrderId> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagAlgoIdOfOM>();
    const auto range = idx.equal_range(algoId);
    for (auto iter = range.first; iter != range.second; ++iter) {
      ret.emplace_back((*iter)->orderId_);
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::vector<OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfoGroup(
    AcctId acctId, MarketCode marketCode, SymbolType symbolType,
    const std::string& symbolCode) const {
  std::vector<OrderInfoSPtr> ret;
  const auto hashOfSymbolCode =
      XXH3_64bits(symbolCode.c_str(), symbolCode.size());
  const auto key = std::make_tuple(acctId, marketCode, symbolType,
                                   hashOfSymbolCode, PosDirection::Open);
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagAcctIdAndSymInfoOfOM>();
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if constexpr (deepClone == DeepClone::True) {
        ret.emplace_back(std::make_shared<OrderInfo>(**iter));
      } else {
        ret.emplace_back(*iter);
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
Decimal OrdMgr<IndexTypes...>::getFrozenPos(const OrderInfoSPtr& orderInfo,
                                            AcctId acctId) {
  Decimal ret = 0;

  const auto hashOfSymbolCode =
      XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));
  const auto key =
      std::make_tuple(acctId, orderInfo->marketCode_, orderInfo->symbolType_,
                      hashOfSymbolCode, PosDirection::Close);
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    auto& idx = orderInfoGroup_->template get<TagAcctIdAndSymInfoOfOM>();
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      const auto& orderInfoInOrdMgr = *iter;
      //! 平多才算多头头寸冻结，平空才算空头头寸冻结
      if (orderInfoInOrdMgr->posSide_ != orderInfo->posSide_) {
        continue;
      }
      ret += orderInfoInOrdMgr->getUndealedSize();
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::tuple<IsSomeFieldOfOrderUpdated, OrderInfoSPtr>
OrdMgr<IndexTypes...>::updateByOrderInfoFromExch(
    const OrderInfoSPtr& orderInfoFromExch, std::uint64_t noUsedToCalcPos,
    const FeeInfoCacheSPtr& feeInfoCache,
    const OpenedContractGroupSPtr& openedContractGroup) {
  IsSomeFieldOfOrderUpdated isTheOrderInfoUpdated =
      IsSomeFieldOfOrderUpdated::False;

  IsTheKeyFieldOfOrderUpdated isTheKeyFieldOfOrderUpdated =
      IsTheKeyFieldOfOrderUpdated::False;

  OrderInfoSPtr orderInfoInOrdMgr = nullptr;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    if (orderInfoFromExch->orderId_ != 0) {
      //! 根据 orderId 获取订单
      auto& idx = orderInfoGroup_->template get<TagOrderIdOfOM>();
      auto iter = idx.find(orderInfoFromExch->orderId_);
      if (iter != std::end(idx)) {
        orderInfoInOrdMgr = *iter;

        //! 那么更新此订单
        std::tie(isTheOrderInfoUpdated, isTheKeyFieldOfOrderUpdated) =
            orderInfoInOrdMgr->updateByOrderInfoFromExch(
                orderInfoFromExch, noUsedToCalcPos, feeInfoCache,
                openedContractGroup);

        //! 如果订单完结那么移动到最近完结的订单列表中
        if (orderInfoInOrdMgr->closed()) {
          (*iter)->closedTime_ = GetTotalUSSince1970();
          orderInfoOfClosedGroup_->emplace(*iter);
          if (orderInfoOfClosedGroup_->size() >
              maxSizeOfOrderInfoOfClosedGroup_) {
            auto& idxOfClosed =
                orderInfoOfClosedGroup_->template get<TagClosedTimeOfOM>();
            idxOfClosed.erase(std::begin(idxOfClosed));
          }
          idx.erase(iter);
        } else {
          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idx.replace(iter, orderInfoInOrdMgr);
          }
        }

      } else {
        //! 继续尝试从已完结订单列表中获取
        auto& idxOfClosed =
            orderInfoOfClosedGroup_->template get<TagOrderIdOfOM>();
        auto iterOfClosed = idxOfClosed.find(orderInfoFromExch->orderId_);
        if (iterOfClosed != std::end(idxOfClosed)) {
          orderInfoInOrdMgr = *iterOfClosed;
          LOG_I("Get order info from order info of closed group. {}",
                orderInfoFromExch->toShortStr());

          //! 那么更新此订单
          std::tie(isTheOrderInfoUpdated, isTheKeyFieldOfOrderUpdated) =
              orderInfoInOrdMgr->updateByOrderInfoFromExch(
                  orderInfoFromExch, noUsedToCalcPos, feeInfoCache,
                  openedContractGroup);

          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idxOfClosed.replace(iterOfClosed, orderInfoInOrdMgr);
          }

        } else {
          LOG_I(
              "Update by order info from exch failed, may be the rtn trade"
              "of orders out of order and severly delayed. {}",
              orderInfoFromExch->toShortStr());
          return {isTheOrderInfoUpdated, nullptr};
        }
      }

      if constexpr (deepClone == DeepClone::True) {
        return {isTheOrderInfoUpdated,
                std::make_shared<OrderInfo>(*orderInfoInOrdMgr)};
      } else {
        return {isTheOrderInfoUpdated, orderInfoInOrdMgr};
      }

    } else if (orderInfoFromExch->marketCode_ != MarketCode::Others &&
               orderInfoFromExch->exchOrderId_[0] != '\0') {
      //! 根据 marketCode 和 exchOrderId 获取订单
      auto& idx = orderInfoGroup_->template get<TagMarketCodeExchOrderIdOfOM>();
      const auto hashOfExchOrderId =
          XXH3_64bits(orderInfoFromExch->exchOrderId_,
                      strlen(orderInfoFromExch->exchOrderId_));
      const auto iter = idx.find(
          std::make_tuple(orderInfoFromExch->marketCode_, hashOfExchOrderId));
      if (iter != std::end(idx)) {
        orderInfoInOrdMgr = *iter;

        //! 那么更新此订单
        std::tie(isTheOrderInfoUpdated, isTheKeyFieldOfOrderUpdated) =
            orderInfoInOrdMgr->updateByOrderInfoFromExch(
                orderInfoFromExch, noUsedToCalcPos, feeInfoCache,
                openedContractGroup);

        //! 如果订单完结那么移动到最近完结的订单列表中
        if (orderInfoInOrdMgr->closed()) {
          (*iter)->closedTime_ = GetTotalUSSince1970();
          orderInfoOfClosedGroup_->emplace(*iter);
          if (orderInfoOfClosedGroup_->size() >
              maxSizeOfOrderInfoOfClosedGroup_) {
            auto& idxOfClosed =
                orderInfoOfClosedGroup_->template get<TagClosedTimeOfOM>();
            idxOfClosed.erase(std::begin(idxOfClosed));
          }
          idx.erase(iter);
        } else {
          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idx.replace(iter, orderInfoInOrdMgr);
          }
        }

      } else {
        //! 继续尝试从已完结订单列表中获取
        auto& idxOfClosed = orderInfoOfClosedGroup_
                                ->template get<TagMarketCodeExchOrderIdOfOM>();
        auto iterOfClosed = idxOfClosed.find(
            std::make_tuple(orderInfoFromExch->marketCode_, hashOfExchOrderId));
        if (iterOfClosed != std::end(idxOfClosed)) {
          orderInfoInOrdMgr = *iterOfClosed;
          LOG_I("Get order info from order info of closed group. {}",
                orderInfoFromExch->toShortStr());

          //! 那么更新此订单
          std::tie(isTheOrderInfoUpdated, isTheKeyFieldOfOrderUpdated) =
              orderInfoInOrdMgr->updateByOrderInfoFromExch(
                  orderInfoFromExch, noUsedToCalcPos, feeInfoCache,
                  openedContractGroup);

          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idxOfClosed.replace(iterOfClosed, orderInfoInOrdMgr);
          }

        } else {
          LOG_I(
              "Update by order info from exch failed, may be the rtn trade"
              "of orders out of order and severly delayed. {}",
              orderInfoFromExch->toShortStr());
          return {isTheOrderInfoUpdated, nullptr};
        }
      }

      if constexpr (deepClone == DeepClone::True) {
        return {isTheOrderInfoUpdated,
                std::make_shared<OrderInfo>(*orderInfoInOrdMgr)};
      } else {
        return {isTheOrderInfoUpdated, orderInfoInOrdMgr};
      }

    } else {
      return {isTheOrderInfoUpdated, nullptr};
    }
  }
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
std::tuple<IsTheOrderCanBeUsedCalcPos, OrderInfoSPtr>
OrdMgr<IndexTypes...>::updateByOrderInfoFromTDGW(
    const OrderInfoSPtr& orderInfoFromTDGW) {
  IsTheOrderCanBeUsedCalcPos isTheOrderCanBeUsedCalcPos =
      IsTheOrderCanBeUsedCalcPos::False;

  IsTheKeyFieldOfOrderUpdated isTheKeyFieldOfOrderUpdated =
      IsTheKeyFieldOfOrderUpdated::False;

  OrderInfoSPtr orderInfoInOrdMgr = nullptr;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    if (orderInfoFromTDGW->orderId_ != 0) {
      //! 根据 orderId 获取订单
      auto& idx = orderInfoGroup_->template get<TagOrderIdOfOM>();
      auto iter = idx.find(orderInfoFromTDGW->orderId_);
      if (iter != std::end(idx)) {
        orderInfoInOrdMgr = *iter;

        //! 那么更新此订单
        std::tie(isTheOrderCanBeUsedCalcPos, isTheKeyFieldOfOrderUpdated) =
            orderInfoInOrdMgr->updateByOrderInfoFromTDGW(orderInfoFromTDGW);

        //! 如果订单完结那么移动到最近完结的订单列表中
        if (orderInfoInOrdMgr->closed()) {
          (*iter)->closedTime_ = GetTotalUSSince1970();
          orderInfoOfClosedGroup_->emplace(*iter);
          if (orderInfoOfClosedGroup_->size() >
              maxSizeOfOrderInfoOfClosedGroup_) {
            auto& idxOfClosed =
                orderInfoOfClosedGroup_->template get<TagClosedTimeOfOM>();
            idxOfClosed.erase(std::begin(idxOfClosed));
          }
          idx.erase(iter);
        } else {
          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idx.replace(iter, orderInfoInOrdMgr);
          }
        }

      } else {
        //! 继续尝试从已完结订单列表中获取
        auto& idxOfClosed =
            orderInfoOfClosedGroup_->template get<TagOrderIdOfOM>();
        auto iterOfClosed = idxOfClosed.find(orderInfoFromTDGW->orderId_);
        if (iterOfClosed != std::end(idxOfClosed)) {
          orderInfoInOrdMgr = *iterOfClosed;
          LOG_I("Get order info from order info of closed group. {}",
                orderInfoFromTDGW->toShortStr());

          //! 那么更新此订单
          std::tie(isTheOrderCanBeUsedCalcPos, isTheKeyFieldOfOrderUpdated) =
              orderInfoInOrdMgr->updateByOrderInfoFromTDGW(orderInfoFromTDGW);

          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idxOfClosed.replace(iterOfClosed, orderInfoInOrdMgr);
          }

        } else {
          LOG_I(
              "Update by order info from exch failed, may be the rtn trade"
              "of orders out of order and severly delayed. {}",
              orderInfoFromTDGW->toShortStr());
          return {isTheOrderCanBeUsedCalcPos, nullptr};
        }
      }

      return {isTheOrderCanBeUsedCalcPos, orderInfoInOrdMgr};

    } else if (orderInfoFromTDGW->marketCode_ != MarketCode::Others &&
               orderInfoFromTDGW->exchOrderId_[0] != '\0') {
      //! 根据 marketCode 和 exchOrderId 获取订单
      auto& idx = orderInfoGroup_->template get<TagMarketCodeExchOrderIdOfOM>();
      const auto hashOfExchOrderId =
          XXH3_64bits(orderInfoFromTDGW->exchOrderId_,
                      strlen(orderInfoFromTDGW->exchOrderId_));
      const auto iter = idx.find(
          std::make_tuple(orderInfoFromTDGW->marketCode_, hashOfExchOrderId));
      if (iter != std::end(idx)) {
        orderInfoInOrdMgr = *iter;

        //! 那么更新此订单
        std::tie(isTheOrderCanBeUsedCalcPos, isTheKeyFieldOfOrderUpdated) =
            orderInfoInOrdMgr->updateByOrderInfoFromTDGW(orderInfoFromTDGW);

        //! 如果订单完结
        if (orderInfoInOrdMgr->closed()) {
          (*iter)->closedTime_ = GetTotalUSSince1970();
          orderInfoOfClosedGroup_->emplace(*iter);
          if (orderInfoOfClosedGroup_->size() >
              maxSizeOfOrderInfoOfClosedGroup_) {
            auto& idxOfClosed =
                orderInfoOfClosedGroup_->template get<TagClosedTimeOfOM>();
            idxOfClosed.erase(std::begin(idxOfClosed));
          }
          idx.erase(iter);
        } else {
          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idx.replace(iter, orderInfoInOrdMgr);
          }
        }

      } else {
        //! 继续尝试从已完结订单列表中获取
        auto& idxOfClosed = orderInfoOfClosedGroup_
                                ->template get<TagMarketCodeExchOrderIdOfOM>();
        auto iterOfClosed = idxOfClosed.find(
            std::make_tuple(orderInfoFromTDGW->marketCode_, hashOfExchOrderId));
        if (iterOfClosed != std::end(idxOfClosed)) {
          orderInfoInOrdMgr = *iterOfClosed;
          LOG_I("Get order info from order info of closed group. {}",
                orderInfoFromTDGW->toShortStr());

          //! 那么更新此订单
          std::tie(isTheOrderCanBeUsedCalcPos, isTheKeyFieldOfOrderUpdated) =
              orderInfoInOrdMgr->updateByOrderInfoFromTDGW(orderInfoFromTDGW);

          //! 如果索引字段发生变化，那么需要replace
          if (isTheKeyFieldOfOrderUpdated ==
              IsTheKeyFieldOfOrderUpdated::True) {
            idxOfClosed.replace(iterOfClosed, orderInfoInOrdMgr);
          }

        } else {
          LOG_I(
              "Update by order info from exch failed, may be the rtn trade"
              "of orders out of order and severly delayed. {}",
              orderInfoFromTDGW->toShortStr());
          return {isTheOrderCanBeUsedCalcPos, nullptr};
        }
      }

      return {isTheOrderCanBeUsedCalcPos, orderInfoInOrdMgr};

    } else {
      return {isTheOrderCanBeUsedCalcPos, nullptr};
    }
  }

  return {IsTheOrderCanBeUsedCalcPos::False, nullptr};
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
bool OrdMgr<IndexTypes...>::compAndCheckIfUpdate(
    const OrderInfoSPtr& orderInfoFromExch) {
  int statusCode = 0;
  OrderInfoSPtr orderInfoInOrdMgr = nullptr;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    if (orderInfoFromExch->orderId_ != 0) {
      //! 先通过orderInfoFromExch->orderId_查找订单
      std::tie(statusCode, orderInfoInOrdMgr) =
          getOrderInfo<LockFunc::False, DeepClone::False>(
              orderInfoFromExch->orderId_, QryFromClosedOrder::False,
              WriteLog::False);
    }

    //! 如果通过orderId没有获取到订单
    if (!orderInfoInOrdMgr) {
      if (orderInfoFromExch->exchOrderId_[0] != '\0' &&
          orderInfoFromExch->marketCode_ != MarketCode::Others) {
        std::tie(statusCode, orderInfoInOrdMgr) =
            getOrderInfo<LockFunc::False, DeepClone::False>(
                orderInfoFromExch->marketCode_, orderInfoFromExch->exchOrderId_,
                QryFromClosedOrder::False, WriteLog::False);
      }
    }
  }

  if (orderInfoInOrdMgr == nullptr) {
    LOG_W(
        "Get order info from order info group "
        "when compare and check if update by order failed.",
        orderInfoFromExch->toShortStr());
    return false;
  }

  const auto ret = orderInfoInOrdMgr->compAndCheckIfUpdate(orderInfoFromExch);
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
std::vector<OrderInfoSPtr> OrdMgr<IndexTypes...>::getOrderInfoGroup(
    const ConditionTemplate& conditionTemplate,
    const ConditionFieldGroup& conditionFieldGroup) const {
  std::vector<OrderInfoSPtr> ret;
  {
    SPIN_LOCK(mtxOrderInfoGroup_);
    for (const auto& orderInfo : *orderInfoGroup_) {
      //! 根据条件字段得到map类型的conditionValue
      const auto conditionValue =
          CreateConditioValue(orderInfo, conditionFieldGroup);
      //! 比较两个map类型conditionValue, conditionTemplate
      const auto [statusCode, statusMsg, matchCond] =
          MatchConditionTemplate(conditionValue, conditionTemplate);
      if (matchCond) {
        if constexpr (deepClone == DeepClone::True) {
          ret.emplace_back(std::make_shared<OrderInfo>(*orderInfo));
        } else {
          ret.emplace_back(orderInfo);
        }
      }
    }
  }
  return ret;
}

}  // namespace bq
