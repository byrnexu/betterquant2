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

namespace bq::md::svc {
class MDSvcOfCN;
}

namespace bq::md::svc::ctp {

class BQTraderSpi : public CThostFtdcTraderSpi {
 public:
  BQTraderSpi(MDSvcOfCN *mdSvc, CThostFtdcTraderApi *api);
  ~BQTraderSpi();

  //! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() final;

  //! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自
  //! 动重新连接，客户端可不做处理。
  void OnFrontDisconnected(int nReason) final;

  //! 心跳超时警告。当长时间未收到报文时，该方法被调用。
  void OnHeartBeatWarning(int nTimeLapse) final;

  //! 客户端认证响应
  void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField,
                         CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                         bool bIsLast) final;

  //! 登录请求响应
  void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                      CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                      bool bIsLast) final;

  //! 登出请求响应
  void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) final;

  //! 请求查询合约响应
  void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
                          CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                          bool bIsLast) final;

  //! 错误应答
  void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                  bool bIsLast) final;

 private:
  MDSvcOfCN *mdSvc_{nullptr};
  CThostFtdcTraderApi *api_{nullptr};

  std::atomic_int requestID_{0};
};

}  // namespace bq::md::svc::ctp
