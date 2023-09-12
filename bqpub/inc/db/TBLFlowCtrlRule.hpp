/*!
 * \file TBLFlowCtrlRule.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/10
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq::db::flowCtrlRule {

struct FieldGroupOfKey {
  std::string step;
  std::string target;
  std::string condition;
  JSER(FieldGroupOfKey, step, target, condition)
};

struct FieldGroupOfVal {
  std::string limitValue;
  JSER(FieldGroupOfVal, limitValue)
};

struct FieldGroupOfAll {
  std::string step;
  std::string target;
  std::string condition;
  std::string limitValue;
  std::string action;
  std::uint32_t no;
  std::string name;

  std::string getKey() const {
    const auto ret = fmt::format("{}{}{}{}{}", step, SEP_OF_REC_IDENTITY,
                                 target, SEP_OF_REC_IDENTITY, condition);
    return ret;
  }

  JSER(FieldGroupOfAll, step, target, condition, limitValue, action, no, name)
};

struct TableSchema {
  inline const static std::string TableName =
      "`BetterQuant`.`riskFlowCtrlRule`";
  using KeyFields = FieldGroupOfKey;
  using ValFields = FieldGroupOfVal;
  using AllFields = FieldGroupOfAll;
};

using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
using RecordWPtr = std::weak_ptr<Record>;

}  // namespace bq::db::flowCtrlRule

using TBLFlowCtrlRule = bq::db::flowCtrlRule::TableSchema;
