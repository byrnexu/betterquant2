/*!
 * \file SHM.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/08
 *
 * \brief
 */

#pragma once

#include "def/SHMDef.hpp"

namespace bq {

std::string FreeSegmentPerDesc(const bip::managed_shared_memory& segment);

}  // namespace bq
