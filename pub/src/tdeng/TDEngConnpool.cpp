/*!
 * \file TDEngConnpool.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "tdeng/TDEngConnpool.hpp"

#include "taos.h"
#include "tdeng/TDEngParam.hpp"
#include "util/Logger.hpp"

namespace bq::tdeng {

TDEngConnpool::TDEngConnpool(const TDEngParamSPtr& tdEngParam)
    : tdEngParam_(tdEngParam) {}

int TDEngConnpool::init() {
  for (int no = 0; no < tdEngParam_->connPoolSize_; ++no) {
    auto conn = std::make_shared<Conn>();
    conn->no_ = no;
    conn->taos_ =
        taos_connect(tdEngParam_->host_.c_str(), tdEngParam_->username_.c_str(),
                     tdEngParam_->password_.c_str(), tdEngParam_->db_.c_str(),
                     tdEngParam_->port_);
    if (conn->taos_ != nullptr) {
      LOG_I("Connect to tdeng success. [no = {}]", no);
    } else {
      const auto err = taos_errno(nullptr);
      const auto errMsg = taos_errstr(nullptr);
      LOG_E("Connect to tdeng failed. [{} - ()]", err, errMsg);
      return -1;
    }

    {
      std::lock_guard<std::ext::spin_mutex> lock(mtxConnGroup_);
      connGroup_.emplace_back(conn);
    }
  }

  return 0;
}

void TDEngConnpool::uninit() {
  for (int no = 0; no < tdEngParam_->connPoolSize_; ++no) {
    {
      std::lock_guard<std::ext::spin_mutex> lock(mtxConnGroup_);
      taos_close(connGroup_[no]->taos_);
      taos_cleanup();
    }
  }
}

std::uint32_t TDEngConnpool::getSize() const {
  {
    std::lock_guard<std::ext::spin_mutex> lock(mtxConnGroup_);
    return connGroup_.size();
  }
}

ConnSPtr TDEngConnpool::getIdleConn() const {
  while (true) {
    {
      std::lock_guard<std::ext::spin_mutex> lock(mtxConnGroup_);
      for (const auto& conn : connGroup_) {
        if (conn->idle_) {
          conn->idle_ = false;
          return conn;
        }
      }
    }
    LOG_I("Wait for idle connection. ");
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void TDEngConnpool::giveBackConn(const ConnSPtr& conn) {
  std::lock_guard<std::ext::spin_mutex> lock(mtxConnGroup_);
  for (const auto& curConn : connGroup_) {
    if (curConn->no_ == conn->no_) {
      curConn->idle_ = true;
    }
  }
}

}  // namespace bq::tdeng
