/*!
 * \file Datetime.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/Datetime.hpp"

#include "def/Def.hpp"
#include "util/Logger.hpp"
#include "util/StdExt.hpp"

namespace bq {

std::uint64_t GetTotalNSSince1970() {
  struct timespec time_start = {0, 0};
  clock_gettime(CLOCK_REALTIME, &time_start);
  std::uint64_t ret = time_start.tv_sec * 1000000000 + time_start.tv_nsec;
  return ret;
}

std::uint64_t GetTotalUSSince1970() {
  boost::posix_time::ptime epoch(
      boost::gregorian::date(1970, boost::gregorian::Jan, 1));
  const auto now = boost::posix_time::microsec_clock::universal_time();
  const auto time_duration = now - epoch;
  return time_duration.total_microseconds();
}

std::uint64_t GetTotalMSSince1970() {
  const auto ret = GetTotalUSSince1970() / 1000;
  return ret;
}

std::uint64_t GetTotalSecSince1970() {
  const auto ret = GetTotalUSSince1970() / 1000000;
  return ret;
}

std::string ConvertTsToDBTime(std::uint64_t ts) {
  const auto t = boost::posix_time::from_time_t(ts / 1000000);
  const auto us = ts % 1000000;
  std::string ret = boost::posix_time::to_iso_extended_string(t);
  ret[10] = ' ';
  ret = fmt::format("{}.{:06}", ret, us);
  return ret;
}

std::string ConvertTsToPtime(std::uint64_t ts) {
  const auto t = boost::posix_time::from_time_t(ts / 1000000);
  const auto us = ts % 1000000;
  const auto ret = fmt::format("{}.{:06}", to_iso_string(t), us);
  return ret;
}

std::uint64_t ConvertDBTimeToTS(std::string dbTime) {
  dbTime[10] = 'T';
  std::ext::erase_if(dbTime, [](char c) { return c == '-' || c == ':'; });
  const auto [ret, ts] = ConvertISODatetimeToTs(dbTime);
  return ts;
}

std::tuple<int, std::uint64_t> ConvertISODatetimeToTs(
    const std::string& isoDatetime) {
  try {
    const auto t = boost::posix_time::from_iso_string(isoDatetime);
    boost::posix_time::ptime epoch(
        boost::gregorian::date(1970, boost::gregorian::Jan, 1));
    const auto time_duration = t - epoch;
    return {0, time_duration.total_microseconds()};
  } catch (const std::exception& e) {
    const auto statusMsg = fmt::format(
        "Convert iso datetime {} to ts failed. [{}]", isoDatetime, e.what());
    LOG_W(statusMsg);
    return {-1, 0};
  }
}

std::uint64_t ConvertDatetimeToTs(std::int64_t datetime) {
  const auto dt = fmt::format("{}", datetime);
  const auto l = dt.substr(0, 8);
  const auto r = dt.substr(8, dt.size() - 8);
  const auto isoDatetime = fmt::format("{}T{}", l, r);
  const auto [statusCode, ret] = ConvertISODatetimeToTs(isoDatetime);
  return ret;
}

std::string GetDateInStrFmtFromTs(std::uint64_t ts) {
  const auto datetime = ConvertTsToPtime(ts);
  const auto ret = datetime.substr(0, 8);
  return ret;
}

boost::gregorian::date GetDateFromTs(std::uint64_t ts) {
  const auto datetime = boost::posix_time::from_time_t(ts);
  const auto ret = datetime.date();
  return ret;
}

//! updateTime = "00:00:00"
int GetTotalSec(const std::string& updateTime) {
  if (updateTime.size() < 8) {
    const auto statusMsg =
        fmt::format("Get total sec from {} failed. [size < 8]", updateTime);
    LOG_W(statusMsg);
    return 0;
  }
  const auto hours = CONV(int, updateTime.substr(0, 2));
  const auto minutes = CONV(int, updateTime.substr(3, 2));
  const auto seconds = CONV(int, updateTime.substr(6, 2));
  const auto ret = hours * 3600 + minutes * 60 + seconds;
  return ret;
}

//! update_time = 20230211204400436
int GetTotalSecFromDatetime(std::uint64_t updateTime) {
  const auto dt = fmt::format("{}", updateTime);
  if (dt.size() < 14) {
    const auto statusMsg =
        fmt::format("Get total sec from datetime {} failed. [size < 14]", dt);
    LOG_W(statusMsg);
    return 0;
  }
  const auto hours = CONV(int, dt.substr(8, 2));
  const auto minutes = CONV(int, dt.substr(10, 2));
  const auto seconds = CONV(int, dt.substr(12, 2));
  const auto ret = hours * 3600 + minutes * 60 + seconds;
  return ret;
}

int GetSecOffsetOfDay(const boost::posix_time::ptime& t) {
  const auto ret = t - boost::posix_time::ptime(
                           t.date(), boost::posix_time::time_duration(0, 0, 0));
  return ret.total_seconds();
}

}  // namespace bq
