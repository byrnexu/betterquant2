/*!
 * \file PosMgrOfStgInst.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/28
 *
 * \brief
 */

#include "PosMgrOfStgInst.hpp"

#include "OrdMgr.hpp"
#include "PosMgr.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/PosOfStgInst.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Decimal.hpp"
#include "util/Pch.hpp"

namespace bq::stg {

PosMgrOfStgInst::PosMgrOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                                 const StgPosMgrSPtr& posMgr,
                                 const StgOrdMgrSPtr& ordMgr)
    : posOfStgInstTable_(std::make_shared<PosOfStgInstTable>()) {
  //! 从 posMgr 获取已有的仓位信息
  const auto posInfoGroupInPosMgr =
      posMgr->getPosInfoGroupOfStgInst<LockFunc::True, DeepClone::True>(
          stgInstInfo);
  for (const auto& posInfo : posInfoGroupInPosMgr) {
    const auto posOfStgInst = MakePosOfStgInst(stgInstInfo, posInfo);
    posOfStgInstTable_->emplace(posInfo->getKeyOfStgInst(), posOfStgInst);
  }

  //! 从 ordMgr 获取在途的仓位信息
  const auto orderInfoGroupInOrdMgr =
      ordMgr->getOrderInfoGroupOfStgInst<LockFunc::True, DeepClone::True>(
          stgInstInfo);

  //! 获取在途订单列表
  for (const auto& orderInfo : orderInfoGroupInOrdMgr) {
    //! 根据在途订单列表创建仓位信息
    const auto posOfStgInst = MakePosOfStgInst(stgInstInfo, orderInfo);
    auto iter = posOfStgInstTable_->find(posOfStgInst->getKey());
    if (iter == std::end(*posOfStgInstTable_)) {
      posOfStgInstTable_->emplace(posOfStgInst->getKey(), posOfStgInst);
      continue;
    }
    const auto oldPosUnreal = iter->second->posUnreal_;
    const auto oldAvgOrderPrice = iter->second->avgOrderPrice_;
    const auto newPosUnreal = posOfStgInst->posUnreal_;
    const auto newAvgOrderPrice = posOfStgInst->avgOrderPrice_;

    const auto totalPosUnreal = oldPosUnreal + newPosUnreal;
    const auto totalOrderAmt =
        oldAvgOrderPrice * oldPosUnreal + newAvgOrderPrice * newPosUnreal;

    iter->second->posUnreal_ += newPosUnreal;
    if (!DEC::ZERO(totalPosUnreal)) {
      iter->second->avgOrderPrice_ = totalOrderAmt / totalPosUnreal;
    }
  }
}

PosOfStgInstGroupSPtr PosMgrOfStgInst::get() const {
  auto ret = std::make_shared<PosOfStgInstGroup>();
  for (const auto& rec : *posOfStgInstTable_) {
    ret->emplace_back(rec.second);
  }
  return ret;
}

}  // namespace bq::stg
