/*!
 * \file SessionTable.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/03/10
 *
 * \brief
 */

#include "SessionTable.hpp"

#include "WebSrvConst.hpp"
#include "def/StatusCode.hpp"
#include "util/Logger.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"

namespace bq {

Session::Session()
    : loginTime_(boost::posix_time::second_clock::local_time()),
      lastActiveTime_(boost::posix_time::second_clock::local_time()) {}

std::string Session::toJson() {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("userId");
  writer.Uint(userId_);
  writer.Key("username");
  writer.String(username_.c_str());
  writer.Key("password");
  writer.String(password_.c_str());
  writer.Key("token");
  writer.String(token_.c_str());
  writer.EndObject();
  return strBuf.GetString();
}

//! 增加一个会话信息
SessionSPtr SessionTable::addSession(UserId userId, const std::string& username,
                                     const std::string& password) {
  auto session = std::make_shared<Session>();
  session->userId_ = userId;
  session->username_ = username;
  session->password_ = password;
  session->token_ = RandomStr::get_mutable_instance().get(TOKEN_SIZE);
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    auto ret = token2SessionGroup_.emplace(session->token_, session);
    if (ret.second != true) {
      LOG_W("Add session failed. {}", session->toJson());
    } else {
      LOG_I("Add session success. {}", session->toJson());
    }
    if (token2SessionGroup_.size() % 1000 == 0) {
      LOG_W("Size of session table is {}.", token2SessionGroup_.size());
    }
  }
  return session;
}

//! 根据token移除一个会话信息
int SessionTable::removeSession(const std::string& token) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    const auto iter = token2SessionGroup_.find(token);
    if (iter != std::end(token2SessionGroup_)) {
      auto& session = iter->second;
      LOG_I("Remove session success. {}", session->toJson());
      token2SessionGroup_.erase(iter);
      return 0;
    }
  }
  LOG_W("Remove session failed because of session not exists.");
  return SCODE_WEB_SRV_REMOVE_SESSION_FAILED;
}

//! 根据userId移除一个会话信息
void SessionTable::removeSession(UserId userId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    std::ext::erase_if(token2SessionGroup_, [userId](const auto& rec) {
      const auto& session = rec.second;
      if (session->userId_ == userId) {
        LOG_I("Remove session because of another login. {}", session->toJson());
        return true;
      }
      return false;
    });
  }
}

//! 移除超时的连接
void SessionTable::removeExpiredSession(
    std::uint32_t thresholdForSessionTimeout) {
  const auto now = boost::posix_time::second_clock::local_time();
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    std::ext::erase_if(token2SessionGroup_, [&](const auto& token2Session) {
      auto& session = token2Session.second;
      const auto timeDur = now - session->lastActiveTime_;
      if (timeDur.total_seconds() > thresholdForSessionTimeout) {
        LOG_W(
            "Remove Session of user {} because of it is expired for {} "
            "seconds. [threshold = {}] [session table size = {}]",
            session->userId_, timeDur.total_seconds(),
            thresholdForSessionTimeout, token2SessionGroup_.size());
        return true;
      }
      return false;
    });
  }
}

//! 根据token获取会话
SessionSPtr SessionTable::getSession(const std::string& token) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    const auto iter = token2SessionGroup_.find(token);
    if (iter != std::end(token2SessionGroup_)) {
      auto& session = iter->second;
      return std::make_shared<Session>(*session);
    }
  }
  return nullptr;
}

//! 更新会话的最后活跃时间
bool SessionTable::checkAndUpdateLastActiveTimeOfSession(
    const std::string& token) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxToken2SessionGroup_);
    const auto iter = token2SessionGroup_.find(token);
    if (iter != std::end(token2SessionGroup_)) {
      auto& session = iter->second;
      session->lastActiveTime_ = boost::posix_time::second_clock::local_time();
      return true;
    }
  }
  return false;
}

}  // namespace bq
