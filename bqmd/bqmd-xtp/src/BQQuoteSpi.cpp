/*!
 * \file BQQuoteSpi.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "BQQuoteSpi.hpp"

#include "Config.hpp"
#include "MDSvcOfXTP.hpp"
#include "MDSvcOfXTPUtil.hpp"
#include "RawMDHandler.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/RawMD.hpp"
#include "util/Logger.hpp"

namespace bq::md::svc::xtp {

BQQuoteSpi::BQQuoteSpi(MDSvcOfCN *mdSvc, QuoteApi *api)
    : mdSvc_(mdSvc), api_(api) {}

BQQuoteSpi::~BQQuoteSpi() {}

void BQQuoteSpi::OnDisconnected(int reason) {
  //! 行情服务器断线后，此函数会被调用
  LOG_W("Disconnect from quote server. [reason: {}]", reason);

  //! 重新登陆行情服务器
  int ret = -1;
  while (0 != ret) {
    const auto [statusCode, protocolType] =
        GetXTPProtocolType(CONFIG["api"]["protocolType"].as<std::string>());

    ret = api_->Login(CONFIG["api"]["quoteServerIP"].as<std::string>().c_str(),
                      CONFIG["api"]["quoteServerPort"].as<std::uint32_t>(),
                      CONFIG["api"]["quoteUsername"].as<std::string>().c_str(),
                      CONFIG["api"]["quotePassword"].as<std::string>().c_str(),
                      protocolType,
                      CONFIG["api"]["localIP"].as<std::string>().empty()
                          ? nullptr
                          : CONFIG["api"]["localIP"].as<std::string>().c_str());
    if (0 != ret) {
      //! 登录失败，获取错误信息
      XTPRI *error_info = api_->GetApiLastError();
      LOG_W("Login to server error. [{} - {}]", error_info->error_id,
            error_info->error_msg);

      //! 等待10s以后再次连接，可修改此等待时间，建议不要小于3s
      std::this_thread::sleep_for(std::chrono::seconds(std::chrono::seconds(
          CONFIG["api"]["secIntervalOfReconnect"].as<std::uint32_t>())));
    }
  }

  //! 重连成功
  mdSvc_->setTradingDay(api_->GetTradingDay());
  LOG_I("Login to server {}:{} success. [tradingDay: {}]",
        CONFIG["api"]["quoteServerIP"].as<std::string>(),
        CONFIG["api"]["quoteServerPort"].as<std::uint32_t>(),
        mdSvc_->getTradingDay());

  //! 再次订阅行情快照
  subscribeAllMarketData();
}

void BQQuoteSpi::OnQueryAllTickersFullInfo(XTPQFI *ticker_info,
                                           XTPRI *error_info, bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Query all tickers full info failed. [{} - {}]", error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_D("Query tickers full info success. [{} - {}]", ticker_info->ticker,
        ticker_info->ticker_name);

  if (CONFIG["enableSymbolTableMaint"].as<bool>(false)) {
    auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
        MsgType::NewSymbol, ticker_info, sizeof(XTPQFI), is_last);
    mdSvc_->getRawMDHandler()->dispatch(rawMD);
  }

  //! 如果是最后一个响应，通知后续逻辑
  if (is_last == true) {
    if (ticker_info->exchange_id == XTP_EXCHANGE_SH) {
      //! 上海市场代码表处理完毕
      api_->QueryAllTickersFullInfo(XTP_EXCHANGE_SZ);
    } else {
      //! 深圳市场代码表处理完毕，开始订阅行情
      subscribeAllMarketData();
    }
  }
}

void BQQuoteSpi::subscribeAllMarketData() {
  if (CONFIG["subAllMarketData"].as<bool>(false) == true) {
    LOG_I("Begin to subscribe all marketdata.");
    api_->SubscribeAllOrderBook(XTP_EXCHANGE_SH);
    api_->SubscribeAllOrderBook(XTP_EXCHANGE_SZ);
    api_->SubscribeAllTickByTick(XTP_EXCHANGE_SH);
    api_->SubscribeAllTickByTick(XTP_EXCHANGE_SZ);
    api_->SubscribeAllMarketData(XTP_EXCHANGE_SH);
    api_->SubscribeAllMarketData(XTP_EXCHANGE_SZ);
  }
  return;
}

//! 订阅特定品种tickers应答
void BQQuoteSpi::OnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                 bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub ticker of {} failed. [{} - {}]", ticker->ticker,
          error_info->error_id, error_info->error_msg);
    return;
  }
  LOG_I("Sub ticker of {} success.", ticker->ticker);
}

//! 订阅所有品种tickers应答
void BQQuoteSpi::OnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                          XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub all tickers of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Sub all tickers of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

//! 取消订阅特定品种tickers应答
void BQQuoteSpi::OnUnSubMarketData(XTPST *ticker, XTPRI *error_info,
                                   bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub ticker failed. [{} - {}]", error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub ticker of {} success. ", ticker->ticker);
}

//! 取消订阅所有品种tickers应答
void BQQuoteSpi::OnUnSubscribeAllMarketData(XTP_EXCHANGE_TYPE exchange_id,
                                            XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub all tickers of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub all tickers of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

//! 订阅特定品种逐笔成交或者委托的应答
void BQQuoteSpi::OnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                 bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub tbt of {} failed. [{} - {}]", ticker->ticker,
          error_info->error_id, error_info->error_msg);
    return;
  }
  LOG_I("Sub tbt of {} success.", ticker->ticker);
}

//! 订阅所有品种逐笔成交或者委托的应答
void BQQuoteSpi::OnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                          XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub all tbts of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Sub all tbts of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

//! 取消订阅特定品种逐笔成交或者委托的应答
void BQQuoteSpi::OnUnSubTickByTick(XTPST *ticker, XTPRI *error_info,
                                   bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub tbt failed. [{} - {}]", error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub tbt of {} success. ", ticker->ticker);
}

//! 取消订阅所有品种逐笔成交或者委托的应答
void BQQuoteSpi::OnUnSubscribeAllTickByTick(XTP_EXCHANGE_TYPE exchange_id,
                                            XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub all tbts of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub all tbts of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

//! 订阅特定品种OrderBook应答
void BQQuoteSpi::OnSubOrderBook(XTPST *ticker, XTPRI *error_info,
                                bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub order book of {} failed. [{} - {}]", ticker->ticker,
          error_info->error_id, error_info->error_msg);
    return;
  }
  LOG_I("Sub order book of {} success.", ticker->ticker);
}

//! 订阅所有OrderBook应答
void BQQuoteSpi::OnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                         XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Sub all order books of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Sub all order books of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

//! 取消订阅特定品种OrderBook应答
void BQQuoteSpi::OnUnSubOrderBook(XTPST *ticker, XTPRI *error_info,
                                  bool is_last) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub order book failed. [{} - {}]", error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub order book of {} success. ", ticker->ticker);
};

//! 取消订阅所有OrderBook应答
void BQQuoteSpi::OnUnSubscribeAllOrderBook(XTP_EXCHANGE_TYPE exchange_id,
                                           XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_W("Unsub all order books of exchange {} failed. [{} - {}]",
          GetMarketName(GetMarketCode(exchange_id)), error_info->error_id,
          error_info->error_msg);
    return;
  }
  LOG_I("Unsub all order books of exchange {} success.",
        GetMarketName(GetMarketCode(exchange_id)));
}

void BQQuoteSpi::OnDepthMarketData(XTPMD *market_data, int64_t bid1_qty[],
                                   int32_t bid1_count, int32_t max_bid1_count,
                                   int64_t ask1_qty[], int32_t ask1_count,
                                   int32_t max_ask1_count) {
  //! ticker行情
  auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
      MsgType::Tickers, market_data, sizeof(XTPMD), true);
  mdSvc_->getRawMDHandler()->dispatch(rawMD);
}

void BQQuoteSpi::OnTickByTick(XTPTBT *tbt_data) {
  switch (tbt_data->type) {
    //! 逐笔成交
    case XTP_TBT_TRADE: {
      auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
          MsgType::Trades, tbt_data, sizeof(XTPTBT), true);
      mdSvc_->getRawMDHandler()->dispatch(rawMD);
    } break;

    //! 逐笔委托
    case XTP_TBT_ENTRUST: {
      auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
          MsgType::Orders, tbt_data, sizeof(XTPTBT), true);
      mdSvc_->getRawMDHandler()->dispatch(rawMD);
    } break;

    default:
      break;
  }
}

void BQQuoteSpi::OnOrderBook(XTPOB *order_book) {
  LOG_I("Recv order book of {}.", order_book->ticker);
  //! 深度行情
  auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(  //
      MsgType::Books, order_book, sizeof(XTPOB), true);
  mdSvc_->getRawMDHandler()->dispatch(rawMD);
}

}  // namespace bq::md::svc::xtp
