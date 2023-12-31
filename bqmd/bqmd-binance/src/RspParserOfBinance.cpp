/*!
 * \file RspParserOfBinance.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "RspParserOfBinance.hpp"

#include "ConnMetadata.hpp"
#include "MDSvc.hpp"
#include "MDSvcDef.hpp"
#include "MDSvcOfBinanceUtil.hpp"
#include "WSTask.hpp"
#include "def/MDWSCliAsyncTaskArg.hpp"
#include "util/Json.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::md::svc::binance {

/*
 * {
 *   "result": null,
 *   "id": 1
 * }
 *
 * {"code": 1, "msg": "Invalid value type: expected Boolean", "id": '%s'}
 *
 */
//!
//! 此函数仅仅打印订阅应答，默认订阅都是成功的，如果失败的话长时间收不到行情其他
//! 模块会自动重新发起订阅。
//!
TopicGroupNeedMaintSPtr RspParserOfBinance::doGetTopicGroupForSubOrUnSubAgain(
    WSCliAsyncTaskSPtr& asyncTask) {
  auto topicGroupNeedMaint = std::make_shared<TopicGroupNeedMaint>();

  const auto& rsp = asyncTask->task_->msg_->get_payload();
  const auto& connMetadata = asyncTask->task_->connMetadata_;

  const auto arg = std::any_cast<WSCliAsyncTaskArgSPtr>(asyncTask->arg_);

  const auto result = yyjson_obj_get(arg->root_, "result");
  const auto id = yyjson_obj_get(arg->root_, "id");
  if (result != nullptr && id != nullptr) {
    LOG_I("{} {}", rsp, connMetadata->toStr());
  }

  const auto code = yyjson_obj_get(arg->root_, "code");
  const auto msg = yyjson_obj_get(arg->root_, "msg");
  if (code != nullptr && msg != nullptr) {
    LOG_W("{} {}", rsp, connMetadata->toStr());
  }

  return topicGroupNeedMaint;
}

}  // namespace bq::md::svc::binance
