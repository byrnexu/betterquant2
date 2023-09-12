/*!
 * \file BQTraderSpi.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/31
 *
 * \brief
 */

#include "BQTraderSpi.hpp"

#include "Config.hpp"
#include "MDSvcOfCN.hpp"
#include "MDSvcOfCTP.hpp"
#include "MDSvcOfCTPUtil.hpp"
#include "RawMDHandlerOfCTP.hpp"
#include "SHMIPC.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/StatusCode.hpp"
#include "util/BQUtilOfCTP.hpp"
#include "util/Datetime.hpp"
#include "util/Decimal.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

using namespace std;

namespace bq::md::svc::ctp {

BQTraderSpi::BQTraderSpi(MDSvcOfCN *mdSvc, CThostFtdcTraderApi *api)
    : mdSvc_(mdSvc), api_(api) {}

BQTraderSpi::~BQTraderSpi() {}

//! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void BQTraderSpi::OnFrontConnected() {
  CThostFtdcReqAuthenticateField auth;
  memset(&auth, 0, sizeof(auth));

  const auto appID = CONFIG["apiOfTD"]["appID"].as<std::string>();
  const auto authCode = CONFIG["apiOfTD"]["authCode"].as<std::string>();
  const auto brokerID = CONFIG["apiOfTD"]["brokerID"].as<std::string>();
  const auto userID = CONFIG["apiOfTD"]["userID"].as<std::string>();

  strncpy(auth.AppID, appID.c_str(), sizeof(auth.AppID) - 1);
  strncpy(auth.AuthCode, authCode.c_str(), sizeof(auth.AuthCode) - 1);
  strncpy(auth.BrokerID, brokerID.c_str(), sizeof(auth.BrokerID) - 1);
  strncpy(auth.UserID, userID.c_str(), sizeof(auth.UserID) - 1);

  int ret = api_->ReqAuthenticate(&auth, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} try to authenticate failed. [ret = {}]", userID,
          brokerID, ret);
    return;
  }
}

//! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动
//! 重新连接，客户端可不做处理。
void BQTraderSpi::OnFrontDisconnected(int nReason) {
  LOG_W("Front disconnected. [{} - {}]", nReason,
        GetFrontDisConnectErrorMsg(nReason));
}

//! 心跳超时警告。当长时间未收到报文时，该方法被调用。
void BQTraderSpi::OnHeartBeatWarning(int nTimeLapse) {
  LOG_W("Heartbeat delay warning. [timeElapse: {}]", nTimeLapse);
}

//! 客户端认证响应
void BQTraderSpi::OnRspAuthenticate(
    CThostFtdcRspAuthenticateField *pRspAuthenticateField,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  const auto brokerID = CONFIG["apiOfTD"]["brokerID"].as<std::string>();
  const auto userID = CONFIG["apiOfTD"]["userID"].as<std::string>();
  const auto password = CONFIG["apiOfTD"]["password"].as<std::string>();
  const auto frontAddr = CONFIG["apiOfTD"]["frontAddr"].as<std::string>();

  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_I("User {} of broker authenticate {} failed. [{} - {}]", userID,
          brokerID, frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  CThostFtdcReqUserLoginField loginReq;
  memset(&loginReq, 0, sizeof(loginReq));

  strncpy(loginReq.BrokerID, brokerID.c_str(), sizeof(loginReq.BrokerID) - 1);
  strncpy(loginReq.UserID, userID.c_str(), sizeof(loginReq.UserID) - 1);
  strncpy(loginReq.Password, password.c_str(), sizeof(loginReq.Password) - 1);

  int ret = api_->ReqUserLogin(&loginReq, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} try to login {} failed. [ret = {}]", userID,
          brokerID, frontAddr, ret);
    return;
  }
}

//! 登录请求响应
void BQTraderSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                 CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) {
  const auto brokerID = CONFIG["apiOfTD"]["brokerID"].as<std::string>();
  const auto userID = CONFIG["apiOfTD"]["userID"].as<std::string>();
  const auto password = CONFIG["apiOfTD"]["password"].as<std::string>();
  const auto frontAddr = CONFIG["apiOfTD"]["frontAddr"].as<std::string>();
  if (pRspInfo->ErrorID == 0) {
    const auto tradingDay = api_->GetTradingDay();
    if (tradingDay && strlen(tradingDay) != 0) {
      mdSvc_->setTradingDay(tradingDay);
    }
    LOG_I("User {} of broker {} login {} success. [tradingDay: {}]",
          pRspUserLogin->UserID, pRspUserLogin->BrokerID, frontAddr,
          pRspUserLogin->TradingDay);
  } else {
    LOG_E("User {} of broker {} login {} failed. [{} - {}]",
          pRspUserLogin->UserID, pRspUserLogin->BrokerID, frontAddr,
          pRspInfo->ErrorID, CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }

  //! 请求查询合约
  LOG_I("User {} of broker {} begin to query symbol table.", userID, brokerID);
  CThostFtdcQryInstrumentField req;
  memset(&req, 0, sizeof(req));
  const auto ret = api_->ReqQryInstrument(&req, ++requestID_);
  if (ret != 0) {
    LOG_W("User {} of broker {} query symbol table from {} failed. [ret = {}]",
          userID, brokerID, frontAddr, ret);
    return;
  }
}

//! 登出请求响应
void BQTraderSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                                  CThostFtdcRspInfoField *pRspInfo,
                                  int nRequestID, bool bIsLast) {}

//! 请求查询合约响应
void BQTraderSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
                                     CThostFtdcRspInfoField *pRspInfo,
                                     int nRequestID, bool bIsLast) {
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_W("Query symbol table failed. [{} - {}]", pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
    return;
  }
  LOG_D("Query symbol table success. [{} - {} - {}]", pInstrument->ExchangeID,
        pInstrument->InstrumentID,
        CodeConvert(pInstrument->InstrumentName, "gb2312", "utf8"));

  auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
      MsgType::NewSymbol, pInstrument, sizeof(CThostFtdcInstrumentField),
      bIsLast);
  mdSvc_->getRawMDHandler()->dispatch(rawMD);
}

//! 错误应答
void BQTraderSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                             bool bIsLast) {
  const auto frontAddr = CONFIG["apiOfTD"]["frontAddr"].as<std::string>();
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_E("Recv rsp of error from {}. [{} - {}]", frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

}  // namespace bq::md::svc::ctp
