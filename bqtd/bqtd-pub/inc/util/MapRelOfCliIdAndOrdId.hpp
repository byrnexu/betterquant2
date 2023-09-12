/*!
 * \file MapRelOfCliIdAndOrdId.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/01/04
 *
 * \brief
 */

#pragma once

#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/SHMDef.hpp"
#include "util/Pch.hpp"
#include "util/StdExt.hpp"

namespace bq::td {

using ClientId = std::uint32_t;

struct RelOfCliIdAndOrdId {
  RelOfCliIdAndOrdId(ClientId clientId, OrderId orderId)
      : clientId_(clientId), orderId_(orderId) {}
  ClientId clientId_;
  OrderId orderId_;
  std::uint64_t closedTime_{0};
};
using DelType = bip::deleter<RelOfCliIdAndOrdId,
                             bip::managed_shared_memory::segment_manager>;
using RelOfCliIdAndOrdIdSHMSPtr =
    bip::shared_ptr<RelOfCliIdAndOrdId, VoidAlloc, DelType>;

class MapRelOfCliIdAndOrdId {
  inline const static std::string SEP_OF_LINE = ":";
  inline const static std::size_t MAX_SIZE_OF_MAP_REPL_CLOSED = 1024;
  inline const static char* NAME_OF_LAST_CLIENT_ID = "lastClientId";
  inline const static char* NAME_OF_MAP_REL = "RelOfCliIdAndOrdIdGroup";
  inline const static char* NAME_OF_MAP_REL_OF_CLOSED =
      "RelOfCliIdAndOrdIdOfClosedGroup";

  //! ClientId索引
  struct TagClientId {};
  struct KeyClientId
      : boost::multi_index::composite_key<RelOfCliIdAndOrdId,
                                          MIDX_MEMBER(RelOfCliIdAndOrdId,
                                                      ClientId, clientId_)> {};
  using MIdxClientId = boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagClientId>, KeyClientId,
      boost::multi_index::composite_key_result_less<KeyClientId::result_type>>;

  //! OrderId索引
  struct TagOrderId {};
  struct KeyOrderId
      : boost::multi_index::composite_key<RelOfCliIdAndOrdId,
                                          MIDX_MEMBER(RelOfCliIdAndOrdId,
                                                      OrderId, orderId_)> {};
  using MIdxOrderId = boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagOrderId>, KeyOrderId,
      boost::multi_index::composite_key_result_less<KeyOrderId::result_type>>;

  //! ClosedTime索引
  struct TagClosedTime {};
  struct KeyClosedTime
      : boost::multi_index::composite_key<
            RelOfCliIdAndOrdId,
            MIDX_MEMBER(RelOfCliIdAndOrdId, std::uint64_t, closedTime_)> {};
  using MIdxClosedTime = boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagClosedTime>, KeyClosedTime,
      boost::multi_index::composite_key_result_less<
          KeyClosedTime::result_type>>;

  //! 用于存放未完结的映射关系的RelOfCliIdAndOrdIdGroup
  using RelOfCliIdAndOrdIdGroup = boost::multi_index::multi_index_container<
      RelOfCliIdAndOrdIdSHMSPtr,
      boost::multi_index::indexed_by<MIdxClientId, MIdxOrderId>,
      bip::managed_shared_memory::allocator<RelOfCliIdAndOrdIdSHMSPtr>::type>;
  using RelOfCliIdAndOrdIdGroupSPtr = std::shared_ptr<RelOfCliIdAndOrdIdGroup>;

  //! 用于存放已完结的映射关系的RelOfCliIdAndOrdIdGroup
  using RelOfCliIdAndOrdIdOfClosedGroup =
      boost::multi_index::multi_index_container<
          RelOfCliIdAndOrdIdSHMSPtr,
          boost::multi_index::indexed_by<MIdxClientId, MIdxOrderId,
                                         MIdxClosedTime>,
          bip::managed_shared_memory::allocator<
              RelOfCliIdAndOrdIdSHMSPtr>::type>;
  using RelOfCliIdAndOrdIdOfClosedGroupSPtr =
      std::shared_ptr<RelOfCliIdAndOrdIdOfClosedGroup>;

 public:
  MapRelOfCliIdAndOrdId(const MapRelOfCliIdAndOrdId&) = delete;
  MapRelOfCliIdAndOrdId& operator=(const MapRelOfCliIdAndOrdId&) = delete;
  MapRelOfCliIdAndOrdId(const MapRelOfCliIdAndOrdId&&) = delete;
  MapRelOfCliIdAndOrdId& operator=(const MapRelOfCliIdAndOrdId&&) = delete;

  MapRelOfCliIdAndOrdId() = default;

 public:
  void load(const std::string& segmentIdentity, std::size_t len);

 private:
  void loadLastClientId(const std::string& identity);
  void loadRelOfCliIdAndOrdIdGroup(const std::string& identity);
  void loadRelOfCliIdAndOrdIdOfClosedGroup(const std::string& identity);

 public:
  void add(ClientId clientId, OrderId orderId);
  void remove(ClientId clientId);

  OrderId getOrderId(ClientId clientId);

  std::uint32_t genClientId() const {
    std::lock_guard<std::ext::spin_mutex> guard(mtxMapRelOfCliIdAndOrdId_);
    return ++*lastClientId_;
  }

 private:
  mutable ClientId* lastClientId_{nullptr};

  std::unique_ptr<bip::managed_shared_memory> segment_{nullptr};

  RelOfCliIdAndOrdIdGroup* relOfCliIdAndOrdIdGroup_{nullptr};
  //! relOfCliIdAndOrdIdOfClosedGroup_这个结构是为了应付回报乱序的情况下还能根据
  //! clientId_得到orderId_，比如部成部撤状态先到，最后一笔成交回报后到，那么因为
  //! 委托回报的部成部撤状态处理中删除了relOfCliIdAndOrdIdGroup_中的记录，导致从
  //! relOfCliIdAndOrdIdGroup_无法获取到orderId_，因此增加了这个数据结构使得我们仍
  //! 然可以从中获取到orderId_
  RelOfCliIdAndOrdIdOfClosedGroup* relOfCliIdAndOrdIdOfClosedGroup_{nullptr};
  mutable std::ext::spin_mutex mtxMapRelOfCliIdAndOrdId_;
};

}  // namespace bq::td
