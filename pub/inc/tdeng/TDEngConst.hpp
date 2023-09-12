/*!
 * \file TDEngConst.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/23
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::tdeng {

const static std::string DEFAULT_TDENG_PARAM =
    "host=0.0.0.0; port=0; db=; username=root; password=taosdata; "
    "connPoolSize=4";

}  // namespace bq::tdeng
