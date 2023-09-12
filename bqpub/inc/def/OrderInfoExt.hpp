/*!
 * \file OrderInfoExt.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"

namespace bq::db::orderInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;
}  // namespace bq::db::orderInfo

namespace bq::db::tradeInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;
}  // namespace bq::db::tradeInfo

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

OrderInfoSPtr MakeOrderInfo(const db::orderInfo::RecordSPtr& recOrderInfo);
OrderInfoSPtr MakeOrderInfo(const db::tradeInfo::RecordSPtr& recTradeInfo);
OrderInfoSPtr MakeOrderInfo(
    const StgInstInfoSPtr& stgInstInfo, ProductGrpId productGrpId,
    StgGrpId stgGrpId, AcctGrpId acctGrpId, AcctId acctId, TrdAcctId trdAcctId,
    MarketCode marketCode, const std::string& symbolCode, Side side,
    PosDirection posDirection, Decimal orderPrice, Decimal orderSize,
    CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
    AlgoId algoId = 0, const std::string& simedTDInfo = "");

}  // namespace bq
