/*!
 * \file StgEngPY.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/09
 *
 * \brief
 */

#include <boost/python.hpp>
#include <boost/python/suite/indexing/map_indexing_suite.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/utility.hpp>
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>

#include "ArrayIndexingSuite.hpp"
#include "ArrayRef.hpp"
#include "CXX2PYTuple.hpp"
#include "PosMgrOfStgInst.hpp"
#include "StgEng.hpp"
#include "StgEngImpl.hpp"
#include "StgInstTaskHandlerImpl.hpp"
#include "def/AssetInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfStg.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/OrderInfo.hpp"
#include "def/PosInfo.hpp"
#include "def/PosOfStgInst.hpp"
#include "def/SimedTDInfo.hpp"
#include "def/StgInstInfo.hpp"
#include "def/SymbolCode.hpp"
#include "util/PosSnapshot.hpp"

using namespace boost::python;

namespace bq::stg {

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(queryPnlOverloads, queryPnl, 1, 4)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(queryPnlGroupByOverloads,
                                       queryPnlGroupBy, 1, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(orderOverloads, order, 8, 11)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(orderOverloads2, order, 7, 10)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(queryHisMDBetween2TsOverloadsByFields,
                                       queryHisMDBetween2Ts, 7, 8)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(
    querySpecificNumOfHisMDBeforeTsOverloadsByFields,
    querySpecificNumOfHisMDBeforeTs, 7, 8)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(
    querySpecificNumOfHisMDAfterTsOverloadsByFields,
    querySpecificNumOfHisMDAfterTs, 7, 8)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(installStgInstTimerOfFixedTimeOverloads,
                                       installStgInstTimer, 3, 4)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(installStgInstTimerOfIntervalOverloads,
                                       installStgInstTimer, 4, 5)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logTraceOfIntervalOverloads, logTrace, 2,
                                       3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logTraceWithArgsOfIntervalOverloads,
                                       logTrace, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logDebugOfIntervalOverloads, logDebug, 2,
                                       3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logDebugWithArgsOfIntervalOverloads,
                                       logDebug, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logInfoOfIntervalOverloads, logInfo, 2,
                                       3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logInfoWithArgsOfIntervalOverloads,
                                       logInfo, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logWarnOfIntervalOverloads, logWarn, 2,
                                       3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logWarnWithArgsOfIntervalOverloads,
                                       logWarn, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logErrorOfIntervalOverloads, logError, 2,
                                       3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logErrorWithArgsOfIntervalOverloads,
                                       logError, 3, 4)

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logCriticalOfIntervalOverloads,
                                       logCritical, 2, 3)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(logCriticalWithArgsOfIntervalOverloads,
                                       logCritical, 3, 4)

BOOST_PYTHON_MODULE(bqstgeng) {
  // char array
  class_<array_ref<char>>("CharArray")
      .def(array_indexing_suite<array_ref<char>>());

  // enum ExecAtStartup
  enum_<ExecAtStartup>("ExecAtStartup")
      .value("IsTrue", ExecAtStartup::True)
      .value("IsFalse", ExecAtStartup::False);

  // enum MarketCode
  enum_<MarketCode>("MarketCode")
      .value("Okex", MarketCode::Okex)
      .value("Binance", MarketCode::Binance)
      .value("Coinbase", MarketCode::Coinbase)
      .value("Kraken", MarketCode::Kraken)
      .value("SSE", MarketCode::SSE)
      .value("SZSE", MarketCode::SZSE)
      .value("SHFE", MarketCode::SHFE)
      .value("CZCE", MarketCode::CZCE)
      .value("DCE", MarketCode::DCE)
      .value("CFFEX", MarketCode::CFFEX)
      .value("Others", MarketCode::Others);

  // enum SymbolType
  enum_<SymbolType>("SymbolType")
      .value("Spot", SymbolType::Spot)
      .value("Futures", SymbolType::Futures)
      .value("CFutures", SymbolType::CFutures)
      .value("Perp", SymbolType::Perp)
      .value("CPerp", SymbolType::CPerp)
      .value("Option", SymbolType::Option)
      .value("Index", SymbolType::index)
      .value("CN_MainBoard", SymbolType::CN_MainBoard)
      .value("CN_SecondBoard", SymbolType::CN_SecondBoard)
      .value("CN_StartupBoard", SymbolType::CN_StartupBoard)
      .value("CN_Index", SymbolType::CN_Index)
      .value("CN_TechBoard", SymbolType::CN_TechBoard)
      .value("CN_StateBond", SymbolType::CN_StateBond)
      .value("CN_EnterpriseBond", SymbolType::CN_EnterpriseBond)
      .value("CN_CompanyBond", SymbolType::CN_CompanyBond)
      .value("CN_ConvertableBond", SymbolType::CN_ConvertableBond)
      .value("CN_NationalBondReverseRepo ",
             SymbolType::CN_NationalBondReverseRepo)
      .value("CN_ETF_SingleMarketStock", SymbolType::CN_ETF_SingleMarketStock)
      .value("CN_ETF_InterMarketStock", SymbolType::CN_ETF_InterMarketStock)
      .value("CN_ETF_CrossBorderStock", SymbolType::CN_ETF_CrossBorderStock)
      .value("CN_ETF_SingleMarketBond", SymbolType::CN_ETF_SingleMarketBond)
      .value("CN_ETF_Gold", SymbolType::CN_ETF_Gold)
      .value("CN_StructuredFundChild", SymbolType::CN_StructuredFundChild)
      .value("CN_SZSE_RecreationFund", SymbolType::CN_SZSE_RecreationFund)
      .value("CN_StockOption", SymbolType::CN_StockOption)
      .value("CN_ETF_Option", SymbolType::CN_ETF_Option)
      .value("CN_Allotment", SymbolType::CN_Allotment)
      .value("CN_MoneyMonetaryFundSHCR", SymbolType::CN_MoneyMonetaryFundSHCR)
      .value("CN_MonetaryFundSHTR", SymbolType::CN_MonetaryFundSHTR)
      .value("CN_MonetaryFundSZ", SymbolType::CN_MonetaryFundSZ)
      .value("CN_Futures", SymbolType::CN_Futures)
      .value("CN_FuturesOptions", SymbolType::CN_FuturesOptions)
      .value("CN_FuturesComb", SymbolType::CN_FuturesComb)
      .value("CN_SpotFutures", SymbolType::CN_SpotFutures)
      .value("CN_FuturesEFP", SymbolType::CN_FuturesEFP)
      .value("CN_SpotOptions", SymbolType::CN_SpotOptions)
      .value("CN_FuturesTAS", SymbolType::CN_FuturesTAS)
      .value("CN_FuturesMI", SymbolType::CN_FuturesMI)
      .value("Others", SymbolType::Others);

  // enum Side
  enum_<Side>("Side")
      .value("Bid", Side::Bid)
      .value("Ask", Side::Ask)
      .value("Others", Side::Others);

  // enum PosDirection
  enum_<PosDirection>("PosDirection")
      .value("Open", PosDirection::Open)
      .value("Close", PosDirection::Close)
      .value("CloseTDay", PosDirection::CloseTDay)
      .value("CloseYDay", PosDirection::CloseYDay)
      .value("Both", PosDirection::Both)
      .value("Others", PosDirection::Others);

  // enum PosSide
  enum_<PosSide>("PosSide")
      .value("Long", PosSide::Long)
      .value("Short", PosSide::Short)
      .value("Both", PosSide::Both)
      .value("Others", PosSide::Others);

  // enum OrderType
  enum_<OrderType>("OrderType")
      .value("Limit", OrderType::Limit)
      .value("Others", OrderType::Others);

  // enum OrderTypeExtra
  enum_<OrderTypeExtra>("OrderTypeExtra")
      .value("Normal", OrderTypeExtra::Normal)
      .value("MakeOnly", OrderTypeExtra::MakeOnly)
      .value("Ioc", OrderTypeExtra::Ioc)
      .value("Fok", OrderTypeExtra::Fok)
      .value("Others", OrderTypeExtra::Others);

  // enum CloseTDayStg
  enum_<CloseTDayStg>("CloseTDayStg")
      .value("AllowCloseTDay", CloseTDayStg::AllowCloseTDay)
      .value("RejectCloseTDay", CloseTDayStg::RejectCloseTDay)
      .value("RejectUntilOpenTDay", CloseTDayStg::RejectUntilOpenTDay)
      .value("Others", CloseTDayStg::Others);

  // enum MDType
  enum_<MDType>("MDType")
      .value("Trades", MDType::Trades)
      .value("Orders", MDType::Orders)
      .value("Books", MDType::Books)
      .value("Tickers", MDType::Tickers)
      .value("Candle", MDType::Candle)
      .value("Bid1Ask1", MDType::Bid1Ask1)
      .value("LastPrice", MDType::LastPrice)
      .value("DynCandle", MDType::DynCandle)
      .value("Others", MDType::Others);

  // enum OrderStatus
  enum_<OrderStatus>("OrderStatus")
      .value("Created", OrderStatus::Created)
      .value("ConfirmedInLocal", OrderStatus::ConfirmedInLocal)
      .value("Pending", OrderStatus::Pending)
      .value("ConfirmedByExch", OrderStatus::ConfirmedByExch)
      .value("PartialFilled", OrderStatus::PartialFilled)
      .value("Filled", OrderStatus::Filled)
      .value("Canceled", OrderStatus::Canceled)
      .value("PartialFilledCanceled", OrderStatus::PartialFilledCanceled)
      .value("Failed", OrderStatus::Failed)
      .value("Others", OrderStatus::Others);

  // enum LiquidityDirection
  enum_<LiquidityDirection>("LiquidityDirection")
      .value("Maker", LiquidityDirection::Maker)
      .value("Taker", LiquidityDirection::Taker);

  // enum NotifyToTerminal
  enum_<NotifyToTerminal>("NotifyToTerminal")
      .value("IsTrue", NotifyToTerminal::True)
      .value("IsFalse", NotifyToTerminal::False);

  // StgInstInfo
  class_<StgInstInfo, std::shared_ptr<StgInstInfo>>("StgInstInfo", init<>())
      .def_readwrite("product_id", &StgInstInfo::productId_)
      .def_readwrite("stg_id", &StgInstInfo::stgId_)
      .def_readwrite("stg_name", &StgInstInfo::stgName_)
      .def_readwrite("user_id_of_author", &StgInstInfo::userIdOfAuthor_)
      .def_readwrite("stg_inst_id", &StgInstInfo::stgInstId_)
      .def_readwrite("stg_inst_params", &StgInstInfo::stgInstParams_)
      .def_readwrite("stg_inst_name", &StgInstInfo::stgInstName_)
      .def_readwrite("user_id", &StgInstInfo::userId_)
      .def_readwrite("is_del", &StgInstInfo::isDel_)
      .def("to_str", &StgInstInfo::toStr);

  // SHMHeader
  class_<SHMHeader>("SHMHeader", init<>())
      .def(init<std::uint16_t>())
      .def("to_str", &SHMHeader::toStr);

  // OrderInfo
  class_<OrderInfo, std::shared_ptr<OrderInfo>>("order_info", init<>())
      .def_readwrite("shm_header", &OrderInfo::shmHeader_)
      .def_readwrite("product_grp_id", &OrderInfo::productGrpId_)
      .def_readwrite("product_id", &OrderInfo::productId_)
      .def_readwrite("user_id", &OrderInfo::userId_)
      .def_readwrite("acct_grp_id", &OrderInfo::acctGrpId_)
      .def_readwrite("acct_id", &OrderInfo::acctId_)
      .def_readwrite("trd_acct_id", &OrderInfo::trdAcctId_)
      .def_readwrite("stg_grp_id", &OrderInfo::stgGrpId_)  // add stgGrpId
      .def_readwrite("stg_id", &OrderInfo::stgId_)
      .def_readwrite("stg_inst_id", &OrderInfo::stgInstId_)
      .def_readwrite("algo_id", &OrderInfo::algoId_)
      .def_readwrite("order_id", &OrderInfo::orderId_)
      .add_property(
          "exch_order_id",
          static_cast<array_ref<char> (*)(OrderInfo*)>([](OrderInfo* obj) {
            return array_ref<char>(obj->exchOrderId_);
          }),
          "Data bytes array of exchOrderId")
      .def_readwrite("parent_order_id", &OrderInfo::parentOrderId_)
      .def_readwrite("market_code", &OrderInfo::marketCode_)
      .def_readwrite("symbol_type", &OrderInfo::symbolType_)
      .add_property(
          "symbol_code",
          static_cast<array_ref<char> (*)(OrderInfo*)>(
              [](OrderInfo* obj) { return array_ref<char>(obj->symbolCode_); }),
          "Data bytes array of symbolcode")
      .add_property(
          "exch_symbol_code",
          static_cast<array_ref<char> (*)(OrderInfo*)>([](OrderInfo* obj) {
            return array_ref<char>(obj->exchSymbolCode_);
          }),
          "Data bytes array of exch symbolcode")
      .def_readwrite("side", &OrderInfo::side_)
      .def_readwrite("pos_direction", &OrderInfo::posDirection_)
      .def_readwrite("pos_side", &OrderInfo::posSide_)
      .def_readwrite("order_price", &OrderInfo::orderPrice_)
      .def_readwrite("order_size", &OrderInfo::orderSize_)
      .def_readwrite("par_value", &OrderInfo::parValue_)
      .def_readwrite("order_type", &OrderInfo::orderType_)
      .def_readwrite("order_type_extra", &OrderInfo::orderTypeExtra_)
      .def_readwrite("close_tday_stg", &OrderInfo::closeTDayStg_)
      .def_readwrite("order_time", &OrderInfo::orderTime_)
      .def_readwrite("fee", &OrderInfo::fee_)
      .add_property(
          "fee_currency",
          static_cast<array_ref<char> (*)(OrderInfo*)>([](OrderInfo* obj) {
            return array_ref<char>(obj->feeCurrency_);
          }),
          "Data bytes array of fee currency")
      .def_readwrite("deal_size", &OrderInfo::dealSize_)
      .def_readwrite("avg_deal_price", &OrderInfo::avgDealPrice_)
      .add_property(
          "last_trade_id",
          static_cast<array_ref<char> (*)(OrderInfo*)>([](OrderInfo* obj) {
            return array_ref<char>(obj->lastTradeId_);
          }),
          "Data bytes array of fee currency")
      .def_readwrite("last_deal_price", &OrderInfo::lastDealPrice_)
      .def_readwrite("last_deal_size", &OrderInfo::lastDealSize_)
      .def_readwrite("last_deal_time", &OrderInfo::lastDealTime_)
      .def_readwrite("order_status", &OrderInfo::orderStatus_)
      .def_readonly("no_used_to_calc_pos", &OrderInfo::noUsedToCalcPos_)
      .def_readwrite("status_code", &OrderInfo::statusCode_)
      .def("to_short_str", &OrderInfo::toShortStr);

  boost::python::def<OrderInfoSPtr (*)()>("make_order_info", MakeOrderInfo);

  // symbolCode
  class_<SymbolCode, std::shared_ptr<SymbolCode>>(
      "SymbolCode", init<MarketCode, SymbolType, std::string>())
      .def_readwrite("market_code", &SymbolCode::marketCode_)
      .def_readwrite("symbol_code", &SymbolCode::symbolCode_)
      .def_readwrite("symbol_type", &SymbolCode::symbolType_);

  // symbolCodeGroup
  class_<std::vector<std::shared_ptr<SymbolCode>>>("SymbolCodeGroup")
      .def(vector_indexing_suite<std::vector<std::shared_ptr<SymbolCode>>,
                                 true>());

  // assetInfo
  class_<AssetInfo, std::shared_ptr<AssetInfo>>("AssetInfo", init<>())
      .def_readwrite("acct_id", &AssetInfo::acctId_)
      .def_readwrite("market_code", &AssetInfo::marketCode_)
      .def_readwrite("symbol_type", &AssetInfo::symbolType_)
      .add_property(
          "asset_name",
          static_cast<array_ref<char> (*)(AssetInfo*)>(
              [](AssetInfo* obj) { return array_ref<char>(obj->assetName_); }),
          "Data bytes array of asset name")
      .def_readwrite("vol", &AssetInfo::vol_)
      .def_readwrite("cross_vol", &AssetInfo::crossVol_)
      .def_readwrite("frozen", &AssetInfo::frozen_)
      .def_readwrite("available", &AssetInfo::available_)
      .def_readwrite("pnl_unreal", &AssetInfo::pnlUnreal_)
      .def_readwrite("max_withdraw", &AssetInfo::maxWithdraw_)
      .def_readwrite("updatetime", &AssetInfo::updateTime_)
      .def_readonly("key_hash", &AssetInfo::keyHash_)
      .def("to_str", &AssetInfo::toStr);

  // posInfo
  class_<PosInfo, std::shared_ptr<PosInfo>>("PosInfo", init<>())
      .def_readonly("key_hash", &PosInfo::keyHash_)
      .def_readwrite("product_grp_id", &PosInfo::productGrpId_)
      .def_readwrite("product_id", &PosInfo::productId_)
      .def_readwrite("user_id", &PosInfo::userId_)
      .def_readwrite("acct_grp_id", &PosInfo::acctGrpId_)
      .def_readwrite("acct_id", &PosInfo::acctId_)
      .def_readwrite("trd_acct_id", &PosInfo::trdAcctId_)
      .def_readwrite("stg_grp_id", &PosInfo::stgGrpId_)  // add stgGrpId
      .def_readwrite("stg_id", &PosInfo::stgId_)
      .def_readwrite("stg_inst_id", &PosInfo::stgInstId_)
      .def_readwrite("algo_id", &PosInfo::algoId_)
      .def_readwrite("market_code", &PosInfo::marketCode_)
      .def_readwrite("symbol_type", &PosInfo::symbolType_)
      .add_property(
          "symbol_code",
          static_cast<array_ref<char> (*)(OrderInfo*)>(
              [](OrderInfo* obj) { return array_ref<char>(obj->symbolCode_); }),
          "Data bytes array of symbolcode")
      .def_readwrite("side", &PosInfo::side_)
      .def_readwrite("pos_side", &PosInfo::posSide_)
      .def_readwrite("par_value", &PosInfo::parValue_)
      .add_property(
          "fee_currency",
          static_cast<array_ref<char> (*)(OrderInfo*)>([](OrderInfo* obj) {
            return array_ref<char>(obj->feeCurrency_);
          }),
          "Data bytes array of fee currency")
      .def_readwrite("fee", &PosInfo::fee_)
      .def_readwrite("pos", &PosInfo::pos_)
      .def_readwrite("pre_pos", &PosInfo::prePos_)
      .def_readwrite("pre_avg_open_price", &PosInfo::preAvgOpenPrice_)
      .def_readwrite("avg_open_price", &PosInfo::avgOpenPrice_)
      .def_readwrite("pnl_unreal", &PosInfo::pnlUnReal_)
      .def_readwrite("pnl_real", &PosInfo::pnlReal_)
      .def_readwrite("total_bid_size", &PosInfo::totalBidSize_)
      .def_readwrite("total_ask_size", &PosInfo::totalAskSize_)
      .def_readwrite("pre_total_bid_size", &PosInfo::preTotalBidSize_)
      .def_readwrite("pre_total_ask_size", &PosInfo::preTotalAskSize_)
      .def_readwrite("total_open_size", &PosInfo::totalOpenSize_)
      .def_readwrite("pre_total_open_size", &PosInfo::preTotalOpenSize_)
      .def_readonly("last_no_used_to_calc_pos", &PosInfo::lastNoUsedToCalcPos_)
      .def_readwrite("updatetime", &PosInfo::updateTime_)
      .def("to_str", &PosInfo::toStr);

  // pnl
  class_<Pnl, std::shared_ptr<Pnl>>("Pnl", init<>())
      .def_readwrite("query_cond", &Pnl::queryCond_)
      .def_readwrite("pnl_un_real", &Pnl::pnlUnReal_)
      .def_readwrite("pnl_real", &Pnl::pnlReal_)
      .def_readwrite("fee", &Pnl::fee_)
      .def_readwrite("upate_time", &Pnl::updateTime_)
      .def_readwrite("quote_currency_for_calc", &Pnl::quoteCurrencyForCalc_)
      .def_readwrite("status_code", &Pnl::statusCode_)
      .def("to_str", &Pnl::toStr)
      .def("get_total_pnl", &Pnl::getTotalPnl)
      .def("is_valid", &Pnl::isValid);

  // PosInfoDetail
  class_<std::map<std::string, PosInfoSPtr>>("PosInfoDetail")
      .def(map_indexing_suite<std::map<std::string, PosInfoSPtr>, true>());

  // RetOfQueryPnl
  using RetOfQueryPnl = std::tuple<int, PnlSPtr>;
  class_<RetOfQueryPnl>("ret_of_query_pnl", init<int, PnlSPtr>())
      .def("__len__", &tuple_length<RetOfQueryPnl>)
      .def("__getitem__", &get_tuple_item<RetOfQueryPnl>);

  // Key2PnlGroup
  class_<Key2PnlGroup, std::shared_ptr<Key2PnlGroup>>("Key2PnlGroup")
      .def(map_indexing_suite<Key2PnlGroup, true>());

  // RetOfQueryPnlGroupBy
  using RetOfQueryPnlGroupBy = std::tuple<int, Key2PnlGroupSPtr>;
  class_<RetOfQueryPnlGroupBy>("ret_of_query_pnl_group_by",
                               init<int, Key2PnlGroupSPtr>())
      .def("__len__", &tuple_length<RetOfQueryPnlGroupBy>)
      .def("__getitem__", &get_tuple_item<RetOfQueryPnlGroupBy>);

  // PosInfoGroupSPtr
  class_<PosInfoGroup, std::shared_ptr<PosInfoGroup>>("PosInfoGroup")
      .def(vector_indexing_suite<PosInfoGroup, true>());

  // RetOfQUeryPosInfoGroup
  using RetOfQUeryPosInfoGroup = std::tuple<int, PosInfoGroupSPtr>;
  class_<RetOfQUeryPosInfoGroup>("ret_of_query_pos_info_group",
                                 init<int, PosInfoGroupSPtr>())
      .def("__len__", &tuple_length<RetOfQUeryPosInfoGroup>)
      .def("__getitem__", &get_tuple_item<RetOfQUeryPosInfoGroup>);

  // Key2PosInfoBundle
  class_<Key2PosInfoBundle, std::shared_ptr<Key2PosInfoBundle>>(
      "Key2PosInfoBundle")
      .def(map_indexing_suite<Key2PosInfoBundle, true>());

  // RetOfQueryPosInfoGroupBy
  using RetOfQueryPosInfoGroupBy = std::tuple<int, Key2PosInfoBundleSPtr>;
  class_<RetOfQueryPosInfoGroupBy>("ret_of_query_pos_info_group_by",
                                   init<int, Key2PosInfoBundleSPtr>())
      .def("__len__", &tuple_length<RetOfQueryPosInfoGroupBy>)
      .def("__getitem__", &get_tuple_item<RetOfQueryPosInfoGroupBy>);

  // AssetsUpdate
  class_<AssetsUpdate, std::shared_ptr<AssetsUpdate>>("AssetsUpdate")
      .def(map_indexing_suite<AssetsUpdate, true>());

  // posSnapshot
  class_<PosSnapshot, std::shared_ptr<PosSnapshot>, boost::noncopyable>(
      "PosSnapshot",
      init<std::map<std::string, PosInfoSPtr>, MarketDataCacheSPtr>())
      .def("get_pos_info_detail", &PosSnapshot::getPosInfoDetail,
           return_value_policy<reference_existing_object>())
      .def("query_pnl", &PosSnapshot::queryPnl,
           queryPnlOverloads(args("query_cond", "quote_currency_for_calc",
                                  "quote_currency_for_conv",
                                  "orig_quote_currency_of_ubased_contract")))
      .def("query_pnl_group_by", &PosSnapshot::queryPnlGroupBy,
           queryPnlGroupByOverloads(
               args("group_cond", "quote_currency_for_calc",
                    "quote_currency_for_conv",
                    "orig_quote_currency_of_ubased_contract")))
      .def("query_pos_info_group", &PosSnapshot::queryPosInfoGroup,
           args("query_cond"))
      .def("query_pos_info_group_by", &PosSnapshot::queryPosInfoGroupBy,
           args("query_cond"));

  using RetOfIUI64 = std::tuple<int, OrderId>;

  using OrderByFields = RetOfIUI64 (StgEng::*)(
      const StgInstInfoSPtr&, MarketCode, const std::string&, Side,
      PosDirection, Decimal, Decimal, TrdAcctId, CloseTDayStg, AlgoId,
      const SimedTDInfoSPtr&);
  using OrderByFields2 = RetOfIUI64 (StgEng::*)(
      const StgInstInfoSPtr&, MarketCode, const std::string&, Side, Decimal,
      Decimal, TrdAcctId, CloseTDayStg, AlgoId, const SimedTDInfoSPtr&);
  using OrderByOrderInfo = RetOfIUI64 (StgEng::*)(OrderInfoSPtr&);

  using CancelAllOrderByInfo =
      std::vector<OrderId> (StgEng::*)(const StgInstInfoSPtr&);
  using CancelAllOrderById = std::vector<OrderId> (StgEng::*)(StgInstId);
  using CancelAllOrderByAlgoId = std::vector<OrderId> (StgEng::*)(AlgoId);

  using InstallStgInstTimerOfFixedTime = void (StgEng::*)(
      StgInstId, const std::string&, const std::string&, std::uint32_t);
  using InstallStgInstTimerOfInterval =
      void (StgEng::*)(StgInstId, const std::string&, ExecAtStartup,
                       std::uint32_t, std::uint64_t);

  class_<RetOfIUI64>("ret_of_order", init<int, OrderId>())
      .def("__len__", &tuple_length<RetOfIUI64>)
      .def("__getitem__", &get_tuple_item<RetOfIUI64>);

  class_<RetOfIUI64>("ret_of_algo_order", init<int, AlgoId>())
      .def("__len__", &tuple_length<RetOfIUI64>)
      .def("__getitem__", &get_tuple_item<RetOfIUI64>);

  using RetOfGetOrderInfo = std::tuple<int, OrderInfoSPtr>;
  class_<RetOfGetOrderInfo>("ret_of_get_order_info", init<int, OrderInfoSPtr>())
      .def("__len__", &tuple_length<RetOfGetOrderInfo>)
      .def("__getitem__", &get_tuple_item<RetOfGetOrderInfo>);

  using RetOfIS = std::tuple<int, std::string>;

  class_<RetOfIS>("ret_of_qry_his_md", init<int, std::string>())
      .def("__len__", &tuple_length<RetOfIS>)
      .def("__getitem__", &get_tuple_item<RetOfIS>);

  using QryHisMDBetweenTsByFields = RetOfIS (StgEng::*)(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext);

  using QryHisMDBetweenTsByTopic = RetOfIS (StgEng::*)(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t tsBegin, std::uint64_t tsEnd);

  using QryHisMDBeforeTsByFields = RetOfIS (StgEng::*)(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext);

  using QryHisMDBeforeTsByTopic =
      RetOfIS (StgEng::*)(const StgInstInfoSPtr& stgInstInfo,
                          const std::string& topic, std::uint64_t ts, int num);

  using QryHisMDAfterTsByFields = RetOfIS (StgEng::*)(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext);

  using QryHisMDAfterTsByTopic =
      RetOfIS (StgEng::*)(const StgInstInfoSPtr& stgInstInfo,
                          const std::string& topic, std::uint64_t ts, int num);

  class_<RetOfIS>("ret_of_exec_sql", init<int, std::string>())
      .def("__len__", &tuple_length<RetOfIS>)
      .def("__getitem__", &get_tuple_item<RetOfIS>);

  // PosOfStgInst
  class_<PosOfStgInst, std::shared_ptr<PosOfStgInst>>("pos_of_stg_inst",
                                                      init<>())
      .def_readwrite("stg_inst_info", &PosOfStgInst::stgInstInfo_)
      .def_readwrite("acct_id", &PosOfStgInst::acctId_)
      .def_readwrite("trd_acct_id", &PosOfStgInst::trdAcctId_)
      .def_readwrite("algo_id", &PosOfStgInst::algoId_)
      .def_readwrite("market_code", &PosOfStgInst::marketCode_)
      .def_readwrite("symbol_type", &PosOfStgInst::symbolType_)
      .def_readwrite("symbol_code", &PosOfStgInst::symbolCode_)
      .def_readwrite("side", &PosOfStgInst::side_)
      .def_readwrite("pos_side", &PosOfStgInst::posSide_)
      .def_readwrite("par_value", &PosOfStgInst::parValue_)
      .def_readwrite("fee_currency", &PosOfStgInst::feeCurrency_)
      .def_readwrite("fee", &PosOfStgInst::fee_)
      .def_readwrite("pos", &PosOfStgInst::pos_)
      .def_readwrite("avg_open_price", &PosOfStgInst::avgOpenPrice_)
      .def_readwrite("pos_unreal", &PosOfStgInst::posUnreal_)
      .def_readwrite("avg_order_price", &PosOfStgInst::avgOrderPrice_)
      .def("to_str", &PosOfStgInst::toStr)
      .def("get_key", &PosOfStgInst::getKey);

  // PosOfStgInstGroup
  class_<PosOfStgInstGroup, std::shared_ptr<PosOfStgInstGroup>>(
      "PosOfStgInstGroup")
      .def(vector_indexing_suite<PosOfStgInstGroup, true>());

  // OrderIdGroup
  class_<std::vector<OrderId>>("OrderIdGroup")
      .def(vector_indexing_suite<std::vector<OrderId>, true>());

  // OrderInfoGroup
  class_<std::vector<OrderInfoSPtr>>("OrderInfoGroup")
      .def(vector_indexing_suite<std::vector<OrderInfoSPtr>, true>());

  // StgEng
  class_<StgEng, boost::noncopyable>("StgEng", init<std::string>())
      .def("get_dft_stg_inst_info", &StgEng::getDftStgInstInfo)
      .def("init", &StgEng::init, args("stg_inst_task_handler"))
      .def("run", &StgEng::run)
      .def<OrderByFields>(
          "order", &StgEng::order,
          orderOverloads(args("stg_inst_info", "market_code", "symbol_code",
                              "side", "pos_direction", "order_price",
                              "order_size", "trd_acct_id", "close_tday_stg",
                              "algo_id", "simed_td_info")))
      .def<OrderByFields2>(
          "order", &StgEng::order,
          orderOverloads2(args("stg_inst_info", "market_code", "symbol_code",
                               "side", "order_price", "order_size",
                               "trd_acct_id", "close_tday_stg", "algo_id",
                               "simed_td_info")))
      .def<OrderByOrderInfo>("order", &StgEng::order, args("order_info"))
      .def("cancel_order", &StgEng::cancelOrder, args("order_id"))
      .def("cancel_all_order_of_stg", &StgEng::cancelAllOrderOfStg)
      .def<CancelAllOrderByInfo>("cancel_all_order_of_stg_inst",
                                 &StgEng::cancelAllOrderOfStgInst,
                                 args("stg_inst_info"))
      .def<CancelAllOrderById>("cancel_all_order_of_stg_inst",
                               &StgEng::cancelAllOrderOfStgInst,
                               args("stg_inst_id"))
      .def<CancelAllOrderByAlgoId>("cancel_all_order_of_algo",
                                   &StgEng::cancelAllOrderOfAlgo,
                                   args("algo_id"))
      .def("algo_order", &StgEng::algoOrder,
           args("stg_inst_info", "algo_type", "algo_name", "lifetime",
                "algo_params_in_json_fmt"))
      .def("cancel_algo_order", &StgEng::cancelAlgoOrder, args("algo_id"))
      .def("get_progress_of_algo_order", &StgEng::getProgressOfAlgoOrder,
           args("algo_id"))
      .def("get_order_info", &StgEng::getOrderInfo, args("order_id"))
      .def("sub", &StgEng::sub, args("subscriber", "topic"))
      .def("unsub", &StgEng::sub, args("subscriber", "topic"))
      .def<QryHisMDBetweenTsByFields>(
          "query_his_md_between_2_ts", &StgEng::queryHisMDBetween2Ts,
          queryHisMDBetween2TsOverloadsByFields(
              args("stg_inst_info", "market_code", "symbol_type", "symbol_code",
                   "mdtype", "ts_begin", "ts_end", "ext")))
      .def<QryHisMDBetweenTsByTopic>(
          "query_his_md_between_2_ts", &StgEng::queryHisMDBetween2Ts,
          args("stg_inst_info", "topic", "ts_begin", "ts_end"))
      .def<QryHisMDBeforeTsByFields>(
          "query_specific_num_of_his_md_before_ts",
          &StgEng::querySpecificNumOfHisMDBeforeTs,
          querySpecificNumOfHisMDBeforeTsOverloadsByFields(
              args("stg_inst_info", "market_code", "symbol_type", "symbol_code",
                   "mdtype", "ts", "num", "ext")))
      .def<QryHisMDBeforeTsByTopic>("query_specific_num_of_his_md_before_ts",
                                    &StgEng::querySpecificNumOfHisMDBeforeTs,
                                    args("stg_inst_info", "topic", "ts", "num"))
      .def<QryHisMDAfterTsByFields>(
          "query_specific_num_of_his_md_after_ts",
          &StgEng::querySpecificNumOfHisMDAfterTs,
          querySpecificNumOfHisMDAfterTsOverloadsByFields(
              args("stg_inst_info", "market_code", "symbol_type", "symbol_code",
                   "mdtype", "ts", "num", "ext")))
      .def<QryHisMDAfterTsByTopic>("query_specific_num_of_his_md_after_ts",
                                   &StgEng::querySpecificNumOfHisMDAfterTs,
                                   args("stg_inst_info", "topic", "ts", "num"))
      .def<InstallStgInstTimerOfFixedTime>(
          "install_stg_inst_timer", &StgEng::installStgInstTimer,
          installStgInstTimerOfFixedTimeOverloads(
              args("stg_inst_id", "timer_name", "exec_time", "time_zone")))
      .def<InstallStgInstTimerOfInterval>(
          "install_stg_inst_timer", &StgEng::installStgInstTimer,
          installStgInstTimerOfIntervalOverloads(
              args("stg_inst_id", "timer_name", "exec_at_startup",
                   "millisec_interval", "max_exec_times")))
      .def("uninstall_stg_inst_timer", &StgEng::uninstallStgInstTimer,
           args("stg_inst_id", "timer_name"))
      .def("get_unclosed_order_info_group", &StgEng::getUnclosedOrderInfoGroup,
           args("stg_inst_info"))
      .def("get_pos_of_stg_inst", &StgEng::getPosOfStgInst,
           args("stg_inst_info"))
      .def("save_stg_private_data", &StgEng::saveStgPrivateData,
           args("stg_inst_id", "json_str"))
      .def("load_stg_private_data", &StgEng::loadStgPrivateData,
           args("stg_inst_id"))
      .def("sync_exec_sql", &StgEng::syncExecSql, args("sql"))
      .def("save_to_db", &StgEng::saveToDB, args("pnl"))
      .def("log_trace",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logTrace),
           logTraceOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_trace",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logTrace),
           logTraceWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")))
      .def("log_debug",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logDebug),
           logDebugOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_debug",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logDebug),
           logDebugWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")))
      .def("log_info",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logInfo),
           logInfoOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_info",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logInfo),
           logInfoWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")))
      .def("log_warn",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logWarn),
           logWarnOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_warn",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logWarn),
           logWarnWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")))
      .def("log_error",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logError),
           logErrorOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_error",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logError),
           logErrorWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")))
      .def("log_critical",
           static_cast<void (StgEng::*)(
               const std::string&, const StgInstInfoSPtr&, NotifyToTerminal)>(
               &StgEng::logCritical),
           logCriticalOfIntervalOverloads(
               args("fmt", "stg_inst_info", "notify_to_terminal")))
      .def("log_critical",
           static_cast<void (StgEng::*)(
               const std::string&, const boost::python::list&,
               const StgInstInfoSPtr&, NotifyToTerminal)>(&StgEng::logCritical),
           logCriticalWithArgsOfIntervalOverloads(
               args("fmt", "args", "stg_inst_info", "notify_to_terminal")));

  def("get_status_msg", GetStatusMsg, args("status_code"));

  // TransDetail
  class_<TransDetail, TransDetailSPtr>("TransDetail", init<>())
      .def(init<Decimal, Decimal, LiquidityDirection>(
          args("self", "slippage", "filled_per", "ld")))
      .def_readwrite("slippage", &TransDetail::slippage_)
      .def_readwrite("filled_per", &TransDetail::filledPer_)
      .def_readwrite("liquidity_direction", &TransDetail::liquidityDirection_)
      .def("to_str", &TransDetail::toStr);

  def("make_trans_detail", MakeTransDetail, args("trans_detail_in_json_fmt"));

  // TransDetailGroup
  class_<TransDetailGroup>("TransDetailGroup")
      .def(vector_indexing_suite<TransDetailGroup, true>());

  // SimedTDInfo
  class_<SimedTDInfo, SimedTDInfoSPtr>("SimedTDInfo", init<>())
      .def(init<OrderStatus, std::vector<TransDetailSPtr>>())
      .def_readwrite("order_status", &SimedTDInfo::orderStatus_)
      .def_readwrite("trans_detail_group", &SimedTDInfo::transDetailGroup_);

  // RetOfMakeSimedTDInfo
  using RetOfMakeSimedTDInfo = std::tuple<int, SimedTDInfoSPtr>;
  class_<RetOfMakeSimedTDInfo>("ret_of_make_simed_td_info",
                               init<int, SimedTDInfoSPtr>())
      .def("__len__", &tuple_length<RetOfMakeSimedTDInfo>)
      .def("__getitem__", &get_tuple_item<RetOfMakeSimedTDInfo>);
}

}  // namespace bq::stg
