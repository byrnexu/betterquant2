/*!
 * \file PnlMonitorRangeMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#include "PnlMonitorRangeMgr.hpp"

#include "SHMIPCTask.hpp"
#include "TDSrv.hpp"
#include "db/TBLPnlMonitorRange.hpp"
#include "def/ConditionUtil.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/PosInfo.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/PosSnapshot.hpp"
#include "util/SubMgr.hpp"

namespace bq {

PnlMonitorRangeMgr::PnlMonitorRangeMgr(
    const std::string& step, std::uint32_t threadNo,
    const std::string& fieldGroupUsedToGenHash)
    : RiskCtrlDataMgr(step, threadNo, fieldGroupUsedToGenHash),
      condition2PnlGroup_(std::make_shared<Condition2PnlGroup>()) {}

PnlMonitorRangeMgr::~PnlMonitorRangeMgr() {
  if (subMgr_) {
    subMgr_->stop();
  }
}

std::tuple<int, std::string> PnlMonitorRangeMgr::init(
    const std::vector<db::pnlMonitorRange::RecordSPtr>& recSet) {
  for (const auto& rec : recSet) {
    const auto [statusCode, statusMsg] = handleCurPnlMonitorRec(rec);
    if (statusCode != 0) {
      return {statusCode, statusMsg};
    }
  }
  initSubMgr();
  return {0, ""};
}

std::tuple<int, std::string> PnlMonitorRangeMgr::handleCurPnlMonitorRec(
    const db::pnlMonitorRange::RecordSPtr& rec) {
  //! conditionFieldGroup ["acctId", "trdAcctId"]
  auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);
  if (statusCode != 0) {
    LOG_W("Handle cur pnl monitor rec failed. [{} - {}]", statusCode,
          statusMsg);
    return {statusCode, statusMsg};
  }

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxKey2PnlMonitorRangeGroup_);
    const auto iter = key2PnlMonitorRangeGroup_.find(conditionGroupInStrFmt);
    if (iter == std::end(key2PnlMonitorRangeGroup_)) {
      auto pnlMonitorRange = std::make_shared<PnlMonitorRange>();

      //! conditionGroupInStrFmt = acctId&trdAcctId
      pnlMonitorRange->conditionGroupInStrFmt_ = conditionGroupInStrFmt;

      //! conditionFieldGroup = ["acctId", "trdAcctId"]
      pnlMonitorRange->conditionFieldGroup_ = conditionFieldGroup;

      const auto hashOfCondition =
          XXH3_64bits(rec->condition.data(), rec->condition.size());
      const auto pnlType = magic_enum::enum_cast<PnlType>(rec->pnlType).value();
      const auto limitValue = CONV(Decimal, rec->limitValue);
      const auto pnlThreshold =
          std::make_shared<PnlThreshold>(rec->condition, pnlType, limitValue);
      pnlMonitorRange->hash2PnlThresholdGroup_.emplace(hashOfCondition,
                                                       pnlThreshold);

      key2PnlMonitorRangeGroup_.emplace(conditionGroupInStrFmt,
                                        pnlMonitorRange);

      LOG_I("Add pnl monitor of {} for {}-{}, limit value is {} of {}.",
            rec->condition, step_, threadNo_, limitValue, rec->pnlType);

    } else {
      auto& pnlMonitorRange = iter->second;

      const auto hashOfCondition =
          XXH3_64bits(rec->condition.data(), rec->condition.size());
      const auto pnlType = magic_enum::enum_cast<PnlType>(rec->pnlType).value();
      const auto limitValue = CONV(Decimal, rec->limitValue);
      const auto pnlThreshold =
          std::make_shared<PnlThreshold>(rec->condition, pnlType, limitValue);
      pnlMonitorRange->hash2PnlThresholdGroup_.emplace(hashOfCondition,
                                                       pnlThreshold);

      LOG_I("Add pnl monitor of {} for {}-{}, limit value is {} of {}.",
            rec->condition, step_, threadNo_, limitValue, rec->pnlType);
    }
  }

  return {0, ""};
}

void PnlMonitorRangeMgr::initSubMgr() {
  // 因为每个风控模组都会调用这个代码，为了避免不必要的订阅，增加以下判断
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxKey2PnlMonitorRangeGroup_);
    if (key2PnlMonitorRangeGroup_.empty()) {
      return;
    }
  }

  if (subMgr_) {
    return;
  }

  const auto appNameOfSubscriber =
      fmt::format("pnl-monitor-{}-{}", step_, threadNo_);
  subMgr_ = std::make_shared<SubMgr>(
      appNameOfSubscriber, [this](const void* shmBuf, std::size_t shmBufLen) {
        onDataRecv(shmBuf, shmBufLen);
      });
  subMgr_->start();

  subMgr_->sub(0, "RISK@PubChannel@Trade@PosInfo@All");
}

void PnlMonitorRangeMgr::onDataRecv(const void* shmBuf, std::size_t shmBufLen) {
  const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
  if (shmHeader->msgId_ != MSG_ID_POS_SNAPSHOT_OF_ALL) return;

  LOG_T("Step {} of thread {} recv {} bytes of data.", step_, threadNo_,
        shmBufLen);

  //! 根据订阅得到的消息构建 posSnapshot
  auto buf = malloc(shmBufLen);
  memcpy(buf, shmBuf, shmBufLen);

  const auto posUpdateOfAllForPub =
      MakeMsgSPtrByTask<PosUpdateOfAllForPub>(buf);
  const auto posUpdateOfAll = MakePosUpdateOfAll(posUpdateOfAllForPub);
  posSnapshot_ =
      std::make_shared<PosSnapshot>(std::move(posUpdateOfAll), nullptr);

  //! 以最快的速度通过 key2PnlMonitorRangeGroup_ 包含所有 condition 和空的 pnl
  //! 的 map
  auto condition2PnlGroup = initCondition2PnlGroup();

  //! 将上面得到的 condition2PnlGroup 的 pnl 全填上
  initPnlInCondition2PnlGroup(condition2PnlGroup);

  //! 更新风控用的 condition2PnlGroup_
  updateCondition2PnlGroup(condition2PnlGroup);
}

void PnlMonitorRangeMgr::updateCondition2PnlGroup(
    const Condition2PnlGroupSPtr& condition2PnlGroup) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxCondition2PnlGroup_);
    condition2PnlGroup_ = condition2PnlGroup;
  }
}

PnlSPtr PnlMonitorRangeMgr::getPnl(const std::string& condition) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxCondition2PnlGroup_);
    const auto iter = condition2PnlGroup_->find(condition);
    if (iter != std::end(*condition2PnlGroup_)) {
      return iter->second;
    }
  }
  return nullptr;
}

std::tuple<int, std::string> PnlMonitorRangeMgr::checkIfTriggerRiskCtrl(
    const OrderInfoSPtr& orderInfo, std::uint32_t secDelayOfPrice) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxKey2PnlMonitorRangeGroup_);

    for (const auto& rec : key2PnlMonitorRangeGroup_) {
      const auto& pnlMonitorRange = rec.second;

      //! conditionFieldGroup ["acctId", "trdAcctId"]
      const auto& conditionFieldGroup = pnlMonitorRange->conditionFieldGroup_;

      //! condition = acctId=10000&trdAcctId=100000
      const auto condition =
          MakeConditioFieldInfoInStrFmt(orderInfo.get(), conditionFieldGroup);
      const auto hashOfCondition =
          XXH3_64bits(condition.data(), condition.size());

      const auto iter =
          pnlMonitorRange->hash2PnlThresholdGroup_.find(hashOfCondition);
      if (iter == std::end(pnlMonitorRange->hash2PnlThresholdGroup_)) {
        //! 没有 acctId=10000&trdAcctId=100000 的风控条目
        continue;
      }

      const auto pnl = getPnl(condition);
      if (pnl == nullptr) {
        const auto statusMsg = fmt::format(
            "Trigger risk ctrl because pnl of {} is null.", condition);
        return {SCODE_TD_SRV_RISK_PNL_IS_NULL, statusMsg};
      }

      const auto delay = pnl->delay();
      if (delay > secDelayOfPrice) {
        const auto statusMsg = fmt::format(
            "Trigger risk ctrl because delay {} of pnl {} greater than {}.",
            delay, condition, secDelayOfPrice);
        return {SCODE_TD_SRV_RISK_PNL_IS_TIMEOUT, statusMsg};
      }

      const auto totalPnl = pnl->getTotalPnl();
      const auto& pnlThreshold = iter->second;
      const auto pnlType = pnlThreshold->pnlType_;
      const auto limitValue = pnlThreshold->limitValue_;

      if (pnlType == PnlType::Profit) {
        if (DEC::GT(totalPnl, limitValue)) {
          const auto statusMsg = fmt::format(
              "Trigger risk ctrl because total profit {} of {} "
              "greater than limit value {}.",
              totalPnl, condition, limitValue);
          return {SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT, statusMsg};
        }
      } else {
        if (totalPnl < 0) {
          if (DEC::GT(std::fabs(totalPnl), limitValue)) {
            const auto statusMsg = fmt::format(
                "Trigger risk ctrl because total loss {} of {} "
                "greater than limit value {}.",
                std::fabs(totalPnl), condition, limitValue);
            return {SCODE_TD_SRV_RISK_PNL_EXCEED_LIMIT, statusMsg};
          }
        }
      }
    }
  }

  return {0, ""};
}

Condition2PnlGroupSPtr PnlMonitorRangeMgr::initCondition2PnlGroup() {
  //! 为了加快速度，先将所有的condition取出来放到一个conditionGroup中
  std::vector<std::string> conditionGroup;
  conditionGroup.reserve(1024);
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxKey2PnlMonitorRangeGroup_);
    for (const auto& rec : key2PnlMonitorRangeGroup_) {
      const auto& pnlMonitorRange = rec.second;
      const auto& hash2PnlThresholdGroup =
          pnlMonitorRange->hash2PnlThresholdGroup_;
      for (const auto& rec : hash2PnlThresholdGroup) {
        const auto& pnlThreshold = rec.second;
        conditionGroup.emplace_back(pnlThreshold->condition_);
      }
    }
  }

  //! 根据 conditionGroup 生成 condition2PnlGroup
  const auto ret = std::make_shared<Condition2PnlGroup>();
  for (const auto& condition : conditionGroup) {
    ret->emplace(condition, PnlSPtr());
  }

  return ret;
}

void PnlMonitorRangeMgr::initPnlInCondition2PnlGroup(
    Condition2PnlGroupSPtr& condition2PnlGroup) {
  for (auto& rec : *condition2PnlGroup) {
    const auto& condition = rec.first;
    const auto [statusCode, pnl] =
        posSnapshot_->queryPnl(condition, QuoteCurrencyForCalc("CNY"));
    if (statusCode == 0 && pnl) {
      rec.second = pnl;
      LOG_I("Recv {}", pnl->toStr());
    } else {
      if (pnl) {
        LOG_W("Init pnl of {} in condition2PnlGroup failed. [{} - {}]",
              condition, statusCode, GetStatusMsg(statusCode));
      }
    }
  }
}

/*
{
        "pluginName": "risk-plugin-pnl-monitor",
        "tableChgType": "Add",
        "data": {
                "step": "acctId-trdAcctId",
                "condition": "acctId=10000&trdAcctId=100000",
                "pnlType": "Loss",
                "limitValue": 1000000,
                "name": "",
                "updateTime": "2023-03-18 20:55:32.991380"
        }
}
*/
std::tuple<int, std::string> PnlMonitorRangeMgr::handleTableChgTypeOfAdd(
    const Doc& doc) {
  const auto rec = makeRec(doc);

  const auto [statusCode, statusMsg] = handleCurPnlMonitorRec(rec);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  initSubMgr();

  return {statusCode, statusMsg};
}

std::tuple<int, std::string> PnlMonitorRangeMgr::handleTableChgTypeOfDel(
    const Doc& doc) {
  auto rec = makeRec(doc);
  //! conditionFieldGroup ["acctId", "trdAcctId"]
  auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(rec->condition);

  //! acctId&trdAcctId
  const auto conditionGroupInStrFmt =
      boost::join(conditionFieldGroup, SEP_OF_COND_AND);

  //! key = conditionGroupInStrFmt = acctId&trdAcctId
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxKey2PnlMonitorRangeGroup_);
    const auto iter = key2PnlMonitorRangeGroup_.find(conditionGroupInStrFmt);
    if (iter == std::end(key2PnlMonitorRangeGroup_)) {
      const auto statusMsg =
          fmt::format("Can't find rec when handle table chg type of del. {}",
                      conditionGroupInStrFmt);
      LOG_W(statusMsg);
      return {-1, statusMsg};
    }

    auto& pnlMonitorRange = iter->second;
    //! rec->condition == acctId=10000&trdAcctId=100000
    const auto hashOfCondition =
        XXH3_64bits(rec->condition.data(), rec->condition.size());
    const auto iterRange =
        pnlMonitorRange->hash2PnlThresholdGroup_.find(hashOfCondition);
    if (iterRange == std::end(pnlMonitorRange->hash2PnlThresholdGroup_)) {
      const auto statusMsg =
          fmt::format("Can't find rec when handle table chg type of del. {}",
                      rec->condition);
      LOG_W(statusMsg);
      return {-1, statusMsg};
    }

    LOG_I("Del pnl monitor of {} for {}-{}, limit value is {} of {}.",
          rec->condition, step_, threadNo_, iterRange->second->limitValue_,
          ENUM_TO_STR(iterRange->second->pnlType_));
    pnlMonitorRange->hash2PnlThresholdGroup_.erase(iterRange);
  }

  return {0, ""};
}

std::tuple<int, std::string> PnlMonitorRangeMgr::handleTableChgTypeOfChg(
    const Doc& doc) {
  int statusCode = 0;
  std::string statusMsg;

  std::tie(statusCode, statusMsg) = handleTableChgTypeOfDel(doc);
  if (statusCode != 0) {
    return {statusCode, statusMsg};
  }

  return handleTableChgTypeOfAdd(doc);
}

db::pnlMonitorRange::RecordSPtr PnlMonitorRangeMgr::makeRec(const Doc& doc) {
  auto rec = std::make_shared<db::pnlMonitorRange::Record>();

  rec->step = doc["data"]["step"].GetString();
  rec->condition = doc["data"]["condition"].GetString();
  rec->pnlType = doc["data"]["pnlType"].GetString();
  rec->name = doc["data"]["name"].GetString();
  rec->updateTime = doc["data"]["updateTime"].GetString();
  rec->limitValue = doc["data"]["limitValue"].GetString();

  return rec;
}

}  // namespace bq
