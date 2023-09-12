/*!
 * \file TDSvcOfCN.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "TDSvcOfCN.hpp"

#include "AssetsMgr.hpp"
#include "Config.hpp"
#include "OrdMgr.hpp"
#include "SHMIPC.hpp"
#include "SimedOrderInfoHandler.hpp"
#include "TDGateway.hpp"
#include "TDSrvTaskHandler.hpp"
#include "TDSvcDef.hpp"
#include "TDSvcUtil.hpp"
#include "db/DBE.hpp"
#include "db/TBLAssetInfo.hpp"
#include "db/TBLMonitor.hpp"
#include "db/TBLMonitorOfSymbolInfo.hpp"
#include "db/TBLOrderInfo.hpp"
#include "db/TBLPosInfo.hpp"
#include "db/TBLRecSetMaker.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/Def.hpp"
#include "def/SyncTask.hpp"
#include "util/ExceedFlowCtrlHandler.hpp"
#include "util/ExternalStatusCodeCache.hpp"
#include "util/FeeInfoCache.hpp"
#include "util/FlowCtrlSvc.hpp"
#include "util/Literal.hpp"
#include "util/MapRelOfCliIdAndOrdId.hpp"
#include "util/OpenedContractGroup.hpp"
#include "util/ScheduleTaskBundle.hpp"
#include "util/Scheduler.hpp"
#include "util/SignalHandler.hpp"
#include "util/String.hpp"
#include "util/TaskDispatcher.hpp"
#include "util/TrdSymbolCache.hpp"

namespace bq::td::svc {

int TDSvcOfCN::prepareInit() {
  syncTaskGroup_ = std::make_shared<SyncTaskGroup>();
  syncTaskGroup_->reserve(1024);

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

  acctId_ = CONFIG["acctId"].as<AcctId>();
  apiName_ = CONFIG["api"]["apiName"].as<std::string>();

  return 0;
}

int TDSvcOfCN::doInit() {
  //! appName_ = TD-XTP-INSTANCE-10001
  appName_ = fmt::format("{}-{}-INSTANCE-{}", TOPIC_PREFIX_OF_TRADE_DATA,
                         getApiName(), getAcctId());

  //! 初始化共享内存块，用于存放clientId和orderId的对应关系以及已开仓合约列表
  initSegmentData();

  if (const auto ret = initDBEng(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  initTBLMonitorOfSymbolInfo();
  initTrdSymbolCache();
  initExternalStatusCodeCache();

  //! 获取acctId下最大的noUsedToCalcPos，这个值是仓位和订单编号的对应关系，用于交易服务重建仓位
  if (const auto ret = queryMaxNoUsedToCalcPos(); ret != 0) {
    LOG_E("Do init failed.");
    return ret;
  }

  initAssetsMgr();
  initOrdMgr();

  //! 获取当前账户的手续费配置信息
  feeInfoCache_ = std::make_shared<FeeInfoCache>(getDBEng());
  feeInfoCache_->load(getAcctId());

  if (CONFIG["simedMode"]["enabled"].as<bool>(false) == true) {
    //! 模拟成交模式
    simedOrderInfoHandler_ = std::make_shared<SimedOrderInfoHandler>(this);
  } else {
    //! 实盘模式
    assert(tdGateway_ != nullptr && "tdGateway_ != nullptr");
    if (const auto statusCode = tdGateway_->init(); statusCode != 0) {
      LOG_E("Do init failed. ");
      return statusCode;
    }
  }

  //! 来自交易服务的消息处理模块
  tdSrvTaskHandler_ = std::make_shared<TDSrvTaskHandler>(this);
  //! 来自交易服务的消息分发模块
  initTDSrvTaskDispatcher();
  //! 连接交易服务的shm客户端
  initSHMCliOfTDSrv();

  //! 连接风控子系统的shm客户端
  initSHMCliOfRiskMgr();

  //! 流控管理服务
  flowCtrlSvc_ = std::make_shared<FlowCtrlSvc>(CONFIG);
  //! 创建超流控消息的处理模块，超流控消息在scheduleTaskBundle_被定时处理
  exceedFlowCtrlHandler_ =
      std::make_shared<ExceedFlowCtrlHandler>([this](auto& asyncTask) {
        getTDSrvTaskDispatcher()->dispatch(asyncTask);
      });

  //! 初始化定时任务处理模块处理模块
  scheduleTaskBundle_ = std::make_shared<ScheduleTaskBundle>();
  initScheduleTaskBundle();
  scheduleTaskBundleExecutor_ = std::make_shared<Scheduler>(
      appName_, [this]() { ExecScheduleTaskBundle(getScheduleTaskBundle()); },
      1 * 1000);

  return 0;
}

void TDSvcOfCN::initSegmentData() {
  //! 保存clientId和orderId对应关系
  const auto shmSegmentSizeOfMapRel =
      CONFIG["shmSegmentSizeOfMapRel"].as<std::size_t>(100 * 1024 * 1024);
  const auto identityOfMapRel = getIdentityOfMapRelOfCliIdAndOrdId();
  mapRelOfCliIdAndOrdId_ = std::make_shared<MapRelOfCliIdAndOrdId>();
  mapRelOfCliIdAndOrdId_->load(identityOfMapRel, shmSegmentSizeOfMapRel);

  //! 已开仓合约列表，具体的网关子类中计算合约平今仓手续费的时候用到，比如中金所
  //! 平今仓指的是你对于同一个期货合约，当日有先开仓再平仓操作，这就算平今仓。
  const auto shmSegmentSizeOfOpenedContracts =
      CONFIG["shmSegmentSizeOfOpenedContracts"].as<std::size_t>(0);
  if (shmSegmentSizeOfOpenedContracts != 0) {
    const auto identityOfOpened = getIdentityOfOpenContractGroup();
    openedContractGroup_ = std::make_shared<OpenedContractGroup>();
    openedContractGroup_->load<LockFunc::True>(identityOfOpened,
                                               shmSegmentSizeOfOpenedContracts);
  }
}

int TDSvcOfCN::initDBEng() {
  const auto dbEngParam = SetParam(db::DEFAULT_DB_ENG_PARAM,
                                   CONFIG["dbEngParam"].as<std::string>());
  int retOfMakeDBEng = 0;
  std::tie(retOfMakeDBEng, dbEng_) = db::MakeDBEng(
      dbEngParam, [this](db::DBTaskSPtr& dbTask, const StringSPtr& dbExecRet) {
        LOG_D("[{}] Exec sql finished. [{}] [exec result = {}]", appName_,
              dbTask->toStr(), *dbExecRet);
      });
  if (retOfMakeDBEng != 0) {
    LOG_E("[{}] Init dbeng failed. {}", appName_, dbEngParam);
    return retOfMakeDBEng;
  }

  if (auto retOfInit = getDBEng()->init(); retOfInit != 0) {
    LOG_E("[{}] Init dbeng failed. {}", appName_, dbEngParam);
    return retOfInit;
  }

  return 0;
}

void TDSvcOfCN::initTBLMonitorOfSymbolInfo() {
  const auto prefixOfSql =
      fmt::format("SELECT * from {}", TBLSymbolInfo::TableName);
  const auto condOfSql = CONFIG["tblMonitorOfSymbolInfo"].as<std::string>();
  std::string sql;
  if (condOfSql.empty()) {
    sql = fmt::format("{};", prefixOfSql);
  } else {
    sql = fmt::format("{} WHERE {};", prefixOfSql, condOfSql);
  }
  tblMonitorOfSymbolInfo_ = std::make_shared<db::TBLMonitorOfSymbolInfo>(
      getDBEng(), UINT32_MAX, sql, nullptr, db::EnableMonitoring::False);
}

void TDSvcOfCN::initTrdSymbolCache() {
  trdSymbolCache_ = std::make_shared<TrdSymbolCache>(getDBEng());
  trdSymbolCache_->load("", "", getAcctId());
}

void TDSvcOfCN::initExternalStatusCodeCache() {
  externalStatusCodeCache_ =
      std::make_shared<ExternalStatusCodeCache>(getDBEng());
  externalStatusCodeCache_->load("", 0);
}

//! maxNoUsedToCalcPos_用于重建PosMgr
int TDSvcOfCN::queryMaxNoUsedToCalcPos() {
  const auto sql = fmt::format(
      "SELECT * FROM {} WHERE acctId = {} "
      "ORDER BY noUsedToCalcPos DESC LIMIT 1;",
      TBLOrderInfo::TableName, getAcctId());
  const auto [ret, tblRecSet] =
      db::TBLRecSetMaker<TBLOrderInfo>::ExecSql(getDBEng(), sql);
  if (ret != 0) {
    LOG_W("Query maxNoUsedToCalcPos from db failed. {}", sql);
    return ret;
  }

  if (!tblRecSet->empty()) {
    const auto& tblRec = std::begin(*tblRecSet);
    const auto& orderInfo = tblRec->second;
    maxNoUsedToCalcPos_ = orderInfo->getRecWithAllFields()->noUsedToCalcPos;
  }
  LOG_I("Query maxNoUsedToCalcPos success. [maxNoUsedToCalcPos = {}]",
        maxNoUsedToCalcPos_);

  return 0;
}

void TDSvcOfCN::initAssetsMgr() {
  assetsMgr_ = std::make_shared<AssetsMgr>();
  const auto sql = fmt::format("SELECT * FROM {} WHERE `acctId` = {};",
                               TBLAssetInfo::TableName, getAcctId());
  getAssetsMgr()->init(CONFIG, getDBEng(), sql);
}

void TDSvcOfCN::initOrdMgr() {
  const auto filled = magic_enum::enum_integer(OrderStatus::Filled);
  ordMgr_ = std::make_shared<TDOrdMgr>();
  const auto sql = fmt::format(
      "SELECT * FROM {} WHERE `orderStatus` < {} AND `acctId` = {}; ",
      TBLOrderInfo::TableName, filled, getAcctId());
  getOrdMgr()->init(CONFIG, getDBEng(), sql);
}

//! 初始化TDSrvTaskDispatcher
int TDSvcOfCN::initTDSrvTaskDispatcher() {
  const auto tdSrvTaskDispatcherParamInStrFmt =
      SetParam(DEFAULT_TASK_DISPATCHER_PARAM,
               CONFIG["tdSrvTaskDispatcherParam"].as<std::string>());
  const auto [ret, tdSrvTaskDispatcherParam] =
      MakeTaskDispatcherParam(tdSrvTaskDispatcherParamInStrFmt);
  if (ret != 0) {
    LOG_E("[{}] Init taskdispatcher failed. {}", appName_,
          tdSrvTaskDispatcherParamInStrFmt);
    return ret;
  }

  const auto getThreadForAsyncTask = [](const auto& asyncTask,
                                        auto taskSpecificThreadPoolSize) {
    const auto shmHeader =
        static_cast<const SHMHeader*>(asyncTask->task_->data_);
    switch (shmHeader->msgId_) {
      //! 来自TDSrv的下单和撤单消息均由0号线程处理
      case MSG_ID_ON_ORDER:
      case MSG_ID_ON_CANCEL_ORDER:
        return ThreadNo(0);
      //! 其他消息由1号线程处理
      default:
        return ThreadNo(1);
    }
  };

  //! 使用tdSrvTaskHandler_处理来自TDSrv的消息
  const auto handleAsyncTask = [this](auto& asyncTask) {
    tdSrvTaskHandler_->handleAsyncTask(asyncTask);
  };

  tdSrvTaskDispatcher_ = std::make_shared<TaskDispatcher<SHMIPCTaskSPtr>>(
      tdSrvTaskDispatcherParam, nullptr, getThreadForAsyncTask,
      handleAsyncTask);
  tdSrvTaskDispatcher_->init();

  return ret;
}

void TDSvcOfCN::initSHMCliOfTDSrv() {
  //! channel = "TD@TDGWChannel@Trade"
  const auto tdSrvChannel = CONFIG["tdSrvChannel"].as<std::string>();

  //! addr = "TD-XTP-INSTANCE-10001@TD@TDGWChannel@Trade"
  const auto addr =
      fmt::format("{}{}{}", appName_, SEP_OF_SHM_SVC, tdSrvChannel);

  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    const auto task = std::make_shared<SHMIPCTask>(shmBuf, shmBufLen);
    auto asyncTask = std::make_shared<SHMIPCAsyncTask>(task);
    tdSrvTaskDispatcher_->dispatch(asyncTask);
  };

  shmCliOfTDSrv_ = std::make_shared<SHMCli>(addr, onSHMDataRecv);
  shmCliOfTDSrv_->setClientChannel(acctId_);
}

void TDSvcOfCN::initSHMCliOfRiskMgr() {
  //! "RISK@TDGWChannel@Trade"
  const auto riskMgrChannel = CONFIG["riskMgrChannel"].as<std::string>();

  //! addr = "TD-XTP-INSTANCE-10001@RISK@TDGWChannel@Trade"
  const auto addr =
      fmt::format("{}{}{}", appName_, SEP_OF_SHM_SVC, riskMgrChannel);

  const auto onSHMDataRecv = [this](const void* shmBuf, std::size_t shmBufLen) {
    //! 暂时不处理来自riskMgr的消息
    const auto shmHeader = static_cast<const SHMHeader*>(shmBuf);
    LOG_I("Recv {} - {} from risk mgr.", shmHeader->msgId_,
          GetMsgName(shmHeader->msgId_));
  };

  shmCliOfRiskMgr_ = std::make_shared<SHMCli>(addr, onSHMDataRecv);
  shmCliOfRiskMgr_->setClientChannel(acctId_);
}

void TDSvcOfCN::beforeInitScheduleTaskBundle() {
  //! 以下都把请求丢给SHMCli的请求处理回调
  if (CONFIG["simedMode"]["enabled"].as<bool>(false) == false) {
    //! 定时查询资产
    const auto secIntervalOfSyncAssetsSnapshot =
        CONFIG["secIntervalOfSyncAssetsSnapshot"].as<std::uint32_t>();
    getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
        "syncAssetsSnapshot",
        [this]() {
          auto asyncTask = MakeTDSrvSignal(MSG_ID_SYNC_ASSETS_SNAPSHOT);
          getTDSrvTaskDispatcher()->dispatch(asyncTask);
          return true;
        },
        ExecAtStartup::True, secIntervalOfSyncAssetsSnapshot * 1000));

    //! 定时查询未完结订单
    const auto thresholdOfQryAllOrder =
        CONFIG["thresholdOfQryAllOrder"].as<std::uint32_t>(UINT32_MAX);

    //! 需要同步的是secAgoTheOrderNeedToBeSynced之前的订单，刚报出的订单等待回报即可，无需同步
    const auto secAgoTheOrderNeedToBeSynced =
        CONFIG["secAgoTheOrderNeedToBeSynced"].as<std::uint32_t>();

    const auto secIntervalOfSyncUnclosedOrderInfo =
        CONFIG["secIntervalOfSyncUnclosedOrderInfo"].as<std::uint32_t>();

    getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
        "syncUnclosedOrderInfo",
        [this, thresholdOfQryAllOrder, secAgoTheOrderNeedToBeSynced]() {
          //! 从OrdMgr获取所有未完结订单信息
          const auto orderInfoGroup =
              getOrdMgr()->getOrderInfoGroup<LockFunc::True, DeepClone::True>(
                  secAgoTheOrderNeedToBeSynced);
          if (orderInfoGroup.size() > thresholdOfQryAllOrder) {
            //! 如果未完结订单数量大于thresholdOfQryAllOrder，那么全量查询
            auto asyncTask = MakeTDSrvSignal(  //
                MSG_ID_SYNC_UNCLOSED_ORDER_INFO, std::any());
            getTDSrvTaskDispatcher()->dispatch(asyncTask);
          } else {
            //! 如果未完结订单数量小于thresholdOfQryAllOrder，那么逐个查询
            for (const auto& orderInfo : orderInfoGroup) {
              auto asyncTask = MakeTDSrvSignal(  //
                  MSG_ID_SYNC_UNCLOSED_ORDER_INFO, orderInfo);
              getTDSrvTaskDispatcher()->dispatch(asyncTask);
            }
          }
          return true;
        },
        ExecAtStartup::True, secIntervalOfSyncUnclosedOrderInfo * 1000));
  }

  //! 定时处理超流控消息
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "exceedFlowCtrlHandler",
      [this]() {
        exceedFlowCtrlHandler_->handleExceedFlowCtrlTask();
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(1000), UINT64_MAX,
      WriteLog::False));

  //! 定时重新加载ExternalStatusCode
  const auto secIntervalOfReloadExternalStatusCode =
      CONFIG["secIntervalOfReloadExternalStatusCode"].as<std::uint32_t>();
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "reloadExternalStatusCode",
      [this]() {
        externalStatusCodeCache_->reload("", 0);
        return true;
      },
      ExecAtStartup::False, secIntervalOfReloadExternalStatusCode * 1000));

  //! 定时发送TDGWReg，保持和TDSrv的连接活跃
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "sendTDGWReg",
      [this]() {
        sendTDGWReg();
        return true;
      },
      ExecAtStartup::False, MilliSecInterval(5000)));

  //! 定时同步任务到数据库和riskMgr
  const auto milliSecIntervalOfSyncTask =
      CONFIG["milliSecIntervalOfSyncTask"].as<std::uint32_t>();
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "syncTask",
      [this]() {
        handleSyncTaskGroup();
        return true;
      },
      ExecAtStartup::False, milliSecIntervalOfSyncTask));

  //! 定时重新加载手续费信息
  const auto secIntervalOfReloadFeeInfo =
      CONFIG["secIntervalOfReloadFeeInfo"].as<std::uint32_t>();
  getScheduleTaskBundle()->emplace_back(std::make_shared<ScheduleTask>(
      "reloadFeeInfo",
      [this]() {
        feeInfoCache_->reload(getAcctId());
        return true;
      },
      ExecAtStartup::False, secIntervalOfReloadFeeInfo * 1000));
}

int TDSvcOfCN::doRun() {
  getDBEng()->start();

  if (const auto ret = tblMonitorOfSymbolInfo_->start(); ret != 0) {
    LOG_E("Run failed.");
    return ret;
  }

  if (CONFIG["simedMode"]["enabled"].as<bool>(false) == false) {
    if (const auto statusCode = tdGateway_->start(); statusCode != 0) {
      LOG_E("Run failed. ");
      return statusCode;
    }
  }

  tdSrvTaskDispatcher_->start();
  shmCliOfTDSrv_->start();

  shmCliOfRiskMgr_->start();

  sendTDGWReg();

  if (const auto ret = scheduleTaskBundleExecutor_->start(); ret != 0) {
    LOG_E("Start scheduler of multi task failed.");
    return ret;
  }

  return 0;
}

void TDSvcOfCN::sendTDGWReg() {
  shmCliOfTDSrv_->asyncSendReqWithZeroCopy(
      [&](void* shmBufOfReq) {
        auto tdGWReg = static_cast<TDGWReg*>(shmBufOfReq);
      },
      MSG_ID_ON_TDGW_REG, sizeof(TDGWReg));

  shmCliOfRiskMgr_->asyncSendReqWithZeroCopy(
      [&](void* shmBufOfReq) {
        auto tdGWReg = static_cast<TDGWReg*>(shmBufOfReq);
      },
      MSG_ID_ON_TDGW_REG, sizeof(TDGWReg));
}

void TDSvcOfCN::cacheSyncTaskGroup(MsgId msgId, const std::any& task,
                                   SyncToRiskMgr syncToRiskMgr,
                                   SyncToDB syncToDB) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    syncTaskGroup_->emplace_back(
        std::make_shared<SyncTask>(msgId, task, syncToRiskMgr, syncToDB));
  }
}

void TDSvcOfCN::handleSyncTaskGroup() {
  auto syncTaskGroup = std::make_shared<SyncTaskGroup>();
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxSyncTaskGroup_);
    syncTaskGroup.swap(syncTaskGroup_);
  }

  if (syncTaskGroup->size() > 100) {
    LOG_W("Too many unprocessed task of sync. [num = {}]",
          syncTaskGroup->size());
  }

  //! 同步任务到riskMgr
  for (const auto& rec : *syncTaskGroup) {
    if (rec->syncToRiskMgr_ == SyncToRiskMgr::False) continue;

    if (rec->msgId_ == MSG_ID_ON_ORDER ||  //
        rec->msgId_ == MSG_ID_ON_ORDER_RET ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      const auto orderInfo = std::any_cast<OrderInfoSPtr>(rec->task_);
      shmCliOfRiskMgr_->asyncSendMsgWithZeroCopy(
          [&](void* shmBuf) {
            InitMsgBodyExt(shmBuf, *orderInfo);
            LOG_D("Send order info to risk mgr. {}",
                  static_cast<OrderInfo*>(shmBuf)->toShortStr());
          },
          rec->msgId_, orderInfo->size());

    } else if (rec->msgId_ == MSG_ID_SYNC_ASSETS) {
      const auto updateInfoOfAssetGroup =
          std::any_cast<UpdateInfoOfAssetGroupSPtr>(rec->task_);
      NotifyAssetInfo(shmCliOfRiskMgr_, getAcctId(), updateInfoOfAssetGroup);

    } else {
      LOG_W("Unhandled task of sync to riskmgr. {} - {}", rec->msgId_,
            GetMsgName(rec->msgId_))
    }
  }

  //! 同步任务到数据库
  for (const auto& rec : *syncTaskGroup) {
    if (rec->syncToDB_ == SyncToDB::False) continue;

    if (rec->msgId_ == MSG_ID_ON_ORDER ||  //
        rec->msgId_ == MSG_ID_ON_ORDER_RET ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER ||
        rec->msgId_ == MSG_ID_ON_CANCEL_ORDER_RET) {
      const auto orderInfo = std::any_cast<OrderInfoSPtr>(rec->task_);
      const auto identity = GET_RAND_STR();
      const auto sql = orderInfo->getSqlOfUSPOrderInfoUpdate();
      const auto [ret, execRet] = getDBEng()->asyncExec(identity, sql);
      if (ret != 0) {
        LOG_W("Sync order info to db failed. [{}]", sql);
      }

    } else if (rec->msgId_ == MSG_ID_SYNC_ASSETS) {
      const auto updateInfoOfAssetGroup =
          std::any_cast<UpdateInfoOfAssetGroupSPtr>(rec->task_);

      for (const auto& assetInfo :
           *updateInfoOfAssetGroup->assetInfoGroupAdd_) {
        const auto identity = GET_RAND_STR();
        const auto sql = assetInfo->getSqlOfInsert();
        const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
        if (ret != 0) {
          LOG_W("Insert asset info to db failed. [{}]", sql);
        }
      }

      for (const auto& assetInfo :
           *updateInfoOfAssetGroup->assetInfoGroupDel_) {
        const auto identity = GET_RAND_STR();
        const auto sql = assetInfo->getSqlOfDelete();
        const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
        if (ret != 0) {
          LOG_W("Del asset info from db failed. [{}]", sql);
        }
      }

      for (const auto& assetInfo :
           *updateInfoOfAssetGroup->assetInfoGroupChg_) {
        const auto identity = GET_RAND_STR();
        const auto sql = assetInfo->getSqlOfUpdate();
        const auto [ret, execRet] = dbEng_->asyncExec(identity, sql);
        if (ret != 0) {
          LOG_W("Update asset info from db failed. [{}]", sql);
        }
      }

    } else {
      LOG_W("Unhandled task of sync to db. {} - {}", rec->msgId_,
            GetMsgName(rec->msgId_))
    }
  }
}

void TDSvcOfCN::doExit(const boost::system::error_code* ec, int signalNum) {
  scheduleTaskBundleExecutor_->stop();
  getAssetsMgr()->syncUpdateInfoOfAssetGroupToDB();
  shmCliOfRiskMgr_->stop();
  shmCliOfTDSrv_->stop();
  tdSrvTaskDispatcher_->stop();
  if (CONFIG["simedMode"]["enabled"].as<bool>(false) == false) {
    tdGateway_->stop();
  }
  tblMonitorOfSymbolInfo_->stop();
  getDBEng()->stop();
}

std::string TDSvcOfCN::getIdentityOfMapRelOfCliIdAndOrdId() const {
  const auto ret = fmt::format("{}-MAP-REL.dat", appName_);
  return ret;
}

std::string TDSvcOfCN::getIdentityOfOpenContractGroup() const {
  const auto ret = fmt::format("{}-OPENED-CONTRACTS.dat", appName_);
  return ret;
}

}  // namespace bq::td::svc
