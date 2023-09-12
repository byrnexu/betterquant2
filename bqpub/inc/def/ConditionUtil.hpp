/*!
 * \file FlowCtrlUtil.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "def/ConditionConst.hpp"
#include "def/ConditionDef.hpp"
#include "def/Field.hpp"
#include "util/Pch.hpp"

namespace bq {

struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;

//! vector of ["acctId", "marketCode"]
using ConditionFieldGroup = std::vector<std::string>;

//! acctId=10000&marketCode=SZSE -> ["acctId", "marketCode"]
std::tuple<int, std::string, ConditionFieldGroup> MakeConditionFieldGroup(
    const std::string& conditionInStrFmt);

bool matchPrefix(std::string_view str1, std::string_view str2);

//! acctId=10000&marketCode=SZSE -> {{"acctId":"10000"}, {"marketCode":"SZSE"}}
std::tuple<int, std::string, ConditionTemplate> MakeConditionTemplate(
    const std::string& conditionInStrFmt);

//! check if {{"acctId":"10000"}, {"marketCode":"SZSE"}} match {{"acctId":""},
//! {"marketCode":"SZSE"}}
std::tuple<int, std::string, bool> MatchConditionTemplate(
    const ConditionValue& conditionValue,
    const ConditionTemplate& conditionTemplate);

//! acctId=10000&marketCode=SSE -> acctId&marketCode
std::string ExtractFieldName(const std::string& cond);

//! targetStr = acctId=10000; requiredFields = acctId&marketCode
//! returns acctId=10000&marketCode=
std::string AddRequiredFields(const std::string& targetStr,
                              const std::string& requiredFields);

//! 返回 (statusCode, statusMsg, ConditionValueInStrFmt, ConditionValue)
template <typename T>
std::tuple<int, std::string, std::string, ConditionValue> MakeConditioValue(
    const T& rec, const ConditionFieldGroup& conditionFieldGroup) {
  std::string conditionValueInStrFmt;
  ConditionValue conditionValue;

  constexpr auto fmtStr = FMT_COMPILE("{}{}{}{}");
  for (const auto& fieldName : conditionFieldGroup) {
    if (fieldName == FIELD_ACCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ACCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->acctId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_ACCT_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->acctId_);

    } else if (fieldName == FIELD_ACCT_GRP_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ACCT_GRP_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->acctGrpId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_ACCT_GRP_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->acctGrpId_);

    } else if (fieldName == FIELD_TRD_ACCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_TRD_ACCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->trdAcctId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_TRD_ACCT_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->trdAcctId_);

    } else if (fieldName == FIELD_MARKET_CODE) {
      const auto marketCode = GetMarketName(rec->marketCode_);
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_MARKET_CODE), FMT_STRING(SEP_OF_COND_FIELD),
          marketCode, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_MARKET_CODE] =
          fmt::format(FMT_COMPILE("{}"), marketCode);

    } else if (fieldName == FIELD_SYMBOL_CODE) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SYMBOL_CODE), FMT_STRING(SEP_OF_COND_FIELD),
          rec->symbolCode_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_SYMBOL_CODE] =
          fmt::format(FMT_COMPILE("{}"), rec->symbolCode_);

    } else if (fieldName == FIELD_PRODUCT_GRP_ID) {
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_PRODUCT_GRP_ID),
                      FMT_STRING(SEP_OF_COND_FIELD), rec->productGrpId_,
                      FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_PRODUCT_GRP_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->productGrpId_);

    } else if (fieldName == FIELD_PRODUCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_PRODUCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->productId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_PRODUCT_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->productId_);

    } else if (fieldName == FIELD_USER_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_USER_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->userId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_USER_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->userId_);

    } else if (fieldName == FIELD_STG_GRP_ID) {  // add stgGrpId
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_GRP_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgGrpId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_STG_GRP_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->stgGrpId_);

    } else if (fieldName == FIELD_STG_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_STG_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->stgId_);

    } else if (fieldName == FIELD_STG_INST_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_INST_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgInstId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_STG_INST_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->stgInstId_);

    } else if (fieldName == FIELD_ALGO_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ALGO_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->algoId_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_ALGO_ID] =
          fmt::format(FMT_COMPILE("{}"), rec->algoId_);

    } else if (fieldName == FIELD_SYMBOL_TYPE) {
      const auto symbolType =
          fmt::format("{}", magic_enum::enum_name(rec->symbolType_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SYMBOL_TYPE), FMT_STRING(SEP_OF_COND_FIELD),
          symbolType, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_SYMBOL_TYPE] =
          fmt::format(FMT_COMPILE("{}"), symbolType);

    } else if (fieldName == FIELD_SIDE) {
      const auto side = fmt::format("{}", magic_enum::enum_name(rec->side_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SIDE), FMT_STRING(SEP_OF_COND_FIELD), side,
          FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_SIDE] = fmt::format(FMT_COMPILE("{}"), side);

    } else if (fieldName == FIELD_POS_DIRECTION) {
      const auto posDirection =
          fmt::format("{}", magic_enum::enum_name(rec->posDirection_));
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_POS_DIRECTION),
                      FMT_STRING(SEP_OF_COND_FIELD), posDirection,
                      FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_POS_DIRECTION] =
          fmt::format(FMT_COMPILE("{}"), posDirection);

    } else if (fieldName == FIELD_POS_SIDE) {
      const auto posSide =
          fmt::format("{}", magic_enum::enum_name(rec->posSide_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_POS_SIDE), FMT_STRING(SEP_OF_COND_FIELD),
          posSide, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_POS_SIDE] = fmt::format(FMT_COMPILE("{}"), posSide);

    } else if (fieldName == FIELD_PAR_VALUE) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_PAR_VALUE), FMT_STRING(SEP_OF_COND_FIELD),
          rec->parValue_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_PAR_VALUE] =
          fmt::format(FMT_COMPILE("{}"), rec->parValue_);

    } else if (fieldName == FIELD_ORDER_TYPE) {
      const auto orderType =
          fmt::format("{}", magic_enum::enum_name(rec->orderType_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ORDER_TYPE), FMT_STRING(SEP_OF_COND_FIELD),
          orderType, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_ORDER_TYPE] =
          fmt::format(FMT_COMPILE("{}"), orderType);

    } else if (fieldName == FIELD_ORDER_TYPE_EXTRA) {
      const auto orderTypeExtra =
          fmt::format("{}", magic_enum::enum_name(rec->orderTypeExtra_));
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_ORDER_TYPE_EXTRA),
                      FMT_STRING(SEP_OF_COND_FIELD), orderTypeExtra,
                      FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_ORDER_TYPE_EXTRA] =
          fmt::format(FMT_COMPILE("{}"), orderTypeExtra);

    } else if (fieldName == FIELD_FEE_CURRENCY) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_FEE_CURRENCY), FMT_STRING(SEP_OF_COND_FIELD),
          rec->feeCurrency_, FMT_STRING(SEP_OF_COND_AND)));
      conditionValue[FIELD_FEE_CURRENCY] =
          fmt::format(FMT_COMPILE("{}"), rec->feeCurrency_);

    } else {
      const auto statusMsg = fmt::format(
          "Make condition value failed "
          "because of invalid field name {} in condition field Group.",
          fieldName);
      return {-1, statusMsg, conditionValueInStrFmt, conditionValue};
    }
  }
  if (!conditionValueInStrFmt.empty()) conditionValueInStrFmt.pop_back();

  return {0, "", std::move(conditionValueInStrFmt), std::move(conditionValue)};
}

//! 返回 ConditionValue
template <typename T>
ConditionValue CreateConditioValue(
    const T& rec, const ConditionFieldGroup& conditionFieldGroup) {
  ConditionValue conditionValue;

  for (const auto& fieldName : conditionFieldGroup) {
    if (fieldName == FIELD_ACCT_ID) {
      const auto acctId = fmt::format(FMT_COMPILE("{}"), rec->acctId_);
      conditionValue[FIELD_ACCT_ID] = std::move(acctId);

    } else if (fieldName == FIELD_ACCT_GRP_ID) {
      const auto acctGrpId = fmt::format(FMT_COMPILE("{}"), rec->acctGrpId_);
      conditionValue[FIELD_ACCT_GRP_ID] = std::move(acctGrpId);

    } else if (fieldName == FIELD_TRD_ACCT_ID) {
      const auto trdAcctId = fmt::format(FMT_COMPILE("{}"), rec->trdAcctId_);
      conditionValue[FIELD_TRD_ACCT_ID] = std::move(trdAcctId);

    } else if (fieldName == FIELD_MARKET_CODE) {
      const auto marketCode = GetMarketName(rec->marketCode_);
      conditionValue[FIELD_MARKET_CODE] = std::move(marketCode);

    } else if (fieldName == FIELD_SYMBOL_CODE) {
      conditionValue[FIELD_SYMBOL_CODE] = rec->symbolCode_;

    } else if (fieldName == FIELD_PRODUCT_GRP_ID) {
      const auto productGrpId =
          fmt::format(FMT_COMPILE("{}"), rec->productGrpId_);
      conditionValue[FIELD_PRODUCT_GRP_ID] = std::move(productGrpId);

    } else if (fieldName == FIELD_PRODUCT_ID) {
      const auto productId = fmt::format(FMT_COMPILE("{}"), rec->productId_);
      conditionValue[FIELD_PRODUCT_ID] = std::move(productId);

    } else if (fieldName == FIELD_USER_ID) {
      const auto userId = fmt::format(FMT_COMPILE("{}"), rec->userId_);
      conditionValue[FIELD_USER_ID] = std::move(userId);

    } else if (fieldName == FIELD_STG_GRP_ID) {  // add stgGrpId
      const auto stgGrpId = fmt::format(FMT_COMPILE("{}"), rec->stgGrpId_);
      conditionValue[FIELD_STG_GRP_ID] = std::move(stgGrpId);

    } else if (fieldName == FIELD_STG_ID) {
      const auto stgId = fmt::format(FMT_COMPILE("{}"), rec->stgId_);
      conditionValue[FIELD_STG_ID] = std::move(stgId);

    } else if (fieldName == FIELD_STG_INST_ID) {
      const auto stgInstId = fmt::format(FMT_COMPILE("{}"), rec->stgInstId_);
      conditionValue[FIELD_STG_INST_ID] = std::move(stgInstId);

    } else if (fieldName == FIELD_ALGO_ID) {
      const auto algoId = fmt::format(FMT_COMPILE("{}"), rec->algoId_);
      conditionValue[FIELD_ALGO_ID] = std::move(algoId);

    } else if (fieldName == FIELD_SYMBOL_TYPE) {
      const auto symbolType = fmt::format(
          FMT_COMPILE("{}"), magic_enum::enum_name(rec->symbolType_));
      conditionValue[FIELD_SYMBOL_TYPE] = std::move(symbolType);

    } else if (fieldName == FIELD_SIDE) {
      const auto side =
          fmt::format(FMT_COMPILE("{}"), magic_enum::enum_name(rec->side_));
      conditionValue[FIELD_SIDE] = std::move(side);

    } else if (fieldName == FIELD_POS_SIDE) {
      const auto posSide =
          fmt::format(FMT_COMPILE("{}"), magic_enum::enum_name(rec->posSide_));
      conditionValue[FIELD_POS_SIDE] = std::move(posSide);

    } else if (fieldName == FIELD_PAR_VALUE) {
      const auto parValue = fmt::format(FMT_COMPILE("{}"), rec->parValue_);
      conditionValue[FIELD_PAR_VALUE] = std::move(parValue);

    } else if (fieldName == FIELD_FEE_CURRENCY) {
      conditionValue[FIELD_FEE_CURRENCY] = rec->feeCurrency_;

    } else {
      const auto statusMsg = fmt::format(
          "Make condition value failed "
          "because of invalid field name {} in condition field Group.",
          fieldName);
      return conditionValue;
    }
  }

  return conditionValue;
}

//! 返回 acctId=10000&marketCode=SZSE
template <typename T>
std::string MakeConditioFieldInfoInStrFmt(
    const T* rec, const ConditionFieldGroup& conditionFieldGroup) {
  std::string conditionValueInStrFmt;

  constexpr auto fmtStr = FMT_COMPILE("{}{}{}{}");
  for (const auto& fieldName : conditionFieldGroup) {
    if (fieldName == FIELD_ACCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ACCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->acctId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_ACCT_GRP_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ACCT_GRP_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->acctGrpId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_TRD_ACCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_TRD_ACCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->trdAcctId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_MARKET_CODE) {
      const auto marketCode = GetMarketName(rec->marketCode_);
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_MARKET_CODE), FMT_STRING(SEP_OF_COND_FIELD),
          marketCode, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_SYMBOL_CODE) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SYMBOL_CODE), FMT_STRING(SEP_OF_COND_FIELD),
          rec->symbolCode_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_PRODUCT_GRP_ID) {
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_PRODUCT_GRP_ID),
                      FMT_STRING(SEP_OF_COND_FIELD), rec->productGrpId_,
                      FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_PRODUCT_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_PRODUCT_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->productId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_USER_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_USER_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->userId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_STG_GRP_ID) {  // add stgGrpId
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_GRP_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgGrpId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_STG_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_STG_INST_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_STG_INST_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->stgInstId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_ALGO_ID) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ALGO_ID), FMT_STRING(SEP_OF_COND_FIELD),
          rec->algoId_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_SYMBOL_TYPE) {
      const auto symbolType =
          fmt::format("{}", magic_enum::enum_name(rec->symbolType_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SYMBOL_TYPE), FMT_STRING(SEP_OF_COND_FIELD),
          symbolType, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_SIDE) {
      const auto side = fmt::format("{}", magic_enum::enum_name(rec->side_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_SIDE), FMT_STRING(SEP_OF_COND_FIELD), side,
          FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_POS_DIRECTION) {
      const auto posDirection =
          fmt::format("{}", magic_enum::enum_name(rec->posDirection_));
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_POS_DIRECTION),
                      FMT_STRING(SEP_OF_COND_FIELD), posDirection,
                      FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_POS_SIDE) {
      const auto posSide =
          fmt::format("{}", magic_enum::enum_name(rec->posSide_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_POS_SIDE), FMT_STRING(SEP_OF_COND_FIELD),
          posSide, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_PAR_VALUE) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_PAR_VALUE), FMT_STRING(SEP_OF_COND_FIELD),
          rec->parValue_, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_ORDER_TYPE) {
      const auto orderType =
          fmt::format("{}", magic_enum::enum_name(rec->orderType_));
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_ORDER_TYPE), FMT_STRING(SEP_OF_COND_FIELD),
          orderType, FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_ORDER_TYPE_EXTRA) {
      const auto orderTypeExtra =
          fmt::format("{}", magic_enum::enum_name(rec->orderTypeExtra_));
      conditionValueInStrFmt.append(
          fmt::format(fmtStr, FMT_STRING(FIELD_ORDER_TYPE_EXTRA),
                      FMT_STRING(SEP_OF_COND_FIELD), orderTypeExtra,
                      FMT_STRING(SEP_OF_COND_AND)));

    } else if (fieldName == FIELD_FEE_CURRENCY) {
      conditionValueInStrFmt.append(fmt::format(
          fmtStr, FMT_STRING(FIELD_FEE_CURRENCY), FMT_STRING(SEP_OF_COND_FIELD),
          rec->feeCurrency_, FMT_STRING(SEP_OF_COND_AND)));

    } else {
      const auto statusMsg = fmt::format(
          "Make condition value failed "
          "because of invalid field name {} in condition field Group.",
          fieldName);
      return "";
    }
  }
  if (!conditionValueInStrFmt.empty()) conditionValueInStrFmt.pop_back();

  return conditionValueInStrFmt;
}

}  // namespace bq
