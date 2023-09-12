/*!
 * \file TDSrvDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/02
 *
 * \brief
 */

#pragma once

#include "OrdMgr.hpp"
#include "PosMgr.hpp"

namespace bq {

using TDOrdMgr = OrdMgr<MIdxOrderIdOfOM, MIdxMarketCodeExchOrderIdOfOM,
                        MIdxAcctIdAndSymInfoOfOM>;
using TDOrdMgrSPtr = std::shared_ptr<TDOrdMgr>;

using TDPosMgr = PosMgr<MIdxMainOfPM>;
using TDPosMgrSPtr = std::shared_ptr<TDPosMgr>;

using OPOrdMgr = OrdMgr<MIdxOrderIdOfOM, MIdxAcctIdAndSymInfoOfOM,
                        MIdxMarketCodeExchOrderIdOfOM>;
using OPOrdMgrSPtr = std::shared_ptr<OPOrdMgr>;

using OPPosMgr = PosMgr<MIdxMainOfPM, MIdxAcctIdAndSymInfoOfPM>;
using OPPosMgrSPtr = std::shared_ptr<OPPosMgr>;

}  // namespace bq
