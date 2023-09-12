/*!
 * \file SessionTable.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

struct Session {
  Session();
  UserId userId_{0};
  std::string username_;
  std::string password_;
  std::string token_;
  boost::posix_time::ptime loginTime_;
  boost::posix_time::ptime lastActiveTime_;
  std::string toJson();
};
using SessionSPtr = std::shared_ptr<Session>;

class SessionTable {
 public:
  SessionSPtr addSession(UserId userId, const std::string& username,
                         const std::string& password);
  int removeSession(const std::string& token);
  void removeSession(UserId userId);
  void removeExpiredSession(std::uint32_t thresholdForSessionTimeout);
  SessionSPtr getSession(const std::string& token);
  bool checkAndUpdateLastActiveTimeOfSession(const std::string& token);

 private:
  std::map<std::string, SessionSPtr> token2SessionGroup_;
  std::ext::spin_mutex mtxToken2SessionGroup_;
};

}  // namespace bq
