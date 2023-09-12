/*!
 * \file RawMDHandler.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief 接收spi传递过来的二进制的代码或者行情信息并异步处理
 *
 */

#include "RawMDHandler.hpp"

#include "Config.hpp"
#include "MDStorageSvc.hpp"
#include "MDSvcOfCN.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "db/TBLSymbolInfo.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/RawMD.hpp"
#include "def/RawMDAsyncTaskArg.hpp"
#include "util/BQMDUtil.hpp"
#include "util/BQUtil.hpp"
#include "util/Datetime.hpp"
#include "util/File.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::md::svc {

RawMDHandler::RawMDHandler(MDSvcOfCN* mdSvc)
    : mdSvc_(mdSvc),
      topic2LastExchTsGroup_(std::make_shared<Topic2LastTsGroup>()),
      saveBooks_(CONFIG["saveBooks"].as<bool>(false)),
      saveTrades_(CONFIG["saveTrades"].as<bool>(false)),
      saveOrders_(CONFIG["saveOrders"].as<bool>(false)),
      saveTickers_(CONFIG["saveTickers"].as<bool>(false)),
      createBid1Ask1ByTickers_(CONFIG["createBid1Ask1ByTickers"].as<bool>()),
      createLastPriceByTickers_(CONFIG["createLastPriceByTickers"].as<bool>()),
      checkIFExchTsOfMDIsInc_(CONFIG["checkIFExchTsOfMDIsInc"].as<bool>(true)),
      loggerThresholdOfTopicRecvTimes_(
          CONFIG["loggerThresholdOfTopicRecvTimes"].as<std::size_t>(100)) {}

int RawMDHandler::init() {
  const auto rawMDHandlerParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               CONFIG["rawMDHandlerParam"].as<std::string>());
  const auto [ret, rawMDHandlerParam] =
      MakeTaskDispatcherParam(rawMDHandlerParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init failed. {}", rawMDHandlerParamInStrFmt);
    return ret;
  }

  taskDispatcher_ = std::make_shared<TaskDispatcher<RawMDSPtr>>(
      rawMDHandlerParam,

      //! 将 RawMD 也就是 task 填上 MarketCode SymbolType SymbolCode 三个字段，
      //! 通过这三个字段的 hash 在线程中分流行情信息，发布的 topic 也是根据这
      //! 三个字段，因此这三个字段都必须是本地统一的格式
      [this](const auto& task) { return makeAsyncTask(task); },

      //! NewSymbol类型消息都在0号线程处理，防止isLast判断乱序
      [this](auto& asyncTask, auto taskSpecificThreadPoolSize) {
        if (asyncTask->task_->msgType_ == MsgType::NewSymbol) {
          return ThreadNo(0);
        }

        //! 计算rawMD也就是asyncTask->task_的topic和topicHash
        InitTopicInfo(asyncTask->task_);

#ifndef _NDEBUG
        const auto& topic = asyncTask->task_->topic_;
        LOG_T("Init task of topic {}", topic);
        topic2CntGroup_[topic] += 1;
        if (topic2CntGroup_[topic] % loggerThresholdOfTopicRecvTimes_ == 0) {
          LOG_I("Recv {} {} times", topic, topic2CntGroup_[topic]);
        }
#endif
        const std::uint32_t threadNo =
            asyncTask->task_->topicHash_ % taskSpecificThreadPoolSize;

        return threadNo;
      },

      [this](auto& asyncTask) { handle(asyncTask); });

  taskDispatcher_->init();

  return ret;
}

RawMDSPtr RawMDHandler::makeRawMD(MsgType msgType, const void* data,
                                  std::size_t dataLen, bool isLast) {
  switch (msgType) {
    case MsgType::NewSymbol:
      return std::make_shared<RawMD>(  //
          MsgType::NewSymbol, data, dataLen, isLast);

    case MsgType::Tickers:
      return std::make_shared<RawMD>(  //
          MsgType::Tickers, data, dataLen, isLast);

    case MsgType::Trades:
      return std::make_shared<RawMD>(  //
          MsgType::Trades, data, dataLen, isLast);

    case MsgType::Orders:
      return std::make_shared<RawMD>(  //
          MsgType::Orders, data, dataLen, isLast);

    case MsgType::Books:
      return std::make_shared<RawMD>(  //
          MsgType::Books, data, dataLen, isLast);

    default:
      return nullptr;
  }

  return nullptr;
}

void RawMDHandler::start() { taskDispatcher_->start(); }
void RawMDHandler::stop() { taskDispatcher_->stop(); }

void RawMDHandler::dispatch(RawMDSPtr& task) {
  taskDispatcher_->dispatch(task);
}

void RawMDHandler::handle(RawMDAsyncTaskSPtr& asyncTask) {
  switch (asyncTask->task_->msgType_) {
    case MsgType::NewSymbol:
      handleNewSymbol(asyncTask);
      break;

    case MsgType::Tickers:
      if (handleMDTickers(asyncTask)) {
        //! 合约的Bid1Ask1和LastPrice都通过Tickers生成
        handleMDBid1Ask1ByTickers(asyncTask);
        handleMDLastPriceByTickers(asyncTask);
        if (saveTickers_ && !Config::get_const_instance().isSimedMode()) {
          mdSvc_->getMDStorageSvc()->dispatch(asyncTask);
        }
      }
      break;

    case MsgType::Trades:
      if (handleMDTrades(asyncTask)) {
        //! 现货的LastPrice通过Trades生成
        handleMDLastPriceByTrades(asyncTask);
        if (saveTrades_ && !Config::get_const_instance().isSimedMode()) {
          mdSvc_->getMDStorageSvc()->dispatch(asyncTask);
        }
      }
      break;

    case MsgType::Orders:
      if (handleMDOrders(asyncTask)) {
        if (saveOrders_ && !Config::get_const_instance().isSimedMode()) {
          mdSvc_->getMDStorageSvc()->dispatch(asyncTask);
        }
      }
      break;

    case MsgType::Books:
      if (handleMDBooks(asyncTask)) {
        //! 如果配置中现货的Bid1Ask1不通过Tickers生成，那么就通过Books生成
        handleMDBid1Ask1ByBooks(asyncTask);
        if (saveBooks_ && !Config::get_const_instance().isSimedMode()) {
          mdSvc_->getMDStorageSvc()->dispatch(asyncTask);
        }
      }
      break;

    default:
      assert(1 == 2 && "Entered an impossible code segment");
      break;
  }
}

void RawMDHandler::handleMDBid1Ask1ByTickers(RawMDAsyncTaskSPtr& asyncTask) {
  const auto& rawMD = asyncTask->task_;

  if (IsCNMarketOfFutures(rawMD->marketCode_)) {
    //! 如果是国内期货市场，那么就用tickers生成bid1ask1
  } else {
    //! 如果是国内现货市场
    if (createBid1Ask1ByTickers_) {
      //! 如果使用tickers生成bid1ask1
    } else {
      return;
    }
  }

  const auto marketName = GetMarketName(rawMD->marketCode_);
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,        //
                  SEP_OF_TOPIC, marketName,                                 //
                  SEP_OF_TOPIC, magic_enum::enum_name(rawMD->symbolType_),  //
                  SEP_OF_TOPIC, rawMD->symbolCode_,                         //
                  SEP_OF_TOPIC, magic_enum::enum_name(MDType::Bid1Ask1));   //

  const auto tickers = static_cast<Tickers*>(asyncTask->task_->dataAfterConv_);

  auto bid1Ask1 = std::make_unique<Bid1Ask1>();
  bid1Ask1->shmHeader_.topicHash_ = XXH3_64bits(topic.data(), topic.size());
  memcpy(&bid1Ask1->mdHeader_, &tickers->mdHeader_, sizeof(MDHeader));
  bid1Ask1->mdHeader_.mdType_ = MDType::Bid1Ask1;
  bid1Ask1->askPrice_ = tickers->askPrice_;
  bid1Ask1->askSize_ = tickers->askSize_;
  bid1Ask1->bidPrice_ = tickers->bidPrice_;
  bid1Ask1->bidSize_ = tickers->bidSize_;
  strcpy(bid1Ask1->tradingDay_, tickers->tradingDay_);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(bid1Ask1->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.", marketName);
    return;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_BID1_ASK1, bid1Ask1.get(),
                  sizeof(Bid1Ask1));
}

void RawMDHandler::handleMDLastPriceByTickers(RawMDAsyncTaskSPtr& asyncTask) {
  const auto& rawMD = asyncTask->task_;

  if (IsCNMarketOfFutures(rawMD->marketCode_)) {
    //! 如果是国内期货市场，那么就用tickers生成lastPrice
  } else {
    //! 如果是国内现货市场
    if (createLastPriceByTickers_) {
      //! 如果使用tickers生成lastPrice
    } else {
      return;
    }
  }

  const auto marketName = GetMarketName(rawMD->marketCode_);
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,        //
                  SEP_OF_TOPIC, marketName,                                 //
                  SEP_OF_TOPIC, magic_enum::enum_name(rawMD->symbolType_),  //
                  SEP_OF_TOPIC, rawMD->symbolCode_,                         //
                  SEP_OF_TOPIC, magic_enum::enum_name(MDType::LastPrice));  //

  const auto tickers = static_cast<Tickers*>(asyncTask->task_->dataAfterConv_);

  auto lastPrice = std::make_unique<LastPrice>();
  lastPrice->shmHeader_.topicHash_ = XXH3_64bits(topic.data(), topic.size());
  memcpy(&lastPrice->mdHeader_, &tickers->mdHeader_, sizeof(MDHeader));
  lastPrice->mdHeader_.mdType_ = MDType::LastPrice;
  lastPrice->lastPrice_ = tickers->lastPrice_;
  lastPrice->lastSize_ = tickers->lastSize_;
  strcpy(lastPrice->tradingDay_, tickers->tradingDay_);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(lastPrice->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.", marketName);
    return;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_LAST_PRICE, lastPrice.get(),
                  sizeof(LastPrice));
}

void RawMDHandler::handleMDBid1Ask1ByBooks(RawMDAsyncTaskSPtr& asyncTask) {
  const auto& rawMD = asyncTask->task_;

  if (IsCNMarketOfFutures(rawMD->marketCode_)) {
    //! 如果是国内期货市场
    return;
  } else {
    //! 如果是国内现货市场
    if (createBid1Ask1ByTickers_) {
      return;
    } else {
    }
  }

  const auto marketName = GetMarketName(rawMD->marketCode_);
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,        //
                  SEP_OF_TOPIC, marketName,                                 //
                  SEP_OF_TOPIC, magic_enum::enum_name(rawMD->symbolType_),  //
                  SEP_OF_TOPIC, rawMD->symbolCode_,                         //
                  SEP_OF_TOPIC, magic_enum::enum_name(MDType::Bid1Ask1));   //

  const auto books = static_cast<Books*>(asyncTask->task_->dataAfterConv_);

  auto bid1Ask1 = std::make_unique<Bid1Ask1>();
  bid1Ask1->shmHeader_.topicHash_ = XXH3_64bits(topic.data(), topic.size());
  memcpy(&bid1Ask1->mdHeader_, &books->mdHeader_, sizeof(MDHeader));
  bid1Ask1->mdHeader_.mdType_ = MDType::Bid1Ask1;
  bid1Ask1->askPrice_ = books->asks_[0].price_;
  bid1Ask1->askSize_ = books->asks_[0].size_;
  bid1Ask1->bidPrice_ = books->bids_[0].price_;
  bid1Ask1->bidSize_ = books->bids_[0].size_;
  strcpy(bid1Ask1->tradingDay_, books->tradingDay_);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(bid1Ask1->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.", marketName);
    return;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_BID1_ASK1, bid1Ask1.get(),
                  sizeof(Bid1Ask1));
}

void RawMDHandler::handleMDLastPriceByTrades(RawMDAsyncTaskSPtr& asyncTask) {
  const auto& rawMD = asyncTask->task_;

  if (IsCNMarketOfFutures(rawMD->marketCode_)) {
    //! 如果是国内期货市场
    return;
  } else {
    //! 如果是国内现货市场
    if (createLastPriceByTickers_) {
      return;
    } else {
    }
  }

  const auto marketName = GetMarketName(rawMD->marketCode_);
  const auto topic =
      fmt::format("{}{}{}{}{}{}{}{}{}", TOPIC_PREFIX_OF_MARKET_DATA,        //
                  SEP_OF_TOPIC, marketName,                                 //
                  SEP_OF_TOPIC, magic_enum::enum_name(rawMD->symbolType_),  //
                  SEP_OF_TOPIC, rawMD->symbolCode_,                         //
                  SEP_OF_TOPIC, magic_enum::enum_name(MDType::LastPrice));  //

  const auto trades = static_cast<Trades*>(asyncTask->task_->dataAfterConv_);

  auto lastPrice = std::make_unique<LastPrice>();
  lastPrice->shmHeader_.topicHash_ = XXH3_64bits(topic.data(), topic.size());
  memcpy(&lastPrice->mdHeader_, &trades->mdHeader_, sizeof(MDHeader));
  lastPrice->mdHeader_.mdType_ = MDType::LastPrice;
  lastPrice->lastPrice_ = trades->price_;
  lastPrice->lastSize_ = trades->size_;
  strcpy(lastPrice->tradingDay_, trades->tradingDay_);

  // push msg
  const auto shmSrv = mdSvc_->getSHMSrv(lastPrice->mdHeader_.marketCode_);
  if (!shmSrv) {
    LOG_W("Invalid market code {}.", marketName);
    return;
  }
  shmSrv->pushMsg(PUB_CHANNEL, MSG_ID_ON_MD_LAST_PRICE, lastPrice.get(),
                  sizeof(LastPrice));
}

//! 用于检查行情中的时间戳是否乱序，有一定的性能开销，生产环境中可以考虑关闭检测
bool RawMDHandler::checkIfMDIsLegal(RawMDAsyncTaskSPtr& asyncTask,
                                    std::uint64_t exchTs) {
  if (!checkIFExchTsOfMDIsInc_) return true;
  const auto& topic = asyncTask->task_->topic_;
  {
    std::lock_guard<std::mutex> guard(mtxTopic2LastExchTsGroup_);
    auto iter = topic2LastExchTsGroup_->find(topic);
    if (iter == std::end(*topic2LastExchTsGroup_)) {
      (*topic2LastExchTsGroup_)[topic] = exchTs;
      return true;
    } else {
      auto& exchTsInCache = iter->second;
      //! 如果cache中的exchTs小于等于当前行情中的exchTs那么说明没有乱续
      if (exchTsInCache <= exchTs) {
        exchTsInCache = exchTs;
        return true;
      } else {
        LOG_W("Found exchTs in cache {} greater than exchTs {}. {}",
              exchTsInCache, exchTs, topic);
        return false;
      }
    }
  }
  return true;
}

}  // namespace bq::md::svc
