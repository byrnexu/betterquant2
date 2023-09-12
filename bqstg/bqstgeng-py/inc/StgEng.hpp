/*!
 * \file PosSnapshotImpl.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/11/09
 *
 * \brief
 */

#pragma once

#include <boost/python.hpp>

#include "BQPub.hpp"
#include "Pub.hpp"
#include "SHMIPCPub.hpp"
#include "util/Pch.hpp"

namespace bq {
struct Pnl;
using PnlSPtr = std::shared_ptr<Pnl>;
struct SimedTDInfo;
using SimedTDInfoSPtr = std::shared_ptr<SimedTDInfo>;

struct PosOfStgInst;
using PosOfStgInstSPtr = std::shared_ptr<PosOfStgInst>;

using PosOfStgInstGroup = std::vector<PosOfStgInstSPtr>;
using PosOfStgInstGroupSPtr = std::shared_ptr<PosOfStgInstGroup>;
}  // namespace bq

namespace bq::stg {

class StgEngImpl;
using StgEngImplSPtr = std::shared_ptr<StgEngImpl>;

class StgEng;
using StgEngSPtr = std::shared_ptr<StgEng>;

class StgInstTaskHandlerBase;
using StgInstTaskHandlerBaseSPtr = std::shared_ptr<StgInstTaskHandlerBase>;

class StgEng {
 public:
  StgEng(const StgEng&) = delete;
  StgEng& operator=(const StgEng&) = delete;
  StgEng(const StgEng&&) = delete;
  StgEng& operator=(const StgEng&&) = delete;

  explicit StgEng(const std::string& configFilename);

 public:
  StgInstInfoSPtr getDftStgInstInfo() const;

 public:
  int init(PyObject* stgInstTaskHandler);

 private:
  void installStgInstTaskHandler(PyObject* value);

 public:
  int run();

 public:
  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           const std::string& execTime,
                           std::uint32_t timeZone = 8);

  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           ExecAtStartup execAtStartUp,
                           std::uint32_t milliSecInterval,
                           std::uint64_t maxExecTimes = UINT64_MAX);

  void uninstallStgInstTimer(StgInstId stgInstId, const std::string& timerName);

 public:
  std::vector<OrderInfoSPtr> getUnclosedOrderInfoGroup(
      const StgInstInfoSPtr& stgInstInfo) const;

  PosOfStgInstGroupSPtr getPosOfStgInst(
      const StgInstInfoSPtr& stgInstInfo) const;

 public:
  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, PosDirection posDirection,
      Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, Decimal orderPrice,
      Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  std::tuple<int, OrderId> order(OrderInfoSPtr& orderInfo);

  int cancelOrder(OrderId orderId);

  std::vector<OrderId> cancelAllOrderOfStg();
  std::vector<OrderId> cancelAllOrderOfStgInst(
      const StgInstInfoSPtr& stgInstInfo);
  std::vector<OrderId> cancelAllOrderOfStgInst(StgInstId stgInstId);
  std::vector<OrderId> cancelAllOrderOfAlgo(AlgoId algoId);

  std::tuple<int, AlgoId> algoOrder(const StgInstInfoSPtr& stgInstInfo,
                                    const std::string& algoType,
                                    const std::string& algoName,
                                    std::uint32_t lifetime,
                                    const std::string& algoParamsInJsonFmt);
  int cancelAlgoOrder(AlgoId algoId);
  std::string getProgressOfAlgoOrder(AlgoId algoId);

  std::tuple<int, OrderInfoSPtr> getOrderInfo(OrderId orderId) const;

  int sub(StgInstId subscriber, const std::string& topic);
  int unSub(StgInstId subscriber, const std::string& topic);

 public:
  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext = "");

  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t tsBegin, std::uint64_t tsEnd);

  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

 public:
  bool saveStgPrivateData(StgInstId stgInstId, const std::string& jsonStr);
  std::string loadStgPrivateData(StgInstId stgInstId);

  std::tuple<int, std::string> syncExecSql(const std::string& sql);
  void saveToDB(const PnlSPtr& pnl);

 public:
  void logTrace(const std::string& fmt, const boost::python::list& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logDebug(const std::string& fmt, const boost::python::list& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logInfo(const std::string& fmt, const boost::python::list& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logWarn(const std::string& fmt, const boost::python::list& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logError(const std::string& fmt, const boost::python::list& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logCritical(const std::string& fmt, const boost::python::list& args,
                   const StgInstInfoSPtr& stgInstInfo = nullptr,
                   NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logTrace(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logDebug(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logInfo(const std::string& fmt,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logWarn(const std::string& fmt,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logError(const std::string& fmt,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logCritical(const std::string& fmt,
                   const StgInstInfoSPtr& stgInstInfo = nullptr,
                   NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

 private:
  void logPYErr(const StgInstInfoSPtr& stgInstInfo, const std::string& pyerr);

 private:
  std::vector<std::string> getArgGroup(const boost::python::list& args);

 private:
  StgEngImplSPtr stgEngImpl_{nullptr};

  PyObject* stgInstTaskHandler_;
  mutable std::mutex mtxPY_;

  absl::node_hash_map<StgInstId, std::uint32_t> stgInstId2RealDepthLevel_;
  mutable std::mutex mtxStgInstId2RealDepthLevel_;
};

}  // namespace bq::stg
