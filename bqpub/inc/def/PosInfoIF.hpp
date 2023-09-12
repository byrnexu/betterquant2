/*!
 * \file PosInfoIF.hpp
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
#include "util/PchBase.hpp"

namespace bq {

struct SymbolCode;
using SymbolCodeSPtr = std::shared_ptr<SymbolCode>;

struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;

class MarketDataCache;
using MarketDataCacheSPtr = std::shared_ptr<MarketDataCache>;

struct PosInfo;
using PosInfoSPtr = std::shared_ptr<PosInfo>;

using PosInfoGroup = std::vector<PosInfoSPtr>;
using PosInfoGroupSPtr = std::shared_ptr<PosInfoGroup>;

struct PosInfo {
  PosInfo();

  std::uint64_t keyHash_;

  ProductGrpId productGrpId_{0};
  ProductId productId_{0};
  UserId userId_{0};
  AcctGrpId acctGrpId_{0};
  AcctId acctId_{0};
  TrdAcctId trdAcctId_{0};
  StgGrpId stgGrpId_{0};  // add stgGrpId
  StgId stgId_{0};
  StgInstId stgInstId_{0};
  AlgoId algoId_{0};

  MarketCode marketCode_{MarketCode::Others};
  SymbolType symbolType_{SymbolType::Others};
  char symbolCode_[MAX_SYMBOL_CODE_LEN];

  Side side_{Side::Others};
  PosSide posSide_{PosSide::Others};

  std::uint32_t parValue_{0};
  char feeCurrency_[MAX_CURRENCY_LEN];

  Decimal fee_{0};
  Decimal pos_{0};
  Decimal prePos_{0};
  Decimal avgOpenPrice_{0};
  Decimal preAvgOpenPrice_{0};
  Decimal pnlUnReal_{0};
  Decimal pnlReal_{0};
  Decimal totalBidSize_{0};
  Decimal totalAskSize_{0};
  Decimal preTotalBidSize_{0};
  Decimal preTotalAskSize_{0};
  Decimal totalOpenSize_{0};
  Decimal preTotalOpenSize_{0};
  std::uint64_t lastNoUsedToCalcPos_{0};
  std::uint64_t updateTime_;

  std::uint64_t hashOfSymbolCode_{0};

  bool isEqual(const PosInfoSPtr& posInfo);

  PnlSPtr calcPnl(const MarketDataCacheSPtr& marketDataCache,
                  const std::string& quoteCurrency,
                  const std::string& quoteCurrencyForConv,
                  const std::string& quoteCurrencyOfOrig) const;

  std::string toStr() const;

  std::string getKey() const;
  std::string getKeyOfStgInst() const;

  std::string getKeyWithoutFeeCurrency() const;
  std::string getTopicPrefixForSub() const;

  bool oneMoreFeeCurrencyThanInput(const PosInfoSPtr& posInfo) const;

  std::string getSqlOfReplace() const;
  std::string getSqlOfUpdatePnl() const;
  std::string getSqlOfInsert() const;
  std::string getSqlOfUpdate() const;
  std::string getSqlOfDelete() const;
};

using Key2PosInfoGroup = std::map<std::string, PosInfoSPtr>;
using Key2PosInfoGroupSPtr = std::shared_ptr<Key2PosInfoGroup>;
using AcctId2Key2PosInfoGroup = std::map<AcctId, Key2PosInfoGroupSPtr>;
using AcctId2Key2PosInfoGroupSPtr = std::shared_ptr<AcctId2Key2PosInfoGroup>;
bool isEqual(const Key2PosInfoGroupSPtr& lhs, const Key2PosInfoGroupSPtr& rhs);

struct PosUpdateOfAcctIdForPub {
  SHMHeader shmHeader_{MSG_ID_POS_UPDATE_OF_ACCT_ID};
  AcctId acctId_{0};
  std::uint16_t num_;
  char posInfoGroup_[0];
};
using PosUpdateOfAcctIdForPubSPtr = std::shared_ptr<PosUpdateOfAcctIdForPub>;
Key2PosInfoGroup MakePosUpdateOfAcctId(
    const PosUpdateOfAcctIdForPubSPtr& posUpdateOfAcctIdForPub);

struct PosUpdateOfStgIdForPub {
  SHMHeader shmHeader_{MSG_ID_POS_UPDATE_OF_STG_ID};
  StgId stgId_{0};
  std::uint16_t num_;
  char posInfoGroup_[0];
};
using PosUpdateOfStgIdForPubSPtr = std::shared_ptr<PosUpdateOfStgIdForPub>;
Key2PosInfoGroup MakePosUpdateOfStgId(
    const PosUpdateOfStgIdForPubSPtr& posUpdateOfStgIdForPub);

struct PosUpdateOfStgInstIdForPub {
  SHMHeader shmHeader_{MSG_ID_POS_UPDATE_OF_STG_INST_ID};
  StgId stgId_{0};
  StgInstId stgInstId_{0};
  std::uint16_t num_;
  char posInfoGroup_[0];
};
using PosUpdateOfStgInstIdForPubSPtr =
    std::shared_ptr<PosUpdateOfStgInstIdForPub>;
Key2PosInfoGroup MakePosUpdateOfStgInstId(
    const PosUpdateOfStgInstIdForPubSPtr& posUpdateOfStgInstIdForPub);

struct PosUpdateOfAllForPub {
  SHMHeader shmHeader_{MSG_ID_POS_UPDATE_OF_STG_INST_ID};
  std::uint16_t num_;
  char posInfoGroup_[0];
};
using PosUpdateOfAllForPubSPtr = std::shared_ptr<PosUpdateOfAllForPub>;
Key2PosInfoGroup MakePosUpdateOfAll(
    const PosUpdateOfAllForPubSPtr& posUpdateOfAllForPub);

using Key2PosInfoBundle = std::map<std::string, PosInfoGroupSPtr>;
using Key2PosInfoBundleSPtr = std::shared_ptr<Key2PosInfoBundle>;

Decimal CalcAvgOpenPrice(const PosInfoSPtr& lhs, const PosInfoSPtr& rhs);

void MergePosInfoHasNoFeeCurrency(PosInfoGroup& posInfoGroup);

}  // namespace bq
