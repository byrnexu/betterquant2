/*!
 * \file TDSvcDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "OrdMgr.hpp"
#include "SHMHeader.hpp"
#include "util/Pch.hpp"

namespace bq {

using TDOrdMgr =
    OrdMgr<MIdxOrderIdOfOM, MIdxMarketCodeExchOrderIdOfOM, MIdxClosedTimeOfOM>;
using TDOrdMgrSPtr = std::shared_ptr<TDOrdMgr>;

}  // namespace bq

namespace bq::td::svc {

struct ApiInfo;
using ApiInfoSPtr = std::shared_ptr<ApiInfo>;

struct ApiInfo {
  std::string apiKey_;
  std::string secKey_;
  std::string password_;
};

struct TDSrvSignal {
  TDSrvSignal(MsgId msgId) : shmHeader_(msgId) {}
  SHMHeader shmHeader_;
};

}  // namespace bq::td::svc
