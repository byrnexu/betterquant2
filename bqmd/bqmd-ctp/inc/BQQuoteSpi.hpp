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

#include "API.hpp"
#include "util/Pch.hpp"

namespace bq::md::svc {
class MDSvcOfCN;
}

namespace bq::md::svc::ctp {

class BQQuoteSpi : public CThostFtdcMdSpi {
 public:
  explicit BQQuoteSpi(MDSvcOfCN *mdSvc, CThostFtdcMdApi *api);
  virtual ~BQQuoteSpi();

  //! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() final;

  //! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自
  //! 动重新连接，客户端可不做处理。
  void OnFrontDisconnected(int nReason) final;

  //! 心跳超时警告。当长时间未收到报文时，该方法被调用。
  void OnHeartBeatWarning(int nTimeLapse) final;

  //! 登录请求响应
  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                      bool bIsLast) final;

  //! 登出请求响应
  void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) final;

  //! 请求查询组播合约响应
  void OnRspQryMulticastInstrument(
      CThostFtdcMulticastInstrumentField *pMulticastInstrument,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 错误应答
  void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                  bool bIsLast) final;

  //! 订阅行情应答
  void OnRspSubMarketData(
      CThostFtdcSpecificInstrumentField *pSpecificInstrument,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 取消订阅行情应答
  void OnRspUnSubMarketData(
      CThostFtdcSpecificInstrumentField *pSpecificInstrument,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 订阅询价应答
  void OnRspSubForQuoteRsp(
      CThostFtdcSpecificInstrumentField *pSpecificInstrument,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 取消订阅询价应答
  void OnRspUnSubForQuoteRsp(
      CThostFtdcSpecificInstrumentField *pSpecificInstrument,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 深度行情通知
  void OnRtnDepthMarketData(
      CThostFtdcDepthMarketDataField *pDepthMarketData) final;

  //! 询价通知
  void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) final;

 private:
  void login();

 private:
  void subscribeAllMarketData();

 private:
  MDSvcOfCN *mdSvc_{nullptr};
  CThostFtdcMdApi *api_{nullptr};

  std::atomic_int requestID_{0};
};

}  // namespace bq::md::svc::ctp
