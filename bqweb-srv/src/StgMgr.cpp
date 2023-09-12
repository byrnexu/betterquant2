/*!
 * \file StgMgr.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/01
 *
 * \brief
 */

#include "StgMgr.hpp"

#include "WebSrv.hpp"
#include "def/StatusCode.hpp"
#include "def/StgInstInfo.hpp"
#include "util/Literal.hpp"
#include "util/Logger.hpp"
#include "util/LoggerUtil.hpp"
#include "util/StgLoggerInfo.hpp"
#include "util/String.hpp"

namespace bq {

StgMgr::StgMgr(WebSrv* webSrv) : webSrv_(webSrv) {}

void StgMgr::start() {
  threadIO_ = std::make_shared<std::thread>([this]() {
    LOG_I("Start IO.");
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        workGuard(io_.get_executor());
    const auto num = io_.run();
    LOG_I("Start IO finished. [task num = {}]", num);
  });
}

void StgMgr::stop() {
  io_.stop();
  threadIO_->join();
  LOG_I("Stop IO.");
}

std::tuple<int, std::string> StgMgr::startStg(UserId userId, StgId stgId,
                                              const std::string& startCmd) {
  //! 先检查该策略是否已经启动
  const auto [stgAlreadyStarted, statusCode, statusMsg] =
      checkIfStgAlreadyStarted(stgId, startCmd);
  if (stgAlreadyStarted) {
    return {statusCode, statusMsg};
  }

  try {
    //! 检查启动参数
    std::vector<std::string> fieldGroup;
    boost::split(fieldGroup, startCmd, boost::is_any_of(" "),
                 boost::token_compress_on);
    if (fieldGroup.size() < 2) {
      LOG_W(
          "Process of stg {} started failed "
          "because of invalid start command. [{}]",
          stgId, startCmd);
      return {SCODE_START_STG_FAILED, GetStatusMsg(SCODE_START_STG_FAILED)};
    }

    //！启动进程
    const auto cmd = fieldGroup[0];
    std::vector<std::string> args(  //
        std::begin(fieldGroup) + 1, std::end(fieldGroup));

    auto processInfo = std::make_shared<ProcessInfo>();
    processInfo->userId_ = userId;
    processInfo->stgId_ = stgId;
    processInfo->pipe_ = std::make_shared<boost::process::async_pipe>(io_);
    processInfo->process_ = std::make_shared<boost::process::child>(
        boost::process::search_path(cmd), boost::process::args(args),
        boost::process::std_err > *processInfo->pipe_, io_);

    //! 捕获启动过程中的输出
    char buff[BUF_LEN] = {0};
    captureOutputOfStg(processInfo, boost::process::buffer(buff));

    LOG_I("Process of stg {} started with PID {}. [{}]", stgId,
          processInfo->process_->id(), startCmd);

    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2ProcGroup_);
      stgId2ProcGroup_.emplace(stgId, processInfo);
    }

    return {0, ""};

  } catch (const std::exception& e) {
    LOG_W("Process of stg {} started failed. [{}] [{}]", stgId, startCmd,
          e.what());
    return {SCODE_START_STG_FAILED, e.what()};
  }
}

std::tuple<bool, int, std::string> StgMgr::checkIfStgAlreadyStarted(
    StgId stgId, const std::string& startCmd) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2ProcGroup_);

    //! 先检查stgId2ProcGroup_是否存在该策略信息
    const auto iter = stgId2ProcGroup_.find(stgId);

    if (iter != std::end(stgId2ProcGroup_)) {
      //! 如果存在那么获取该策略的进程信息
      const auto& processInfo = iter->second;

      if (!processExists(processInfo->process_->id())) {
        //! 如果进程信息对应的pid不能存在(可能是在系统外部直接kill掉了)
        stgId2ProcGroup_.erase(stgId);
        LOG_I("Process pid {} of stg {} not running, clean data in stgmgr. ",
              processInfo->process_->id(), stgId);
        return {false, 0, ""};

      } else {
        LOG_W("Process of stg {} already started. [{}]", stgId, startCmd);
        return {true, SCODE_STG_ALREADY_STARTED,
                GetStatusMsg(SCODE_STG_ALREADY_STARTED)};
      }
    }
  }

  return {false, 0, ""};
}

std::tuple<int, std::string> StgMgr::stopStg(StgId stgId) {
  const auto [stgNotStarted, processInfo, statusCode, statusMsg] =
      checkIfStgNotStarted(stgId);
  if (stgNotStarted) {
    return {statusCode, statusMsg};
  }

  try {
    kill(processInfo->process_->id(), SIGINT);
    LOG_I("Process pid {} of stg {} terminate. ", processInfo->process_->id(),
          stgId);

    std::error_code ec;
    const auto ret =
        processInfo->process_->wait_for(std::chrono::seconds(10), ec);
    if (!ret) {
      LOG_W("Process pid {} of stg {} finished. [{} - {}]",
            processInfo->process_->id(), stgId, ec.value(), ec.message());
    } else {
      LOG_I("Process pid {} of stg {} finished. [{} - {}]",
            processInfo->process_->id(), stgId, ec.value(), ec.message());
    }

    {
      std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2ProcGroup_);
      stgId2ProcGroup_.erase(stgId);
    }

    return {0, ""};

  } catch (const std::exception& e) {
    LOG_W("Process pid {} of stg {} stop failed. [{}]",
          processInfo->process_->id(), stgId, e.what());
    return {SCODE_STOP_STG_FAILED, e.what()};
  }
}

std::tuple<bool, ProcessInfoSPtr, int, std::string>
StgMgr::checkIfStgNotStarted(StgId stgId) {
  ProcessInfoSPtr processInfo{nullptr};
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2ProcGroup_);
    const auto iter = stgId2ProcGroup_.find(stgId);
    if (iter == std::end(stgId2ProcGroup_)) {
      LOG_W("Process pid of stg {} not exists. [{}]", stgId);
      return {true, nullptr, SCODE_STG_NOT_START,
              GetStatusMsg(SCODE_STG_NOT_START)};
    } else {
      processInfo = iter->second;
      if (!processExists(processInfo->process_->id())) {
        // stgId2ProcGroup_有记录但是进程不存在，清理数据
        stgId2ProcGroup_.erase(stgId);
        LOG_I(
            "Process pid {} of stg {} not running, clean data in stgmgr. [{}]",
            processInfo->process_->id(), stgId);
        return {true, processInfo, 0, ""};
      } else {
        return {false, processInfo, 0, ""};
      }
    }
  }
}

bool StgMgr::processExists(int pid) const { return (kill(pid, 0) == 0); }

void StgMgr::captureOutputOfStg(const ProcessInfoSPtr& processInfo,
                                boost::asio::mutable_buffer buff) {
  processInfo->pipe_->async_read_some(
      buff,  //
      [this, processInfo, buff](std::error_code ec, size_t n) {
        if (n >= BUF_LEN) {
          LOG_W("Size of output too big when capture output of stg.");
          return;
        }

        //! 将输出追加到processInfo->message_
        std::string str(boost::asio::buffer_cast<const char*>(buff), n);
        processInfo->message_.append(str);

        if (!ec) {
          //! 如果还有输出，那么继续capture
          captureOutputOfStg(processInfo, boost::process::buffer(buff));

        } else {
          //! 如果不再有输出，那么将所有的输出记录
          auto stgInstInfo = std::make_shared<StgInstInfo>();
          stgInstInfo->userId_ = processInfo->userId_;
          stgInstInfo->stgId_ = processInfo->stgId_;
          stgInstInfo->stgInstId_ = 1;

          //! 将输出写入数据库，如果包含多行的话，每行一条记录写入数据库
          StgLoggerInfoInsert(webSrv_->getDBEng(), LogLevel::WARN, stgInstInfo,
                              processInfo->message_);

          LOG_I("Capture output of stg {} of user {}. {}{}",
                processInfo->stgId_, processInfo->userId_, "\n",
                processInfo->message_);

          processInfo->message_.clear();
        }
      });
}

std::string StgMgr::toJson() const {
  rapidjson::StringBuffer strBuf;
  rapidjson::Writer<rapidjson::StringBuffer> writer(strBuf);
  writer.StartObject();
  writer.Key("stgInfoGroup");
  writer.StartArray();
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxStgId2ProcGroup_);
    for (const auto& rec : stgId2ProcGroup_) {
      const auto& processInfo = rec.second;
      if (!processExists(processInfo->process_->id())) {
        continue;
      }
      writer.StartObject();
      writer.Key("userId");
      writer.Uint(processInfo->userId_);
      writer.Key("stgId");
      writer.Uint(processInfo->stgId_);
      writer.Key("pid");
      writer.Uint(processInfo->process_->id());
      writer.EndObject();
    }
  }
  writer.EndArray();
  writer.EndObject();
  return strBuf.GetString();
}

}  // namespace bq
