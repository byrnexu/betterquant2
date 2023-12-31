/*!
 * \file SymbolTableMaint.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "SymbolTableMaint.hpp"

#include "Config.hpp"
#include "MDSvc.hpp"
#include "MDSvcConst.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMIPCTopicName.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/Logger.hpp"
#include "util/Scheduler.hpp"

namespace bq::md::svc {

int SymbolTableMaint::start() {
  //! 创建定时器
  if (CONFIG["enableSymbolTableMaint"].as<bool>()) {
    const auto milliSecIntervalOfSymbolTableMaint =
        CONFIG["milliSecIntervalOfSymbolTableMaint"].as<std::uint32_t>();
    schedulerSymbolTableMaint_ = std::make_shared<Scheduler>(
        "SYMBOL_TABLE_MAINT", [this]() { execSymbolTableMaint(); },
        milliSecIntervalOfSymbolTableMaint);
  }

  //! 从数据库查询码表
  LOG_D("Begin to query symbol table from db.");
  auto [retOfQryFromDB, tblRecSetFromDB] = querySymbolTableFromDB();
  if (retOfQryFromDB != 0) {
    LOG_W("Do symbol table maintainment failed.");
    return retOfQryFromDB;
  }

  //! 先执行一遍定时器过程同步最新码表
  if ((CONFIG["enableSymbolTableMaint"].as<bool>() == true) ||
      (CONFIG["enableSymbolTableMaint"].as<bool>() == false &&
       tblRecSetFromDB->empty())) {
    execSymbolTableMaint();
  }

  if (CONFIG["enableSymbolTableMaint"].as<bool>()) {
    auto ret = schedulerSymbolTableMaint_->start();
    if (ret != 0) {
      LOG_E("Start symbol table maintainment failed.");
      return -1;
    }
  }

  LOG_D("Start symbol table maintainment success.");
  return 0;
}

int SymbolTableMaint::execSymbolTableMaint() {
  LOG_D("Begin to query symbol table from exch.");
  //! 从交易所查询码表
  const auto [retOfQryFromExch, tblRecSetFromExch] = querySymbolTableFromExch();
  if (retOfQryFromExch != 0 || tblRecSetFromExch == nullptr) {
    LOG_W("Do symbol table maintainment failed.");
    return retOfQryFromExch;
  }

  LOG_D("Begin to query symbol table from db.");
  //! 从数据库查询码表
  const auto [retOfQryFromDB, tblRecSetFromDB] = querySymbolTableFromDB();
  if (retOfQryFromDB != 0) {
    LOG_W("Do symbol table maintainment failed.");
    return retOfQryFromDB;
  }

  LOG_D("Begin to compare symbol table from exch and db.");
  //! 比较交易所和数据库中的码表
  const auto [tblRecSetAdd, tblRecSetDel, tblRecSetChg] =
      db::TBLRecSetCompare(tblRecSetFromExch, tblRecSetFromDB);

  if (!tblRecSetAdd->empty() || !tblRecSetDel->empty() ||
      !tblRecSetChg->empty()) {
    LOG_D("Begin to sync rec to table {}.", TBLSymbolInfo::TableName);
    //! 更新本地码表
    syncRecToTable(tblRecSetAdd, tblRecSetDel, tblRecSetChg);
  }

  if (tblRecSetFromDB->empty() == false) {
    PubTopicOfSymbolChg(tblRecSetAdd, tblRecSetDel, tblRecSetChg);
  }

  return 0;
}

void SymbolTableMaint::PubTopicOfSymbolChg(
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetAdd,
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetDel,
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetChg) {
  // topic = MD@Binance@Spot@SymbolAdd
  for (const auto& rec : *tblRecSetAdd) {
    const auto [statusCode, channel] =
        GetChannelFromAddr(mdSvc_->getAddrOfSHMSrv());
    const auto topic = fmt::format("{}{}SymbolAdd", channel, SEP_OF_TOPIC);
    const auto topicData = rec.second->getJsonStrOfAllFields();
    PubTopic(mdSvc_->getSHMSrv(),TOPIC_NAME_SYMBOL_ONLINE, topic, topicData);
  }
  // topic = MD@Binance@Spot@SymbolDel
  for (const auto& rec : *tblRecSetDel) {
    const auto [statusCode, channel] =
        GetChannelFromAddr(mdSvc_->getAddrOfSHMSrv());
    const auto topic = fmt::format("{}{}SymbolDel", channel, SEP_OF_TOPIC);
    const auto topicData = rec.second->getJsonStrOfAllFields();
    PubTopic(mdSvc_->getSHMSrv(), TOPIC_NAME_SYMBOL_OFFLINE, topic, topicData);
  }
  // topic = MD@Binance@Spot@SymbolChg
  for (const auto& rec : *tblRecSetChg) {
    const auto [statusCode, channel] =
        GetChannelFromAddr(mdSvc_->getAddrOfSHMSrv());
    const auto topic = fmt::format("{}{}SymbolChg", channel, SEP_OF_TOPIC);
    const auto topicData = rec.second->getJsonStrOfAllFields();
    PubTopic(mdSvc_->getSHMSrv(), TOPIC_NAME_SYMBOL_CHANGED, topic, topicData);
  }
}

std::tuple<int, db::TBLRecSetSPtr<TBLSymbolInfo>>
SymbolTableMaint::querySymbolTableFromExch() {
  const auto addrOfSymbolTable = CONFIG["addrOfSymbolTable"].as<std::string>();
  const auto timeoutOfQuerySymbolTableFromExch =
      CONFIG["timeoutOfQuerySymbolTableFromExch"].as<std::uint32_t>();
  cpr::Response rsp = cpr::Get(cpr::Url{addrOfSymbolTable},
                               cpr::Timeout(timeoutOfQuerySymbolTableFromExch));

  if (rsp.status_code != cpr::status::HTTP_OK) {
    LOG_W("Query symbol table from exch failed. [{}:{}] [{}]", rsp.status_code,
          rsp.reason, rsp.url.str());
    return {-1, nullptr};
  }
  LOG_D("Query symbol table from exch success. [text size = {}]",
        rsp.text.size());

  auto [ret, recSet] = convertSymbolTableFromExch(rsp.text);
  if (ret != 0) {
    LOG_W("Query symbol table from exch failed.");
    return {-1, nullptr};
  }
  LOG_D("Query symbol table from exch success. [rec set size = {}]",
        recSet->size());

  return {0, recSet};
}

std::tuple<int, db::TBLRecSetSPtr<TBLSymbolInfo>>
SymbolTableMaint::querySymbolTableFromDB() {
  const auto sql = fmt::format(
      "SELECT * FROM {} "
      "WHERE `marketCode` = '{}' AND `symbolType` = '{}' AND `isDel` = 0;",
      TBLSymbolInfo::TableName, mdSvc_->getMarketCode(),
      mdSvc_->getSymbolType());
  auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLSymbolInfo>::ExecSql(mdSvc_->getDBEng(), sql);
  if (ret != 0) {
    LOG_W("Query symbol table from db failed.");
    return {ret, nullptr};
  }
  LOG_D("Query symbol table from db success. [rec set size = {}]",
        tblRecSet->size());

  return {0, tblRecSet};
}

//!
//! 数据库里查出来的是所有isDel=0的记录，如果是新品种上线，那么replace可以代替
//! insert正常执行，如果是老品种下线后再上线，由于下线后isDel变成了1，上线后
//! isDel修改成0即可，replace也可以正常执行。如果是一些不是key的字段发生变化，
//! update也可以正常执行，如果是老品种下线，那么delete将isDel修改为1，也可以正
//! 常执行。
//!
void SymbolTableMaint::syncRecToTable(
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetAdd,
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetDel,
    const db::TBLRecSetSPtr<TBLSymbolInfo>& tblRecSetChg) {
  ExecTBLReplace(mdSvc_->getDBEng(), tblRecSetAdd);
  ExecTBLDelete(mdSvc_->getDBEng(), tblRecSetDel);
  ExecTBLUpdate(mdSvc_->getDBEng(), tblRecSetChg);
}

void SymbolTableMaint::setDftValForUninitFields(
    db::symbolInfo::RecordSPtr& symbolInfo) const {
  if (symbolInfo->precOfOrderPrice.empty()) {
    symbolInfo->precOfOrderPrice = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->precOfOrderVol.empty()) {
    symbolInfo->precOfOrderVol = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->minOrderVol.empty()) {
    symbolInfo->minOrderVol = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->maxOrderVol.empty()) {
    symbolInfo->maxOrderVol = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->minOrderAmt.empty()) {
    symbolInfo->minOrderAmt = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->maxOrderAmt.empty()) {
    symbolInfo->maxOrderAmt = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->contractMult.empty()) {
    symbolInfo->contractMult = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->maxLeverage.empty()) {
    symbolInfo->maxLeverage = CONV(std::string, UNDEFINED_FIELD_VALUE);
  }

  if (symbolInfo->launchTime.empty()) {
    symbolInfo->launchTime = UNDEFINED_FIELD_MIN_DATETIME;
  } else {
    symbolInfo->launchTime =
        ConvertTsToDBTime(CONV(std::uint64_t, symbolInfo->launchTime) * 1000);
  }

  if (symbolInfo->deliveryTime.empty()) {
    symbolInfo->deliveryTime = UNDEFINED_FIELD_MAX_DATETIME;
  } else {
    symbolInfo->deliveryTime =
        ConvertTsToDBTime(CONV(std::uint64_t, symbolInfo->deliveryTime) * 1000);
  }
}

void SymbolTableMaint::stop() {
  if (CONFIG["enableSymbolTableMaint"].as<bool>()) {
    schedulerSymbolTableMaint_->stop();
  }
  LOG_D("Stop symbol table maintainment finished. ");
}

}  // namespace bq::md::svc
