/*!
 * \file SymbolInfoExt.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/02
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/BQDef.hpp"
#include "def/Const.hpp"
#include "def/Def.hpp"
#include "def/SymbolInfoIF.hpp"
#include "util/PchExt.hpp"

namespace bq {

struct KeySymbolCode
    : boost::multi_index::composite_key<
          SymbolInfo, MIDX_MEMBER(SymbolInfo, std::string, symbolCode_)> {};

struct KeyExchSymbolCode
    : boost::multi_index::composite_key<
          SymbolInfo, MIDX_MEMBER(SymbolInfo, std::string, exchSymbolCode_)> {};

struct TagSymbolCode {};
struct TagExchSymbolCode {};

using MIdxSymbolCode = boost::multi_index::ordered_unique<
    boost::multi_index::tag<TagSymbolCode>, KeySymbolCode,
    boost::multi_index::composite_key_result_less<KeySymbolCode::result_type>>;

using MIdxExchSymbolCode = boost::multi_index::ordered_unique<
    boost::multi_index::tag<TagExchSymbolCode>, KeyExchSymbolCode,
    boost::multi_index::composite_key_result_less<
        KeyExchSymbolCode::result_type>>;

using MIDXSymbolInfo = boost::multi_index::multi_index_container<
    SymbolInfoSPtr,
    boost::multi_index::indexed_by<MIdxSymbolCode, MIdxExchSymbolCode>>;

}  // namespace bq
