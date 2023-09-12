/*!
 * \file MDStorageSvc.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/16
 *
 * \brief
 */

#include "MDStorageSvc.hpp"

#include "Config.hpp"
#include "MDSvcOfCN.hpp"
#include "SHMIPCConst.hpp"
#include "SHMIPCMsgId.hpp"
#include "SHMIPCUtil.hpp"
#include "SHMSrv.hpp"
#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfMD.hpp"
#include "def/RawMD.hpp"
#include "def/RawMDAsyncTaskArg.hpp"
#include "taos.h"
#include "tdeng/TDEngConnpool.hpp"
#include "util/BQMDUtil.hpp"
#include "util/Datetime.hpp"
#include "util/File.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/MarketDataCond.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::md::svc {

MDStorageSvc::MDStorageSvc(MDSvcOfCN const* mdSvc)
    : mdSvc_(mdSvc),
      topic2AsyncTaskGroup_(std::make_shared<Topic2AsyncTaskGroup>()),
      topic2LastExchTsGroup_(std::make_shared<Topic2LastTsGroup>()),
      topic2LastLocalTsGroup_(std::make_shared<Topic2LastTsGroup>()) {}

int MDStorageSvc::init() {
  const auto mdStorageSvcParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               CONFIG["mdStorageSvcParam"].as<std::string>());
  const auto [ret, mdStorageSvcParam] =
      MakeTaskDispatcherParam(mdStorageSvcParamInStrFmt);
  if (ret != 0) {
    LOG_E("Init failed. {}", mdStorageSvcParamInStrFmt);
    return ret;
  }

  const auto cbMsgParser = nullptr;
  taskDispatcher_ = std::make_shared<TaskDispatcher<RawMDSPtr>>(
      mdStorageSvcParam, cbMsgParser,
      [](auto& asyncTask, auto taskSpecificThreadPoolSize) {
        const auto threadNo =
            asyncTask->task_->topicHash_ % taskSpecificThreadPoolSize;
        return threadNo;
      },
      [this](auto& asyncTask) { handle(asyncTask); });

  taskDispatcher_->init();
  return ret;
}

void MDStorageSvc::start() { taskDispatcher_->start(); }
void MDStorageSvc::stop() { taskDispatcher_->stop(); }

void MDStorageSvc::dispatch(RawMDAsyncTaskSPtr& asyncTask) {
  taskDispatcher_->dispatch(asyncTask);
}

void MDStorageSvc::handle(RawMDAsyncTaskSPtr& asyncTask) {
  switch (asyncTask->task_->msgType_) {
    case MsgType::Tickers:
      handleMDTickers(asyncTask);
      break;

    case MsgType::Trades:
      handleMDTrades(asyncTask);
      break;

    case MsgType::Orders:
      handleMDOrders(asyncTask);
      break;

    case MsgType::Books:
      handleMDBooks(asyncTask);
      break;

    default:
      assert(1 == 2 && "Entered an impossible code segment");
      break;
  }
}

void MDStorageSvc::handleMDTickers(RawMDAsyncTaskSPtr& asyncTask) {
  const auto tickers = static_cast<Tickers*>(asyncTask->task_->dataAfterConv_);
  checkAndUpdateTsToAvoidDulplication(asyncTask, &tickers->mdHeader_);
  const auto topic2AsyncTaskGroupWrittenToTDEng =
      getTopic2AsyncTaskGroupWrittenToTDEng(asyncTask);
  //! 为空说明记录数还没达到flushMDToTDEng要求的数量
  if (topic2AsyncTaskGroupWrittenToTDEng->empty()) {
    return;
  }
  flushMDToTDEng(topic2AsyncTaskGroupWrittenToTDEng);
}

void MDStorageSvc::handleMDTrades(RawMDAsyncTaskSPtr& asyncTask) {
  const auto trades = static_cast<Trades*>(asyncTask->task_->dataAfterConv_);
  checkAndUpdateTsToAvoidDulplication(asyncTask, &trades->mdHeader_);
  const auto topic2AsyncTaskGroupWrittenToTDEng =
      getTopic2AsyncTaskGroupWrittenToTDEng(asyncTask);
  //! 为空说明记录数还没达到flushMDToTDEng要求的数量
  if (topic2AsyncTaskGroupWrittenToTDEng->empty()) {
    return;
  }
  flushMDToTDEng(topic2AsyncTaskGroupWrittenToTDEng);
}

void MDStorageSvc::handleMDOrders(RawMDAsyncTaskSPtr& asyncTask) {
  const auto orders = static_cast<Orders*>(asyncTask->task_->dataAfterConv_);
  checkAndUpdateTsToAvoidDulplication(asyncTask, &orders->mdHeader_);
  const auto topic2AsyncTaskGroupWrittenToTDEng =
      getTopic2AsyncTaskGroupWrittenToTDEng(asyncTask);
  //! 为空说明记录数还没达到flushMDToTDEng要求的数量
  if (topic2AsyncTaskGroupWrittenToTDEng->empty()) {
    return;
  }
  flushMDToTDEng(topic2AsyncTaskGroupWrittenToTDEng);
}

void MDStorageSvc::handleMDBooks(RawMDAsyncTaskSPtr& asyncTask) {
  const auto books = static_cast<Books*>(asyncTask->task_->dataAfterConv_);
  checkAndUpdateTsToAvoidDulplication(asyncTask, &books->mdHeader_);
  const auto topic2AsyncTaskGroupWrittenToTDEng =
      getTopic2AsyncTaskGroupWrittenToTDEng(asyncTask);
  //! 为空说明记录数还没达到flushMDToTDEng要求的数量
  if (topic2AsyncTaskGroupWrittenToTDEng->empty()) {
    return;
  }
  flushMDToTDEng(topic2AsyncTaskGroupWrittenToTDEng);
}

//! 数据库中时间戳不能重复，因此如果重复的话，增加1us
void MDStorageSvc::checkAndUpdateTsToAvoidDulplication(
    RawMDAsyncTaskSPtr& asyncTask, MDHeader* mdHeader) {
  const auto topic = asyncTask->task_->topic_;
  {
    std::lock_guard<std::mutex> guard(mtxTopic2LastTsGroup_);
    auto iterExchTs = topic2LastExchTsGroup_->find(topic);
    if (iterExchTs != std::end(*topic2LastExchTsGroup_)) {
      auto& exchTsInCache = iterExchTs->second;
      if (exchTsInCache >= mdHeader->exchTs_) {
        //! 来了一个更加早的消息
        LOG_T("Found exchTs in cache {} greater than exchTs of topic {}.",
              exchTsInCache, topic);
        exchTsInCache += 1;
        mdHeader->exchTs_ = exchTsInCache;
      } else {
        //! 来了一个更加新的消息
        exchTsInCache = mdHeader->exchTs_;
      }
    } else {
      (*topic2LastExchTsGroup_)[topic] = mdHeader->exchTs_;
    }

    auto iterLocalTs = topic2LastLocalTsGroup_->find(topic);
    if (iterLocalTs != std::end(*topic2LastLocalTsGroup_)) {
      auto& localTsInCache = iterLocalTs->second;
      if (localTsInCache >= mdHeader->localTs_) {
        //! 来了一个更加早的消息
        LOG_W("Found localTs in cache {} greater than localTs of topic {}.",
              localTsInCache, topic);
        localTsInCache += 1;
        mdHeader->localTs_ = localTsInCache;
      } else {
        //! 来了一个更加新的消息
        localTsInCache = mdHeader->localTs_;
      }
    } else {
      (*topic2LastLocalTsGroup_)[topic] = mdHeader->localTs_;
    }
  }
}

Topic2AsyncTaskGroupSPtr MDStorageSvc::getTopic2AsyncTaskGroupWrittenToTDEng(
    const RawMDAsyncTaskSPtr& asyncTask) {
  auto ret = std::make_shared<Topic2AsyncTaskGroup>();

  const auto numOfMDWrittenToTDEngAtOneTime =
      CONFIG["numOfMDWrittenToTDEngAtOneTime"].as<std::uint32_t>(100);

  std::uint32_t asyncTaskNumInCache = 0;
  {
    std::lock_guard<std::mutex> guard(mtxTopic2AsyncTaskGroup_);

    //! 将当前记录加入缓存
    (*topic2AsyncTaskGroup_)[asyncTask->task_->topic_].emplace_back(asyncTask);

    //! 获取缓存中记录数量
    for (const auto& rec : *topic2AsyncTaskGroup_) {
      asyncTaskNumInCache += rec.second.size();
    }

    //! 如果缓存中的记录数量超过numOfMDWrittenToTDEngAtOneTime，那么返回记录用于入库
    if (asyncTaskNumInCache >= numOfMDWrittenToTDEngAtOneTime) {
      ret.swap(topic2AsyncTaskGroup_);
    }
  }

  return ret;
}

void MDStorageSvc::flushMDToTDEng() {
  auto topic2AsyncTaskGroup = std::make_shared<Topic2AsyncTaskGroup>();
  {
    std::lock_guard<std::mutex> guard(mtxTopic2AsyncTaskGroup_);
    topic2AsyncTaskGroup.swap(topic2AsyncTaskGroup_);
  }
  flushMDToTDEng(topic2AsyncTaskGroup);
}

void MDStorageSvc::flushMDToTDEng(
    const Topic2AsyncTaskGroupSPtr& topic2AsyncTaskGroup) {
  const auto sql = makeSql(topic2AsyncTaskGroup);
  //! 当停止服务触发此函数的时候sql可能为空
  if (sql.empty()) return;
  LOG_T("Flush market data to tdeng. [sqllen = {}]", sql.size());

  const auto conn = mdSvc_->getTDEngConnpool()->getIdleConn();
  const auto res = taos_query(conn->taos_, sql.c_str());
  const auto statusCode = taos_errno(res);
  if (statusCode != 0) {
    LOG_W("Exec sql of tdeng failed. [{} - {}]", statusCode, taos_errno(res));
  }
  taos_free_result(res);
  mdSvc_->getTDEngConnpool()->giveBackConn(conn);
}

std::string MDStorageSvc::makeSql(
    const Topic2AsyncTaskGroupSPtr& topic2AsyncTaskGroup) {
  std::string sql;
  for (const auto& rec : *topic2AsyncTaskGroup) {
    const auto& asyncTaskGroup = rec.second;
    if (asyncTaskGroup.empty()) continue;

    std::string sqlOfCurTopic;
    std::string sqlOfCurTopicOfOrigData;
    for (const auto& asyncTask : asyncTaskGroup) {
      //! 处理当前topic下的所有asyncTask
      switch (asyncTask->task_->msgType_) {
        case MsgType::Tickers: {
          const auto tickers =
              static_cast<Tickers*>(asyncTask->task_->dataAfterConv_);
          sqlOfCurTopic += fmt::format("{}{}{}", tickers->getTDEngSqlPrefix(),
                                       tickers->getTDEngSqlTagsPart(),
                                       tickers->getTDEngSqlValuesPart());
          sqlOfCurTopicOfOrigData += fmt::format(
              "{}{}{}", tickers->getTDEngSqlRawPrefix(),
              tickers->getTDEngSqlRawTagsPart(mdSvc_->getApiName()),
              tickers->getTDEngSqlRawValuesPart(asyncTask->task_->data_,
                                                asyncTask->task_->dataLen_));
        } break;

        case MsgType::Trades: {
          const auto trades =
              static_cast<Trades*>(asyncTask->task_->dataAfterConv_);
          sqlOfCurTopic += fmt::format("{}{}{}", trades->getTDEngSqlPrefix(),
                                       trades->getTDEngSqlTagsPart(),
                                       trades->getTDEngSqlValuesPart());
          sqlOfCurTopicOfOrigData += fmt::format(
              "{}{}{}", trades->getTDEngSqlRawPrefix(),
              trades->getTDEngSqlRawTagsPart(mdSvc_->getApiName()),
              trades->getTDEngSqlRawValuesPart(asyncTask->task_->data_,
                                               asyncTask->task_->dataLen_));
        } break;

        case MsgType::Books: {
          const auto books =
              static_cast<Books*>(asyncTask->task_->dataAfterConv_);
          sqlOfCurTopic += fmt::format("{}{}{}", books->getTDEngSqlPrefix(),
                                       books->getTDEngSqlTagsPart(),
                                       books->getTDEngSqlValuesPart());
          sqlOfCurTopicOfOrigData += fmt::format(
              "{}{}{}", books->getTDEngSqlRawPrefix(),
              books->getTDEngSqlRawTagsPart(mdSvc_->getApiName()),
              books->getTDEngSqlRawValuesPart(asyncTask->task_->data_,
                                              asyncTask->task_->dataLen_));
        } break;

        case MsgType::Orders: {
          const auto orders =
              static_cast<Orders*>(asyncTask->task_->dataAfterConv_);
          sqlOfCurTopic += fmt::format("{}{}{}", orders->getTDEngSqlPrefix(),
                                       orders->getTDEngSqlTagsPart(),
                                       orders->getTDEngSqlValuesPart());
          sqlOfCurTopicOfOrigData += fmt::format(
              "{}{}{}", orders->getTDEngSqlRawPrefix(),
              orders->getTDEngSqlRawTagsPart(mdSvc_->getApiName()),
              orders->getTDEngSqlRawValuesPart(asyncTask->task_->data_,
                                               asyncTask->task_->dataLen_));
        } break;

        default:
          assert(1 == 2 && "Entered an impossible code segment");
          break;
      }
    }
    sql = sql + sqlOfCurTopic;
    sql = sql + sqlOfCurTopicOfOrigData;
  }

  if (sql.empty()) {
    return sql;
  }
  sql = fmt::format("INSERT INTO {};", sql);
  LOG_T("{}", sql);
  return sql;
}

}  // namespace bq::md::svc
