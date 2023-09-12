/*!
 * \file BQTraderSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */
#pragma once

#include "API.hpp"
#include "util/Pch.hpp"

using namespace XTP::API;

namespace bq {
enum class MarketCode : std::uint16_t;
}

namespace bq::td::svc {
class TDSvcOfCN;
}

namespace bq::td::svc::xtp {

class BQTraderSpi : public TraderSpi {
 public:
  BQTraderSpi(TDSvcOfCN *tdSvc, TraderApi *api);
  ~BQTraderSpi();

  //! 交易服务器断线通知
  void OnDisconnected(uint64_t sessionId, int reason) final;

  //! 报单响应通知
  void OnOrderEvent(XTPOrderInfo *orderInfo, XTPRI *errorInfo,
                    uint64_t sessionId) final;

  //! 成交回报通知
  void OnTradeEvent(XTPTradeReport *tradeInfo, uint64_t sessionId) final;

  //! 撤单失败通知
  void OnCancelOrderError(XTPOrderCancelInfo *cancelInfo, XTPRI *errorInfo,
                          uint64_t sessionId) final;

  //! 查询订单回调响应
  void OnQueryOrderEx(XTPOrderInfoEx *orderInfo, XTPRI *errorInfo,
                      int requestId, bool isLast, uint64_t sessionId) final;

  //! 查询持仓回调响应
  void OnQueryPosition(XTPQueryStkPositionRsp *pos, XTPRI *errorInfo,
                       int requestId, bool isLast, uint64_t sessionId) final;

  //! 查询资金回调响应
  void OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *errorInfo, int requestId,
                    bool isLast, uint64_t sessionId) final;

 private:
  TDSvcOfCN *tdSvc_{nullptr};
  TraderApi *api_{nullptr};

  std::vector<MarketCode> marketCodeGroup_;
};

}  // namespace bq::td::svc::xtp
