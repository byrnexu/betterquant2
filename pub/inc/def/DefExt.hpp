/*!
 * \file DefExt.hpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2022/09/08
 *
 * \brief
 */

#pragma once

#include "util/Pch.hpp"
#include "util/Random.hpp"
#include "util/StdExt.hpp"

namespace bq {

enum class LockFunc { True = 1, False = 2 };

template <LockFunc Func>
std::enable_if_t<Func == LockFunc::True,
                 std::unique_ptr<std::lock_guard<std::mutex>>>
MakeLockGuard(std::mutex& mtx) {
  return std::make_unique<std::lock_guard<std::mutex>>(mtx);
}
template <LockFunc Func>
std::enable_if_t<Func == LockFunc::False, std::nullptr_t> MakeLockGuard(
    std::mutex& mtx) {
  return nullptr;
}
#define LOCK(mtx) auto guard = MakeLockGuard<lockFunc>(mtx)

template <LockFunc Func>
std::enable_if_t<Func == LockFunc::True,
                 std::unique_ptr<std::lock_guard<std::ext::spin_mutex>>>
MakeSpinLockGuard(std::ext::spin_mutex& mtx) {
  return std::make_unique<std::lock_guard<std::ext::spin_mutex>>(mtx);
}
template <LockFunc Func>
std::enable_if_t<Func == LockFunc::False, std::nullptr_t> MakeSpinLockGuard(
    std::ext::spin_mutex& mtx) {
  return nullptr;
}
#define SPIN_LOCK(mtx) auto guard = MakeSpinLockGuard<lockFunc>(mtx)

#define BQAPI extern "C" BOOST_SYMBOL_EXPORT

#define CONFIG Config::get_mutable_instance().get()
#define CONV_OPT(type, val) boost::convert<type>((val))
#define CONV(type, val) boost::convert<type>((val)).value()

#define GET_RAND_STR() RandomStr::get_mutable_instance().get()
#define GET_RAND_INT() RandomInt::get_mutable_instance().get()

#define JSER(Type, ...)                                                        \
  friend void to_json(nlohmann::ordered_json& nlohmann_json_j,                 \
                      const Type& nlohmann_json_t) {                           \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))   \
  }                                                                            \
  friend void from_json(const nlohmann::ordered_json& nlohmann_json_j,         \
                        Type& nlohmann_json_t) {                               \
    NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__)) \
  }

#define MIDX_MEMBER BOOST_MULTI_INDEX_MEMBER

#define LOWER_ENUM(enum_value) \
  boost::to_lower_copy(std::string(magic_enum::enum_name((enum_value))))

#define UPPER_ENUM(enum_value) \
  boost::to_upper_copy(std::string(magic_enum::enum_name((enum_value))))

#define ENUM_TO_STR(enum_value) std::string(magic_enum::enum_name((enum_value)))
#define ENUM_TO_CSTR(enum_value) magic_enum::enum_name((enum_value)).data()

using Doc = rapidjson::Document;
using DocSPtr = std::shared_ptr<Doc>;
using Val = rapidjson::Value;
using ValSPtr = std::shared_ptr<Val>;

struct JsonData {
  JsonData(const std::string& str) {
    doc_ = yyjson_read(str.data(), str.size(), 0);
    root_ = yyjson_doc_get_root(doc_);
  }
  JsonData(yyjson_doc* doc, yyjson_val* root) : doc_(doc), root_(root) {}

  ~JsonData() {
    if (doc_) {
      yyjson_doc_free(doc_);
      doc_ = nullptr;
    }
  }

  yyjson_doc* doc_{nullptr};
  yyjson_val* root_{nullptr};
};
using JsonDataSPtr = std::shared_ptr<JsonData>;

}  // namespace bq

struct boost::cnv::by_default : boost::cnv::spirit {};
