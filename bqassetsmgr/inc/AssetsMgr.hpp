/*!
 * \file AssetsMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "db/DBEng.hpp"
#include "db/TBLAssetInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"
#include "def/Const.hpp"
#include "def/DataStruOfAssets.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/Decimal.hpp"
#include "util/Json.hpp"
#include "util/Logger.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq {

class AssetsMgr {
 public:
  AssetsMgr(const AssetsMgr&) = delete;
  AssetsMgr& operator=(const AssetsMgr&) = delete;
  AssetsMgr(const AssetsMgr&&) = delete;
  AssetsMgr& operator=(const AssetsMgr&&) = delete;

  AssetsMgr();

 public:
  int init(const YAML::Node& node, const db::DBEngSPtr& dbEng,
           const std::string& sql);

 private:
  int initAssetInfoGroup(const std::string& sql);

 public:
  //! 此函数用于交易所查回来的全量资产信息和本地资产信息的比较
  UpdateInfoOfAssetGroupSPtr compareWithAssetsSnapshot(
      const AssetInfoGroupSPtr& assetInfoGroupFromExch);

 private:
  IsTheAssetInfoUpdated updateByAssetInfoFromExch(
      AssetInfoSPtr& assetInfoInAssetsMgr,
      const AssetInfoSPtr& assetInfoFromExch);

 public:
  //! 此函数用于交易所返回的资产变动信息和本地资产信息的比较
  AssetChgType compareWithAssetsUpdate(const AssetInfoSPtr& assetInfo);

 public:
  template <LockFunc lockFunc>
  void add(const AssetInfoSPtr& assetInfo);

  template <LockFunc lockFunc>
  void remove(const AssetInfoSPtr& assetInfo);

  template <LockFunc lockFunc>
  void update(const AssetInfoSPtr& assetInfo);

 public:
  template <LockFunc lockFunc>
  std::vector<AssetInfoSPtr> getAssetInfoGroup() const;

 public:
  template <LockFunc lockFunc>
  std::string toStr() const;

 public:
  void cacheUpdateInfoOfAssetGroupOfSyncToDB(
      const UpdateInfoOfAssetGroupSPtr& updateInfoOfAssetGroup);
  int syncUpdateInfoOfAssetGroupToDB();

 private:
  YAML::Node node_;
  db::DBEngSPtr dbEng_{nullptr};

  AssetInfoGroupSPtr assetInfoGroup_{nullptr};
  mutable std::ext::spin_mutex mtxAssetInfoGroup_;

  std::vector<UpdateInfoOfAssetGroupSPtr> updateInfoOfAssetGroupOfSyncToDB_;
  mutable std::ext::spin_mutex mtxUpdateInfoOfAssetGroupOfSyncToDB_;
};

template <LockFunc lockFunc>
void AssetsMgr::add(const AssetInfoSPtr& assetInfo) {
  assetInfo->initKeyHash();
  {
    SPIN_LOCK(mtxAssetInfoGroup_);
    const auto iter = assetInfoGroup_->find(assetInfo->keyHash_);
    if (iter == std::end(*assetInfoGroup_)) {
      assetInfoGroup_->emplace(assetInfo->keyHash_, assetInfo);
    } else {
      //! tdSrv理论上不可能进入这里
      LOG_W("The asset info to be added already exists. {}",
            assetInfo->toStr());
    }
  }
}

template <LockFunc lockFunc>
void AssetsMgr::remove(const AssetInfoSPtr& assetInfo) {
  assetInfo->initKeyHash();
  {
    SPIN_LOCK(mtxAssetInfoGroup_);
    const auto iter = assetInfoGroup_->find(assetInfo->keyHash_);
    if (iter != std::end(*assetInfoGroup_)) {
      assetInfoGroup_->erase(iter);
    } else {
      //! tdSrv理论上不可能进入这里
      LOG_W("The asset info to be deleted does not exist. {}",
            assetInfo->toStr());
    }
  }
}

template <LockFunc lockFunc>
void AssetsMgr::update(const AssetInfoSPtr& assetInfo) {
  assetInfo->initKeyHash();
  {
    SPIN_LOCK(mtxAssetInfoGroup_);
    const auto iter = assetInfoGroup_->find(assetInfo->keyHash_);
    if (iter != std::end(*assetInfoGroup_)) {
      iter->second = assetInfo;
    } else {
      //! tdSrv理论上不可能进入这里
      LOG_W("The asset info to be update does not exist. {}",
            assetInfo->toStr());
    }
  }
}

template <LockFunc lockFunc>
std::vector<AssetInfoSPtr> AssetsMgr::getAssetInfoGroup() const {
  std::vector<AssetInfoSPtr> ret;
  {
    SPIN_LOCK(mtxAssetInfoGroup_);
    for (const auto& rec : *assetInfoGroup_) {
      ret.emplace_back(std::make_shared<AssetInfo>(*rec.second));
    }
  }
  return ret;
}

template <LockFunc lockFunc>
std::string AssetsMgr::toStr() const {
  std::string ret;
  {
    SPIN_LOCK(mtxAssetInfoGroup_);
    for (const auto& rec : *assetInfoGroup_) {
      const auto& assetInfo = rec.second;
      ret = ret + "\n" + assetInfo->toStr();
    }
  }
  return ret;
}

}  // namespace bq
