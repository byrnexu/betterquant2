/*!
 * \file LRUCache.cpp
 * \project BetterQuant
 *
 * \author byrnexu
 * \date 2023/02/09
 *
 * \brief
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <list>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace std::ext {

template <typename key_t, typename value_t>
class lru_cache;
template <typename key_t, typename value_t>
using lru_cache_sptr = std::shared_ptr<lru_cache<key_t, value_t>>;

template <typename key_t, typename value_t>
class lru_cache {
 public:
  typedef typename std::pair<key_t, value_t> key_value_pair_t;
  typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

  explicit lru_cache(std::size_t capacity) : capacity_(capacity) {}

  void push(const key_t& key, const value_t& value,
            bool key_must_be_unique = true) {
    assert(cache_items_map_.size() == cache_items_list_.size());
    cache_items_list_.push_front(key_value_pair_t(key, value));
    if (key_must_be_unique) {
      auto it = cache_items_map_.find(key);
      if (it != cache_items_map_.end()) {
        cache_items_list_.erase(it->second);
        cache_items_map_.erase(it);
      }
    }
    cache_items_map_.emplace(key, cache_items_list_.begin());

    if (cache_items_map_.size() > capacity_) {
      auto last = cache_items_list_.end();
      last--;
      cache_items_map_.erase(last->first);
      cache_items_list_.pop_back();
    }
  }

  std::pair<bool, value_t> pop(const key_t& key) {
    assert(cache_items_map_.size() == cache_items_list_.size());
    value_t value{};
    const auto it = cache_items_map_.find(key);
    if (it == cache_items_map_.end()) {
      return std::make_pair(false, value);
    } else {
      value = it->second->second;
      cache_items_list_.erase(it->second);
      cache_items_map_.erase(it);
      return std::make_pair(true, value);
    }
    return std::make_pair(false, value);
  }

  std::pair<bool, value_t> get(const key_t& key) {
    assert(cache_items_map_.size() == cache_items_list_.size());
    value_t value{};
    const auto it = cache_items_map_.find(key);
    if (it == cache_items_map_.end()) {
      return std::make_pair(false, value);
    } else {
      cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_,
                               it->second);
      return std::make_pair(true, it->second->second);
    }
    return std::make_pair(false, value);
  }

  inline bool exists(const key_t& key) const {
    assert(cache_items_map_.size() == cache_items_list_.size());
    return cache_items_map_.find(key) != cache_items_map_.end();
  }

  inline std::string to_string() const {
    std::ostringstream oss;
    for (const auto& rec : cache_items_list_) {
      oss << rec.first << "=" << rec.second << ",";
    }
    std::string ret = oss.str();
    if (ret.empty() == false) ret.pop_back();
    return ret;
  }

  inline void clear() {
    assert(cache_items_map_.size() == cache_items_list_.size());
    cache_items_list_.clear();
    cache_items_map_.clear();
  }

  inline std::size_t size() const { return cache_items_map_.size(); }
  inline std::size_t capacity() const noexcept { return capacity_; }

 private:
  const std::size_t capacity_;

  std::list<key_value_pair_t> cache_items_list_;
  std::unordered_multimap<key_t, list_iterator_t> cache_items_map_;
};

}  // namespace std::ext
