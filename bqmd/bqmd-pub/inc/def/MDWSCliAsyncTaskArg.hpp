/*!
 * \file MDWSCliAsyncTaskArg.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "def/BQConst.hpp"
#include "def/Def.hpp"

namespace bq::md {

struct WSCliAsyncTaskArg;
using WSCliAsyncTaskArgSPtr = std::shared_ptr<WSCliAsyncTaskArg>;

struct WSCliAsyncTaskArg {
  MsgType wsMsgType_;

  yyjson_doc *doc_{nullptr};
  yyjson_val *root_{nullptr};

  //!
  //! 以下三个字段用于保存历史行情数据
  //!
  std::string topic_;
  std::uint64_t exchTs_;
  std::string marketDataOfUnifiedFmt_;

  ~WSCliAsyncTaskArg() {
    if (doc_) {
      yyjson_doc_free(doc_);
      doc_ = nullptr;
    }
  }
};

}  // namespace bq::md
