/*!
 * \file MDSvcOfCTP.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#pragma once

#include "MDSvcOfCN.hpp"
#include "def/BQConst.hpp"
#include "util/Pch.hpp"
#include "util/SvcBase.hpp"

class CThostFtdcMdApi;
class CThostFtdcTraderApi;

namespace bq::md::svc::ctp {

class MDSvcOfCTP : public MDSvcOfCN {
 public:
  MDSvcOfCTP(const MDSvcOfCTP&) = delete;
  MDSvcOfCTP& operator=(const MDSvcOfCTP&) = delete;
  MDSvcOfCTP(const MDSvcOfCTP&&) = delete;
  MDSvcOfCTP& operator=(const MDSvcOfCTP&&) = delete;

  using MDSvcOfCN::MDSvcOfCN;
  ~MDSvcOfCTP();

 private:
  int beforeInit() final;

 private:
  int startGateway() final;
  int startGatewayOfTD();
  int startGatewayOfMD();

 private:
  int doRun() final;

 private:
  void stopGateway() final;
  void stopGatewayOfTD();
  void stopGatewayOfMD();

 private:
  void doSub(const MarketDataCondGroup& marketDataCond) final;
  void doUnSub(const MarketDataCondGroup& marketDataCond) final;

 private:
  void resetBarrierOfGetSymbolTable() {
    barrierOfGetSymbolTable_ = std::make_shared<std::promise<void>>();
  }

 public:
  std::shared_ptr<std::promise<void>>& getBarrierOfGetSymbolTable() {
    return barrierOfGetSymbolTable_;
  }

 private:
  CThostFtdcMdApi* api_{nullptr};
  CThostFtdcTraderApi* apiOfTD_{nullptr};

  std::shared_ptr<std::promise<void>> barrierOfGetSymbolTable_{nullptr};
};

}  // namespace bq::md::svc::ctp
