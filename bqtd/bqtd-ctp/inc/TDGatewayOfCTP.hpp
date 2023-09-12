/*!
 * \file TDSvcOfCTP.hpp
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

namespace bq::td::svc::ctp {

class TDGatewayOfCTP;
using TDGatewayOfCTPSPtr = std::shared_ptr<TDGatewayOfCTP>;

class TDGatewayOfCTP : public TDGateway {
 public:
  TDGatewayOfCTP(const TDGatewayOfCTP&) = delete;
  TDGatewayOfCTP& operator=(const TDGatewayOfCTP&) = delete;
  TDGatewayOfCTP(const TDGatewayOfCTP&&) = delete;
  TDGatewayOfCTP& operator=(const TDGatewayOfCTP&&) = delete;

  using TDGateway::TDGateway;

 public:
  void setFrontId(TThostFtdcFrontIDType value) { frontId_ = value; }
  void setSessionId(TThostFtdcSessionIDType sessionId) {
    sessionId_ = sessionId;
  }

 private:
  int doInit() final;

 private:
  std::tuple<int, std::string> doOrder(const OrderInfoSPtr& orderInfo) final;
  int doCancelOrder(const OrderInfoSPtr& orderInfo) final;
  void doSyncUnclosedOrderInfo(SHMIPCAsyncTaskSPtr& asyncTask) final;
  void doSyncAssetsSnapshot() final;
  void doTestOrder() final;
  void doTestCancelOrder() final;

 private:
  CThostFtdcTraderApi* api_{nullptr};

  std::atomic_int requestId_{0};

  std::string brokerId_;
  std::string userId_;

  TThostFtdcFrontIDType frontId_;
  TThostFtdcSessionIDType sessionId_;
};

}  // namespace bq::td::svc::ctp
