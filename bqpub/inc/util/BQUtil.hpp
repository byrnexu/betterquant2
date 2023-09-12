/*!
 * \file BQUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"
#include "util/Util.hpp"

namespace bq {

enum class MDType : std::uint8_t;

std::string convertTopic(const std::string& topic);

//! MD@Binance@Spot@BTC-USDT@Trades -> $(appName)@MD@Binance@Spot
std::tuple<int, std::string> GetAddrFromTopic(const std::string& appName,
                                              const std::string& topic);

//! shmSvcAddr = MD-SIM@MD@Binance@Spot
//! return MD@Binance@Spot
std::tuple<int, std::string> GetChannelFromAddr(const std::string& shmSvcAddr);

//! returns (MD@Binance@Spot@ADA-USDT@Trades, hashValue)
std::tuple<std::string, TopicHash> MakeTopicInfo(const std::string& marketCode,
                                                 const std::string& symbolType,
                                                 const std::string& symbolCode,
                                                 MDType mdType,
                                                 const std::string& ext = "");

//! RiskMgr 根据 AcctId 分流请求和应答
AcctId GetAcctIdFromTask(const SHMIPCTaskSPtr& task);

std::string ToPrettyStr(Decimal value);

void PrintLogo();

}  // namespace bq
