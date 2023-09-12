/*!
 * \file ReqBody2CallbackGroup.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/16 *
 * \brief
 */

#include "ReqBody2CallbackGroup.hpp"

#include "util/Logger.hpp"

namespace bq {

ReqBody2CallbackGroup::ReqBody2CallbackGroup(WebSrv* webSrv)
    : webSrv_(webSrv) {}

std::function<void(const HttpResponsePtr&)> ReqBody2CallbackGroup::getCallback(
    const std::string& reqBody) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxReqBody2CallbackGroup_);
    auto iter = reqBody2CallbackGroup_.find(reqBody);
    if (iter != std::end(reqBody2CallbackGroup_)) {
      const auto callback = iter->second;
      reqBody2CallbackGroup_.erase(iter);
      return callback;
    } else {
      LOG_W("Callback not exists when handle http response. {}", reqBody);
      return nullptr;
    }
  }
}

void ReqBody2CallbackGroup::cacheCallback(
    const std::string& reqBody,
    const std::function<void(const HttpResponsePtr&)>& callback) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxReqBody2CallbackGroup_);
    reqBody2CallbackGroup_.emplace(reqBody, callback);
    if (reqBody2CallbackGroup_.size() % 100 == 0) {
      LOG_W("Size of reqBody to callback group is {}",
            reqBody2CallbackGroup_.size());
    }
  }
}

}  // namespace bq
