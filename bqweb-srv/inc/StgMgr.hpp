/*!
 * \file StgMgr.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/06/01
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq {

class WebSrv;

struct ProcessInfo {
  UserId userId_;
  StgId stgId_;
  std::shared_ptr<boost::process::child> process_{nullptr};
  std::shared_ptr<boost::process::async_pipe> pipe_{nullptr};
  std::string message_;
};
using ProcessInfoSPtr = std::shared_ptr<ProcessInfo>;
using StgId2ProcGroup = std::map<StgId, ProcessInfoSPtr>;

class StgMgr {
 public:
  StgMgr(const StgMgr&) = delete;
  StgMgr& operator=(const StgMgr&) = delete;
  StgMgr(const StgMgr&&) = delete;
  StgMgr& operator=(const StgMgr&&) = delete;

  explicit StgMgr(WebSrv* webSrv);

 public:
  void start();
  void stop();

 public:
  std::tuple<int, std::string> startStg(UserId userId, StgId stgId,
                                        const std::string& startCmd);

 private:
  std::tuple<bool, int, std::string> checkIfStgAlreadyStarted(
      StgId stgId, const std::string& startCmd);

  std::tuple<bool, ProcessInfoSPtr, int, std::string> checkIfStgNotStarted(
      StgId stgId);

 public:
  std::tuple<int, std::string> stopStg(StgId stgId);

 private:
  bool processExists(int pid) const;

  void captureOutputOfStg(const ProcessInfoSPtr& processInfo,
                          boost::asio::mutable_buffer buff);

 public:
  std::string toJson() const;

 private:
  WebSrv* webSrv_{nullptr};

  boost::asio::io_context io_;
  std::shared_ptr<std::thread> threadIO_{nullptr};

  StgId2ProcGroup stgId2ProcGroup_;
  mutable std::ext::spin_mutex mtxStgId2ProcGroup_;

  inline const static std::size_t BUF_LEN = 4096;
};

}  // namespace bq
