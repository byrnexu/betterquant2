/*!
 * \file MDSvcOfCNDef.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/22
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq::md::svc {

using Topic2LastTsGroup = std::map<std::string, std::uint64_t>;
using Topic2LastTsGroupSPtr = std::shared_ptr<Topic2LastTsGroup>;

}  // namespace bq::md::svc
