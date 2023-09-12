/*!
 * \file MDSvcOfCN.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/12/20
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "util/Pch.hpp"
#include "util/SvcBase.hpp"

namespace bq {
class TopicMgr;
using TopicMgrSPtr = std::shared_ptr<TopicMgr>;

class SHMSrv;
using SHMSrvSPtr = std::shared_ptr<SHMSrv>;

using MarketCode2SHMSrvGroup = std::map<MarketCode, SHMSrvSPtr>;
using MarketCode2SHMSrvGroupSPtr = std::shared_ptr<MarketCode2SHMSrvGroup>;

struct MarketDataCond;
using MarketDataCondSPtr = std::shared_ptr<MarketDataCond>;
using MarketDataCondGroup = std::vector<MarketDataCondSPtr>;

class SymbolInfoTable;
using SymbolInfoTableSPtr = std::shared_ptr<SymbolInfoTable>;
}  // namespace bq

namespace bq::db {
class DBEng;
using DBEngSPtr = std::shared_ptr<DBEng>;
}  // namespace bq::db

namespace bq::db::symbolInfo {
struct FieldGroupOfAll;
using Record = FieldGroupOfAll;
using RecordSPtr = std::shared_ptr<Record>;
}  // namespace bq::db::symbolInfo

namespace bq::tdeng {
class TDEngConnpool;
using TDEngConnpoolSPtr = std::shared_ptr<TDEngConnpool>;
}  // namespace bq::tdeng

namespace bq::md::svc {

class SHMSrvMsgHandler;
using SHMSrvMsgHandlerSPtr = std::shared_ptr<SHMSrvMsgHandler>;

class RawMDHandler;
using RawMDHandlerSPtr = std::shared_ptr<RawMDHandler>;

class MDStorageSvc;
using MDStorageSvcSPtr = std::shared_ptr<MDStorageSvc>;

class MDCache;
using MDCacheSPtr = std::shared_ptr<MDCache>;

class MDPlayback;
using MDPlaybackSPtr = std::shared_ptr<MDPlayback>;

class MDSvcOfCN : public SvcBase {
 public:
  MDSvcOfCN(const MDSvcOfCN&) = delete;
  MDSvcOfCN& operator=(const MDSvcOfCN&) = delete;
  MDSvcOfCN(const MDSvcOfCN&&) = delete;
  MDSvcOfCN& operator=(const MDSvcOfCN&&) = delete;

  using SvcBase::SvcBase;
  ~MDSvcOfCN();

 private:
  int prepareInit() final;
  int doInit() final;

 private:
  int initDBEng();
  int initTDEng();
  void initSHMSrvGroup();

  void initTopicMgr();
  void handleTopicNeedSubAndUnSub(
      const std::tuple<TopicGroup, TopicGroup>& topicGroupNeedSubAndUnsub);
  virtual void doSub(const MarketDataCondGroup& marketDataCond) = 0;
  virtual void doUnSub(const MarketDataCondGroup& marketDataCond) = 0;

 protected:
  int beforeRun() final;

 private:
  virtual int startGateway() = 0;
  int startGatewayOfSim();

 protected:
  int afterRun() final;

 private:
  void beforeExit(const boost::system::error_code* ec, int signalNum) final;

 private:
  virtual void stopGateway() {}
  void stopGatewayOfSim();

 private:
  void doExit(const boost::system::error_code* ec, int signalNum) final;

 protected:
  void setRawMDHandler(const RawMDHandlerSPtr& value) { rawMDHandler_ = value; }

 public:
  void setTradingDay(const std::string& value);

 public:
  db::DBEngSPtr getDBEng() const { return dbEng_; }
  tdeng::TDEngConnpoolSPtr getTDEngConnpool() const { return tdEngConnpool_; }

  SymbolInfoTableSPtr getSymbolInfoTable() const { return symbolInfoTable_; }

  SHMSrvSPtr getSHMSrv(MarketCode marketCode) const;

  RawMDHandlerSPtr getRawMDHandler() const { return rawMDHandler_; }
  MDStorageSvcSPtr getMDStorageSvc() const { return mdStorageSvc_; }

  TopicMgrSPtr getTopicMgr() const { return topicMgr_; }
  MDCacheSPtr getMDCache() const { return mdCache_; }

  std::string getTradingDay() const { return tradingDay_; }
  std::string getApiName() const { return apiName_; }

 private:
  db::DBEngSPtr dbEng_;
  tdeng::TDEngConnpoolSPtr tdEngConnpool_{nullptr};

  SymbolInfoTableSPtr symbolInfoTable_{nullptr};

  SHMSrvMsgHandlerSPtr shmSrvMsgHandler_{nullptr};
  MarketCode2SHMSrvGroupSPtr marketCode2SHMSrvGroup_{nullptr};

  RawMDHandlerSPtr rawMDHandler_{nullptr};
  MDStorageSvcSPtr mdStorageSvc_{nullptr};

  TopicMgrSPtr topicMgr_{nullptr};

  MDCacheSPtr mdCache_{nullptr};
  MDPlaybackSPtr mdPlayback_{nullptr};

  std::string tradingDay_;
  std::string apiName_;
};

}  // namespace bq::md::svc
