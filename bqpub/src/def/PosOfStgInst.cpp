/*!
 * \file PosOfStgInst.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/28
 *
 * \brief
 */

#include "def/PosOfStgInst.hpp"

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/OrderInfoIF.hpp"
#include "def/PosInfoIF.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Decimal.hpp"
#include "util/Pch.hpp"

namespace bq {

std::string PosOfStgInst::toStr() const {
  const auto ret = fmt::format(
      "{} {} pos: {}; "
      "avgOpenPrice: {}; posUnreal: {}; "
      "avgOrderPrice：{}; fee: {}",
      stgInstInfo_->toStr(), getKey(), pos_, avgOpenPrice_, posUnreal_,
      avgOrderPrice_, fee_);
  return ret;
}

std::string PosOfStgInst::getKey() const {
  const auto ret = fmt::format(
      "{}/{}/{}/{}/{}/{}/{}/{}/{}", acctId_, algoId_,
      GetMarketName(marketCode_), magic_enum::enum_name(symbolType_),
      symbolCode_, magic_enum::enum_name(side_),
      magic_enum::enum_name(posSide_), parValue_, feeCurrency_);
  return ret;
}

PosOfStgInstSPtr MakePosOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                                  const PosInfoSPtr& posInfo) {
  auto ret = std::make_shared<PosOfStgInst>();

  if (!ret->stgInstInfo_) {
    ret->stgInstInfo_ = stgInstInfo;
  }

  ret->acctId_ = posInfo->acctId_;
  ret->algoId_ = posInfo->algoId_;

  ret->marketCode_ = posInfo->marketCode_;
  ret->symbolType_ = posInfo->symbolType_;
  ret->symbolCode_ = posInfo->symbolCode_;

  ret->side_ = posInfo->side_;
  ret->posSide_ = posInfo->posSide_;

  ret->parValue_ = posInfo->parValue_;
  ret->feeCurrency_ = posInfo->feeCurrency_;

  ret->fee_ = posInfo->fee_;

  ret->pos_ = posInfo->pos_;
  ret->avgOpenPrice_ = posInfo->avgOpenPrice_;

  return ret;
}

PosOfStgInstSPtr MakePosOfStgInst(const StgInstInfoSPtr& stgInstInfo,
                                  const OrderInfoSPtr& orderInfo) {
  auto ret = std::make_shared<PosOfStgInst>();

  if (!ret->stgInstInfo_) {
    ret->stgInstInfo_ = stgInstInfo;
  }

  ret->acctId_ = orderInfo->acctId_;
  ret->algoId_ = orderInfo->algoId_;

  ret->marketCode_ = orderInfo->marketCode_;
  ret->symbolType_ = orderInfo->symbolType_;
  ret->symbolCode_ = orderInfo->symbolCode_;

  ret->side_ = orderInfo->side_;
  ret->posSide_ = orderInfo->posSide_;

  ret->parValue_ = orderInfo->parValue_;
  ret->feeCurrency_ = orderInfo->feeCurrency_;

  ret->avgOrderPrice_ = orderInfo->orderPrice_;

  if (orderInfo->side_ == Side::Bid) {
    ret->posUnreal_ = orderInfo->orderSize_ - orderInfo->dealSize_;
  } else {  // Side::Ask
    ret->posUnreal_ = std::fabs(orderInfo->orderSize_ + orderInfo->dealSize_);
  }

  return ret;
}

}  // namespace bq
