/*!
 * \file MDSvcOfCN.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#include "MDSvcOfCN.hpp"

#include "Config.hpp"
#include "MDCache.hpp"
#include "MDPlayback.hpp"
#include "MDStorageSvc.hpp"
#include "RawMDHandler.hpp"
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
#include "util/Logger.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"
#include "util/SysParams.hpp"
#include "util/TopicMgr.hpp"

using namespace std;

namespace bq::md::svc {

int MDSvcOfCN::prepareInit() {
  auto retOfConfInit = Config::get_mutable_instance().init(configFilename_);
  if (retOfConfInit != 0) {
    const auto statusMsg = fmt::format("Prepare init failed.");
    std::cerr << statusMsg << std::endl;
    return retOfConfInit;
  }

  const auto retOfLoggerInit = InitLogger(CONFIG);
  if (retOfLoggerInit != 0) {
    const auto statusMsg = fmt::format("Prepare init failed.");
    std::cerr << statusMsg << std::endl;
    return retOfLoggerInit;
  }

  //! 每个市场一个独立的SHMSrv
  marketCode2SHMSrvGroup_ = std::make_shared<MarketCode2SHMSrvGroup>();

  //! 时序数据库中记录行情来自哪个api
  apiName_ = CONFIG["api"]["apiName"].as<std::string>();

  return 0;
}

MDSvcOfCN::~MDSvcOfCN() {}

int MDSvcOfCN::doInit() {
  if (const auto ret = initDBEng(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  if (const auto ret = initTDEng(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  //! 主要用于订阅的时候获取交易所代码和全订阅的时候获取交易所的代码列表
  symbolInfoTable_ = std::make_shared<SymbolInfoTable>();

  shmSrvMsgHandler_ = std::make_shared<SHMSrvMsgHandler>(this);
  initSHMSrvGroup();

  //! 子类中将二进制的行情或者代码信息dispatch到rawMDHandler
  assert(rawMDHandler_ != nullptr && "rawMDHandler_ != nullptr");
  if (const auto statusCode = rawMDHandler_->init(); statusCode != 0) {
    return statusCode;
  }

  if (Config::get_const_instance().isSimedMode()) {
    //! 行情回放缓存
    mdCache_ = std::make_shared<MDCache>(this);
    //! 行情回放模块
    mdPlayback_ = std::make_shared<MDPlayback>(this);

  } else {
    //! 历史行情存储模块
    mdStorageSvc_ = std::make_shared<MDStorageSvc>(this);
    if (const auto statusCode = mdStorageSvc_->init(); statusCode != 0) {
      return statusCode;
    }
  }

  //! topic订阅管理器
  initTopicMgr();

  return 0;
}

int MDSvcOfCN::initDBEng() {
  const auto dbEngParam = SetParam(db::DEFAULT_DB_ENG_PARAM,
                                   CONFIG["dbEngParam"].as<std::string>());
  int retOfMakeDBEng = 0;
  std::tie(retOfMakeDBEng, dbEng_) = db::MakeDBEng(
      dbEngParam, [](db::DBTaskSPtr& dbTask, const StringSPtr& dbExecRet) {
        LOG_D("Exec sql finished. [{}] [exec result = {}]", dbTask->toStr(),
              *dbExecRet);
      });
  if (retOfMakeDBEng != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfMakeDBEng;
  }

  if (auto retOfInit = getDBEng()->init(); retOfInit != 0) {
    LOG_E("Init dbeng failed. {}", dbEngParam);
    return retOfInit;
  }

  return 0;
}

int MDSvcOfCN::initTDEng() {
  const auto tdEngParamInStrFmt = SetParam(
      tdeng::DEFAULT_TDENG_PARAM, CONFIG["tdEngParam"].as<std::string>());
  const auto [ret, tdEngParam] = tdeng::MakeTDEngParam(tdEngParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init failed. {}", tdEngParamInStrFmt);
    return ret;
  }

  tdEngConnpool_ = std::make_shared<tdeng::TDEngConnpool>(tdEngParam);
  if (const auto statusCode = tdEngConnpool_->init(); statusCode != 0) {
    LOG_E("Init failed. {}", tdEngParamInStrFmt);
    return -1;
  }

  return 0;
}

void MDSvcOfCN::initSHMSrvGroup() {
  const auto symbolType = CONFIG["symbolType"].as<std::string>();
  for (std::size_t i = 0; i < CONFIG["marketCodeGroup"].size(); ++i) {
    const auto marketCode = CONFIG["marketCodeGroup"][i].as<std::string>();

    //! addrOfSHMSrv = MD-XTP-INSTANCE@MD@SSE@Spot 用于连接和订阅
    const auto addrOfSHMSrv =
        fmt::format("{}-{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,
                    getApiName(), SEP_OF_SHM_SVC, TOPIC_PREFIX_OF_MARKET_DATA,
                    SEP_OF_SHM_SVC, marketCode, SEP_OF_SHM_SVC, symbolType);
    const auto shmSrv = std::make_shared<SHMSrv>(
        addrOfSHMSrv, [this](const auto* shmBuf, std::size_t shmBufLen) {
          assert(shmSrvMsgHandler_ != nullptr &&
                 "shmSrvMsgHandler_ != nullptr");
          //! shmSrvMsgHandler_处理所有SHMSrv的请求，比如说订阅请求
          shmSrvMsgHandler_->handleReq(shmBuf, shmBufLen);
        });

    //! 每个marketCode一个SHMSrv
    const auto m = GetMarketCode(marketCode);

    //! rawMDHandler或者网关子类中将生成统一格式的行情之后，通过getSHMSrv根据marketCode获取shmSrv并pushMsg
    marketCode2SHMSrvGroup_->emplace(m, shmSrv);
  }
}

void MDSvcOfCN::initTopicMgr() {
  if (topicMgr_ == nullptr) {
    topicMgr_ = std::make_shared<TopicMgr>(
        TopicMgrRole::Srv,
        //! 在SHMSrvMsgHandler收到订阅或者取消订阅请求之后中更新订阅信息，如下：
        //! 第一步：mdSvc_->getTopicMgr()->updateForSrv(subscriber2TopicGroupInJsonFmt);
        //! 第二步：topicMgr_中的定时器调用下面的回调，使用api发起订阅和取消订阅请求
        [this](const auto& anyData) {
          const auto topicNeedSubAndUnSub =
              std::any_cast<std::tuple<TopicGroup, TopicGroup>>(anyData);
          assert(shmSrvMsgHandler_ != nullptr &&
                 "shmSrvMsgHandler_ != nullptr");
          handleTopicNeedSubAndUnSub(topicNeedSubAndUnSub);
        });
  }
}

void MDSvcOfCN::handleTopicNeedSubAndUnSub(
    const std::tuple<TopicGroup, TopicGroup>& topicGroupNeedSubAndUnsub) {
  //! 如果已经开启了全市场订阅，那么无需处理来自客户端的个股订阅请求
  if (CONFIG["subAllMarketData"].as<bool>(false) == true) {
    return;
  }

  //! 获取订阅和取消订阅列表
  const auto [topicGroupNeedSub, topicGroupNeedUnSub] =
      topicGroupNeedSubAndUnsub;

  if (!topicGroupNeedSub.empty()) {
    LOG_T("+++ Topic need sub: {}", boost::join(topicGroupNeedSub, ","));
  }

  if (!topicGroupNeedUnSub.empty()) {
    LOG_T("--- Topic need unsub: {}", boost::join(topicGroupNeedUnSub, ","));
  }

  //! 获取订阅列表marketDataCondNeedSub
  MarketDataCondGroup marketDataCondNeedSub;
  for (const auto& topic : topicGroupNeedSub) {
    const auto [statusCode, marketDataCond] = GetMarketDataCondFromTopic(topic);
    if (statusCode != 0) {
      LOG_W("Handle topic of sub failed because of invalid topic. {}", topic);
      continue;
    }
    marketDataCondNeedSub.emplace_back(marketDataCond);
  }

  //! 发起订阅
  if (!marketDataCondNeedSub.empty()) {
    if (Config::get_const_instance().isSimedMode() == false) {
      doSub(marketDataCondNeedSub);
    }
  }

  //! 获取取消订阅列表marketDataCondNeedUnSub
  MarketDataCondGroup marketDataCondNeedUnSub;
  for (const auto& topic : topicGroupNeedUnSub) {
    const auto [statusCode, marketDataCond] = GetMarketDataCondFromTopic(topic);
    if (statusCode != 0) {
      LOG_W("Handle topic of unsub failed because of invalid topic. {}", topic);
      continue;
    }
    marketDataCondNeedUnSub.emplace_back(marketDataCond);
  }

  //! 发起取消订阅
  if (!marketDataCondNeedUnSub.empty()) {
    if (Config::get_const_instance().isSimedMode() == false) {
      doUnSub(marketDataCondNeedUnSub);
    }
  }
}

int MDSvcOfCN::beforeRun() {
  LOG_I("Run market data service of {}.", getApiName());

  //! 如果是实盘，那么启动mdStorageSvc_存储历史行情
  if (Config::get_const_instance().isSimedMode() == false) {
    mdStorageSvc_->start();
  }

  //! 在beforeInit已经setRawMDHandler(raMDHandler); rawMDHandler_主要是
  //! makeAsyncTask和handleMDTickers等功能
  rawMDHandler_->start();

  //! 逐个启动SHMSrv
  for (const auto marketCode2SHMSrv : *marketCode2SHMSrvGroup_) {
    const auto& shmSrv = marketCode2SHMSrv.second;
    shmSrv->start();
  }

  if (Config::get_const_instance().isSimedMode()) {
    const auto statusCode = startGatewayOfSim();
    if (statusCode != 0) {
      LOG_E("Before run failed because of start gateway of sim failed.");
      return statusCode;
    }
  } else {
    const auto statusCode = startGateway();
    if (statusCode != 0) {
      LOG_E("Before run failed because of start gateway failed.");
      return statusCode;
    }
  }

  return 0;
}

int MDSvcOfCN::startGatewayOfSim() {
  const auto statusCode = mdCache_->start();
  if (statusCode != 0) {
    return statusCode;
  }
  mdPlayback_->start();
  return 0;
}

int MDSvcOfCN::afterRun() {
  //! 因为topicMgr_要用到gateway的订阅和取消订阅，所以必须等doRun中的gateway就绪
  topicMgr_->start();

  return SvcBase::afterRun();
}

void MDSvcOfCN::beforeExit(const boost::system::error_code* ec, int signalNum) {
  if (Config::get_const_instance().isSimedMode()) {
    stopGatewayOfSim();
  } else {
    stopGateway();
  }
}

void MDSvcOfCN::stopGatewayOfSim() {
  mdPlayback_->stop();
  mdCache_->stop();
}

void MDSvcOfCN::doExit(const boost::system::error_code* ec, int signalNum) {
  topicMgr_->stop();

  for (const auto marketCode2SHMSrv : *marketCode2SHMSrvGroup_) {
    const auto& shmSrv = marketCode2SHMSrv.second;
    shmSrv->stop();
  }
  rawMDHandler_->stop();

  if (Config::get_const_instance().isSimedMode() == false) {
    mdStorageSvc_->flushMDToTDEng();
    mdStorageSvc_->stop();
  }

  tdEngConnpool_->uninit();

  LOG_I("Exit market data service of {}.", getApiName());
}

void MDSvcOfCN::setTradingDay(const std::string& value) {
  tradingDay_ = value;
  for (std::size_t i = 0; i < CONFIG["marketCodeGroup"].size(); ++i) {
    const auto marketCode = CONFIG["marketCodeGroup"][i].as<std::string>();
    const auto paramName = "tradingDay";
    const auto paramType = marketCode;
    const auto paramValue = value;
    UpdateSysParams(dbEng_, paramName, paramType, paramValue);
  }
}

SHMSrvSPtr MDSvcOfCN::getSHMSrv(MarketCode marketCode) const {
  const auto iter = marketCode2SHMSrvGroup_->find(marketCode);
  if (iter != std::end(*marketCode2SHMSrvGroup_)) {
    return iter->second;
  }
  return nullptr;
}

}  // namespace bq::md::svc
