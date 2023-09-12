/*!
 * \file RiskMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "OrdMgr.hpp"
#include "PosMgr.hpp"

namespace bq {

using RiskOrdMgr = OrdMgr<MIdxOrderIdOfOM, MIdxMarketCodeExchOrderIdOfOM>;
using RiskOrdMgrSPtr = std::shared_ptr<RiskOrdMgr>;

using RiskPosMgr = PosMgr<MIdxMainOfPM, MIdxStgInstIdOfPM>;
using RiskPosMgrSPtr = std::shared_ptr<RiskPosMgr>;

}  // namespace bq
