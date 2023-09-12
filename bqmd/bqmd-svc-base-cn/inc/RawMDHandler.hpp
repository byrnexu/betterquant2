/*!
 * \file RawMDHandler.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#pragma once

#include "MDSvcOfCNDef.hpp"
#include "def/BQMDDef.hpp"
#include "def/ConstIF.hpp"
#include "util/Pch.hpp"

namespace bq {

template <typename Task, BlockType blockType>
class TaskDispatcher;
template <typename Task, BlockType blockType>
using TaskDispatcherSPtr = std::shared_ptr<TaskDispatcher<Task, blockType>>;

template <typename Task>
struct AsyncTask;
template <typename Task>
using AsyncTaskSPtr = std::shared_ptr<AsyncTask<Task>>;

struct RawMD;
using RawMDSPtr = std::shared_ptr<RawMD>;
using RawMDAsyncTask = AsyncTask<RawMDSPtr>;
using RawMDAsyncTaskSPtr = std::shared_ptr<RawMDAsyncTask>;

enum class MsgType : std::uint8_t;
}  // namespace bq

namespace bq::md {
struct RawMDAsyncTaskArg;
using RawMDAsyncTaskArgSPtr = std::shared_ptr<RawMDAsyncTaskArg>;
}  // namespace bq::md

namespace bq::md::svc {

class MDSvcOfCN;

class RawMDHandler {
 public:
  RawMDHandler(const RawMDHandler&) = delete;
  RawMDHandler& operator=(const RawMDHandler&) = delete;
  RawMDHandler(const RawMDHandler&&) = delete;
  RawMDHandler& operator=(const RawMDHandler&&) = delete;

  explicit RawMDHandler(MDSvcOfCN* mdSvc);
  ~RawMDHandler() {}

 public:
  int init();

 public:
  RawMDSPtr makeRawMD(MsgType msgType, const void* data, std::size_t dataLen,
                      bool isLast);

 private:
  virtual std::tuple<int, RawMDAsyncTaskSPtr> makeAsyncTask(
      const RawMDSPtr& task) = 0;

 public:
  void start();
  void stop();

 public:
  void dispatch(RawMDSPtr& task);

 private:
  void handle(RawMDAsyncTaskSPtr& asyncTask);

  virtual void handleNewSymbol(RawMDAsyncTaskSPtr& asyncTask) {}
  virtual bool handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) { return true; }
  virtual bool handleMDTrades(RawMDAsyncTaskSPtr& asyncTask) { return true; }
  virtual bool handleMDOrders(RawMDAsyncTaskSPtr& asyncTask) { return true; }
  virtual bool handleMDBooks(RawMDAsyncTaskSPtr& asyncTask) { return true; }

  void handleMDBid1Ask1ByTickers(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDLastPriceByTickers(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDBid1Ask1ByBooks(RawMDAsyncTaskSPtr& asyncTask);
  void handleMDLastPriceByTrades(RawMDAsyncTaskSPtr& asyncTask);

 protected:
  bool checkIfMDIsLegal(RawMDAsyncTaskSPtr& asyncTask, std::uint64_t exchTs);

 protected:
  MDSvcOfCN* mdSvc_{nullptr};

  Topic2LastTsGroupSPtr topic2LastExchTsGroup_{nullptr};
  mutable std::mutex mtxTopic2LastExchTsGroup_;

  TaskDispatcherSPtr<RawMDSPtr, BlockType::Block> taskDispatcher_{nullptr};

 private:
  bool saveBooks_{false};
  bool saveTrades_{false};
  bool saveOrders_{false};
  bool saveTickers_{false};

  bool createBid1Ask1ByTickers_{true};
  bool createLastPriceByTickers_{true};

  bool checkIFExchTsOfMDIsInc_{true};

  std::size_t loggerThresholdOfTopicRecvTimes_{100};
  std::map<std::string, std::uint64_t> topic2CntGroup_;
};

}  // namespace bq::md::svc
