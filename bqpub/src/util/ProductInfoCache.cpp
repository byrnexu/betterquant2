/*!
 * \file ProductInfoCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/ProductInfoCache.hpp"

#include "db/DBE.hpp"
#include "db/TBLProductInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQConst.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"

namespace bq {

ProductInfoCache::ProductInfoCache(const db::DBEngSPtr& dbEng)
    : dbEng_(dbEng),
      productId2ProductInfo_(std::make_shared<ProductId2ProductInfo>()) {}

int ProductInfoCache::load() {
  auto sql = fmt::format("SELECT * FROM {}; ", TBLProductInfo::TableName);

  auto [retOfExec, tblRecSet] =
      db::TBLRecSetMaker<TBLProductInfo>::ExecSql(dbEng_, sql);
  if (retOfExec != 0) {
    LOG_W("Query product info failed.");
    return retOfExec;
  }

  std::uint64_t lastUpdateTimeInDB = 0;
  auto productId2ProductInfo = std::make_shared<ProductId2ProductInfo>();
  for (const auto& tblRec : *tblRecSet) {
    const auto rec = tblRec.second->getRecWithAllFields();
    auto updateTime = ConvertDBTimeToTS(rec->updateTime);
    if (updateTime > lastUpdateTimeInDB) {
      lastUpdateTimeInDB = updateTime;
    }
    productId2ProductInfo->emplace(rec->productId, rec);
  }
  LOG_D("Load product info. [size = {}]", productId2ProductInfo_->size());

  if (lastUpdateTimeInDB > lastUpdateTime_) {
    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxProductId2ProductInfo_);
      productId2ProductInfo_ = productId2ProductInfo;
    }
    lastUpdateTime_ = lastUpdateTimeInDB;
  }

  return 0;
}

ProductGrpId ProductInfoCache::getProductGrpId(ProductId productId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxProductId2ProductInfo_);
    const auto iter = productId2ProductInfo_->find(productId);
    if (iter != std::end(*productId2ProductInfo_)) {
      return iter->second->productGrpId;
    }
  }
  return 0;
}

}  // namespace bq
