/*!
 * \file OpenedContractGroup.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/24
 *
 * \brief
 *
 * 这个模块主要用于记录当日已开仓合约，有两个主要用途：
 *
 * 1、平今仓风控插件检查是否已经有当日开仓
 * 2、用于交易网关计算平今手续费以及平今风控插件，上交所和能源所计算手续费的时候
 *    根据下单的时候指定的平仓方式，如果下单的时候指定的是 CloseYDay，计算手续费
 *    的时候会转换为Close
 *
 */

#include "util/OpenedContractGroup.hpp"

namespace bq {

//! tdSrv由于已经提供了segmentGroup，因此用这个函数load
std::vector<std::string> OpenedContractGroup::load(
    const std::shared_ptr<bip::managed_shared_memory>& segment) {
  segment_ = segment;
  return loadImpl(WriteLog::False);
}

std::vector<std::string> OpenedContractGroup::loadImpl(WriteLog writeLog) {
  std::vector<std::string> ret;

  openedContractTable_ = segment_->find_or_construct<OpenedContractTable>(
      NAME_OF_OPEN_CONTRACT_GROUP)(OpenedContractTable::ctor_args_list(),
                                   segment_->get_allocator<OpenedContract>());

  const auto msg = fmt::format(
      "Load {} numbers of rec from opened contract group in shm. {}",
      openedContractTable_->size(), FreeSegmentPerDesc(*segment_));
  if (writeLog == WriteLog::True) {
    ret.emplace_back(msg);
    LOG_I(msg);
  }

  for (const auto& openedContract : *openedContractTable_) {
    const auto msg = ("Opened contract: {}", openedContract->toStr());
    if (writeLog == WriteLog::True) {
      ret.emplace_back(msg);
      LOG_I(msg);
    }
  }

  return ret;
}

}  // namespace bq
