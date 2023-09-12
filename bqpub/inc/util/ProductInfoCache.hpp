/*!
 * \file ProductInfoCache.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "db/DBEngDef.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::productInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::productInfo

namespace bq {

using ProductId2ProductInfo =
    absl::node_hash_map<ProductId, bq::db::productInfo::RecordSPtr>;
using ProductId2ProductInfoSPtr = std::shared_ptr<ProductId2ProductInfo>;

class ProductInfoCache;
using ProductInfoCacheSPtr = std::shared_ptr<ProductInfoCache>;

class ProductInfoCache {
 public:
  ProductInfoCache(const ProductInfoCache&) = delete;
  ProductInfoCache& operator=(const ProductInfoCache&) = delete;
  ProductInfoCache(const ProductInfoCache&&) = delete;
  ProductInfoCache& operator=(const ProductInfoCache&&) = delete;

  explicit ProductInfoCache(const db::DBEngSPtr& dbEng);

  ProductGrpId getProductGrpId(ProductId productId);

 public:
  int load();
  int reload() { return load(); }

 private:
  db::DBEngSPtr dbEng_;

  ProductId2ProductInfoSPtr productId2ProductInfo_{nullptr};
  mutable std::ext::spin_mutex mtxProductId2ProductInfo_;

  std::uint64_t lastUpdateTime_{0};
};

}  // namespace bq
