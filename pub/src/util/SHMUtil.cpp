/*!
 * \file SHM.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/08
 *
 * \brief
 */

#include "util/SHMUtil.hpp"

namespace bq {

std::string FreeSegmentPerDesc(const bip::managed_shared_memory& segment) {
  double per = 0;
  if (segment.get_size() == 0) {
    per = 1;
  } else {
    per = static_cast<double>(segment.get_free_memory()) /
          static_cast<double>(segment.get_size());
    if (per > 1) per = 1;
  }
  const auto sizeOfMB = segment.get_size() / (1024.0 * 1024.0);
  const auto ret =
      fmt::format("{:.2f}% of {:.2f}mb available", per * 100, sizeOfMB);
  return ret;
}

}  // namespace bq
