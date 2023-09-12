/*!
 * \file StgInstInfoIF.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQDefIF.hpp"
#include "util/PchBase.hpp"

namespace bq {

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct StgInstInfo {
  ProductId productId_;
  StgId stgId_;
  std::string stgName_;
  UserId userIdOfAuthor_;
  StgInstId stgInstId_;
  std::string stgInstParams_;
  std::string stgInstName_;
  UserId userId_;
  int isDel_;

  std::string toStr() const;
};

StgInstId StgInstIdOfTriggerSignal(const StgInstInfoSPtr& stgInstInfo);

}  // namespace bq
