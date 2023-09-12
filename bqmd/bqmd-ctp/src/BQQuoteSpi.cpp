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
#include "MDSvcOfCTP.hpp"
#include "MDSvcOfCTPUtil.hpp"
#include "RawMDHandler.hpp"
#include "db/TBLSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/RawMD.hpp"
#include "def/SymbolInfoTable.hpp"
#include "util/BQUtilOfCTP.hpp"
#include "util/Logger.hpp"
#include "util/String.hpp"

namespace bq::md::svc::ctp {

BQQuoteSpi::BQQuoteSpi(MDSvcOfCN *mdSvc, CThostFtdcMdApi *api)
    : mdSvc_(mdSvc), api_(api) {}

BQQuoteSpi::~BQQuoteSpi() {}

//! 当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void BQQuoteSpi::OnFrontConnected() { login(); }

//! 当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动
//! 重新连接，客户端可不做处理。
void BQQuoteSpi::OnFrontDisconnected(int nReason) {
  LOG_W("Front disconnected. [{} - {}]", nReason,
        GetFrontDisConnectErrorMsg(nReason));
}

//! 心跳超时警告。当长时间未收到报文时，该方法被调用。
void BQQuoteSpi::OnHeartBeatWarning(int nTimeLapse) {
  LOG_W("Heartbeat delay warning. [timeElapse: {}]", nTimeLapse);
}

//! 登录请求响应
void BQQuoteSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
                                CThostFtdcRspInfoField *pRspInfo,
                                int nRequestID, bool bIsLast) {
  const auto frontAddr = CONFIG["api"]["frontAddr"].as<std::string>();
  if (pRspInfo->ErrorID == 0) {
    //! 登录成功，获取交易日
    const auto tradingDay = api_->GetTradingDay();
    if (tradingDay && strlen(tradingDay) != 0) {
      mdSvc_->setTradingDay(tradingDay);
    }
    LOG_I("Login to {} success. [tradingDay: {}]", frontAddr,
          pRspUserLogin->TradingDay);
    //! 订阅全市场行情
    subscribeAllMarketData();
  } else {
    LOG_E("Login to {} failed. [{} - {}]", frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

void BQQuoteSpi::subscribeAllMarketData() {
  if (CONFIG["subAllMarketData"].as<bool>(false) == true) {
    //! 如果配置中配置了订阅全市场行情，那么订阅全市场
    LOG_I("Begin to subscribe all marketdata.");

    //! 申请内存，初始化订阅参数
    auto exchSymbolCodeGroup =
        mdSvc_->getSymbolInfoTable()->getExchSymbolCodeGroup();
    const auto tickerCount = exchSymbolCodeGroup.size();

    char **ppInstrumentID = new char *[tickerCount];
    for (std::size_t i = 0; i < tickerCount; i++) {
      ppInstrumentID[i] = new char[sizeof(TThostFtdcInstrumentIDType)];
    }

    for (auto iter = std::begin(exchSymbolCodeGroup);
         iter != std::end(exchSymbolCodeGroup); ++iter) {
      const auto pos = std::distance(std::begin(exchSymbolCodeGroup), iter);
      strncpy(ppInstrumentID[pos], iter->c_str(),
              sizeof(ppInstrumentID[pos]) - 1);
    }

    api_->SubscribeMarketData(ppInstrumentID, tickerCount);

    //! 释放内存
    for (std::size_t i = 0; i < tickerCount; i++) {
      delete[] ppInstrumentID[i];
      ppInstrumentID[i] = NULL;
    }

    delete[] ppInstrumentID;
    ppInstrumentID = NULL;
  }

  return;
}

//! 登出请求响应
void BQQuoteSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
                                 CThostFtdcRspInfoField *pRspInfo,
                                 int nRequestID, bool bIsLast) {}

//! 请求查询组播合约响应
void BQQuoteSpi::OnRspQryMulticastInstrument(
    CThostFtdcMulticastInstrumentField *pMulticastInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

//! 错误应答
void BQQuoteSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
                            bool bIsLast) {
  const auto frontAddr = CONFIG["api"]["frontAddr"].as<std::string>();
  if (pRspInfo && pRspInfo->ErrorID != 0) {
    LOG_E("Recv rsp of error from {}. [{} - {}]", frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 订阅行情应答
void BQQuoteSpi::OnRspSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  const auto frontAddr = CONFIG["api"]["frontAddr"].as<std::string>();
  if (pRspInfo->ErrorID == 0) {
    LOG_D("Sub ticker of {} from {} success. ",
          pSpecificInstrument->InstrumentID, frontAddr);
  } else {
    LOG_E("Sub ticker of {} from {} failed. [{} - {}]",
          pSpecificInstrument->InstrumentID, frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 取消订阅行情应答
void BQQuoteSpi::OnRspUnSubMarketData(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
  const auto frontAddr = CONFIG["api"]["frontAddr"].as<std::string>();
  if (pRspInfo->ErrorID == 0) {
    LOG_I("Unsub ticker of {} from {} success. ",
          pSpecificInstrument->InstrumentID, frontAddr);
  } else {
    LOG_E("Unsub ticker of {} from {} failed. [{} - {}]",
          pSpecificInstrument->InstrumentID, frontAddr, pRspInfo->ErrorID,
          CodeConvert(pRspInfo->ErrorMsg, "gb2312", "utf8"));
  }
}

//! 订阅询价应答
void BQQuoteSpi::OnRspSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

//! 取消订阅询价应答
void BQQuoteSpi::OnRspUnSubForQuoteRsp(
    CThostFtdcSpecificInstrumentField *pSpecificInstrument,
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {}

//! 深度行情通知
void BQQuoteSpi::OnRtnDepthMarketData(
    CThostFtdcDepthMarketDataField *pDepthMarketData) {
  auto rawMD = mdSvc_->getRawMDHandler()->makeRawMD(
      MsgType::Tickers, pDepthMarketData,
      sizeof(CThostFtdcDepthMarketDataField), true);
  mdSvc_->getRawMDHandler()->dispatch(rawMD);
}

//! 询价通知
void BQQuoteSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp) {
  std::cout << __func__ << std::endl;
}

//! 登录柜台
void BQQuoteSpi::login() {
  CThostFtdcReqUserLoginField loginReq;
  memset(&loginReq, 0, sizeof(loginReq));

  const auto brokerID = CONFIG["api"]["brokerID"].as<std::string>();
  const auto userID = CONFIG["api"]["userID"].as<std::string>();
  const auto password = CONFIG["api"]["password"].as<std::string>();
  const auto frontAddr = CONFIG["api"]["frontAddr"].as<std::string>();

  strncpy(loginReq.BrokerID, brokerID.c_str(), sizeof(loginReq.BrokerID) - 1);
  strncpy(loginReq.UserID, userID.c_str(), sizeof(loginReq.UserID) - 1);
  strncpy(loginReq.Password, password.c_str(), sizeof(loginReq.Password) - 1);

  LOG_I("Try login to {}. ", frontAddr);
  const auto ret = api_->ReqUserLogin(&loginReq, ++requestID_);
  if (ret != 0) {
    LOG_W("Try login to {} failed. [ret = {}]", frontAddr, ret);
  }
}

}  // namespace bq::md::svc::ctp
