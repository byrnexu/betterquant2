/*!
 * \file MapRelOfCliIdAndOrdId.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/01/04
 *
 * \brief
 *
 *  报单的时候生成clientId，并保存clientId和orderId的关系
 *  报单应答和回报的时候根据clientId的到系统内orderId
 *  同步未完结订单的时候根据查询到的订单的clientId获取内部orderId
 *
 */

#include "util/MapRelOfCliIdAndOrdId.hpp"

#include "def/Def.hpp"
#include "util/Datetime.hpp"
#include "util/File.hpp"
#include "util/Logger.hpp"
#include "util/SHMUtil.hpp"

namespace bq::td {

void MapRelOfCliIdAndOrdId::load(const std::string& segmentIdentity,
                                 std::size_t len) {
  //! 创建shm文件
  segment_ = std::make_unique<bip::managed_shared_memory>(
      bip::open_or_create, segmentIdentity.c_str(), len);
  loadLastClientId(segmentIdentity);
  loadRelOfCliIdAndOrdIdGroup(segmentIdentity);
  loadRelOfCliIdAndOrdIdOfClosedGroup(
      fmt::format("{}.{}", segmentIdentity, "closed"));
}

//! 启动的时候先从共享内存获取目前最大的clientId也就是lastClientId_
void MapRelOfCliIdAndOrdId::loadLastClientId(const std::string& identity) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    lastClientId_ = segment_->find_or_construct<ClientId>(  //
        NAME_OF_LAST_CLIENT_ID)(0);
    LOG_I("Get last client id {} from segment.", *lastClientId_);
  }
}

//! 加载共享内存中未完结的clientId和orderId的对应关系
void MapRelOfCliIdAndOrdId::loadRelOfCliIdAndOrdIdGroup(
    const std::string& identity) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    relOfCliIdAndOrdIdGroup_ =
        segment_->find_or_construct<RelOfCliIdAndOrdIdGroup>(NAME_OF_MAP_REL)(
            RelOfCliIdAndOrdIdGroup::ctor_args_list(),
            segment_->get_allocator<RelOfCliIdAndOrdId>());
    LOG_I("Load {} numbers of rec from segment {} . {}",
          relOfCliIdAndOrdIdGroup_->size(), identity,
          FreeSegmentPerDesc(*segment_));
  }
}

//! 加载共享内存中已完结的clientId和orderId的对应关系
void MapRelOfCliIdAndOrdId::loadRelOfCliIdAndOrdIdOfClosedGroup(
    const std::string& identity) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    relOfCliIdAndOrdIdOfClosedGroup_ =
        segment_->find_or_construct<RelOfCliIdAndOrdIdOfClosedGroup>(
            NAME_OF_MAP_REL_OF_CLOSED)(
            RelOfCliIdAndOrdIdOfClosedGroup::ctor_args_list(),
            segment_->get_allocator<RelOfCliIdAndOrdId>());
    LOG_I("Load {} numbers of rec from segment {}. {}",
          relOfCliIdAndOrdIdOfClosedGroup_->size(), identity,
          FreeSegmentPerDesc(*segment_));
  }
}

//! 往共享内存添加一条未完结的clientId和orderId的对应关系
void MapRelOfCliIdAndOrdId::add(ClientId clientId, OrderId orderId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    const auto rec = bip::make_managed_shared_ptr(
        segment_->construct<RelOfCliIdAndOrdId>(bip::anonymous_instance)(
            clientId, orderId),
        *segment_);
    relOfCliIdAndOrdIdGroup_->emplace(rec);

    if (relOfCliIdAndOrdIdGroup_->size() % 100 == 0) {
      LOG_W("Size of rel of cli id and ord id group is {}. {}",
            relOfCliIdAndOrdIdGroup_->size(), FreeSegmentPerDesc(*segment_));
    }
  }
}

//! 将共享内存中完结的clientId和orderId对应关系移除，然后移动到已完结的clientId
//! 和orderId对应关系中。
void MapRelOfCliIdAndOrdId::remove(ClientId clientId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    const auto iter = relOfCliIdAndOrdIdGroup_->find(clientId);
    if (iter != std::end(*relOfCliIdAndOrdIdGroup_)) {
      (*iter)->closedTime_ = GetTotalUSSince1970();

      // 将记录移到 relOfCliIdAndOrdIdOfClosedGroup_
      relOfCliIdAndOrdIdOfClosedGroup_->emplace(*iter);
      if (relOfCliIdAndOrdIdOfClosedGroup_->size() >
          MAX_SIZE_OF_MAP_REPL_CLOSED) {
        auto& idx = relOfCliIdAndOrdIdOfClosedGroup_->get<TagClosedTime>();
        idx.erase(std::begin(idx));
      }

      //! 从 relOfCliIdAndOrdIdGroup_ 中删除
      relOfCliIdAndOrdIdGroup_->erase(iter);
      if (relOfCliIdAndOrdIdGroup_->size() % 100 == 0) {
        LOG_I("Size of rel of cli id and ord id group is {}. {}",
              relOfCliIdAndOrdIdGroup_->size(), FreeSegmentPerDesc(*segment_));
      }

      return;
    }

    LOG_W(
        "Remove rec in map rel of cli id and ord id failed"
        "because of cli id {} not found.",
        clientId);
  }
}

//! 根据交易所订单号 clientId 获取本地 orderId
OrderId MapRelOfCliIdAndOrdId::getOrderId(ClientId clientId) {
  {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    auto& idx = relOfCliIdAndOrdIdGroup_->get<TagClientId>();
    const auto iter = idx.find(clientId);
    if (iter != std::end(*relOfCliIdAndOrdIdGroup_)) {
      return (*iter)->orderId_;
    }

    auto& idxOfClosed = relOfCliIdAndOrdIdOfClosedGroup_->get<TagClientId>();
    const auto iterOfClosed = idxOfClosed.find(clientId);
    if (iterOfClosed != std::end(idxOfClosed)) {
      LOG_I("Get order id {} from rel of cli id and ord id of closed.",
            (*iterOfClosed)->orderId_);
      return (*iterOfClosed)->orderId_;
    }
  }

  LOG_W(
      "Get rec in map rel of cli id and ord id failed"
      "because of cli id {} not found.",
      clientId);

  return 0;
}

}  // namespace bq::td
