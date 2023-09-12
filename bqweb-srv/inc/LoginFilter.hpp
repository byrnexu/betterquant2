/*!
 * \file LoginFilter.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#pragma once

#include <drogon/HttpFilter.h>

#include "def/BQDef.hpp"
#include "util/Pch.hpp"

using namespace drogon;

namespace bq {

class LoginFilter : public drogon::HttpFilter<LoginFilter> {
 public:
  void doFilter(const HttpRequestPtr &req, FilterCallback &&fcb,
                FilterChainCallback &&fccb) final;
};

}  // namespace bq
