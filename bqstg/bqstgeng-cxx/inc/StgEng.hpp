/*!
 * \file StgEng.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "BQPub.hpp"
#include "Pub.hpp"
#include "SHMIPCPub.hpp"

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

  /**
   * @Synopsis
   *
   * @Param configFilename 配置文件
   */
  explicit StgEng(const std::string& configFilename);

 public:
  /**
   * @Synopsis
   *
   * @Returns
   */
  StgInstInfoSPtr getDftStgInstInfo() const;

 public:
  /**
   * @Synopsis
   *
   * @Param taskHandler 处理各种回调比如委托回报的函数
   *
   * @Returns
   */
  int init(const StgInstTaskHandlerBaseSPtr& taskHandler);

  /**
   * @Synopsis 开始运行策略引擎
   *
   * @Returns statusCode (0：成功；其他：失败)
   */
  int run();

 private:
  void installStgInstTaskHandler(const StgInstTaskHandlerBaseSPtr& taskHandler);

 public:
  /**
   * @Synopsis
   *
   * @Param stgInstInfo  子策略信息，包含了子策略id、子策略参数等信息
   * @Param acctId       内部账号
   * @Param trdAcctId    交易账号
   * @Param marketCode   市场
   * @Param symbolCode   代码
   * @Param side         买卖
   * @Param posDirection 开平
   * @Param orderPrice   下单价格
   * @Param orderSize    下单数量
   * @Param closeTDayStg     下单策略，比如是否允许平今、拒绝平今等
   * @Param algoId       算法单id
   * @Param simedTDInfo  指定模拟成交的方式，用于回测，实盘传入 nullptr
   *
   * @Returns statusCode (0：成功；其他：失败) 和 orderId
   */
  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, PosDirection posDirection,
      Decimal orderPrice, Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  /**
   * @Synopsis
   *
   * @Param stgInstInfo  子策略信息，包含了子策略id、子策略参数等信息
   * @Param acctId       内部账号
   * @Param trdAcctId    交易账号
   * @Param marketCode   市场
   * @Param symbolCode   代码
   * @Param side         买卖
   * @Param orderPrice   下单价格
   * @Param orderSize    下单数量
   * @Param closeTDayStg     下单策略，比如是否允许平今、拒绝平今等
   * @Param algoId       算法单id
   * @Param simedTDInfo  指定模拟成交的方式，用于回测，实盘传入 nullptr
   *
   * @Returns statusCode (0：成功；其他：失败) 和 orderId
   */
  std::tuple<int, OrderId> order(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      const std::string& symbolCode, Side side, Decimal orderPrice,
      Decimal orderSize, TrdAcctId trdAcctId,
      CloseTDayStg closeTDayStg = CloseTDayStg::RejectCloseTDay,
      AlgoId algoId = 0, const SimedTDInfoSPtr& simedTDInfo = nullptr);

  /**
   * @Synopsis
   *
   * @Param orderInfo 订单信息
   *
   * @Returns statusCode (0：成功；其他：失败) 和 orderId
   */
  std::tuple<int, OrderId> order(OrderInfoSPtr& orderInfo);

  /**
   * @Synopsis
   *
   * @Param orderId 需要撤单的orderId
   *
   * @Returns statusCode (0：成功；其他：失败)
   */
  int cancelOrder(OrderId orderId);

  /**
   * @Synopsis
   *
   * @Returns
   */
  std::vector<OrderId> cancelAllOrderOfStg();

  /**
   * @Synopsis
   *
   * @Param stgInstInfo 子策略信息
   *
   * @Returns
   */
  std::vector<OrderId> cancelAllOrderOfStgInst(
      const StgInstInfoSPtr& stgInstInfo);

  /**
   * @Synopsis
   *
   * @Param stgInstId 子策略id
   *
   * @Returns
   */
  std::vector<OrderId> cancelAllOrderOfStgInst(StgInstId stgInstId);

  /**
   * @Synopsis
   *
   * @Param algoId
   *
   * @Returns
   */
  std::vector<OrderId> cancelAllOrderOfAlgo(AlgoId algoId);

 public:
  /**
   * @Synopsis
   *
   * @Param stgInstInfo
   * @Param algoType
   * @Param algoName
   * @Param algoParamsInJsonFmt
   *
   * @Returns
   */
  std::tuple<int, AlgoId> algoOrder(const StgInstInfoSPtr& stgInstInfo,
                                    const std::string& algoType,
                                    const std::string& algoName,
                                    std::uint32_t lifetime,
                                    const std::string& algoParamsInJsonFmt);

  /**
   * @Synopsis
   *
   * @Param algoId
   *
   * @Returns
   */
  int cancelAlgoOrder(AlgoId algoId);

  /**
   * @Synopsis
   *
   * @Param algoId
   *
   * @Returns
   */
  std::string getProgressOfAlgoOrder(AlgoId algoId);

 public:
  /**
   * @Synopsis 获取缓存的ticker集合对象
   *
   * @Returns
   */
  MarketDataCacheSPtr getMarketDataCache() const;

  /**
   * @Synopsis
   *
   * @Param orderId 获取订单信息
   *
   * @Returns statusCode (0：成功；其他：失败) and 订单信息
   */
  std::tuple<int, OrderInfoSPtr> getOrderInfo(OrderId orderId) const;

 public:
  /**
   * @Synopsis
   *
   * @Param subscriber 订阅的topic的策略实例id
   * @Param topic      订阅的topic，如：shm://MD.SZSE.Spot/000002/Trades
   *
   * @Returns statusCode (0：成功；其他：失败)
   */
  int sub(StgInstId subscriber, const std::string& topic);

  /**
   * @Synopsis
   *
   * @Param subscriber 订阅的topic的策略实例id
   * @Param topic      订阅的topic，如：shm://MD.SZSE.Spot/000002/Trades
   *
   * @Returns statusCode (0：成功；其他：失败)
   */
  int unSub(StgInstId subscriber, const std::string& topic);

 public:
  /**
   * @Synopsis 查询 [tsBegin, tsEnd) 区间内的历史行情
   *
   * @Param topic   需要查询的历史行情的topic，如：MD@SZSE@Spot@000001@Orders
   * @Param tsBegin 需要查询的历史行情的起始时间
   * @Param tsEnd   需要查询的历史行情的结束时间
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t tsBegin, std::uint64_t tsEnd);

  /**
   * @Synopsis 查询 [tsBegin, tsEnd) 区间内的历史行情
   *
   * @Param marketCode 市场
   * @Param symbolType 代码类型
   * @Param symbolCode 代码
   * @Param mdType     行情类型，比如Tickers
   * @Param tsBegin    需要查询的历史行情的起始时间
   * @Param tsEnd      需要查询的历史行情的结束时间
   * @Param ext
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> queryHisMDBetween2Ts(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t tsBegin, std::uint64_t tsEnd, const std::string& ext = "");

  /**
   * @Synopsis 从 ts 开始往前查询 num 条记录
   *
   * @Param topic 需要查询的历史行情的topic，如：MD@SZSE@Spot@000001@Orders
   * @Param ts    往前查询历史行情的起始时间点
   * @Param num   需要查询的历史行情的记录数
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

  /**
   * @Synopsis
   *
   * @Param marketCode 市场
   * @Param symbolType 代码类型
   * @Param symbolCode 代码
   * @Param mdType     行情类型，比如Tickers
   * @Param ts         往前查询历史行情的起始时间点
   * @Param num        需要查询的历史行情的记录数
   * @Param ext
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> querySpecificNumOfHisMDBeforeTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

  /**
   * @Synopsis 从 ts 开始往后查询 num 条记录
   *
   * @Param topic 需要查询的历史行情的topic，如：MD@SZSE@Spot@000001@Orders
   * @Param ts    往后查询历史行情的起始时间点
   * @Param num   需要查询的历史行情的记录数
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, const std::string& topic,
      std::uint64_t ts, int num);

  /**
   * @Synopsis
   *
   * @Param marketCode 市场
   * @Param symbolType 代码类型
   * @Param symbolCode 代码
   * @Param mdType     行情类型，比如Tickers
   * @Param ts         往后查询历史行情的起始时间点
   * @Param num        需要查询的历史行情的记录数
   * @Param ext
   *
   * @Returns statusCode (0：成功；其他：失败) 和 json格式的数据
   */
  std::tuple<int, std::string> querySpecificNumOfHisMDAfterTs(
      const StgInstInfoSPtr& stgInstInfo, MarketCode marketCode,
      SymbolType symbolType, const std::string& symbolCode, MDType mdType,
      std::uint64_t ts, int num, const std::string& ext = "");

 public:
  /**
   * @Synopsis 给子策略安装一个定时器
   *
   * @Param stgInstId        子策略id
   * @Param timerName        定时器名称
   * @Param extcTime         定时器是否启动后立即执行
   * @Param timeZone         定时器按照什么时区的时间执行
   */
  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           const std::string& execTime,
                           std::uint32_t timeZone = 8);

  /**
   * @Synopsis 给子策略安装一个定时器
   *
   * @Param stgInstId        子策略id
   * @Param timerName        定时器名称
   * @Param execAtStartUp    定时器是否启动后立即执行
   * @Param milliSecInterval 定时器执行间隔
   * @Param maxExecTimes     定时器执行次数
   */
  void installStgInstTimer(StgInstId stgInstId, const std::string& timerName,
                           ExecAtStartup execAtStartUp,
                           std::uint32_t milliSecInterval,
                           std::uint64_t maxExecTimes = UINT64_MAX);

  /**
   * @Synopsis
   *
   * @Param stgInstId 子策略id
   * @Param timerName 定时器名称
   */
  void uninstallStgInstTimer(StgInstId stgInstId, const std::string& timerName);

 public:
  /**
   * @Synopsis
   *
   * @Param stgInstInfo 子策略信息
   *
   * @Returns
   */
  std::vector<OrderInfoSPtr> getUnclosedOrderInfoGroup(
      const StgInstInfoSPtr& stgInstInfo) const;

  /**
   * @Synopsis
   *
   * @Param stgInstInfo 子策略信息
   *
   * @Returns
   */
  PosOfStgInstGroupSPtr getPosOfStgInst(
      const StgInstInfoSPtr& stgInstInfo) const;

 public:
  /**
   * @Synopsis 子策略允许中的一些中间数据的保存
   *
   * @Param stgInstId 子策略id
   * @Param jsonStr   需要保存的数据
   *
   * @Returns 成功或者失败
   */
  bool saveStgPrivateData(StgInstId stgInstId, const std::string& jsonStr);

  /**
   * @Synopsis 获取子策略运行中的中间数据
   *
   * @Param stgInstId 子策略id
   *
   * @Returns 中间数据
   */
  std::string loadStgPrivateData(StgInstId stgInstId);

 public:
  std::tuple<int, std::string> syncExecSql(const std::string& sql);
  void saveToDB(const PnlSPtr& pnl);

 public:
  void logTrace(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logDebug(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::False);

  void logInfo(const std::string& fmt, const std::vector<std::string>& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logWarn(const std::string& fmt, const std::vector<std::string>& args,
               const StgInstInfoSPtr& stgInstInfo = nullptr,
               NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logError(const std::string& fmt, const std::vector<std::string>& args,
                const StgInstInfoSPtr& stgInstInfo = nullptr,
                NotifyToTerminal notifyToTerminal = NotifyToTerminal::True);

  void logCritical(const std::string& fmt, const std::vector<std::string>& args,
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
  StgEngImplSPtr stgEngImpl_{nullptr};
};

}  // namespace bq::stg
