/*!
 * \file PosSnapshotImpl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/PosSnapshotImpl.hpp"

#include "def/ConditionUtil.hpp"
#include "def/Pnl.hpp"
#include "def/PosInfo.hpp"
#include "def/StatusCode.hpp"
#include "def/SymbolCode.hpp"
#include "util/BQUtil.hpp"
#include "util/Logger.hpp"

namespace bq {

PosSnapshotImpl::PosSnapshotImpl(
    const std::map<std::string, PosInfoSPtr>& posInfoDetail,
    const MarketDataCacheSPtr& marketDataCache)
    : posInfoDetail_(posInfoDetail), marketDataCache_(marketDataCache) {}

const std::map<std::string, PosInfoSPtr>& PosSnapshotImpl::getPosInfoDetail()
    const {
  return posInfoDetail_;
}

std::tuple<int, PnlSPtr> PosSnapshotImpl::queryPnl(
    const std::string& queryCond, const std::string& quoteCurrencyForCalc,
    const std::string& quoteCurrencyForConv,
    const std::string& origQuoteCurrencyOfUBasedContract) {
  const auto keyGroup2Str = [](const auto& key2PnlGroup) {
    std::string ret;
    for (const auto& rec : *key2PnlGroup) {
      ret = ret + rec.first + ", ";
    }
    if (ret.size() > 1) ret = ret.substr(0, ret.length() - 2);
    return ret;
  };

  //! stgId=10000&stgInstId=1 -> stgId&stgInstId
  const auto groupCond = ExtractFieldName(queryCond);
  if (groupCond.empty()) {
    LOG_W("Query pnl by {} failed, maybe the query cond is invalid.",
          queryCond);
    return {SCODE_BQPUB_INVALID_QRY_COND, nullptr};
  }

  const auto [statusCode, key2PnlGroup] =
      queryPnlGroupBy(groupCond, quoteCurrencyForCalc, quoteCurrencyForConv,
                      origQuoteCurrencyOfUBasedContract);
  if (statusCode != 0) {
    return {statusCode, nullptr};
  }

  const auto iter = key2PnlGroup->find(queryCond);
  if (iter != std::end(*key2PnlGroup)) {
    return {0, iter->second};
  }

  LOG_W("Query pnl of {} failed, all pnl is {{{}}}", queryCond,
        keyGroup2Str(key2PnlGroup));

  return {SCODE_BQPUB_PNL_NOT_EXISTS, nullptr};
}

//! key2PnlGroupSPtr = std::map<"stgId=1&stgInstId=1", PnlSPtr>;
std::tuple<int, Key2PnlGroupSPtr> PosSnapshotImpl::queryPnlGroupBy(
    const std::string& groupCond, const std::string& quoteCurrencyForCalc,
    const std::string& quoteCurrencyForConv,
    const std::string& origQuoteCurrencyOfUBasedContract) {
  //! 先从缓存中获取
  const auto iter = cond2Key2PnlGroup_.find(groupCond);
  if (iter != std::end(cond2Key2PnlGroup_)) {
    LOG_D("Query key2PnlGroup of {} from cache success.", groupCond);
    return {0, iter->second};
  }

  //! groupCond = stgId&stgInstId
  const auto [statusCode, key2PosInfoBundle] = queryPosInfoGroupBy(groupCond);
  if (statusCode != 0) {
    return {statusCode, nullptr};
  }

  //! key2PnlGroup = std::map<"stgId=1&stgInstId=1", PnlSPtr>;
  auto key2PnlGroup = std::make_shared<Key2PnlGroup>();
  //! key2PosInfoBundle = std::map<"stgId=1&stgInstId=1", PosInfoBundle vector>;
  for (const auto& rec : *key2PosInfoBundle) {
    auto pnl = std::make_shared<Pnl>();
    pnl->queryCond_ = rec.first;
    pnl->quoteCurrencyForCalc_ = quoteCurrencyForCalc;

    const auto& posInfoGroup = rec.second;
    for (const auto& posInfo : *posInfoGroup) {
      //! 计算当前仓位的pnl也就是curPnl
      const auto curPnl = posInfo->calcPnl(
          marketDataCache_, quoteCurrencyForCalc, quoteCurrencyForConv,
          origQuoteCurrencyOfUBasedContract);
      pnl->fee_ += curPnl->fee_;
      pnl->pnlUnReal_ += curPnl->pnlUnReal_;
      pnl->pnlReal_ += curPnl->pnlReal_;

      if (curPnl->updateTime_ < pnl->updateTime_ || 0 == pnl->updateTime_) {
        pnl->updateTime_ = curPnl->updateTime_;
      }

      if (curPnl->statusCode_ != 0) {
        pnl->statusCode_ = curPnl->statusCode_;
        pnl->symbolGroupForCalc_ = std::move(curPnl->symbolGroupForCalc_);
        LOG_W(
            "Query pnl group by {} failed, "
            "maybe some of the symbolcode below is not sub. [{}] {}",
            groupCond, SymbolCodeGroup2Str(pnl->symbolGroupForCalc_),
            posInfo->toStr());
      }
    }
    const auto& keyOfCond = rec.first;
    key2PnlGroup->emplace(keyOfCond, pnl);
  }

  cond2Key2PnlGroup_[groupCond] = key2PnlGroup;
  return {0, key2PnlGroup};
}

std::tuple<int, PosInfoGroupSPtr> PosSnapshotImpl::queryPosInfoGroup(
    const std::string& queryCond) {
  const auto keyGroup2Str = [](const auto& key2PosInfoBundle) {
    std::string ret;
    for (const auto& rec : *key2PosInfoBundle) {
      ret = ret + rec.first + ", ";
    }
    if (ret.size() > 1) ret = ret.substr(0, ret.length() - 2);
    return ret;
  };

  //! stgId=10000&stgInstId=1 -> stgId&stgInstId
  const auto groupCond = ExtractFieldName(queryCond);
  if (groupCond.empty()) {
    LOG_W("Query posinfo group by {} failed, maybe the query cond is invalid.",
          groupCond);
    return {SCODE_BQPUB_INVALID_QRY_COND, nullptr};
  }

  const auto [statusCode, key2PosInfoBundle] = queryPosInfoGroupBy(groupCond);
  if (statusCode != 0) {
    return {statusCode, nullptr};
  }

  const auto iter = key2PosInfoBundle->find(queryCond);
  if (iter != std::end(*key2PosInfoBundle)) {
    return {0, iter->second};
  }

  LOG_W("Query posinfo by {} failed, all posinfo is {{{}}}", queryCond,
        keyGroup2Str(key2PosInfoBundle));

  return {SCODE_BQPUB_POS_INFO_GROUP_NOT_EXISTS, nullptr};
}

//! groupCond = stgId&stgInstId
//! 返回 {stgId=10000&stgInstId=1, [posInfo]}
std::tuple<int, Key2PosInfoBundleSPtr> PosSnapshotImpl::queryPosInfoGroupBy(
    const std::string& groupCond) {
  //!
  //! fieldNameGroupInCond {stgId, stgInstId}
  //! fieldValueGroupInKey {1, 2, 3, 4, 10000, 1, Binance, Spot ... }
  //!
  //! return stgId=10000&stgInstId=1
  //!
  const auto makeKeyOfCond =
      [this](const std::vector<std::string>& fieldNameGroupInCond,
             const std::vector<std::string>& fieldValueGroupInKey) {
        std::string ret;
        for (const auto& fieldNameInCond : fieldNameGroupInCond) {
          const auto iter = FieldName2NoInPosInfo.find(fieldNameInCond);
          if (iter == std::end(FieldName2NoInPosInfo)) return std::string("");
          const auto fieldValue = fieldValueGroupInKey[iter->second];
          ret = ret + fmt::format("{}={}{}", fieldNameInCond, fieldValue,
                                  SEP_OF_COND_AND);
        }
        if (!ret.empty()) ret.pop_back();
        return ret;
      };

  //! 先从缓存中获取
  const auto iter = cond2Key2PosInfoBundle_.find(groupCond);
  if (iter != std::end(cond2Key2PosInfoBundle_)) {
    LOG_D("Query Key2PosInfoBundle of {} from cache success.", groupCond);
    return {0, iter->second};
  }

  //! 从groupCond(stgId&stgInstId)获取字段名称
  std::vector<std::string> fieldNameGroupInCond;
  boost::algorithm::split(fieldNameGroupInCond, groupCond,
                          boost::is_any_of(SEP_OF_COND_AND));
  if (fieldNameGroupInCond.empty()) {
    LOG_W(
        "Query posinfo group by {} failed "
        "because of the query cond is invalid.",
        groupCond);
    return {SCODE_BQPUB_INVALID_QRY_COND, nullptr};
  }

  auto key2PosInfoBundle = std::make_shared<Key2PosInfoBundle>();
  for (const auto& rec : posInfoDetail_) {
    std::vector<std::string> fieldValueGroupInKey;
    //! key = 1/2/3/4/10000/1...
    const auto& key = rec.first;
    boost::algorithm::split(fieldValueGroupInKey, key, boost::is_any_of("/"));

    //! keyOfCond = stgId=10000&stgInstId=1
    const auto keyOfCond =
        makeKeyOfCond(fieldNameGroupInCond, fieldValueGroupInKey);
    if (keyOfCond.empty()) {
      LOG_W(
          "Query posinfo group by {} failed, "
          "maybe the query cond is invalid.",
          groupCond);
      return {SCODE_BQPUB_INVALID_QRY_COND, nullptr};
    }

    //! 查找stgId=10000&stgInstId=1失败
    if (key2PosInfoBundle->find(keyOfCond) == std::end(*key2PosInfoBundle)) {
      (*key2PosInfoBundle)[keyOfCond] = std::make_shared<PosInfoGroup>();
    }
    (*key2PosInfoBundle)[keyOfCond]->emplace_back(rec.second);
  }

  //! 缓存结果，下次使用
  cond2Key2PosInfoBundle_[groupCond] = key2PosInfoBundle;
  return {0, key2PosInfoBundle};
}

}  // namespace bq
