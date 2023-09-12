/*!
 * \file TDSvcOfXTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/01/01
 *
 * \brief
 */

#pragma once

#include "API.hpp"
#include "TDGateway.hpp"

using namespace XTP::API;

namespace bq::td::svc::xtp {

class TDGatewayOfXTP;
using TDGatewayOfXTPSPtr = std::shared_ptr<TDGatewayOfXTP>;

class TDGatewayOfXTP : public TDGateway {
 public:
  TDGatewayOfXTP(const TDGatewayOfXTP&) = delete;
  TDGatewayOfXTP& operator=(const TDGatewayOfXTP&) = delete;
  TDGatewayOfXTP(const TDGatewayOfXTP&&) = delete;
  TDGatewayOfXTP& operator=(const TDGatewayOfXTP&&) = delete;

  using TDGateway::TDGateway;

 private:
  int doInit() final;

 private:
  std::tuple<int, std::string> doOrder(const OrderInfoSPtr& orderInfo) final;
  int doCancelOrder(const OrderInfoSPtr& orderInfo) final;
  void doSyncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) final;
  void doSyncAssetsSnapshot() final;
  void doTestOrder() final;
  void doTestCancelOrder() final;

 public:
  void setSessionId(std::uint64_t value) { sessionId_ = value; }
  std::uint64_t getSessionId() const { return sessionId_; }

 private:
  TraderApi* api_{nullptr};
  std::uint64_t sessionId_{0};
  std::atomic_int requestId_{0};
  std::atomic_uint clientId_{0};
};

}  // namespace bq::td::svc::xtp
