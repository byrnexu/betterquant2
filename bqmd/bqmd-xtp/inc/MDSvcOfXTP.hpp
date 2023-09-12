/*!
 * \file MDSvcOfXTP.hpp
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

namespace XTP::API {
class QuoteApi;
}

namespace bq::md::svc::xtp {

class MDSvcOfXTP : public MDSvcOfCN {
 public:
  MDSvcOfXTP(const MDSvcOfXTP&) = delete;
  MDSvcOfXTP& operator=(const MDSvcOfXTP&) = delete;
  MDSvcOfXTP(const MDSvcOfXTP&&) = delete;
  MDSvcOfXTP& operator=(const MDSvcOfXTP&&) = delete;

  using MDSvcOfCN ::MDSvcOfCN;
  ~MDSvcOfXTP();

 private:
  int beforeInit() final;

 private:
  int startGateway() final;

 private:
  int doRun() final;

 private:
  void stopGateway() final;

 private:
  void doSub(const MarketDataCondGroup& marketDataCond) final;
  void doUnSub(const MarketDataCondGroup& marketDataCond) final;

 private:
  XTP::API::QuoteApi* api_{nullptr};
};

}  // namespace bq::md::svc::xtp
