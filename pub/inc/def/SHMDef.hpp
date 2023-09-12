/*!
 * \file SHM.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"

namespace bq {

namespace bip = boost::interprocess;

using SegmentMgr = bip::managed_shared_memory::segment_manager;

using VoidAlloc = bip::allocator<void, SegmentMgr>;
using CharAlloc = bip::allocator<char, SegmentMgr>;

using Int16Alloc = bip::allocator<std::int32_t, SegmentMgr>;
using Uint16Alloc = bip::allocator<std::uint32_t, SegmentMgr>;

using Int32Alloc = bip::allocator<std::int32_t, SegmentMgr>;
using Uint32Alloc = bip::allocator<std::uint32_t, SegmentMgr>;

using Int64Alloc = bip::allocator<std::int64_t, SegmentMgr>;
using Uint64Alloc = bip::allocator<std::uint64_t, SegmentMgr>;

using SHMString = bip::basic_string<char, std::char_traits<char>, CharAlloc>;
using SHMStringSPtr =
    bip::managed_shared_ptr<SHMString, bip::managed_shared_memory>::type;

}  // namespace bq
