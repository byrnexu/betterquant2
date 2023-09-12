/*!
 * \file MDSvcOfXTP.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "MDSvcOfXTP.hpp"

#include "API.hpp"
#include "BQQuoteSpi.hpp"
#include "Config.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvcOfXTPUtil.hpp"
#include "RawMDHandlerOfXTP.hpp"
#include "SHMIPCConst.hpp"
#include "SHMSrv.hpp"
#include "SHMSrvMsgHandler.hpp"
#include "db/DBE.hpp"
#include "db/DBEng.hpp"
#include "db/DBEngConst.hpp"
#include "tdeng/TDEngConnpool.hpp"
#include "tdeng/TDEngConst.hpp"
#include "tdeng/TDEngParam.hpp"
#include "util/Logger.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"

using namespace std;

namespace bq::md::svc::xtp {

MDSvcOfXTP::~MDSvcOfXTP() {
  if (api_) {
    api_->Release();
    api_ = nullptr;
  }
}

int MDSvcOfXTP::beforeInit() {
  //! 创建RawMDHandlerOfXTP用于处理spi中传递过来的二进制行情
  const auto raMDHandler = std::make_shared<RawMDHandlerOfXTP>(this);
  setRawMDHandler(raMDHandler);
  return 0;
}

int MDSvcOfXTP::doRun() { return 0; }

//! 此函数在基类的beforeRun中被调用
int MDSvcOfXTP::startGateway() {
  if (api_) {
    api_->Release();
    api_ = nullptr;
  }

  api_ = XTP::API::QuoteApi::CreateQuoteApi(
      CONFIG["api"]["clientId"].as<std::uint8_t>(),
      CONFIG["api"]["loggerPath"].as<std::string>().c_str(),
      XTP_LOG_LEVEL_INFO);
  api_->SetHeartBeatInterval(
      CONFIG["api"]["secIntervalOfHeartBeat"].as<std::uint32_t>());

  const auto spi = new BQQuoteSpi(this, api_);
  api_->RegisterSpi(spi);

  const auto [statusCode, protocolType] =
      GetXTPProtocolType(CONFIG["api"]["protocolType"].as<std::string>());
  if (statusCode != 0) {
    LOG_E(
        "Start gateway of {} failed "
        "because of invalid protocol type {} in config.",
        getApiName(), CONFIG["api"]["protocolType"].as<std::string>());
    return statusCode;
  }

  const auto ret = api_->Login(
      CONFIG["api"]["quoteServerIP"].as<std::string>().c_str(),
      CONFIG["api"]["quoteServerPort"].as<std::uint32_t>(),
      CONFIG["api"]["quoteUsername"].as<std::string>().c_str(),
      CONFIG["api"]["quotePassword"].as<std::string>().c_str(), protocolType,
      CONFIG["api"]["localIP"].as<std::string>().empty()
          ? nullptr
          : CONFIG["api"]["localIP"].as<std::string>().c_str());
  if (0 != ret) {
    //! 登录失败，获取错误信息
    XTPRI* error_info = api_->GetApiLastError();
    LOG_W("Login to server error. [{} - {}]", error_info->error_id,
          error_info->error_msg);
    return ret;
  }
  setTradingDay(api_->GetTradingDay());
  LOG_I("Login to server {}:{} success. [tradingDay: {}]",
        CONFIG["api"]["quoteServerIP"].as<std::string>(),
        CONFIG["api"]["quoteServerPort"].as<std::uint32_t>(), getTradingDay());

  //! 查询上海市场中的代码表（回调中继续查询深圳市场代码表）
  api_->QueryAllTickersFullInfo(XTP_EXCHANGE_SH);
  return 0;
}

void MDSvcOfXTP::stopGateway() {
  if (api_) {
    api_->Release();
    api_ = nullptr;
  }
}

void MDSvcOfXTP::doSub(const MarketDataCondGroup& marketDataCondGroup) {
  //! 按照exchMarketCode和mdType分组
  using KeyType = std::tuple<XTP_EXCHANGE_TYPE, MDType>;
  using ValType = std::vector<MarketDataCondSPtr>;

  std::map<KeyType, ValType> m;

  for (const auto& marketDataCond : marketDataCondGroup) {
    const auto exchMarketCode = GetExchMarketCode(marketDataCond->marketCode_);
    const auto key = std::make_tuple(exchMarketCode, marketDataCond->mdType_);
    m[key].emplace_back(marketDataCond);
  }

  for (const auto& rec : m) {
    const auto [exchMarketCode, mdType] = rec.first;
    const auto& mdCondGroup = rec.second;

    //! 申请内存，初始化订阅参数
    const auto tickerCount = mdCondGroup.size();

    char** ppInstrumentID = new char*[tickerCount];
    for (std::size_t i = 0; i < tickerCount; i++) {
      ppInstrumentID[i] = new char[XTP_TICKER_LEN];
    }

    for (std::size_t i = 0; i < mdCondGroup.size(); ++i) {
      strncpy(ppInstrumentID[i], mdCondGroup[i]->symbolCode_.c_str(),
              sizeof(ppInstrumentID[i]) - 1);
    }

    switch (mdType) {
      case MDType::Tickers:
        api_->SubscribeMarketData(ppInstrumentID, tickerCount, exchMarketCode);
        break;
      case MDType::Trades:
      case MDType::Orders:
        api_->SubscribeTickByTick(ppInstrumentID, tickerCount, exchMarketCode);
        break;
      case MDType::Books:
        api_->SubscribeOrderBook(ppInstrumentID, tickerCount, exchMarketCode);
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

void MDSvcOfXTP::doUnSub(const MarketDataCondGroup& marketDataCondGroup) {
  //! 按照exchMarketCode和mdType分组
  using KeyType = std::tuple<XTP_EXCHANGE_TYPE, MDType>;
  using ValType = std::vector<MarketDataCondSPtr>;

  std::map<KeyType, ValType> m;

  for (const auto& marketDataCond : marketDataCondGroup) {
    const auto exchMarketCode = GetExchMarketCode(marketDataCond->marketCode_);
    const auto key = std::make_tuple(exchMarketCode, marketDataCond->mdType_);
    m[key].emplace_back(marketDataCond);
  }

  for (const auto& rec : m) {
    const auto [exchMarketCode, mdType] = rec.first;
    const auto& mdCondGroup = rec.second;

    //! 申请内存，初始化订阅参数
    const auto tickerCount = mdCondGroup.size();

    char** ppInstrumentID = new char*[tickerCount];
    for (std::size_t i = 0; i < tickerCount; i++) {
      ppInstrumentID[i] = new char[XTP_TICKER_LEN];
    }

    for (std::size_t i = 0; i < mdCondGroup.size(); ++i) {
      strncpy(ppInstrumentID[i], mdCondGroup[i]->symbolCode_.c_str(),
              sizeof(ppInstrumentID[i]) - 1);
    }

    switch (mdType) {
      case MDType::Tickers:
        api_->UnSubscribeMarketData(ppInstrumentID, tickerCount,
                                    exchMarketCode);
        break;
      case MDType::Trades:
      case MDType::Orders:
        api_->UnSubscribeTickByTick(ppInstrumentID, tickerCount,
                                    exchMarketCode);
        break;
      case MDType::Books:
        api_->UnSubscribeOrderBook(ppInstrumentID, tickerCount, exchMarketCode);
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

}  // namespace bq::md::svc::xtp
