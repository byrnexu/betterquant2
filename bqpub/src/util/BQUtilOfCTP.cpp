/*!
 * \file BQUtilOfCTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */

#include "util/BQUtilOfCTP.hpp"

#include "util/Datetime.hpp"
#include "util/Logger.hpp"

namespace bq {

std::string GetFrontDisConnectErrorMsg(int reason) {
  const auto iter = FRONT_DISCONNECT_ERROR_INFO.find(reason);
  if (iter != std::end(FRONT_DISCONNECT_ERROR_INFO)) {
    return iter->second;
  }
  return "N/A";
}

std::uint64_t CalcTs(const std::string& tradingDay,
                     const std::string& updateTime, int updateMillisec) {
  const auto isoDatetime = fmt::format(
      "{}T{}{}{},{}", tradingDay, updateTime.substr(0, 2),
      updateTime.substr(3, 2), updateTime.substr(6, 2), updateMillisec);
  const auto [statusCode, ts] = ConvertISODatetimeToTs(isoDatetime);
  if (statusCode != 0) {
    LOG_W(
        "Calc ts failed. "
        "[tradingDay = {}; updateTime = {}; updateMillisec = {}]",
        tradingDay, updateTime, updateMillisec);
  }
  return ts;
}

}  // namespace bq
