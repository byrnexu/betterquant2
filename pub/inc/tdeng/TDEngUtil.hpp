/*!
 * \file TDEngUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/29
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

#ifdef __cplusplus
extern "C" {
#endif
using TAOS_RES = void;
#ifdef __cplusplus
}
#endif

namespace bq::tdeng {

class TDEngConnpool;
using TDEngConnpoolSPtr = std::shared_ptr<TDEngConnpool>;

std::tuple<int, std::uint32_t, std::string> GetJsonDataFromRes(
    TAOS_RES *res, std::uint32_t maxRecNum);

std::tuple<int, std::string, int, std::string> QueryDataFromTDEng(
    const TDEngConnpoolSPtr &tdEngConnpool, const std::string &sql,
    std::uint32_t maxRecNum);

}  // namespace bq::tdeng
