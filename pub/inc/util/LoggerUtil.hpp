#pragma once

#include "util/Logger.hpp"

namespace bq {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

inline std::string logWithArg(LogLevel lvl, const std::string& fmt) {
  switch (lvl) {
    case LogLevel::TRACE:
      LOG_T(fmt);
      break;
    case LogLevel::DEBUG:
      LOG_D(fmt);
      break;
    case LogLevel::INFO:
      LOG_I(fmt);
      break;
    case LogLevel::WARN:
      LOG_W(fmt);
      break;
    case LogLevel::ERROR:
      LOG_E(fmt);
      break;
    case LogLevel::CRITICAL:
      LOG_C(fmt);
      break;
  }
  return fmt;
}

inline std::string logWithArg(LogLevel lvl, const char* fmt) {
  std::string fmtStr(fmt);
  return logWithArg(lvl, fmt);
}

template <typename Arg, typename... Args>
inline std::string logWithArg(LogLevel lvl, const std::string& fmt,
                              const Arg& arg, const Args&... args) {
  std::string data = fmt::format(fmt, arg, args...);
  switch (lvl) {
    case LogLevel::TRACE:
      LOG_T(data);
      break;
    case LogLevel::DEBUG:
      LOG_D(data);
      break;
    case LogLevel::INFO:
      LOG_I(data);
      break;
    case LogLevel::WARN:
      LOG_W(data);
      break;
    case LogLevel::ERROR:
      LOG_E(data);
      break;
    case LogLevel::CRITICAL:
      LOG_C(data);
      break;
  }
  return data;
}

template <typename... Args>
inline std::string logWithArg(LogLevel lvl, const char* fmt,
                              const Args&... args) {
  std::string fmtStr(fmt);
  return logWithArg(lvl, fmtStr, args...);
}

inline std::string log(LogLevel lvl, const std::string& fmt,
                       const std::vector<std::string>& args) {
  switch (args.size()) {
    case 0:
      return logWithArg(lvl, fmt);
    case 1:
      return logWithArg(lvl, fmt, args[0]);
    case 2:
      return logWithArg(lvl, fmt, args[0], args[1]);
    case 3:
      return logWithArg(lvl, fmt, args[0], args[1], args[2]);
    case 4:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3]);
    case 5:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4]);
    case 6:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5]);
    case 7:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6]);
    case 8:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7]);
    case 9:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8]);
    case 10:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9]);
    case 11:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9], args[10]);
    case 12:
      return logWithArg(lvl, fmt, args[0], args[1], args[2], args[3], args[4],
                        args[5], args[6], args[7], args[8], args[9], args[10],
                        args[11]);
    default:
      LOG_W("Too many args of {}.", fmt);
      break;
  }
  return "";
}

}  // namespace bq
