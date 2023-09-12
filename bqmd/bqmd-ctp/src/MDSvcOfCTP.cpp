/*!
 * \file MDSvcOfCTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "MDSvcOfCTP.hpp"

#include "API.hpp"
#include "BQQuoteSpi.hpp"
#include "BQTraderSpi.hpp"
#include "Config.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvcOfCTPUtil.hpp"
#include "RawMDHandlerOfCTP.hpp"
#include "SHMIPCConst.hpp"
#include "SHMSrv.hpp"
#include "SHMSrvMsgHandler.hpp"
#include "db/DBE.hpp"
#include "db/DBEng.hpp"
#include "db/DBEngConst.hpp"
#include "def/SymbolInfoTable.hpp"
#include "tdeng/TDEngConnpool.hpp"
#include "tdeng/TDEngConst.hpp"
#include "tdeng/TDEngParam.hpp"
#include "util/BQMDUtil.hpp"
#include "util/Logger.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"

using namespace std;

namespace bq::md::svc::ctp {

MDSvcOfCTP::~MDSvcOfCTP() {}

int MDSvcOfCTP::beforeInit() {
  const auto raMDHandler = std::make_shared<RawMDHandlerOfCTP>(this);
  setRawMDHandler(raMDHandler);
  return 0;
}

int MDSvcOfCTP::doRun() { return 0; }

int MDSvcOfCTP::startGateway() {
  int statusCode = 0;

  //! 启动交易网关
  LOG_I("Start gateway of TD for get symbol table.");
  statusCode = startGatewayOfTD();
  if (statusCode != 0) {
    return statusCode;
  }

  //! 开始等待获取代码表
  resetBarrierOfGetSymbolTable();
  const auto secOfWaitForSymbolTable =
      CONFIG["secOfWaitForSymbolTable"].as<std::uint32_t>(30);
  const auto status = getBarrierOfGetSymbolTable()->get_future().wait_for(
      std::chrono::seconds(secOfWaitForSymbolTable));
  if (status == std::future_status::ready) {
    LOG_I("Get symbol table success.");
  } else {
    LOG_W("Get symbol table timeout.");
  }
  stopGatewayOfTD();

  //! 启动行情网关
  LOG_I("Start gateway of MD.");
  statusCode = startGatewayOfMD();
  if (statusCode != 0) {
    return statusCode;
  }

  return 0;
}

int MDSvcOfCTP::startGatewayOfTD() {
  stopGatewayOfTD();

  const auto flowPath = CONFIG["apiOfTD"]["flowPath"].as<std::string>();
  if (!boost::filesystem::exists(flowPath)) {
    boost::filesystem::create_directories(flowPath);
  }
  apiOfTD_ = CThostFtdcTraderApi::CreateFtdcTraderApi(flowPath.c_str());

  const auto spi = new BQTraderSpi(this, apiOfTD_);
  apiOfTD_->RegisterSpi(spi);

  apiOfTD_->RegisterFront(const_cast<char*>(
      CONFIG["apiOfTD"]["frontAddr"].as<std::string>().c_str()));
  apiOfTD_->SubscribePrivateTopic(THOST_TERT_QUICK);
  apiOfTD_->SubscribePublicTopic(THOST_TERT_QUICK);
  apiOfTD_->Init();

  return 0;
}

int MDSvcOfCTP::startGatewayOfMD() {
  stopGatewayOfMD();

  const auto flowPath = CONFIG["api"]["flowPath"].as<std::string>();
  if (!boost::filesystem::exists(flowPath)) {
    boost::filesystem::create_directories(flowPath);
  }
  api_ = CThostFtdcMdApi::CreateFtdcMdApi(flowPath.c_str());

  const auto spi = new BQQuoteSpi(this, api_);
  api_->RegisterSpi(spi);

  api_->RegisterFront(
      const_cast<char*>(CONFIG["api"]["frontAddr"].as<std::string>().c_str()));
  api_->Init();

  return 0;
}

void MDSvcOfCTP::stopGateway() {
  stopGatewayOfTD();
  stopGatewayOfMD();
}

void MDSvcOfCTP::stopGatewayOfTD() {
  if (apiOfTD_) {
    apiOfTD_->Release();
    apiOfTD_ = nullptr;
  }
}

void MDSvcOfCTP::stopGatewayOfMD() {
  if (api_) {
    api_->Release();
    api_ = nullptr;
  }
}

void MDSvcOfCTP::doSub(const MarketDataCondGroup& marketDataCondGroup) {
  using KeyType = MDType;
  using ValType = std::vector<MarketDataCondSPtr>;

  std::map<KeyType, ValType> m;

  for (const auto& marketDataCond : marketDataCondGroup) {
    m[marketDataCond->mdType_].emplace_back(marketDataCond);
  }

  for (const auto& rec : m) {
    const auto mdType = rec.first;
    const auto& mdCondGroup = rec.second;

    //! 申请内存，初始化订阅参数
    const auto tickerCount = mdCondGroup.size();

    char** ppInstrumentID = new char*[tickerCount];
    for (std::size_t i = 0; i < tickerCount; i++) {
      ppInstrumentID[i] = new char[sizeof(TThostFtdcInstrumentIDType)];
    }

    for (std::size_t i = 0; i < tickerCount; ++i) {
      const auto [statusCode, symbolInfo] =
          getSymbolInfoTable()->getSymbolInfoBySym(mdCondGroup[i]->symbolCode_);
      if (statusCode == 0) {
        strncpy(ppInstrumentID[i], symbolInfo->exchSymbolCode_.c_str(),
                sizeof(ppInstrumentID[i]) - 1);
      }
    }

    switch (mdType) {
      case MDType::Tickers:
        api_->SubscribeMarketData(ppInstrumentID, tickerCount);
        break;
      default:
        break;
    }

    //! 释放内存
    for (std::size_t i = 0; i < tickerCount; i++) {
      delete[] ppInstrumentID[i];
      ppInstrumentID[i] = NULL;
    }
    delete[] ppInstrumentID;
    ppInstrumentID = NULL;
  }
}

void MDSvcOfCTP::doUnSub(const MarketDataCondGroup& marketDataCondGroup) {
  using KeyType = MDType;
  using ValType = std::vector<MarketDataCondSPtr>;

  std::map<KeyType, ValType> m;

  for (const auto& marketDataCond : marketDataCondGroup) {
    m[marketDataCond->mdType_].emplace_back(marketDataCond);
  }

  for (const auto& rec : m) {
    const auto mdType = rec.first;
    const auto& mdCondGroup = rec.second;

    //! 申请内存，初始化订阅参数
    const auto tickerCount = mdCondGroup.size();

    char** ppInstrumentID = new char*[tickerCount];
    for (std::size_t i = 0; i < tickerCount; i++) {
      ppInstrumentID[i] = new char[sizeof(TThostFtdcInstrumentIDType)];
    }

    for (std::size_t i = 0; i < mdCondGroup.size(); ++i) {
      const auto [statusCode, symbolInfo] =
          getSymbolInfoTable()->getSymbolInfoBySym(mdCondGroup[i]->symbolCode_);
      if (statusCode == 0) {
        strncpy(ppInstrumentID[i], symbolInfo->exchSymbolCode_.c_str(),
                sizeof(ppInstrumentID[i]) - 1);
      }
    }

    switch (mdType) {
      case MDType::Tickers:
        api_->UnSubscribeMarketData(ppInstrumentID, mdCondGroup.size());
        break;
      default:
        break;
    }

    //! 释放内存
    for (std::size_t i = 0; i < tickerCount; i++) {
      delete[] ppInstrumentID[i];
      ppInstrumentID[i] = NULL;
    }
    delete[] ppInstrumentID;
    ppInstrumentID = NULL;
  }
}

}  // namespace bq::md::svc::ctp
