/*!
 * \file BQMDUtil.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#include "util/BQMDUtil.hpp"

#include "def/BQConst.hpp"
#include "def/Def.hpp"

namespace bq::md {

std::string RemoveDepthInTopicOfBooks(const std::string& topic) {
  assert(topic.empty() == false && "topic.empty() == false");
  //! booksIdentity = @books
  const auto booksIdentity =
      fmt::format("{}{}", SEP_OF_TOPIC, magic_enum::enum_name(MDType::Books));
  const auto range = boost::find_first(topic, booksIdentity);
  std::string ret;
  ret.assign(std::begin(topic), std::end(range));
  return ret;
}

std::string GetSymbolCode(MarketCode marketCode,
                          const std::string& exchSymbolCode) {
  if (marketCode != MarketCode::CZCE) {
    return exchSymbolCode;
  }
  const auto pos = exchSymbolCode.find_first_of("0123456789");
  auto ret = exchSymbolCode.substr(0, pos) + "2" +
             exchSymbolCode.substr(pos, exchSymbolCode.size());
  return ret;
}

}  // namespace bq::md
