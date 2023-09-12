/*!
 * \file BQTraderSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/03
 *
 * \brief
 */
#pragma once

#include "API.hpp"
#include "def/BQDef.hpp"
#include "util/Pch.hpp"

namespace bq {
enum class MarketCode : std::uint16_t;
}

namespace bq::td::svc {
class TDSvcOfCN;
}

namespace bq::td::svc::ctp {

class BQTraderSpi : public CThostFtdcTraderSpi {
 public:
  BQTraderSpi(TDSvcOfCN *tdSvc, CThostFtdcTraderApi *api);
  ~BQTraderSpi();

  //! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
  void OnFrontConnected() final;

  //! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动
  //! 重新连接，客户端可不做处理。
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

  ///投资者结算结果确认响应
  void OnRspSettlementInfoConfirm(
      CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

  //! 登出请求响应
  void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                       CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                       bool bIsLast) final;

  //! 错误应答
  void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                  bool bIsLast) final;

  //! 报单录入请求响应
  void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                        CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) final;

  //! 报单录入错误回报
  void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
                           CThostFtdcRspInfoField *pRspInfo) final;

  //! 报单通知
  void OnRtnOrder(CThostFtdcOrderField *pOrder) final;

  //! 成交通知
  void OnRtnTrade(CThostFtdcTradeField *pTrade) final;

  //! 撤单操作请求响应
  void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction,
                        CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                        bool bIsLast) final;

  //! 撤单操作错误回报
  void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
                           CThostFtdcRspInfoField *pRspInfo) final;

  //! 请求查询报单响应
  void OnRspQryOrder(CThostFtdcOrderField *pOrder,
                     CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                     bool bIsLast) final;

  //! 请求查询投资者持仓响应
  void OnRspQryInvestorPosition(
      CThostFtdcInvestorPositionField *pInvestorPosition,
      CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) final;

 private:
  TDSvcOfCN *tdSvc_{nullptr};
  CThostFtdcTraderApi *api_{nullptr};

  std::string appID_;
  std::string authCode_;
  std::string brokerID_;
  std::string userID_;
  std::string password_;
  std::string frontAddr_;
  std::vector<MarketCode> marketCodeGroup_;

  std::atomic_int requestID_{0};
};

}  // namespace bq::td::svc::ctp
