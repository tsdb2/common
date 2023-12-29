// This header provides `flat_map`, a drop-in replacement for std::map backed by a specified data
// structure. When backed by an std::vector, flat_map behaves like a sorted array and is well suited
// for read-mostly use cases and/or small-ish data structures. In those cases, being allocated in a
// single heap block makes the data much more cache-friendly and efficient. For any uses cases with
// large (but still read-mostly) datasets, you may want to use std::deque instead of std::vector.

#ifndef __TSDB2_COMMON_FLAT_MAP_H__
#define __TSDB2_COMMON_FLAT_MAP_H__

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "absl/base/attributes.h"
#include "common/flat_container_internal.h"

namespace tsdb2 {
namespace common {

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Representation = std::vector<std::pair<Key const, T>>>
class flat_map {
 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key const, T>;
  using representation_type = Representation;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = typename Representation::pointer;
  using const_pointer = typename Representation::const_pointer;
  using iterator = typename Representation::iterator;
  using const_iterator = typename Representation::const_iterator;
  using reverse_iterator = typename Representation::reverse_iterator;
  using const_reverse_iterator = typename Representation::const_reverse_iterator;

  class node {
   public:
    // TODO

   private:
    value_type value;
  };

  using node_type = node;

  struct insert_return_type {
    iterator position;
    bool inserted;
    node_type node;
  };

  class value_compare {
   public:
    explicit value_compare(Compare const& comp) : comp_(comp) {}

    template <typename LHS, typename RHS>
    bool operator()(LHS const& lhs, RHS const& rhs) const {
      return comp_(to_key(lhs), to_key(rhs));
    }

   private:
    static key_type const& to_key(value_type const& value) { return value.first; }
    static key_type const& to_key(key_type const& key) { return key; }

    Compare const& comp_;
  };

  flat_map() : flat_map(Compare()) {}

  explicit flat_map(Compare const& comp) : comp_(comp) {}

  template <class InputIt>
  flat_map(InputIt first, InputIt last, Compare const& comp = Compare()) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  flat_map(flat_map const& other) = default;
  flat_map& operator=(flat_map const& other) = default;
  flat_map(flat_map&& other) noexcept = default;
  flat_map& operator=(flat_map&& other) noexcept = default;

  flat_map(std::initializer_list<value_type> const init, Compare const& comp = Compare())
      : comp_(comp) {
    for (auto& [key, value] : init) {
      try_emplace(key, value);
    }
  }

  flat_map& operator=(std::initializer_list<value_type> const init) {
    rep_.clear();
    for (auto& [key, value] : init) {
      try_emplace(key, value);
    }
    return *this;
  }

  friend bool operator==(flat_map const& lhs, flat_map const& rhs) {
    return !(lhs < rhs) && !(rhs < lhs);
  }

  friend bool operator!=(flat_map const& lhs, flat_map const& rhs) { return !(lhs == rhs); }

  friend bool operator<(flat_map const& lhs, flat_map const& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                        value_compare(lhs.comp_));
  }

  friend bool operator<=(flat_map const& lhs, flat_map const& rhs) { return !(rhs < lhs); }

  friend bool operator>(flat_map const& lhs, flat_map const& rhs) { return rhs < lhs; }

  friend bool operator>=(flat_map const& lhs, flat_map const& rhs) { return !(lhs < rhs); }

  T& at(Key const& key) {
    auto const it = find(key);
    if (it != end()) {
      return it->second;
    } else {
      throw std::out_of_range("key not found");
    }
  }

  T const& at(Key const& key) const {
    auto const it = find(key);
    if (it != end()) {
      return it->second;
    } else {
      throw std::out_of_range("key not found");
    }
  }

  T& operator[](Key const& key) {
    auto const [it, unused] = try_emplace(key);
    return it->second;
  }

  T& operator[](Key&& key) {
    auto const [it, unused] = try_emplace(std::move(key));
    return it->second;
  }

  iterator begin() noexcept { return rep_.begin(); }
  const_iterator begin() const noexcept { return rep_.begin(); }
  const_iterator cbegin() const noexcept { return rep_.cbegin(); }
  iterator end() noexcept { return rep_.end(); }
  const_iterator end() const noexcept { return rep_.end(); }
  const_iterator cend() const noexcept { return rep_.cend(); }

  iterator rbegin() noexcept { return rep_.rbegin(); }
  const_iterator rbegin() const noexcept { return rep_.rbegin(); }
  const_iterator crbegin() const noexcept { return rep_.crbegin(); }
  iterator rend() noexcept { return rep_.rend(); }
  const_iterator rend() const noexcept { return rep_.rend(); }
  const_iterator crend() const noexcept { return rep_.crend(); }

  bool empty() const noexcept { return rep_.empty(); }
  size_t size() const noexcept { return rep_.size(); }
  size_t max_size() const noexcept { return rep_.max_size(); }

  void clear() noexcept { rep_.clear(); }

  std::pair<iterator, bool> insert(value_type const& value) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), value, comp);
    if (it == rep_.end() || comp(value, *it)) {
      it = rep_.insert(it, value);
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  template <typename P>
  std::pair<iterator, bool> insert(P&& value) {
    return emplace(std::forward<P>(value));
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), value, comp);
    if (it == rep_.end() || comp(value, *it)) {
      it = rep_.insert(it, std::move(value));
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  iterator insert(const_iterator const pos, value_type const& value) {
    auto [it, unused] = insert(value);
    return it;
  }

  template <class P>
  iterator insert(const_iterator const pos, P&& value) {
    auto [it, unused] = insert(std::forward<P>(value));
    return it;
  }

  iterator insert(const_iterator const pos, value_type&& value) {
    auto [it, unused] = insert(std::move(value));
    return it;
  }

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  void insert(std::initializer_list<value_type> const init) {
    for (auto& [key, value] : init) {
      try_emplace(key, value);
    }
  }

  // TODO

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(Key const& key, Args&&... args) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      it = rep_.emplace(it, value_type(std::piecewise_construct, std::forward_as_tuple(key),
                                       std::forward_as_tuple(std::forward<Args>(args)...)));
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      it = rep_.emplace(it,
                        value_type(std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                                   std::forward_as_tuple(std::forward<Args>(args)...)));
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  // TODO

 private:
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Compare comp_;
  Representation rep_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FLAT_MAP_H__
