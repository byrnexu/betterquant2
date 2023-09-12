/*!
 * \file BQQuoteSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */
#pragma once

#include "util/Pch.hpp"
#include "xtp_quote_api.h"

using namespace XTP::API;

namespace bq::md::svc {
class MDSvcOfCN;
}

namespace bq::md::svc::xtp {

class BQQuoteSpi : public QuoteSpi {
 public:
  explicit BQQuoteSpi(MDSvcOfCN *mdSvc, QuoteApi *api);
  virtual ~BQQuoteSpi();

  //! 行情服务器断线通知
  void OnDisconnected(int reason) final;

  //! 查询合约完整静态信息的应答
  void OnQueryAllTickersFullInfo(XTPQFI *ticker_info, XTPRI *error_info,
                                 bool is_last) final;

  //! 订阅快照行情应答
  void OnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last);
  void OnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                XTPRI *error_info) final;

  //! 退订快照行情应答
  void OnUnSubMarketData(XTPST *ticker, XTPRI *error_info, bool is_last) final;
  void OnUnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                  XTPRI *error_info) final;

  //! 订阅行情订单簿应答，包括股票、指数和期权
  void OnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last) final;
  void OnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                               XTPRI *error_info) final;

  //! 退订行情订单簿应答，包括股票、指数和期权
  void OnUnSubOrderBook(XTPST *ticker, XTPRI *error_info, bool is_last) final;
  void OnUnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                 XTPRI *error_info) final;

  //! 订阅逐笔行情应答，包括股票、指数和期权
  void OnSubTickByTick(XTPST *ticker, XTPRI *error_info, bool is_last) final;
  void OnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                XTPRI *error_info) final;

  //! 退订逐笔行情应答，包括股票、指数和期权
  void OnUnSubTickByTick(XTPST *ticker, XTPRI *error_info, bool is_last) final;
  void OnUnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                  XTPRI *error_info) final;

  //! 快照行情通知，包含买一卖一队列
  void OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[],
                         int32_t bid1_count, int32_t max_bid1_count,
                         int64_t ask1_qty[], int32_t ask1_count,
                         int32_t max_ask1_count);

  //! 行情订单簿通知，包括股票、指数和期权
  void OnOrderBook(XTPOB *order_book) final;

  //! 逐笔行情通知，包括股票、指数和期权
  void OnTickByTick(XTPTBT *tbt_data) final;

 private:
  void subscribeAllMarketData();

 private:
  MDSvcOfCN *mdSvc_{nullptr};
  QuoteApi *api_{nullptr};
};

}  // namespace bq::md::svc::xtp
