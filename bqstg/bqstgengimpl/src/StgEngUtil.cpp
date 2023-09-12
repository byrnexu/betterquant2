/*!
 * \file StgEngUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "StgEngUtil.hpp"

#include "CommonIPCData.hpp"
#include "SHMIPCTask.hpp"
#include "def/BQDef.hpp"
#include "def/DataStruOfOthers.hpp"
#include "def/DataStruOfTD.hpp"
#include "def/StatusCode.hpp"
#include "util/TaskDispatcher.hpp"

namespace bq::stg {

SHMIPCAsyncTaskSPtr MakeStgSignal(MsgId msgId, StgInstId stgInstId) {
  StgSignal stgSignal(msgId);
  const auto task = std::make_shared<SHMIPCTask>(&stgSignal, sizeof(StgSignal));
  const auto ret = std::make_shared<SHMIPCAsyncTask>(task, stgInstId);
  return ret;
}

//! 为了策略实例回调能得到 timerName
SHMIPCAsyncTaskSPtr MakeStgSignal(MsgId msgId, StgInstId stgInstId,
                                  const std::string& data) {
  const auto commonIPCData = MakeCommonIPCData(data);
  commonIPCData->shmHeader_.msgId_ = msgId;
  const auto task = std::make_shared<SHMIPCTask>(
      commonIPCData.get(), sizeof(CommonIPCData) + commonIPCData->dataLen_ + 1);
  const auto ret = std::make_shared<SHMIPCAsyncTask>(task, stgInstId);
  return ret;
}

std::tuple<int, StgInstId> GetStgInstId(const CommonIPCData* commonIPCData) {
  Doc doc;
  if (doc.Parse(commonIPCData->data_).HasParseError()) {
    LOG_W("Parse data failed. {0} [offset {1}] {2}",
          GetParseError_En(doc.GetParseError()), doc.GetErrorOffset(),
          commonIPCData->data_);
    return {-1, 1};
  }

  if (doc.HasMember("stgInstId") && doc["stgInstId"].IsUint()) {
    const auto stgInstId = doc["stgInstId"].GetUint();
    return {0, stgInstId};
  }

  return {-1, 1};
}

int InitPosSide(OrderInfoSPtr& orderInfo) {
  //!
  //! 目前是双向持仓和单边持仓都支持，关键在于下单的时候选择Both还是Long/Short，目前
  //! 使用Long/Short模式，这样可以知道历史上买入和卖出的总量，净头寸也可以由两者相加
  //! 得到，为了实现这一点，凡是下的现货单，都是属于开仓，也就是说只有买多和卖空两种
  //! 组合，这个再下单的入口点也就是策略引擎的下单接口中强行指定。以后可以考虑由一个
  //! 策略参数或者账户层面的配置指定。
  //!
  //! 如果Long/Short模式下仓位也要pnl和净头寸都要临时计算，所以默认改成了Both
  //!
  //! 测试发现头寸虽然是及净值，但是手续费却是累加的，感觉不大合理，所以最终还是用
  //! Long/Short
  //!
  //! RiskMgr推送PosInfo的时候，如果用Long/Short模式还是要累加后推送，因此采用Both
  //!
  if (orderInfo->symbolType_ == SymbolType::CN_MainBoard ||
      orderInfo->symbolType_ == SymbolType::CN_SecondBoard ||
      orderInfo->symbolType_ == SymbolType::CN_StartupBoard ||
      orderInfo->symbolType_ == SymbolType::CN_TechBoard ||
      orderInfo->symbolType_ == SymbolType::Spot) {
    orderInfo->posSide_ = PosSide::Both;
    orderInfo->posDirection_ = PosDirection::Both;

  } else if (orderInfo->symbolType_ == SymbolType::CN_Futures) {
    if (orderInfo->side_ == Side::Bid) {
      orderInfo->posSide_ = PosSide::Long;

    } else if (orderInfo->side_ == Side::Ask) {
      orderInfo->posSide_ = PosSide::Short;

    } else {
      return SCODE_STG_INVALID_SIDE;
    }

  } else if (orderInfo->symbolType_ == SymbolType::Futures ||
             orderInfo->symbolType_ == SymbolType::CFutures ||
             orderInfo->symbolType_ == SymbolType::Perp ||
             orderInfo->symbolType_ == SymbolType::CPerp) {
    if (orderInfo->side_ == Side::Bid) {
      orderInfo->posSide_ = PosSide::Long;

    } else if (orderInfo->side_ == Side::Ask) {
      orderInfo->posSide_ = PosSide::Short;

    } else {
      return SCODE_STG_INVALID_SIDE;
    }

  } else {
    return SCODE_STG_INVALID_SYMBOL_TYPE;
  }

  return 0;
}

}  // namespace bq::stg
