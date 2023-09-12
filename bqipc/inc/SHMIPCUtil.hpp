/*!
 * \file SHMIPCUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "SHMHeader.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCDef.hpp"
#include "util/PchBase.hpp"

namespace bq {

std::once_flag& GetOnceFlagOfAssignAppName();

template <typename T>
void InitMsgBody(void* target, const T& source) {
  const auto targetAddr = static_cast<char*>(target) + sizeof(SHMHeader);
  const auto sourceAddr =
      reinterpret_cast<const char*>(&source) + sizeof(SHMHeader);
  const auto len = sizeof(T) - sizeof(SHMHeader);
  memcpy(targetAddr, sourceAddr, len);
}

template <typename T>
void InitMsgBodyExt(void* target, const T& source) {
  const auto targetAddr = static_cast<char*>(target) + sizeof(SHMHeader);
  const auto sourceAddr =
      reinterpret_cast<const char*>(&source) + sizeof(SHMHeader);
  const auto len = source.size() - sizeof(SHMHeader);
  memcpy(targetAddr, sourceAddr, len);
}

//!
//! topic like MD@Binance@Spot@BTC-USDT@Candle
//!
//! PubTopic(mdSvc_->getSHMSrv(),
//!   "MD@Binance@Spot@SymbolAdd", R"({"msg":"Hello world!"})");
//!
//! #
//! # in python
//! #
//! self.stg_eng.sub(
//!   stg_inst_info.stg_inst_id, "shm://MD.Binance.Spot/SymbolAdd"
//! )
//!
void PubTopic(const SHMSrvSPtr& shmSrv, const std::string& topicName,
              const std::string& topic, std::string data);

}  // namespace bq
