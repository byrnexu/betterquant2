/*!
 * \file PosMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "PosMgrUtil.hpp"
#include "db/DBEng.hpp"
#include "db/TBLPosInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"
#include "def/ConditionDef.hpp"
#include "def/ConditionUtil.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfAssets.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Decimal.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
}  // namespace bq

namespace bq {

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct TagMainOfPM {};
struct KeyMainOfPM
    : boost::multi_index::composite_key<
          PosInfo, MIDX_MEMBER(PosInfo, std::uint64_t, keyHash_)> {};
using MIdxMainOfPM = boost::multi_index::ordered_unique<
    boost::multi_index::tag<TagMainOfPM>, KeyMainOfPM,
    boost::multi_index::composite_key_result_less<KeyMainOfPM ::result_type>>;

//! acctId + marketCode + symbolType + symbolCode 联合索引，
//! 用于查找某个品种的开仓数量信息
struct TagAcctIdAndSymInfoOfPM {};
struct KeyAcctIdAndSymInfoOfPM
    : boost::multi_index::composite_key<
          PosInfo, MIDX_MEMBER(PosInfo, AcctId, acctId_),
          MIDX_MEMBER(PosInfo, MarketCode, marketCode_),
          MIDX_MEMBER(PosInfo, SymbolType, symbolType_),
          MIDX_MEMBER(PosInfo, std::uint64_t, hashOfSymbolCode_)> {};
using MIdxAcctIdAndSymInfoOfPM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagAcctIdAndSymInfoOfPM>, KeyAcctIdAndSymInfoOfPM,
    boost::multi_index::composite_key_result_less<
        KeyAcctIdAndSymInfoOfPM ::result_type>>;

//! StgInstId 层面索引
struct TagStgInstIdOfPM {};
struct KeyStgInstIdOfPM
    : boost::multi_index::composite_key<
          PosInfo, MIDX_MEMBER(PosInfo, ProductId, productId_),
          MIDX_MEMBER(PosInfo, UserId, userId_),
          MIDX_MEMBER(PosInfo, StgId, stgId_),
          MIDX_MEMBER(PosInfo, StgInstId, stgInstId_)> {};
using MIdxStgInstIdOfPM = boost::multi_index::ordered_non_unique<
    boost::multi_index::tag<TagStgInstIdOfPM>, KeyStgInstIdOfPM,
    boost::multi_index::composite_key_result_less<
        KeyStgInstIdOfPM ::result_type>>;

template <typename... IndexTypes>
class PosMgr {
  using PosInfoTable = boost::multi_index::multi_index_container<
      PosInfoSPtr, boost::multi_index::indexed_by<IndexTypes...>>;
  using PosInfoTableSPtr = std::shared_ptr<PosInfoTable>;

 public:
  PosMgr(const PosMgr&) = delete;
  PosMgr& operator=(const PosMgr&) = delete;
  PosMgr(const PosMgr&&) = delete;
  PosMgr& operator=(const PosMgr&&) = delete;

  PosMgr();

 public:
  int init(const YAML::Node& node, const db::DBEngSPtr& dbEng,
           const std::string& sql);

  void setSyncToDB(SyncToDB value) { syncToDB_ = value; }

 private:
  int initPosInfoTable(const std::string& sql);

 public:
  template <LockFunc lockFunc>
  PosChgInfoSPtr updateByOrderInfoFromTDGW(const OrderInfoSPtr& orderInfo);

 private:
  PosChgInfoSPtr updateByOrderInfo(const OrderInfoSPtr& orderInfo);

  PosChgInfoSPtr updateByOrderInfoSinglePosSide(const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoSinglePosSideOfBid(
      const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoSinglePosSideOfAsk(
      const OrderInfoSPtr& orderInfo);

  PosChgInfoSPtr updateByOrderInfoDoublePosSide(const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoDoublePosSideOfBidLong(
      const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoDoublePosSideOfAskLong(
      const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoDoublePosSideOfAskShort(
      const OrderInfoSPtr& orderInfo);
  PosChgInfoSPtr updateByOrderInfoDoublePosSideOfBidShort(
      const OrderInfoSPtr& orderInfo);

 private:
  Decimal recalcAvgOpenPrice(const OrderInfoSPtr& orderInfo,
                             const PosInfoSPtr& origPosInfo, Decimal newPos);

  Decimal calcCurPnlRealOfCloseShort(const OrderInfoSPtr& orderInfo,
                                     const PosInfoSPtr& origPosInfo,
                                     Decimal newPos);

  Decimal calcCurPnlRealOfCloseLong(const OrderInfoSPtr& orderInfo,
                                    const PosInfoSPtr& origPosInfo,
                                    Decimal newPos);

 public:
  template <LockFunc lockFunc>
  PosInfoGroup getPosInfoGroup() const;

  template <LockFunc lockFunc, DeepClone deepClone>
  PosInfoGroup getPosInfoGroup(AcctId acctId, MarketCode marketCode,
                               SymbolType symbolType,
                               const std::string& symbolCode) const;

  template <LockFunc lockFunc, DeepClone deepClone>
  PosInfoGroup getPosInfoGroupOfStgInst(
      const StgInstInfoSPtr& stgInstInfo) const;

 public:
  template <LockFunc lockFunc, DeepClone deepClone>
  PosInfoGroup getPosInfoGroup(
      const ConditionTemplate& ConditionTemplate,
      const ConditionFieldGroup& conditionFieldGroup) const;

 public:
  template <LockFunc lockFunc>
  Decimal getHoldPos(const OrderInfoSPtr& orderInfo, AcctId acctId);

 public:
  std::string toStr() const;

 public:
  void syncToDB(const PosInfoSPtr& posInfo);

 public:
  YAML::Node& getNode() { return node_; }

 private:
  void addPosInfo(const PosInfoSPtr& posInfo);

 private:
  YAML::Node node_;

  db::DBEngSPtr dbEng_{nullptr};
  SyncToDB syncToDB_{SyncToDB::True};

  PosInfoTableSPtr posInfoTable_{nullptr};
  mutable std::ext::spin_mutex mtxPosInfoTable_;
};

template <typename... IndexTypes>
using PosMgrSPtr = std::shared_ptr<PosMgr<IndexTypes...>>;

template <typename... IndexTypes>
PosMgr<IndexTypes...>::PosMgr()
    : posInfoTable_(std::make_shared<PosInfoTable>()) {}

template <typename... IndexTypes>
int PosMgr<IndexTypes...>::init(const YAML::Node& node,
                                const db::DBEngSPtr& dbEng,
                                const std::string& sql) {
  node_ = node;
  dbEng_ = dbEng;

  const auto ret = initPosInfoTable(sql);
  if (ret != 0) {
    LOG_W("Init failed. [{}]", sql);
    return ret;
  }
  return 0;
}

template <typename... IndexTypes>
int PosMgr<IndexTypes...>::initPosInfoTable(const std::string& sql) {
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLPosInfo>::ExecSql(dbEng_, sql);
  if (ret != 0) {
    LOG_W("Init pos info group failed. {}", sql);
    return ret;
  }

  for (const auto& tblRec : *tblRecSet) {
    const auto recPosInfo = tblRec.second->getRecWithAllFields();
    const auto posInfo = MakePosInfo(recPosInfo);
    addPosInfo(posInfo);
  }
  LOG_I("Init pos info group success. [size = {}]", posInfoTable_->size());

  return 0;
}

template <typename... IndexTypes>
std::string PosMgr<IndexTypes...>::toStr() const {
  std::vector<std::string> strGrp;
  for (const auto& rec : *posInfoTable_) {
    strGrp.emplace_back(rec->toStr());
  }
  std::sort(strGrp.begin(), strGrp.end());

  std::string ret;
  for (const auto& str : strGrp) {
    ret = ret + "\n" + str;
  }
  return ret;
}

// clang-format off
//
//! 单边持仓的情况下，每个品种只有两条记录，只有一条记录 pos 不为零
//! -------------------------------------------------------------------------------
//! | Bid Both  | totalBidSize_ > 0 = 累计做多数 | totalAskSize_ < 0 = 累计做空数 | 
//! | Ask Both  | totalAskSize_ < 0 = 累计做空数 | totalBidSize_ > 0 = 累计做多数 |
//! -------------------------------------------------------------------------------

//! 双边持仓的情况下，每个品种只有两条记录
//! -------------------------------------------------------------------------------
//! | Bid Long  | totalBidSize_ > 0 = 累计开仓数 | totalAskSize_ < 0 = 累计平仓数 | 
//! | Ask Short | totalAskSize_ < 0 = 累计开仓数 | totalBidSize_ > 0 = 累计平仓数 |
//! -------------------------------------------------------------------------------
//
// clang-format on

template <typename... IndexTypes>
template <LockFunc lockFunc>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoFromTDGW(
    const OrderInfoSPtr& orderInfo) {
  //! 只有全成和部成才会引起仓位变化，这里必须注意的是任何仓位变化消息只能发送
  //! 一次(全成/部成），否则会引起重复计算，tdSvc的OrdMgr模块确保了有变化才推送
  if (orderInfo->orderStatus_ != OrderStatus::PartialFilled &&
      orderInfo->orderStatus_ != OrderStatus::Filled) {
    LOG_W("Posmgr recv rtn of order status {}",
          magic_enum::enum_name(orderInfo->orderStatus_));
    return std::make_shared<PosChgInfo>();
  }
  //!
  //! 如果是现货
  //!
  //! 目前是双向持仓和单边持仓都支持，关键在于下单的时候选择Both还是Long/Short，目前
  //! 使用Long/Short模式，这样可以知道历史上买入和卖出的总量，净头寸也可以由两者相加
  //! 得到，为了实现这一点，凡是下的现货单，都是属于开仓，也就是说只有买多和卖空两种
  //! 组合，这个在下单的入口点也就是策略引擎的下单接口中强行指定。以后可以考虑由一个
  //! 策略参数或者账户层面的配置指定。
  //!
  //! 如果Long/Short模式下仓位以及pnl和净头寸都要临时计算，所以默认改成了Both.
  //!
  //! 测试发现头寸虽然是净值，但是手续费却是累加的，感觉不大合理，所以最终还是用
  //! Long/Short.
  //!
  //! RiskMgr推送PosInfo的时候，如果用Long/Short模式还是要累加后推送，因此采用Both.
  //!
  //! ==========================================================================
  //!
  //! 如果是u本位合约单边持仓模式：
  //!
  //! 如果是u本位合约bid：
  //! fee会增加，pos会增加或者减少，avgOpenPrice会变化，pnlReal会变，updateTime会变化
  //! bid会导致pos减少的原因是，当目前持有的是空头仓位的情况下，bid会导致空头仓位减少，
  //! pnlReal也会产生变化
  //!
  //! 如果是u本位合约ask：
  //! fee会增加，pos会减少或者增加，avgOpenPrice会变化，pnlReal会变，updateTime会变化
  //! ask会导致pos增加的原因是，当目前持有的是空头仓位的情况下，bid会导致空头仓位增加，
  //! pnlReal也会产生变化
  //!
  //! --------------------------------------------------------------------------
  //! 如果是u本位合约双向持仓模式：
  //!
  //! 如果是u本位合约bid long (买多)：
  //! fee会增加，多头pos会增加，avgOpenPrice会变化，pnlReal不会变，updateTime会变化
  //! 如果是u本位合约ask long (卖多)：
  //! fee会增加，多头pos会减少，avgOpenPrice不会变化，pnlReal会变，updateTime会变化
  //!
  //! 如果是u本位合约ask short (卖空)：
  //! fee会增加，空头pos会增加，avgOpenPrice会变化，pnlReal不会变，updateTime会变化
  //! 如果是u本位合约bid short (买空)：
  //! fee会增加，空头pos会减少，avgOpenPrice不会变化，pnlReal会变，updateTime会变化
  //!
  //! ==========================================================================
  //! 如果是币本位合约和u本位合约一样，只是持仓均价计算公式不同
  //!

  {
    SPIN_LOCK(mtxPosInfoTable_);

    if (orderInfo->symbolType_ == SymbolType::Spot ||
        orderInfo->symbolType_ == SymbolType::Perp ||
        orderInfo->symbolType_ == SymbolType::Futures ||
        orderInfo->symbolType_ == SymbolType::CPerp ||
        orderInfo->symbolType_ == SymbolType::CFutures ||
        orderInfo->symbolType_ == SymbolType::CN_MainBoard ||
        orderInfo->symbolType_ == SymbolType::CN_SecondBoard ||
        orderInfo->symbolType_ == SymbolType::CN_StartupBoard ||
        orderInfo->symbolType_ == SymbolType::CN_TechBoard ||
        orderInfo->symbolType_ == SymbolType::CN_Futures) {
      return updateByOrderInfo(orderInfo);

    } else {
      LOG_W("Unhandled symbolType {}.",
            magic_enum::enum_name(orderInfo->symbolType_));
      return std::make_shared<PosChgInfo>();
    }
  }
}

template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfo(
    const OrderInfoSPtr& orderInfo) {
  if (orderInfo->posSide_ == PosSide::Both) {
    //! 如果是单边持仓模式
    return updateByOrderInfoSinglePosSide(orderInfo);

  } else if (orderInfo->posSide_ == PosSide::Long ||
             orderInfo->posSide_ == PosSide::Short) {
    //! 如果是双向持仓模式
    return updateByOrderInfoDoublePosSide(orderInfo);

  } else {
    return std::make_shared<PosChgInfo>();
    LOG_W("Unhandled posSide {}.", magic_enum::enum_name(orderInfo->posSide_));
  }
}

template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoSinglePosSide(
    const OrderInfoSPtr& orderInfo) {
  if (orderInfo->side_ == Side::Bid) {
    //! 如果是单边持仓买入
    return updateByOrderInfoSinglePosSideOfBid(orderInfo);

  } else if (orderInfo->side_ == Side::Ask) {
    //! 如果是单边持仓卖出
    return updateByOrderInfoSinglePosSideOfAsk(orderInfo);

  } else {
    return std::make_shared<PosChgInfo>();
    LOG_W("Unhandled side {}.", magic_enum::enum_name(orderInfo->posSide_));
  }
}

//! 如果是单边持仓买入
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoSinglePosSideOfBid(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  PosInfoSPtr origPosInfoOfBid;
  PosInfoSPtr origPosInfoOfAsk;
  auto& idx = posInfoTable_->template get<TagMainOfPM>();

  //! 获取当前合约多头仓位，如果没有生成一条0仓位记录
  const auto posKeyOfBid = orderInfo->getPosKeyOfBid();
  const auto posKeyOfBidHash =
      XXH3_64bits(posKeyOfBid.data(), posKeyOfBid.size());
  auto iterBid = idx.find(posKeyOfBidHash);
  if (iterBid != std::end(idx)) {
    origPosInfoOfBid = *iterBid;
  } else {
    origPosInfoOfBid = MakePosInfoOfContractWithKeyFields(orderInfo, Side::Bid);
    addPosInfo(origPosInfoOfBid);
  }

  //! 获取当前合约空头仓位，如果没有生成一条0仓位记录
  const auto posKeyOfAsk = orderInfo->getPosKeyOfAsk();
  const auto posKeyOfAskHash =
      XXH3_64bits(posKeyOfAsk.data(), posKeyOfAsk.size());
  auto iterAsk = idx.find(posKeyOfAskHash);
  if (iterAsk != std::end(idx)) {
    origPosInfoOfAsk = *iterAsk;
  } else {
    origPosInfoOfAsk = MakePosInfoOfContractWithKeyFields(orderInfo, Side::Ask);
    addPosInfo(origPosInfoOfAsk);
  }

  //! 确定目前仓位是多头还是空头
  const auto origPosSide =
      origPosInfoOfBid->pos_ >= (-1.0 * origPosInfoOfAsk->pos_)
          ? PosSide::Long
          : PosSide::Short;

  if (origPosSide == PosSide::Long) {
    //! 如果目前持有的是多头仓位
    const auto newPos = origPosInfoOfBid->pos_ + orderInfo->lastDealSize_;
    origPosInfoOfBid->avgOpenPrice_ =
        recalcAvgOpenPrice(orderInfo, origPosInfoOfBid, newPos);
    origPosInfoOfBid->pos_ = newPos;
    origPosInfoOfBid->fee_ += orderInfo->getFeeOfLastTrade();
    origPosInfoOfBid->totalBidSize_ += orderInfo->lastDealSize_;
    if (orderInfo->posDirection_ == PosDirection::Open) {
      origPosInfoOfBid->totalOpenSize_ += orderInfo->lastDealSize_;
    }
    origPosInfoOfBid->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
    ret->emplace_back(origPosInfoOfBid);

  } else {
    //! 如果目前持有的是空头仓位
    auto newPos = origPosInfoOfAsk->pos_ + orderInfo->lastDealSize_;
    if (DEC::ZERO(newPos)) newPos = 0;
    if (newPos <= 0) {
      //! bid之后不会产生多头仓位，那么空头仓位的fee、pos、pnlReal会发生变化
      const auto curPnlReal =
          calcCurPnlRealOfCloseShort(orderInfo, origPosInfoOfAsk, newPos);
      origPosInfoOfAsk->pnlReal_ += curPnlReal;
      origPosInfoOfAsk->pos_ = newPos;
      origPosInfoOfAsk->fee_ += orderInfo->getFeeOfLastTrade();
      origPosInfoOfAsk->totalBidSize_ += orderInfo->lastDealSize_;
      if (orderInfo->posDirection_ == PosDirection::Open) {
        origPosInfoOfAsk->totalOpenSize_ += orderInfo->lastDealSize_;
      }
      origPosInfoOfAsk->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfAsk);

    } else {
      //! bid之后会产生多头仓位，那么空头仓位清零并计算pnl，多头产生仓位
      const auto curPnlRealOfAsk =
          calcCurPnlRealOfCloseShort(orderInfo, origPosInfoOfAsk, newPos);
      origPosInfoOfAsk->pnlReal_ += curPnlRealOfAsk;
      origPosInfoOfAsk->pos_ = 0;
      origPosInfoOfAsk->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfAsk);

      origPosInfoOfBid->avgOpenPrice_ = orderInfo->lastDealPrice_;
      origPosInfoOfBid->pos_ = newPos;
      origPosInfoOfBid->fee_ += orderInfo->getFeeOfLastTrade();
      origPosInfoOfAsk->totalBidSize_ += orderInfo->lastDealSize_;
      if (orderInfo->posDirection_ == PosDirection::Open) {
        origPosInfoOfAsk->totalOpenSize_ += orderInfo->lastDealSize_;
      }
      origPosInfoOfBid->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfBid);
    }
  }

  return ret;
}

//! 如果是单边持仓卖出
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoSinglePosSideOfAsk(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  PosInfoSPtr origPosInfoOfBid;
  PosInfoSPtr origPosInfoOfAsk;
  auto& idx = posInfoTable_->template get<TagMainOfPM>();

  //! 获取当前合约多头仓位，如果没有生成一条0仓位记录
  const auto posKeyOfBid = orderInfo->getPosKeyOfBid();
  const auto posKeyOfBidHash =
      XXH3_64bits(posKeyOfBid.data(), posKeyOfBid.size());
  auto iterBid = idx.find(posKeyOfBidHash);
  if (iterBid != std::end(idx)) {
    origPosInfoOfBid = *iterBid;
  } else {
    origPosInfoOfBid = MakePosInfoOfContractWithKeyFields(orderInfo, Side::Bid);
    addPosInfo(origPosInfoOfBid);
  }

  //! 获取当前合约空头仓位，如果没有生成一条0仓位记录
  const auto posKeyOfAsk = orderInfo->getPosKeyOfAsk();
  const auto posKeyOfAskHash =
      XXH3_64bits(posKeyOfAsk.data(), posKeyOfAsk.size());
  auto iterAsk = idx.find(posKeyOfAskHash);
  if (iterAsk != std::end(idx)) {
    origPosInfoOfAsk = *iterAsk;
  } else {
    origPosInfoOfAsk = MakePosInfoOfContractWithKeyFields(orderInfo, Side::Ask);
    addPosInfo(origPosInfoOfAsk);
  }

  //! 确定目前仓位是多头还是空头
  const auto origPosSide =
      (-1.0 * origPosInfoOfAsk->pos_) >= origPosInfoOfBid->pos_ ? PosSide::Short
                                                                : PosSide::Long;

  if (origPosSide == PosSide::Short) {
    //! 如果目前持有的是空头仓位
    const auto newPos = origPosInfoOfAsk->pos_ + orderInfo->lastDealSize_;
    origPosInfoOfAsk->avgOpenPrice_ =
        recalcAvgOpenPrice(orderInfo, origPosInfoOfAsk, newPos);
    origPosInfoOfAsk->pos_ = newPos;
    origPosInfoOfAsk->fee_ += orderInfo->getFeeOfLastTrade();
    origPosInfoOfAsk->totalAskSize_ += orderInfo->lastDealSize_;
    if (orderInfo->posDirection_ == PosDirection::Open) {
      origPosInfoOfAsk->totalOpenSize_ += orderInfo->lastDealSize_;
    }
    origPosInfoOfAsk->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
    ret->emplace_back(origPosInfoOfAsk);

  } else {
    //! 如果目前持有的是多头仓位
    auto newPos = origPosInfoOfBid->pos_ + orderInfo->lastDealSize_;
    if (DEC::ZERO(newPos)) newPos = 0;
    if (newPos >= 0) {
      //! ask之后不会产生空头仓位，那么多头仓位的fee、pos、pnlReal会发生变化
      const auto curPnlReal =
          calcCurPnlRealOfCloseLong(orderInfo, origPosInfoOfBid, newPos);
      origPosInfoOfBid->pnlReal_ += curPnlReal;
      origPosInfoOfBid->pos_ = newPos;
      origPosInfoOfBid->fee_ += orderInfo->getFeeOfLastTrade();
      origPosInfoOfBid->totalAskSize_ += orderInfo->lastDealSize_;
      if (orderInfo->posDirection_ == PosDirection::Open) {
        origPosInfoOfBid->totalOpenSize_ += orderInfo->lastDealSize_;
      }
      origPosInfoOfBid->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfBid);

    } else {
      //! ask之后会产生空头仓位，那么多头仓位清零并计算pnl，空头产生仓位
      const auto curPnlRealOfBid =
          calcCurPnlRealOfCloseLong(orderInfo, origPosInfoOfBid, newPos);
      origPosInfoOfBid->pnlReal_ += curPnlRealOfBid;
      origPosInfoOfBid->pos_ = 0;
      origPosInfoOfBid->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfBid);

      origPosInfoOfAsk->avgOpenPrice_ = orderInfo->lastDealPrice_;
      origPosInfoOfAsk->pos_ = newPos;
      origPosInfoOfAsk->fee_ += orderInfo->getFeeOfLastTrade();
      origPosInfoOfBid->totalAskSize_ += orderInfo->lastDealSize_;
      if (orderInfo->posDirection_ == PosDirection::Open) {
        origPosInfoOfBid->totalOpenSize_ += orderInfo->lastDealSize_;
      }
      origPosInfoOfAsk->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
      ret->emplace_back(origPosInfoOfAsk);
    }
  }

  return ret;
}

//! 如果是双向持仓模式
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoDoublePosSide(
    const OrderInfoSPtr& orderInfo) {
  if (orderInfo->posSide_ == PosSide::Long) {
    if (orderInfo->side_ == Side::Bid) {
      //! 买入开多
      return updateByOrderInfoDoublePosSideOfBidLong(orderInfo);

    } else if (orderInfo->side_ == Side::Ask) {
      //! 卖出平多
      return updateByOrderInfoDoublePosSideOfAskLong(orderInfo);

    } else {
      return std::make_shared<PosChgInfo>();
      LOG_W("Unhandled side {}.", magic_enum::enum_name(orderInfo->side_));
    }

  } else if (orderInfo->posSide_ == PosSide::Short) {
    if (orderInfo->side_ == Side::Ask) {
      //! 卖出开空
      return updateByOrderInfoDoublePosSideOfAskShort(orderInfo);

    } else if (orderInfo->side_ == Side::Bid) {
      //! 买入平空
      return updateByOrderInfoDoublePosSideOfBidShort(orderInfo);

    } else {
      return std::make_shared<PosChgInfo>();
      LOG_W("Unhandled side {}.", magic_enum::enum_name(orderInfo->side_));
    }

  } else {
    return std::make_shared<PosChgInfo>();
    LOG_W("Unhandled posSide {}.", magic_enum::enum_name(orderInfo->side_));
  }
}

//! 双边持仓 买入开多
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoDoublePosSideOfBidLong(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  //! 先获取目前仓位
  auto& idx = posInfoTable_->template get<TagMainOfPM>();
  const auto posKey = orderInfo->getPosKey();
  const auto posKeyHash = XXH3_64bits(posKey.data(), posKey.size());
  auto iter = idx.find(posKeyHash);
  //! 如果是全新的仓位
  if (iter == std::end(idx)) {
    const auto posInfo = MakePosInfoOfContract(orderInfo);
    addPosInfo(posInfo);
    //！双边持仓 买入开多 totalBidSize_ 在这里体现为当前累计开多仓数量
    posInfo->totalBidSize_ += orderInfo->lastDealSize_;
    posInfo->totalOpenSize_ += orderInfo->lastDealSize_;
    posInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
    ret->emplace_back(posInfo);
    return ret;
  }

  //! 如果有老仓，开始计算新的posInfo
  auto& origPosInfo = *iter;
  const auto newPos = origPosInfo->pos_ + orderInfo->lastDealSize_;
  origPosInfo->fee_ = origPosInfo->fee_ + orderInfo->getFeeOfLastTrade();
  //！双边持仓 买入开多 totalBidSize_ 在这里体现为当前累计开多仓数量
  origPosInfo->totalBidSize_ += orderInfo->lastDealSize_;
  origPosInfo->totalOpenSize_ += orderInfo->lastDealSize_;
  origPosInfo->avgOpenPrice_ =
      recalcAvgOpenPrice(orderInfo, origPosInfo, newPos);
  origPosInfo->pos_ = newPos;
  origPosInfo->updateTime_ = GetTotalUSSince1970();
  origPosInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
  ret->emplace_back(origPosInfo);

  return ret;
}

//! 双边持仓 卖出平多
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoDoublePosSideOfAskLong(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  //! 先获取目前多头仓位
  auto& idx = posInfoTable_->template get<TagMainOfPM>();
  const auto posKeyOfBid = orderInfo->getPosKeyOfBid();
  const auto posKeyOfBidHash =
      XXH3_64bits(posKeyOfBid.data(), posKeyOfBid.size());
  auto iter = idx.find(posKeyOfBidHash);
  if (iter == std::end(idx) || (*iter)->pos_ == 0) {
    LOG_W("Can not find long pos when ask long. {}", posKeyOfBid);
    return ret;
  }

  auto& origPosInfo = *iter;

  //! 开始计算新的posInfo，因为lastDealSize_<0所以下面是加
  auto newPos = origPosInfo->pos_ + orderInfo->lastDealSize_;
  if (newPos < 0) {  // like -3.469446951953614e-18 or invalid value
    LOG_I("Long pos {} less than 0 after ask long. {}", newPos, posKeyOfBid);
    //! 目前的逻辑是，置零后如果有超出当前仓位的不正常回报还会继续算，但是下一
    //! 次进入此函数发现仓位为0就不再处理了
    newPos = 0;
  }
  origPosInfo->fee_ = origPosInfo->fee_ + orderInfo->getFeeOfLastTrade();
  //! 双边持仓 卖出平多 totalAskSize_ 体现为当前累计平多仓数量
  origPosInfo->totalAskSize_ += orderInfo->lastDealSize_;
  //! 计算被平的多头仓位的已实现盈亏
  const auto pnlReal =
      calcCurPnlRealOfCloseLong(orderInfo, origPosInfo, newPos);
  origPosInfo->pnlReal_ += pnlReal;
  origPosInfo->pos_ = newPos;
  origPosInfo->updateTime_ = GetTotalUSSince1970();
  origPosInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
  ret->emplace_back(origPosInfo);

  return ret;
}

//! 双边持仓 卖出开空
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoDoublePosSideOfAskShort(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  //! 先获取目前仓位
  auto& idx = posInfoTable_->template get<TagMainOfPM>();
  const auto posKey = orderInfo->getPosKey();
  const auto posKeyHash = XXH3_64bits(posKey.data(), posKey.size());
  auto iter = idx.find(posKeyHash);
  if (iter == std::end(idx)) {
    const auto posInfo = MakePosInfoOfContract(orderInfo);
    addPosInfo(posInfo);
    //! 双边持仓 卖出开空 totalAskSize_体现为当前累计开空仓数量
    posInfo->totalAskSize_ += orderInfo->lastDealSize_;
    posInfo->totalOpenSize_ += orderInfo->lastDealSize_;
    posInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
    ret->emplace_back(posInfo);
    return ret;
  }

  //! 开始计算新的posInfo
  auto& origPosInfo = *iter;
  const auto newPos = origPosInfo->pos_ + orderInfo->lastDealSize_;
  origPosInfo->fee_ = origPosInfo->fee_ + orderInfo->getFeeOfLastTrade();
  //! 双边持仓 卖出开空 totalAskSize_体现为当前累计开空仓数量
  origPosInfo->totalAskSize_ += orderInfo->lastDealSize_;
  origPosInfo->totalOpenSize_ += orderInfo->lastDealSize_;
  origPosInfo->avgOpenPrice_ =
      recalcAvgOpenPrice(orderInfo, origPosInfo, newPos);
  origPosInfo->pos_ = newPos;
  origPosInfo->updateTime_ = GetTotalUSSince1970();
  origPosInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
  ret->emplace_back(origPosInfo);

  return ret;
}

//! 买入平空
template <typename... IndexTypes>
PosChgInfoSPtr PosMgr<IndexTypes...>::updateByOrderInfoDoublePosSideOfBidShort(
    const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosChgInfo>();

  //! 先获取目前空头仓位
  auto& idx = posInfoTable_->template get<TagMainOfPM>();
  const auto posKeyOfAsk = orderInfo->getPosKeyOfAsk();
  const auto posKeyOfAskHash =
      XXH3_64bits(posKeyOfAsk.data(), posKeyOfAsk.size());
  auto iter = idx.find(posKeyOfAskHash);
  if (iter == std::end(idx) || (*iter)->pos_ == 0) {
    LOG_W("Can not find short pos when bid short. {} {}", posKeyOfAsk, toStr());
    return ret;
  }

  auto& origPosInfo = *iter;

  //! 开始计算新的posInfo，因为lastDealSize_>0所以下面是加
  auto newPos = origPosInfo->pos_ + orderInfo->lastDealSize_;
  if (newPos > 0) {  // like 3.469446951953614e-18 or invalid value
    LOG_I("Short pos {} greater than 0 after bid short. {}", newPos,
          posKeyOfAsk);
    //! 目前的逻辑是，置零后如果有超出当前仓位的不正常回报还会继续算，但是下一
    //! 次进入此函数发现仓位为0就不再处理了
    newPos = 0;
  }
  origPosInfo->fee_ = origPosInfo->fee_ + orderInfo->getFeeOfLastTrade();
  //! 双边持仓 买入平空 totalBidSize_ 体现为当前累计平空仓数量
  origPosInfo->totalBidSize_ += orderInfo->lastDealSize_;
  //! 计算被平的多头仓位的已实现盈亏
  const auto pnlReal =
      calcCurPnlRealOfCloseShort(orderInfo, origPosInfo, newPos);
  origPosInfo->pnlReal_ += pnlReal;
  origPosInfo->pos_ = newPos;
  origPosInfo->updateTime_ = GetTotalUSSince1970();
  origPosInfo->lastNoUsedToCalcPos_ = orderInfo->noUsedToCalcPos_;
  ret->emplace_back(origPosInfo);

  return ret;
}

//! 重新计算开仓均价
template <typename... IndexTypes>
Decimal PosMgr<IndexTypes...>::recalcAvgOpenPrice(
    const OrderInfoSPtr& orderInfo, const PosInfoSPtr& origPosInfo,
    Decimal newPos) {
  Decimal ret = 0;
  if (orderInfo->symbolType_ == SymbolType::Spot ||
      orderInfo->symbolType_ == SymbolType::CN_MainBoard ||
      orderInfo->symbolType_ == SymbolType::CN_SecondBoard ||
      orderInfo->symbolType_ == SymbolType::CN_StartupBoard ||
      orderInfo->symbolType_ == SymbolType::CN_TechBoard) {
    //! 重新计算现货开仓均价
    ret = (origPosInfo->pos_ * origPosInfo->avgOpenPrice_ +
           orderInfo->lastDealSize_ * orderInfo->lastDealPrice_) /
          newPos;

  } else if (orderInfo->symbolType_ == SymbolType::CN_Futures) {
    //! 重新计算期货开仓均价
    ret = (origPosInfo->pos_ * origPosInfo->avgOpenPrice_ +
           orderInfo->lastDealSize_ * orderInfo->lastDealPrice_) /
          newPos;

  } else if (orderInfo->symbolType_ == SymbolType::Perp ||
             orderInfo->symbolType_ == SymbolType::Futures) {
    //! 重新计算u本位合约开仓均价
    ret = (origPosInfo->pos_ * origPosInfo->avgOpenPrice_ +
           orderInfo->lastDealSize_ * orderInfo->lastDealPrice_) /
          newPos;

  } else if (orderInfo->symbolType_ == SymbolType::CPerp ||
             orderInfo->symbolType_ == SymbolType::CFutures) {
    //! 重新计算币本位合约开仓均价
    if (!DEC::ZERO(origPosInfo->avgOpenPrice_)) {
      ret = newPos / (origPosInfo->pos_ / origPosInfo->avgOpenPrice_ +
                      orderInfo->lastDealSize_ / orderInfo->lastDealPrice_);
    } else {
      ret = newPos / (orderInfo->lastDealSize_ / orderInfo->lastDealPrice_);
    }

  } else {
    LOG_W("Unhandled symbolType {}.",
          magic_enum::enum_name(orderInfo->symbolType_));
  }
  return ret;
}

//! 计算本次平掉的空头仓位产生的已实现盈亏
template <typename... IndexTypes>
Decimal PosMgr<IndexTypes...>::calcCurPnlRealOfCloseShort(
    const OrderInfoSPtr& orderInfo, const PosInfoSPtr& origPosInfo,
    Decimal newPos) {
  //! 被平的空头仓位数量
  const auto closePos =
      newPos > 0 ? (origPosInfo->pos_ * -1) : orderInfo->lastDealSize_;

  const auto ret = calcPnlOfCloseShort(
      orderInfo->symbolType_, origPosInfo->avgOpenPrice_,
      orderInfo->lastDealPrice_, closePos, orderInfo->parValue_);

  return ret;
}

//! 计算本次平掉的多头仓位产生的已实现盈亏
template <typename... IndexTypes>
Decimal PosMgr<IndexTypes...>::calcCurPnlRealOfCloseLong(
    const OrderInfoSPtr& orderInfo, const PosInfoSPtr& origPosInfo,
    Decimal newPos) {
  //! 被平的多头仓位数量
  const auto closePos =
      newPos < 0 ? origPosInfo->pos_ : (orderInfo->lastDealSize_ * -1);

  const auto ret = calcPnlOfCloseLong(
      orderInfo->symbolType_, origPosInfo->avgOpenPrice_,
      orderInfo->lastDealPrice_, closePos, orderInfo->parValue_);

  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc>
PosInfoGroup PosMgr<IndexTypes...>::getPosInfoGroup() const {
  PosInfoGroup ret;
  {
    SPIN_LOCK(mtxPosInfoTable_);
    for (const auto& posInfo : *posInfoTable_) {
      ret.emplace_back(std::make_shared<PosInfo>(*posInfo));
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
PosInfoGroup PosMgr<IndexTypes...>::getPosInfoGroup(
    AcctId acctId, MarketCode marketCode, SymbolType symbolType,
    const std::string& symbolCode) const {
  PosInfoGroup ret;
  const auto hashOfSymbolCode =
      XXH3_64bits(symbolCode.c_str(), symbolCode.size());
  const auto key =
      std::make_tuple(acctId, marketCode, symbolType, hashOfSymbolCode);
  {
    SPIN_LOCK(mtxPosInfoTable_);
    auto& idx = posInfoTable_->template get<TagAcctIdAndSymInfoOfPM>();
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if constexpr (deepClone == DeepClone::True) {
        ret.emplace_back(std::make_shared<PosInfo>(**iter));
      } else {
        ret.emplace_back(*iter);
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
PosInfoGroup PosMgr<IndexTypes...>::getPosInfoGroupOfStgInst(
    const StgInstInfoSPtr& stgInstInfo) const {
  PosInfoGroup ret;
  const auto key =
      std::make_tuple(stgInstInfo->productId_, stgInstInfo->userId_,
                      stgInstInfo->stgId_, stgInstInfo->stgInstId_);
  {
    SPIN_LOCK(mtxPosInfoTable_);
    auto& idx = posInfoTable_->template get<TagStgInstIdOfPM>();
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if constexpr (deepClone == DeepClone::True) {
        ret.emplace_back(std::make_shared<PosInfo>(**iter));
      } else {
        ret.emplace_back(*iter);
      }
    }
  }
  return ret;
}

template <typename... IndexTypes>
template <LockFunc lockFunc, DeepClone deepClone>
PosInfoGroup PosMgr<IndexTypes...>::getPosInfoGroup(
    const ConditionTemplate& conditionTemplate,
    const ConditionFieldGroup& conditionFieldGroup) const {
  PosInfoGroup ret;
  {
    SPIN_LOCK(mtxPosInfoTable_);
    for (const auto& posInfo : *posInfoTable_) {
      //! 根据条件字段得到map类型的conditionValue
      const auto conditionValue =
          CreateConditioValue(posInfo, conditionFieldGroup);
      //! 比较两个map类型conditionValue, conditionTemplate
      const auto [statusCode, statusMsg, matchCond] =
          MatchConditionTemplate(conditionValue, conditionTemplate);
      if (matchCond) {
        if constexpr (deepClone == DeepClone::True) {
          ret.emplace_back(std::make_shared<PosInfo>(*posInfo));
        } else {
          ret.emplace_back(posInfo);
        }
      }
    }
  }
  return ret;
}

//! 返回负数表示有空头可用头寸，返回正数表示有多头可用头寸
template <typename... IndexTypes>
template <LockFunc lockFunc>
Decimal PosMgr<IndexTypes...>::getHoldPos(const OrderInfoSPtr& orderInfo,
                                          AcctId acctId) {
  Decimal ret = 0.0;
  const auto hashOfSymbolCode =
      XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));
  const auto key = std::make_tuple(acctId, orderInfo->marketCode_,
                                   orderInfo->symbolType_, hashOfSymbolCode);
  //! 如果下的多单，那么可借用的是空头头寸，如果下的是空单，那么可借用的是多头头寸
  {
    SPIN_LOCK(mtxPosInfoTable_);
    auto& idx = posInfoTable_->template get<TagAcctIdAndSymInfoOfPM>();
    const auto range = idx.equal_range(key);
    for (auto iter = range.first; iter != range.second; ++iter) {
      const auto& posInfo = *iter;
      //! 因为空头头寸是负数，所以累加出来才是净头寸
      ret += posInfo->pos_;
    }
  }
  return ret;
}

template <typename... IndexTypes>
void PosMgr<IndexTypes...>::syncToDB(const PosInfoSPtr& posInfo) {
  if (syncToDB_ == SyncToDB::False) {
    return;
  }
  const auto identity = GET_RAND_STR();
  const auto sql = posInfo->getSqlOfReplace();
  const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
  if (ret != 0) {
    LOG_W("Replace pos info from db failed. [{}]", sql);
  }
}

template <typename... IndexTypes>
void PosMgr<IndexTypes...>::addPosInfo(const PosInfoSPtr& posInfo) {
  if (posInfo->symbolCode_[0] != '\0') {
    posInfo->hashOfSymbolCode_ =
        XXH3_64bits(posInfo->symbolCode_, strlen(posInfo->symbolCode_));
  }
  posInfoTable_->emplace(posInfo);
}

}  // namespace bq
