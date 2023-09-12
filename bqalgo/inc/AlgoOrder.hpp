/*!
 * \file AlgoOrder.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/05/16
 *
 * \brief
 */
#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "util/Pch.hpp"

namespace bq {
template <typename Task>
struct AsyncTask;
template <typename Task>
using AsyncTaskSPtr = std::shared_ptr<AsyncTask<Task>>;

struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;

using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;

struct StgInstInfo;
using StgInstInfoSPtr = std::shared_ptr<StgInstInfo>;

struct OrderInfo;
struct Trades;
struct Orders;
struct Tickers;
struct Candle;
struct Books;
struct Bid1Ask1;
struct LastPrice;
}  // namespace bq

namespace bq::stg {
class StgEngImpl;
}  // namespace bq::stg

namespace bq::algo {

using StatusCodeGroupOfRetryOrder = ankerl::unordered_dense::set<int>;

class AlgoMgr;
enum class AlgoStatus;

class AlgoOrder {
 public:
  AlgoOrder(const AlgoOrder&) = delete;
  AlgoOrder& operator=(const AlgoOrder&) = delete;
  AlgoOrder(const AlgoOrder&&) = delete;
  AlgoOrder& operator=(const AlgoOrder&&) = delete;

  explicit AlgoOrder(AlgoMgr* algoMgr, const StgInstInfoSPtr& stgInstInfo,
                     const std::string& algoType, const std::string& algoName,
                     std::uint32_t lifetime = UINT32_MAX);

 public:
  int init(const std::string& algoParamsInJsonFmt) {
    algoParamsInJsonFmt_ = algoParamsInJsonFmt;

    if (const auto statusCode = beforeInit(algoParamsInJsonFmt);
        statusCode != 0) {
      return statusCode;
    }

    if (const auto statusCode = doInit(algoParamsInJsonFmt);  //
        statusCode != 0) {
      return statusCode;
    }

    if (const auto statusCode = afterInit(algoParamsInJsonFmt);
        statusCode != 0) {
      return statusCode;
    }

    return 0;
  }

 private:
  virtual int beforeInit(const std::string& algoParamsInJsonFmt) { return 0; }
  int doInit(const std::string& algoParamsInJsonFmt);
  virtual int afterInit(const std::string& algoParamsInJsonFmt) { return 0; }

  int initStatusCodeGroupOfRetryOrder(const std::string& algoParamsInJsonFmt);

 public:
  std::tuple<int, TopicGroup> sub(const std::string& algoParamsInJsonFmt);
  std::tuple<int, TopicGroup> unSub(const std::string& algoParamsInJsonFmt);

 private:
  std::tuple<int, TopicGroup> getPreSubTopicGroupFromParams(
      const std::string& algoParamsInJsonFmt);

 public:
  std::tuple<int, TopicGroup> getTopicGroupNeedSub() {
    return doGetTopicGroupNeedSub();
  }

 private:
  virtual std::tuple<int, TopicGroup> doGetTopicGroupNeedSub() {
    return {0, TopicGroup()};
  }

 public:
  void onTimer();

 private:
  virtual void doOnTimer() = 0;
  bool algoOrderIsOutOfTime();
  virtual bool algoOrderIsFinished() = 0;

 public:
  void onOrderRet(const OrderInfo* orderInfo) { doOnOrderRet(orderInfo); }

 private:
  virtual void doOnOrderRet(const OrderInfo* orderInfo) {}

 public:
  void onCancelOrderRet(const OrderInfo* orderInfo) {
    doOnCancelOrderRet(orderInfo);
  }

 private:
  virtual void doOnCancelOrderRet(const OrderInfo* orderInfo) {}

 public:
  void onTrades(const Trades* trades) { doOnTrades(trades); }

 private:
  virtual void doOnTrades(const Trades* trades) {}

 public:
  void onOrders(const Orders* orders) { doOnOrders(orders); }

 private:
  virtual void doOnOrders(const Orders* orders) {}

 public:
  void onTickers(const Tickers* tickers) { doOnTickers(tickers); }

 private:
  virtual void doOnTickers(const Tickers* tickers) {}

 public:
  void onCandle(const Candle* candle) { doOnCandle(candle); }

 private:
  virtual void doOnCandle(const Candle* candle) {}

 public:
  void onBooks(const Books* books) { doOnBooks(books); }

 private:
  virtual void doOnBooks(const Books* books) {}

 public:
  void onBid1Ask1(const Bid1Ask1* bid1ask1) { doOnBid1Ask1(bid1ask1); }

 private:
  virtual void doOnBid1Ask1(const Bid1Ask1* bid1ask1) {}

 public:
  void onLastPrice(const LastPrice* lastPrice) { doOnLastPrice(lastPrice); }

 private:
  virtual void doOnLastPrice(const LastPrice* lastPrice) {}

 public:
  void onDynCandle(const Candle* candle) { doOnDynCandle(candle); }

 private:
  virtual void doOnDynCandle(const Candle* candle) {}

 public:
  void cancelAllOrders();

 public:
  AlgoMgr* getAlgoMgr() { return algoMgr_; }

  AlgoStatus getAlgoStatus() const {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoStatus_);
    return algoStatus_;
  }
  void setAlgoStatus(AlgoStatus value) {
    std::lock_guard<std::ext::spin_mutex> guard(mtxAlgoStatus_);
    algoStatus_ = value;
  }

  const StgInstInfoSPtr& getStgInstInfo() { return stgInstInfo_; }
  AlgoId getAlgoId() const { return algoId_; }
  const std::string& getAlgoType() const { return algoType_; }
  const std::string& getAlgoName() const { return algoName_; }
  const std::string& getAlgoParamsInJsonFmt() const {
    return algoParamsInJsonFmt_;
  }

  std::string toStr() const {
    return fmt::format("algoId: {}; algoType: {}; algoName: {}", algoId_,
                       algoType_, algoName_);
  }

  bool needRetoreOrder(int statusCode) {
    const auto iter = statusCodeGroupOfRetryOrder_.find(statusCode);
    if (iter != std::end(statusCodeGroupOfRetryOrder_)) {
      return true;
    }
    return false;
  }

  std::uint32_t getMSIntervalOfRetryOrder() const {
    return msIntervalOfRetryOrder_;
  }

  void setStatusCode(int value) { statusCode_ = value; }
  int getStatusCode() const { return statusCode_; }

 protected:
  virtual std::string notifyProgressOfAlgoOrder() { return ""; }
  void syncToDB(const std::string& data);

 public:
  stg::StgEngImpl* getStgEng();

 private:
  AlgoMgr* algoMgr_{nullptr};

  AlgoStatus algoStatus_;
  mutable std::ext::spin_mutex mtxAlgoStatus_;

  const StgInstInfoSPtr stgInstInfo_{nullptr};
  const AlgoId algoId_{0};
  const std::string algoType_;
  const std::string algoName_;
  const std::uint32_t startTime_;
  const std::uint32_t lifetime_;
  std::string algoParamsInJsonFmt_;

  StatusCodeGroupOfRetryOrder statusCodeGroupOfRetryOrder_;
  std::uint32_t msIntervalOfRetryOrder_;

  int statusCode_{0};
};

}  // namespace bq::algo
