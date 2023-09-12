/*!
 * \file RiskCtrlDataMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/19
 *
 * \brief
 */

#include "RiskCtrlDataMgr.hpp"

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"

namespace bq {

RiskCtrlDataMgr::RiskCtrlDataMgr(
    const std::string& step, std::uint32_t threadNo,
    const std::string& fieldGroupUsedToGenHash) noexcept
    : step_(step),
      threadNo_(threadNo),
      fieldGroupUsedToGenHash_(fieldGroupUsedToGenHash) {}

std::tuple<int, std::string> RiskCtrlDataMgr::update(const Doc& doc) {
  const auto tableChgTypeInStrFmt = doc["tableChgType"].GetString();
  const auto tableChgTypeOpt =
      magic_enum::enum_cast<TableChgType>(tableChgTypeInStrFmt);
  if (tableChgTypeOpt == std::nullopt) {
    const auto statusMsg =
        fmt::format("Invalid table chg type {}.", tableChgTypeInStrFmt);
    return {-1, statusMsg};
  }
  const auto tableChgType = tableChgTypeOpt.value();

  switch (tableChgType) {
    case TableChgType::Add:
      return handleTableChgTypeOfAdd(doc);
    case TableChgType::Del:
      return handleTableChgTypeOfDel(doc);
    case TableChgType::Chg:
      return handleTableChgTypeOfChg(doc);
    default: {
      const auto statusMsg =
          fmt::format("Invalid table chg type {}.", tableChgTypeInStrFmt);
      return {-1, statusMsg};
    }
  }
}

}  // namespace bq
