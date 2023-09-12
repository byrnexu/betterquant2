/*!
 * \file AssetsMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "AssetsMgr.hpp"

#include "db/DBEng.hpp"
#include "db/TBLAssetInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"

namespace bq {

AssetsMgr::AssetsMgr() : assetInfoGroup_(std::make_shared<AssetInfoGroup>()) {}

int AssetsMgr::init(const YAML::Node& node, const db::DBEngSPtr& dbEng,
                    const std::string& sql) {
  node_ = node;
  dbEng_ = dbEng;

  const auto ret = initAssetInfoGroup(sql);
  if (ret != 0) {
    LOG_W("Init failed. [{}]", sql);
    return ret;
  }
  return 0;
}

int AssetsMgr::initAssetInfoGroup(const std::string& sql) {
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLAssetInfo>::ExecSql(dbEng_, sql);
  if (ret != 0) {
    LOG_W("Init asset info group failed. {}", sql);
    return ret;
  }

  for (const auto& tblRec : *tblRecSet) {
    const auto recAssetInfo = tblRec.second->getRecWithAllFields();
    const auto assetInfo = MakeAssetInfo(recAssetInfo);
    assetInfoGroup_->emplace(assetInfo->keyHash_, assetInfo);
  }
  LOG_I("Init asset info group success. [size = {}]", assetInfoGroup_->size());
  return 0;
}

//! compareWithAssetsSnapshot目前只在WSCli单线程中运行，无需加锁
UpdateInfoOfAssetGroupSPtr AssetsMgr::compareWithAssetsSnapshot(
    const AssetInfoGroupSPtr& assetInfoGroupFromExch) {
  auto ret = std::make_shared<UpdateInfoOfAssetGroup>();

  for (const auto& rec : *assetInfoGroupFromExch) {
    const auto iter = assetInfoGroup_->find(rec.first);
    if (iter == std::end(*assetInfoGroup_)) {
      //! 如果从交易所的资产列表中发现新的资产
      assetInfoGroup_->emplace(rec);
      ret->assetInfoGroupAdd_->emplace_back(
          std::make_shared<AssetInfo>(*rec.second));
      LOG_I("Find new asset. {}", rec.second->toStr());
    } else {
      //! 来自交易所的资产信息
      auto& assetInfoInAssetsMgr = iter->second;
      //! AssetsMgr中的资产信息
      const auto& assetInfoGroupFromExch = rec.second;
      //! 检查AssetsMgr中的资产信息是否发生了变化
      const auto isTheAssetInfoUpdated = updateByAssetInfoFromExch(
          assetInfoInAssetsMgr, assetInfoGroupFromExch);
      //! 如果AssetsMgr中的资产发生了变化
      if (isTheAssetInfoUpdated == IsTheAssetInfoUpdated::True) {
        //!
        //! 如果存在全仓仓位，那么pnlUnreal_基本每次都会变化，像币安合约资产信息
        //! 的updateTime每次查询也会不同，上面两种情况都会导致每次查询资产快照都
        //! 会有资产变动信息。
        //!
        ret->assetInfoGroupChg_->emplace_back(
            std::make_shared<AssetInfo>(*assetInfoInAssetsMgr));
        LOG_D("Find asset changed. {}", assetInfoInAssetsMgr->toStr());
      }
    }
  }

  for (const auto& rec : *assetInfoGroup_) {
    const auto iter = assetInfoGroupFromExch->find(rec.first);
    //! 如果AssetsMgr中的资产没有在交易所资产列表里找到，那么说明此此资产的数量
    //! 变成了0
    if (iter == std::end(*assetInfoGroupFromExch)) {
      auto& assetInfoInAssetsMgr = rec.second;
      ret->assetInfoGroupDel_->emplace_back(
          std::make_shared<AssetInfo>(*assetInfoInAssetsMgr));
      LOG_I("Find vol of asset changed to 0. {}",
            assetInfoInAssetsMgr->toStr());
    }
  }

  for (const auto& assetInfo : *ret->assetInfoGroupDel_) {
    assetInfoGroup_->erase(assetInfo->keyHash_);
  }

  return ret;
}

IsTheAssetInfoUpdated AssetsMgr::updateByAssetInfoFromExch(
    AssetInfoSPtr& assetInfoInAssetsMgr,
    const AssetInfoSPtr& assetInfoFromExch) {
  IsTheAssetInfoUpdated isTheAssetInfoUpdated = IsTheAssetInfoUpdated::False;

  if (!DEC::EQ(assetInfoFromExch->vol_, assetInfoInAssetsMgr->vol_)) {
    assetInfoInAssetsMgr->vol_ = assetInfoFromExch->vol_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  if (!DEC::EQ(assetInfoFromExch->crossVol_, assetInfoInAssetsMgr->crossVol_)) {
    assetInfoInAssetsMgr->crossVol_ = assetInfoFromExch->crossVol_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  if (!DEC::EQ(assetInfoFromExch->frozen_, assetInfoInAssetsMgr->frozen_)) {
    assetInfoInAssetsMgr->frozen_ = assetInfoFromExch->frozen_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  if (!DEC::EQ(assetInfoFromExch->available_,
               assetInfoInAssetsMgr->available_)) {
    assetInfoInAssetsMgr->available_ = assetInfoFromExch->available_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  if (!DEC::EQ(assetInfoFromExch->pnlUnreal_,
               assetInfoInAssetsMgr->pnlUnreal_)) {
    assetInfoInAssetsMgr->pnlUnreal_ = assetInfoFromExch->pnlUnreal_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  if (!DEC::EQ(assetInfoFromExch->maxWithdraw_,
               assetInfoInAssetsMgr->maxWithdraw_)) {
    assetInfoInAssetsMgr->maxWithdraw_ = assetInfoFromExch->maxWithdraw_;
    isTheAssetInfoUpdated = IsTheAssetInfoUpdated::True;
  }

  //! 只是updateTime_变化不算资产变化

  return isTheAssetInfoUpdated;
}

AssetChgType AssetsMgr::compareWithAssetsUpdate(
    const AssetInfoSPtr& assetInfo) {
  assetInfo->initKeyHash();
  AssetChgType assetChgType;
  if (DEC::ZERO(assetInfo->vol_)) {
    //! 如果收到的资产变动信息的资产数量为0，那么说明当前资产已经不存在
    assetInfoGroup_->erase(assetInfo->keyHash_);
    assetChgType = AssetChgType::Del;
  } else {
    const auto iter = assetInfoGroup_->find(assetInfo->keyHash_);
    if (iter == std::end(*assetInfoGroup_)) {
      //! 如果在AssetsMgr中找不到资产信息，那么说明是新增的资产
      assetInfoGroup_->emplace(assetInfo->keyHash_, assetInfo);
      assetChgType = AssetChgType::Add;
    } else {
      //! 如果资产不为0，且AssetsMgr中存在此资产，说明资产的数量或者冻结等信息
      //! 发生了变化
      (*assetInfoGroup_)[assetInfo->keyHash_] = assetInfo;
      assetChgType = AssetChgType::Chg;
    }
  }
  return assetChgType;
}

void AssetsMgr::cacheUpdateInfoOfAssetGroupOfSyncToDB(
    const UpdateInfoOfAssetGroupSPtr& updateInfoOfAssetGroup) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxUpdateInfoOfAssetGroupOfSyncToDB_);
    updateInfoOfAssetGroupOfSyncToDB_.emplace_back(updateInfoOfAssetGroup);
  }
}

int AssetsMgr::syncUpdateInfoOfAssetGroupToDB() {
  std::vector<UpdateInfoOfAssetGroupSPtr> updateInfoOfAssetGroupOfSyncToDB;
  {
    std::lock_guard<std::ext::spin_mutex> guard(
        mtxUpdateInfoOfAssetGroupOfSyncToDB_);
    std::swap(updateInfoOfAssetGroupOfSyncToDB,
              updateInfoOfAssetGroupOfSyncToDB_);
  }

  for (const auto& rec : updateInfoOfAssetGroupOfSyncToDB) {
    for (const auto& assetInfo : *rec->assetInfoGroupAdd_) {
      const auto identity = GET_RAND_STR();
      const auto sql = assetInfo->getSqlOfInsert();
      const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
      if (ret != 0) {
        LOG_W("Insert asset info to db failed. [{}]", sql);
      }
    }

    for (const auto& assetInfo : *rec->assetInfoGroupDel_) {
      const auto identity = GET_RAND_STR();
      const auto sql = assetInfo->getSqlOfDelete();
      const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
      if (ret != 0) {
        LOG_W("Del asset info from db failed. [{}]", sql);
      }
    }

    for (const auto& assetInfo : *rec->assetInfoGroupChg_) {
      const auto identity = GET_RAND_STR();
      const auto sql = assetInfo->getSqlOfUpdate();
      const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
      if (ret != 0) {
        LOG_W("Update asset info from db failed. [{}]", sql);
      }
    }
  }

  return 0;
}
}  // namespace bq
