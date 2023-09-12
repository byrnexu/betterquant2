/*!
 * \file TrdSymbolListMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#pragma once

#include "RiskCtrlDataMgr.hpp"
#include "TrdSymbolDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

class TrdSymbolListMgr : public RiskCtrlDataMgr {
 public:
  TrdSymbolListMgr(const TrdSymbolListMgr&) = delete;
  TrdSymbolListMgr& operator=(const TrdSymbolListMgr&) = delete;
  TrdSymbolListMgr(const TrdSymbolListMgr&&) = delete;
  TrdSymbolListMgr& operator=(const TrdSymbolListMgr&&) = delete;

  using RiskCtrlDataMgr::RiskCtrlDataMgr;

 public:
  std::tuple<int, std::string> init(
      const std::vector<db::trdSymbolList::RecordSPtr>& recSet);

  bool whiteListExistsAndNotInIt(const OrderInfoSPtr& orderInfo);
  bool inTheBlackList(const OrderInfoSPtr& orderInfo);

 private:
  std::tuple<int, std::string> handleTableChgTypeOfAdd(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfDel(const Doc& doc);
  std::tuple<int, std::string> handleTableChgTypeOfChg(const Doc& doc);

  db::trdSymbolList::RecordSPtr makeRec(const Doc& doc);

 private:
  std::tuple<int, std::string> handleCurTrdSymbol(
      const db::trdSymbolList::RecordSPtr& rec);

 private:
  // clang-format off
  //!
  //! key 是 "acctId&trdAcctId" key 就是创建的时候用到，用来归集同一类风控范围
  //! val 是 TrdSymbolListSPtr
  //!
  //! TrdSymbolListSPtr
  //! {
  //!   conditionGroupInStrFmt_ : "acctId&trdAcctId" ,
  //!   conditionFieldGroup_ : ["acctId", "trdAcctId"]
  //!   whiteList_: {"acctId=10000&trdAcctId=100000", { trdSymbol->getHash(), trdSymbol }}
  //!   blackList_: {"acctId=10000&trdAcctId=100000", { trdSymbol->getHash(), trdSymbol }}
  //! }
  //!
  // clang-format on
  std::map<std::string, TrdSymbolListSPtr> key2TrdSymbolListGroup_;
};

}  // namespace bq
