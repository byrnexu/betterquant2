/*!
 * \file BQUtilOfCTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

const static std::map<int, std::string> FRONT_DISCONNECT_ERROR_INFO = {
    {0x1001, "Network read failed."},
    {0x1002, "Network write failed."},
    {0x2001, "Recv heartbeat timeout."},
    {0x2002, "Send Heartbeat failed."},
    {0x2003, "Recv invalid message."}};

std::string GetFrontDisConnectErrorMsg(int reason);

/**
 * @Synopsis
 *
 * @Param tradingDay 20230101
 * @Param updateTime 10:10:10
 * @Param updateMillisec 500
 *
 * @Returns
 */
std::uint64_t CalcTs(const std::string& tradingDay,
                     const std::string& updateTime, int updateMillisec);

}  // namespace bq
