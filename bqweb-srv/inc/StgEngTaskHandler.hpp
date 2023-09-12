/*!
 * \file StgEngTaskHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/04/14
 *
 * \brief
 */

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {

class WebSrv;

class StgEngTaskHandler;
using StgEngTaskHandlerSPtr = std::shared_ptr<StgEngTaskHandler>;

class StgEngTaskHandler {
 public:
  StgEngTaskHandler(const StgEngTaskHandler&) = delete;
  StgEngTaskHandler& operator=(const StgEngTaskHandler&) = delete;
  StgEngTaskHandler(const StgEngTaskHandler&&) = delete;
  StgEngTaskHandler& operator=(const StgEngTaskHandler&&) = delete;

  explicit StgEngTaskHandler(WebSrv* webSrv);

 public:
  void handleAsyncTask(const SHMIPCAsyncTaskSPtr& asyncTask);

 private:
  void handleMsgIdOnOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleMsgIdOnCancelOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);

  void handleMsgIdOnManualOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);
  void handleMsgIdOnManualCancelOrderRet(const SHMIPCAsyncTaskSPtr& asyncTask);

  void forwardRspToClient(const std::string& rspInJsonFmt);

 private:
  WebSrv* webSrv_{nullptr};
};

}  // namespace bq
