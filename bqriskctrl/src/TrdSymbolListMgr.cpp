/*!
 * \file TrdSymbolListMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 *
 */

#include "TrdSymbolListMgr.hpp"

#include "db/TBLTrdSymbolList.hpp"
#include "def/ConditionUtil.hpp"
#include "def/DataStruOfTD.hpp"
#include "util/Logger.hpp"

namespace bq {

std::tuple<int, std::string> TrdSymbolListMgr::init(
    const std::vector<db::trdSymbolList::RecordSPtr>& recSet) {
  for (const auto& rec : recSet) {
    const auto [statusCode, statusMsg] = handleCurTrdSymbol(rec);
    if (statusCode != 0) {
      return {statusCode, statusMsg};
    }
  }
  return {0, ""};
}

std::tuple<int, std::string> TrdSymbolListMgr::handleCurTrdSymbol(
    const db::trdSymbolList::RecordSPtr& rec) {
  //! 根据数据库配置生成 conditionFieldGroup ["acctId", "trdAcctId"]
  const auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! trdListType说明当前记录是白名单还是黑名单
  const auto trdListType = magic_enum::enum_cast<TrdListType>(rec->trdListType);
  if (!trdListType.has_value()) {
    const auto statusMsg =
        fmt::format("Invalid trdListType {} in db.", rec->trdListType);
    return {-1, statusMsg};
  }

  //! 生成白名单或者黑名单列表中的代码信息
  const auto trdSymbol = std::make_shared<TrdSymbol>(rec);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  const auto iter = key2TrdSymbolListGroup_.find(conditionGroupInStrFmt);
  if (iter == std::end(key2TrdSymbolListGroup_)) {
    //! 创建并初始化 trdSymbolList
    auto trdSymbolList = std::make_shared<TrdSymbolList>();

    //! conditionGroupInStrFmt_ = "acctId&trdAcctId"
    trdSymbolList->conditionGroupInStrFmt_ = conditionGroupInStrFmt;
    // conditionFieldGroup_ = std::vector of length 2, capacity 2 = {"acctId",
    // "trdAcctId"},
    trdSymbolList->conditionFieldGroup_ = conditionFieldGroup;

    Hash2TrdSymbolGroup hash2TrdSymbolGroup;
    hash2TrdSymbolGroup.emplace(trdSymbol->getHash(), trdSymbol);

    switch (trdListType.value()) {
      case TrdListType::White:
        //! rec->condition = acctId=10000&trdAcctId=100000
        trdSymbolList->whiteList_.emplace(rec->condition, hash2TrdSymbolGroup);
        LOG_I("Add {} to white list of {} for {}-{}.", trdSymbol->toStr(),
              rec->condition, step_, threadNo_);
        break;
      case TrdListType::Black:
        //! rec->condition = acctId=10000&trdAcctId=100000
        trdSymbolList->blackList_.emplace(rec->condition, hash2TrdSymbolGroup);
        LOG_I("Add {} to black list of {} for {}-{}.", trdSymbol->toStr(),
              rec->condition, step_, threadNo_);
        break;
      default:
        break;
    }

    //! 将创建和初始化好的 trdSymbolList 放入 key2TrdSymbolListGroup_
    key2TrdSymbolListGroup_.emplace(conditionGroupInStrFmt, trdSymbolList);

  } else {
    //! trdSymbolList = map of acctId&trdAcctId to
    //! trdSymbol 结构体 (包含黑白名单列表）
    auto& trdSymbolList = iter->second;
    switch (trdListType.value()) {
      case TrdListType::White: {
        //! whiteList is map of acctId=1&trdAcctId=10000 to Hash2TrdSymbolGroup
        auto iterOfList = trdSymbolList->whiteList_.find(rec->condition);
        if (iterOfList == std::end(trdSymbolList->whiteList_)) {
          Hash2TrdSymbolGroup hash2TrdSymbolGroup;
          hash2TrdSymbolGroup.emplace(trdSymbol->getHash(), trdSymbol);
          LOG_I("Add {} to white list of {} for {}-{}.", trdSymbol->toStr(),
                rec->condition, step_, threadNo_);
          trdSymbolList->whiteList_.emplace(rec->condition,
                                            hash2TrdSymbolGroup);
        } else {
          auto& hash2TrdSymbolGroup = iterOfList->second;
          hash2TrdSymbolGroup.emplace(trdSymbol->getHash(), trdSymbol);
        }
      } break;

      case TrdListType::Black: {
        //! blackList is map of acctId=1&trdAcctId=10000 to Hash2TrdSymbolGroup
        auto iterOfList = trdSymbolList->blackList_.find(rec->condition);
        if (iterOfList == std::end(trdSymbolList->blackList_)) {
          Hash2TrdSymbolGroup hash2TrdSymbolGroup;
          hash2TrdSymbolGroup.emplace(trdSymbol->getHash(), trdSymbol);
          LOG_I("Add {} to black list of {} for {}-{}.", trdSymbol->toStr(),
                rec->condition, step_, threadNo_);
          trdSymbolList->blackList_.emplace(rec->condition,
                                            hash2TrdSymbolGroup);
        } else {
          auto& hash2TrdSymbolGroup = iterOfList->second;
          hash2TrdSymbolGroup.emplace(trdSymbol->getHash(), trdSymbol);
        }
      } break;

      default:
        break;
    }
  }

  return {0, ""};
}

//! 白名单存在且不在任何一个白名单里才返回false，其他都返回true
bool TrdSymbolListMgr::whiteListExistsAndNotInIt(
    const OrderInfoSPtr& orderInfo) {
  bool whiteListExists = false;
  bool inTheWhiteList = false;

  //! 遍历 key2TrdSymbolListGroup_ 其中 key = acctId&trdAcctId
  for (const auto& rec : key2TrdSymbolListGroup_) {
    const auto& trd2SymbolList = rec.second;

    //! orderInfo 结合条件字段 [acctId, trdAcctId] 得到 condition =
    //! acctId=10000&trdAcctId=100000
    const auto condition = MakeConditioFieldInfoInStrFmt(
        orderInfo.get(), trd2SymbolList->conditionFieldGroup_);

    //! whiteList_ 是 {"acctId=10000&trdAcctId=100000", Hash2TrdSymbolGroup}
    const auto& whiteList = trd2SymbolList->whiteList_;
    const auto iter = whiteList.find(condition);
    if (iter == std::end(whiteList)) {
      //! 说明没有 "acctId=10000&trdAcctId=100000" 的白名单
      continue;
    }

    //! 说明存在 "acctId=10000&trdAcctId=100000" 的白名单
    const auto& hash2TrdSymbolGroup = iter->second;

    //! 白名单为空，也认为不存在
    if (hash2TrdSymbolGroup.empty()) {
      continue;
    } else {
      whiteListExists = true;
    }

    const auto iterSym =
        hash2TrdSymbolGroup.find(orderInfo->getHashOfSymbolInfo());
    if (iterSym == std::end(hash2TrdSymbolGroup)) {
      //! 可能会在其他白名单里，所以这里不返回 true
    } else {
      inTheWhiteList = true;
      break;
    }
  }

  if (inTheWhiteList) {
    return false;
  } else {
    if (whiteListExists) {
      return true;
    } else {
      return false;
    }
  }
}

bool TrdSymbolListMgr::inTheBlackList(const OrderInfoSPtr& orderInfo) {
  //! 遍历 key2TrdSymbolListGroup_ 其中 key = acctId&trdAcctId
  for (const auto& rec : key2TrdSymbolListGroup_) {
    const auto& trd2SymbolList = rec.second;

    //! orderInfo 结合条件字段 [acctId, trdAcctId] 得到 condition =
    //! acctId=10000&trdAcctId=100000
    const auto condition = MakeConditioFieldInfoInStrFmt(
        orderInfo.get(), trd2SymbolList->conditionFieldGroup_);

    //! blackList_ 是 {"acctId=10000&trdAcctId=100000", Hash2TrdSymbolGroup}
    const auto& blackList = trd2SymbolList->blackList_;
    const auto iter = blackList.find(condition);
    if (iter == std::end(blackList)) {
      //! 说明没有 "acctId=10000&trdAcctId=100000" 的黑名单
      continue;
    }

    //! 说明存在 "acctId=10000&trdAcctId=100000" 的黑名单
    const auto& hash2TrdSymbolGroup = iter->second;
    const auto iterSym =
        hash2TrdSymbolGroup.find(orderInfo->getHashOfSymbolInfo());
    if (iterSym != std::end(hash2TrdSymbolGroup)) {
      return true;
    }
  }

  return false;
}

/*
{
        "pluginName": "risk-plugin-trd-symbol-list",
        "tableChgType": "Add",
        "data": {
                "step": "acctId-trdAcctId",
                "condition": "acctId=10000&trdAcctId=100000",
                "marketCode": "SSE",
                "symbolType": "CN_MainBoard",
                "symbolCode": "600028",
                "trdListType": "Black",
                "name": "",
                "updateTime": "2023-03-18 20:55:32.991380"
        }
}
*/
std::tuple<int, std::string> TrdSymbolListMgr::handleTableChgTypeOfAdd(
    const Doc& doc) {
  const auto rec = makeRec(doc);
  return handleCurTrdSymbol(rec);
}

std::tuple<int, std::string> TrdSymbolListMgr::handleTableChgTypeOfDel(
    const Doc& doc) {
  auto rec = makeRec(doc);
  //! conditionFieldGroup ["acctId", "trdAcctId"]
  auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! trdListType说明当前记录是白名单还是黑名单
  const auto trdListType = magic_enum::enum_cast<TrdListType>(rec->trdListType);
  if (!trdListType.has_value()) {
    const auto statusMsg =
        fmt::format("Invalid trdListType {} in db.", rec->trdListType);
    return {-1, statusMsg};
  }

  //! 生成白名单或者黑名单列表中的代码信息
  const auto trdSymbol = std::make_shared<TrdSymbol>(rec);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  const auto iter = key2TrdSymbolListGroup_.find(conditionGroupInStrFmt);
  if (iter == std::end(key2TrdSymbolListGroup_)) {
    const auto statusMsg =
        fmt::format("Can't find rec when handle table chg type of del. {}",
                    conditionGroupInStrFmt);
    return {-1, statusMsg};
  }

  //! trdSymbolList = map of acctId&trdAcctId to
  //! trdSymbolList 结构体 (包含黑白名单列表）
  auto& trdSymbolList = iter->second;
  switch (trdListType.value()) {
    case TrdListType::White: {
      //! whiteList = map of acctId=10000&trdAcctId=100000 to 代码列表
      //! Hash2TrdSymbolGroup
      auto iterOfList = trdSymbolList->whiteList_.find(rec->condition);
      if (iterOfList == std::end(trdSymbolList->whiteList_)) {
        const auto statusMsg = fmt::format(
            "Can't find rec in whitelist when handle table chg type of del. {}",
            rec->condition);
        return {-1, statusMsg};
      }

      auto& hash2TrdSymbolGroup = iterOfList->second;
      const auto symbolInfo = fmt::format("{}-{}-{}", rec->marketCode,
                                          rec->symbolType, rec->symbolCode);
      const auto hashOfSymbolInfo =
          XXH3_64bits(symbolInfo.data(), symbolInfo.size());
      const auto iterOfSym = hash2TrdSymbolGroup.find(hashOfSymbolInfo);
      if (iterOfSym == std::end(hash2TrdSymbolGroup)) {
        const auto statusMsg = fmt::format(
            "Can't find rec in whitelist when handle table chg type of del. {}",
            symbolInfo);
        return {-1, statusMsg};
      }
      hash2TrdSymbolGroup.erase(iterOfSym);
      LOG_I("Del {} from white list of {} for {}-{}.", symbolInfo,
            rec->condition, step_, threadNo_);

    } break;

    case TrdListType::Black: {
      //! blackList = map of acctId=10000&trdAcctId=100000 to 代码列表
      //! Hash2TrdSymbolGroup
      auto iterOfList = trdSymbolList->blackList_.find(rec->condition);
      if (iterOfList == std::end(trdSymbolList->blackList_)) {
        const auto statusMsg = fmt::format(
            "Can't find rec in blacklist "
            "when handle table chg type of del. {}",
            rec->condition);
        return {-1, statusMsg};
      }

      auto& hash2TrdSymbolGroup = iterOfList->second;
      const auto symbolInfo = fmt::format("{}-{}-{}", rec->marketCode,
                                          rec->symbolType, rec->symbolCode);
      const auto hashOfSymbolInfo =
          XXH3_64bits(symbolInfo.data(), symbolInfo.size());
      const auto iterOfSym = hash2TrdSymbolGroup.find(hashOfSymbolInfo);
      if (iterOfSym == std::end(hash2TrdSymbolGroup)) {
        const auto statusMsg = fmt::format(
            "Can't find rec in blacklist "
            "when handle table chg type of del. {}",
            symbolInfo);
        return {-1, statusMsg};
      }
      hash2TrdSymbolGroup.erase(iterOfSym);
      LOG_I("Del {} from black list of {} for {}-{}.", symbolInfo,
            rec->condition, step_, threadNo_);

    } break;

    default:
      break;
  }

  return {0, ""};
}

std::tuple<int, std::string> TrdSymbolListMgr::handleTableChgTypeOfChg(
    const Doc& doc) {
  int statusCode = 0;
  std::string statusMsg;

  std::tie(statusCode, statusMsg) = handleTableChgTypeOfDel(doc);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  return handleTableChgTypeOfAdd(doc);
}

db::trdSymbolList::RecordSPtr TrdSymbolListMgr::makeRec(const Doc& doc) {
  auto rec = std::make_shared<db::trdSymbolList::Record>();
  rec->step = doc["data"]["step"].GetString();
  rec->condition = doc["data"]["condition"].GetString();
  rec->name = doc["data"]["name"].GetString();
  rec->updateTime = doc["data"]["updateTime"].GetString();
  rec->trdListType = doc["data"]["trdListType"].GetString();
  rec->marketCode = doc["data"]["marketCode"].GetString();
  rec->symbolType = doc["data"]["symbolType"].GetString();
  rec->symbolCode = doc["data"]["symbolCode"].GetString();
  return rec;
}

}  // namespace bq
