/*!
 * \file FlowCtrlUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/11
 *
 * \brief
 */

#include "def/ConditionUtil.hpp"

#include "def/BQConst.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/Def.hpp"
#include "def/Field.hpp"

namespace bq {

//! acctId=10000&marketCode=SZSE -> ["acctId", "marketCode"]
std::tuple<int, std::string, ConditionFieldGroup> MakeConditionFieldGroup(
    const std::string& conditionInStrFmt) {
  ConditionFieldGroup conditionFieldGroup;
  if (conditionInStrFmt.empty()) {
    return {0, "", conditionFieldGroup};
  }

  std::vector<std::string> recGroup;
  boost::split(recGroup, conditionInStrFmt, boost::is_any_of(SEP_OF_COND_AND));

  for (const auto& rec : recGroup) {
    std::vector<std::string> fieldGroup;
    boost::split(fieldGroup, rec, boost::is_any_of(SEP_OF_COND_FIELD));
    if (fieldGroup.size() != 1 && fieldGroup.size() != 2) {
      const auto statusMsg = fmt::format(
          "Make condition field group failed "
          "because of invalid field num {} in condition {}.",
          rec, conditionInStrFmt);
      return {-1, statusMsg, conditionFieldGroup};
    }
    conditionFieldGroup.emplace_back(fieldGroup[0]);
  }

  return {0, "", conditionFieldGroup};
}

//! acctId=10000&marketCode=SZSE -> {{"acctId":"10000"}, {"marketCode":"SZSE"}}
std::tuple<int, std::string, ConditionTemplate> MakeConditionTemplate(
    const std::string& conditionInStrFmt) {
  ConditionTemplate conditionTemplate;
  if (conditionInStrFmt.empty()) {
    return {0, "", conditionTemplate};
  }

  std::vector<std::string> recGroup;
  boost::split(recGroup, conditionInStrFmt, boost::is_any_of(SEP_OF_COND_AND));

  for (const auto& rec : recGroup) {
    std::vector<std::string> fieldGroup;
    boost::split(fieldGroup, rec, boost::is_any_of(SEP_OF_COND_FIELD));
    if (fieldGroup.size() != 2) {
      const auto statusMsg = fmt::format(
          "Make condition field group failed "
          "because of invalid field num {} in condition {}.",
          rec, conditionInStrFmt);
      return {-1, statusMsg, conditionTemplate};
    }
    conditionTemplate.emplace(std::make_pair(fieldGroup[0], fieldGroup[1]));
  }

  return {0, "", conditionTemplate};
}

bool matchPrefix(std::string_view str1, std::string_view str2) {
  assert(!str2.empty() && "!str2.empty()");
  if (str2[str2.size() - 1] != '*') {
    return false;
  }

  //! 解析第一个字符串
  size_t pos1 = str1.find_first_of("0123456789");
  if (pos1 == std::string_view::npos || pos1 == 0) {
    return false;
  }
  std::string_view substr1 = str1.substr(0, pos1);

  //! 解析第二个字符串
  std::string_view substr2 = str2.substr(0, str2.size() - 1);

  //! 比较两个子字符串是否相等
  return substr1 == substr2;
}

std::tuple<int, std::string, bool> MatchConditionTemplate(
    const ConditionValue& conditionValue,
    const ConditionTemplate& conditionTemplate) {
  if (conditionValue.size() != conditionTemplate.size()) {
    const auto statusMsg = fmt::format(
        "Make condition template failed because of "
        "condition value size {} not equal to condition template size {}.",
        conditionValue.size(), conditionTemplate.size());
    return {-1, statusMsg, false};
  }

  auto iterCV = std::begin(conditionValue);
  auto iterCT = std::begin(conditionTemplate);

  for (; iterCV != std::end(conditionValue); ++iterCV, ++iterCT) {
    const auto& cvKey = iterCV->first;
    const auto& ctKey = iterCT->first;

    //! ConditionValue和ConditionTemplate的键值肯定是匹配的
    if (cvKey != ctKey) {
      const auto statusMsg = fmt::format(
          "Make condition template failed because of "
          "condition value key {} not equal to condition template key {}.",
          cvKey, ctKey);
      return {-1, statusMsg, false};
    }

    const auto& cvValue = iterCV->second;
    const auto& ctValue = iterCT->second;

    //! 如果ctValue为空或者是通配符，当前字段符合条件，跳过
    if (ctValue.empty() || ctValue == "*") {
      continue;
    }

    if (matchPrefix(cvValue, ctValue)) {
      continue;
    }

    if (cvValue != ctValue) {
      return {0, "", false};
    }
  }

  return {0, "", true};
}

//! acctId=10000&marketCode=SSE -> acctId&marketCode
std::string ExtractFieldName(const std::string& cond) {
  std::vector<std::string> fieldName2ValueGroup;
  boost::split(fieldName2ValueGroup, cond, boost::is_any_of(SEP_OF_COND_AND));
  if (fieldName2ValueGroup.empty()) {
    return "";
  }

  std::string ret;
  for (const auto& rec : fieldName2ValueGroup) {
    std::vector<std::string> fieldName2Value;
    boost::split(fieldName2Value, rec, boost::is_any_of("="));
    if (fieldName2Value.size() != 2) {
      return "";
    }
    ret = ret + fieldName2Value[0] + SEP_OF_COND_AND;
  }
  if (!ret.empty()) ret.pop_back();

  return ret;
}

//! targetStr = acctId=10000; requiredFields = acctId&marketCode
//! returns acctId=10000&marketCode=
std::string AddRequiredFields(const std::string& targetStr,
                              const std::string& requiredFields) {
  std::string ret = targetStr;
  const auto [statusCode, statusMsg, conditionFieldGroup] =
      MakeConditionFieldGroup(requiredFields);
  for (const auto& field : conditionFieldGroup) {
    if (targetStr.find(field) == std::string::npos) {
      ret.append(SEP_OF_COND_AND).append(field).append("=");
    }
  }
  if (boost::starts_with(ret, SEP_OF_COND_AND)) ret.erase(0, 1);
  return ret;
}

}  // namespace bq
