/*!
 * \file TDGateway.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "util/Pch.hpp"

namespace bq {
struct OrderInfo;
using OrderInfoSPtr = std::shared_ptr<OrderInfo>;
struct SHMIPCTask;
using SHMIPCTaskSPtr = std::shared_ptr<SHMIPCTask>;
template <typename Task>
struct AsyncTask;
using SHMIPCAsyncTask = AsyncTask<SHMIPCTaskSPtr>;
using SHMIPCAsyncTaskSPtr = std::shared_ptr<SHMIPCAsyncTask>;
}  // namespace bq

namespace bq::td::svc {

class TDSvcOfCN;

class TDGateway {
 public:
  TDGateway(const TDGateway&) = delete;
  TDGateway& operator=(const TDGateway&) = delete;
  TDGateway(const TDGateway&&) = delete;
  TDGateway& operator=(const TDGateway&&) = delete;

  explicit TDGateway(TDSvcOfCN* tdSvc);
  virtual ~TDGateway() {}

 public:
  int init() { return doInit(); }
  int start() { return doStart(); }
  void stop() { doStop(); }

 private:
  virtual int doInit() { return 0; }
  virtual int doStart() { return 0; }
  virtual void doStop() {}

 public:
  std::tuple<int, std::string> order(const OrderInfoSPtr& orderInfo) {
    return doOrder(orderInfo);
  }
  int cancelOrder(const OrderInfoSPtr& orderInfo) {
    return doCancelOrder(orderInfo);
  }
  void syncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) {
    doSyncUnclosedOrderInfo(asyncTask);
  }
  void syncAssetsSnapshot() { doSyncAssetsSnapshot(); }
  void testOrder() { doTestOrder(); }
  void testCancelOrder() { doTestCancelOrder(); }

 private:
  virtual std::tuple<int, std::string> doOrder(const OrderInfoSPtr& orderInfo) {
    return {0, ""};
  }
  virtual int doCancelOrder(const OrderInfoSPtr& orderInfo) { return 0; }
  virtual void doSyncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) {}
  virtual void doSyncAssetsSnapshot() {}
  virtual void doTestOrder() {}
  virtual void doTestCancelOrder() {}

 protected:
  TDSvcOfCN* tdSvc_{nullptr};
};

}  // namespace bq::td::svc
