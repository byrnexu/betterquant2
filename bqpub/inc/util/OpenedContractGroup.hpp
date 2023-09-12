/*!
 * \file OpenedContractGroup.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/24
 *
 * \brief
 */

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/OrderInfo.hpp"
#include "def/SHMDef.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"
#include "util/SHMUtil.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

struct OpenedContract;
using OpenedContractSPtr = std::shared_ptr<OpenedContract>;

struct OpenedContract {
  AcctId acctId_;
  MarketCode marketCode_;
  SymbolType symbolType_;
  SHMStringSPtr symbolCode_;
  std::uint64_t hashOfSymbolCode_{0};

  std::string toStr() {
    const auto ret = fmt::format(
        "{}-{}-{}-{} hash: {}", acctId_, GetMarketName(marketCode_),
        magic_enum::enum_name(symbolType_), *symbolCode_, hashOfSymbolCode_);
    return ret;
  }
};
using DelType =
    bip::deleter<OpenedContract, bip::managed_shared_memory::segment_manager>;
using OpenedContractSHMSPtr =
    bip::shared_ptr<OpenedContract, VoidAlloc, DelType>;

class OpenedContractGroup;
using OpenedContractGroupSPtr = std::shared_ptr<OpenedContractGroup>;

class OpenedContractGroup {
  inline const static char* NAME_OF_OPEN_CONTRACT_GROUP = "OpenedContractGroup";

  //! acctId + marketCode + symbolType + symbolCode 联合索引
  struct TagSymInfo {};
  struct KeySymInfo
      : boost::multi_index::composite_key<
            OpenedContract, MIDX_MEMBER(OpenedContract, AcctId, acctId_),
            MIDX_MEMBER(OpenedContract, MarketCode, marketCode_),
            MIDX_MEMBER(OpenedContract, SymbolType, symbolType_),
            MIDX_MEMBER(OpenedContract, std::uint64_t, hashOfSymbolCode_)> {};
  using MIdxSymInfo = boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagSymInfo>, KeySymInfo,
      boost::multi_index::composite_key_result_less<KeySymInfo::result_type>>;

  using OpenedContractTable = boost::multi_index::multi_index_container<
      OpenedContractSHMSPtr, boost::multi_index::indexed_by<MIdxSymInfo>,
      bip::managed_shared_memory::allocator<OpenedContractSHMSPtr>::type>;

 public:
  OpenedContractGroup(const OpenedContractGroup&) = delete;
  OpenedContractGroup& operator=(const OpenedContractGroup&) = delete;
  OpenedContractGroup(const OpenedContractGroup&&) = delete;
  OpenedContractGroup& operator=(const OpenedContractGroup&&) = delete;

  explicit OpenedContractGroup() = default;

 public:
  std::vector<std::string> load(
      const std::shared_ptr<bip::managed_shared_memory>& segment);

  template <LockFunc lockFunc>
  void load(const std::string& segmentIdentity, std::size_t len);

 private:
  std::vector<std::string> loadImpl(WriteLog writeLog);

 public:
  template <LockFunc lockFunc>
  OpenedTDay haveOpenedTDay(const OrderInfo* orderInfo);

  template <LockFunc lockFunc>
  std::string saveOpenedContract(const OrderInfo* orderInfo,
                                 WriteLog writeLog = WriteLog::True);

 private:
  std::shared_ptr<bip::managed_shared_memory> segment_{nullptr};

  OpenedContractTable* openedContractTable_{nullptr};
  mutable std::ext::spin_mutex mtxOpenedContractTable_;
};

template <LockFunc lockFunc>
void OpenedContractGroup::load(const std::string& segmentIdentity,
                               std::size_t len) {
  segment_ = std::make_shared<bip::managed_shared_memory>(
      bip::open_or_create, segmentIdentity.c_str(), len);
  {
    SPIN_LOCK(mtxOpenedContractTable_);
    loadImpl(WriteLog::True);
  }
}

//! 判断是否已经有当日已开仓合约
template <LockFunc lockFunc>
OpenedTDay OpenedContractGroup::haveOpenedTDay(const OrderInfo* orderInfo) {
  const auto hashOfSymbolCode =
      XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));
  const auto sym = std::make_tuple(orderInfo->acctId_, orderInfo->marketCode_,
                                   orderInfo->symbolType_, hashOfSymbolCode);
  {
    SPIN_LOCK(mtxOpenedContractTable_);
    auto& idx = openedContractTable_->get<TagSymInfo>();
    const auto iter = idx.find(sym);
    if (iter != std::end(idx)) {
      return OpenedTDay::True;
    }
  }
  return OpenedTDay::False;
}

template <LockFunc lockFunc>
std::string OpenedContractGroup::saveOpenedContract(const OrderInfo* orderInfo,
                                                    WriteLog writeLog) {
  //! 只有下面几个市场才需要记录完结的开仓订单的交易代码
  if (orderInfo->marketCode_ != MarketCode::CZCE &&
      orderInfo->marketCode_ != MarketCode::DCE &&
      orderInfo->marketCode_ != MarketCode::CFFEX) {
    return "";
  }

  //! 如果不是开仓单，那么跳过
  if (orderInfo->posDirection_ != PosDirection::Open) {
    return "";
  }

  //! 只有在订单有部成、全成、部成部撤的情况下才说明今天已经有成交
  if (orderInfo->orderStatus_ != OrderStatus::PartialFilled &&
      orderInfo->orderStatus_ != OrderStatus::Filled &&
      orderInfo->orderStatus_ != OrderStatus::PartialFilledCanceled) {
    return "";
  }

  //! 如果是完结的开仓单，那么记录openedContract
  auto openedContract = bip::make_managed_shared_ptr(
      segment_->construct<OpenedContract>(bip::anonymous_instance)(),
      *segment_);

  openedContract->acctId_ = orderInfo->acctId_;
  openedContract->marketCode_ = orderInfo->marketCode_;
  openedContract->symbolType_ = orderInfo->symbolType_;
  openedContract->symbolCode_ = bip::make_managed_shared_ptr(
      segment_->construct<SHMString>(bip::anonymous_instance)(
          orderInfo->symbolCode_, segment_->get_segment_manager()),
      *segment_);
  openedContract->hashOfSymbolCode_ =
      XXH3_64bits(orderInfo->symbolCode_, strlen(orderInfo->symbolCode_));

  decltype(std::declval<OpenedContractTable>().emplace(openedContract)) ret;
  {
    SPIN_LOCK(mtxOpenedContractTable_);
    ret = openedContractTable_->emplace(openedContract);
  }

  if (!ret.second) {
    return "";
  }

  const auto msg = fmt::format(
      "Record the contracts {} - {} of acct id {} that have been opened today.",
      GetMarketName(orderInfo->marketCode_), orderInfo->symbolCode_,
      orderInfo->acctId_);
  if (writeLog == WriteLog::True) {
    LOG_I(msg);
  }

  return msg;
}

}  // namespace bq
