// This header provides `flat_set`, a drop-in replacement for std::set backed by a specified data
// structure. When backed by an std::vector, flat_set behaves like a sorted array and is well suited
// for read-mostly use cases and/or small-ish data structures. In those cases, being allocated in a
// single heap block makes the data much more cache-friendly and efficient. For any uses cases with
// large (but still read-mostly) datasets, you may want to use std::deque instead of std::vector.

#ifndef __TSDB2_COMMON_FLAT_SET_H__
#define __TSDB2_COMMON_FLAT_SET_H__

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "common/flat_container_internal.h"
#include "common/to_array.h"

namespace tsdb2 {
namespace common {

template <typename Key, typename Compare = std::less<Key>,
          typename Representation = std::vector<Key>>
class flat_set {
 public:
  class node {
   public:
    constexpr node() : value_(std::nullopt) {}
    explicit node(Key&& value) : value_(std::move(value)) {}

    node(node&& other) noexcept = default;
    node& operator=(node&& other) noexcept = default;

    bool empty() const noexcept { return !value_.has_value(); }
    explicit operator bool() const noexcept { return value_.has_value(); }

    Key& value() & { return *value_; }
    Key const& value() const& { return *value_; }
    Key&& value() && { return *std::move(value_); }

    void swap(node& other) noexcept { value_.swap(other.value_); }
    friend void swap(node& lhs, node& rhs) noexcept { lhs.swap(rhs); }

   private:
    node(node const&) = delete;
    node& operator=(node const&) = delete;

    std::optional<Key> value_;
  };

  using key_type = Key;
  using value_type = Key;
  using representation_type = Representation;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = Compare;
  using value_compare = Compare;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = typename Representation::pointer;
  using const_pointer = typename Representation::const_pointer;
  using iterator = typename Representation::iterator const;
  using const_iterator = typename Representation::const_iterator;
  using reverse_iterator = typename Representation::reverse_iterator const;
  using const_reverse_iterator = typename Representation::const_reverse_iterator;
  using node_type = node;

  struct insert_return_type {
    iterator position;
    bool inserted;
    node_type node;
  };

  template <typename Arg>
  using key_arg_t = typename internal::key_arg<key_type, Arg, Compare>::type;

  flat_set() : flat_set(Compare()) {}

  explicit constexpr flat_set(SortedDeduplicatedContainer, Representation rep,
                              Compare const& comp = Compare())
      : comp_(comp), rep_(std::move(rep)) {}

  explicit flat_set(Compare const& comp) : comp_(comp) {}

  template <typename InputIt>
  explicit flat_set(InputIt first, InputIt last, Compare const& comp = Compare()) : comp_(comp) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  flat_set(flat_set const& other) = default;
  flat_set& operator=(flat_set const& other) = default;
  flat_set(flat_set&& other) noexcept = default;
  flat_set& operator=(flat_set&& other) noexcept = default;

  flat_set(std::initializer_list<value_type> const init, Compare const& comp = Compare())
      : comp_(comp) {
    for (auto& value : init) {
      insert(value);
    }
  }

  flat_set& operator=(std::initializer_list<value_type> const init) {
    rep_.clear();
    for (auto& value : init) {
      insert(value);
    }
    return *this;
  }

  friend bool operator==(flat_set const& lhs, flat_set const& rhs) {
    return !(lhs < rhs) && !(rhs < lhs);
  }

  friend bool operator!=(flat_set const& lhs, flat_set const& rhs) { return !(lhs == rhs); }

  friend bool operator<(flat_set const& lhs, flat_set const& rhs) {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), Compare());
  }

  friend bool operator<=(flat_set const& lhs, flat_set const& rhs) { return !(rhs < lhs); }

  friend bool operator>(flat_set const& lhs, flat_set const& rhs) { return rhs < lhs; }

  friend bool operator>=(flat_set const& lhs, flat_set const& rhs) { return !(lhs < rhs); }

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
  size_type size() const noexcept { return rep_.size(); }
  size_type max_size() const noexcept { return rep_.max_size(); }

  void clear() noexcept { rep_.clear(); }

  std::pair<iterator, bool> insert(value_type const& value) {
    auto it = std::lower_bound(rep_.begin(), rep_.end(), value, comp_);
    if (it == rep_.end() || comp_(value, *it)) {
      it = rep_.insert(it, value);
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    auto it = std::lower_bound(rep_.begin(), rep_.end(), value, comp_);
    if (it == rep_.end() || comp_(value, *it)) {
      it = rep_.insert(it, std::move(value));
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  iterator insert(const_iterator pos, value_type const& value) {
    auto [it, unused] = insert(value);
    return it;
  }

  iterator insert(const_iterator pos, value_type&& value) {
    auto [it, unused] = insert(std::move(value));
    return it;
  }

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  void insert(std::initializer_list<value_type> const init) {
    for (auto& value : init) {
      insert(value);
    }
  }

  insert_return_type insert(node_type&& node) {
    if (node) {
      auto [it, inserted] = insert(std::move(node).value());
      if (inserted) {
        return {it, true};
      } else {
        return {it, false, std::move(node)};
      }
    } else {
      return {rep_.end(), false, std::move(node)};
    }
  }

  iterator insert(const_iterator pos, node_type&& node) {
    auto [it, unused_inserted, unused_node] = insert(std::move(node));
    return it;
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return insert(value_type(std::forward<Args>(args)...));
  }

  iterator erase(const_iterator pos) { return rep_.erase(pos); }

  iterator erase(const_iterator const first, const_iterator const last) {
    return rep_.erase(first, last);
  }

  template <typename KeyArg = key_type>
  size_type erase(key_arg_t<KeyArg> const& key) {
    auto const it = find(key);
    if (it != end()) {
      rep_.erase(it);
      return 1;
    } else {
      return 0;
    }
  }

  void swap(flat_set& other) noexcept {
    std::swap(comp_, other.comp_);
    rep_.swap(other.rep_);
  }

  node_type extract(iterator position) {
    node_type node{std::move(*position)};
    rep_.erase(position);
    return node;
  }

  template <typename KeyArg = key_type>
  node_type extract(key_arg_t<KeyArg> const& key) {
    auto it = find(key);
    if (it != end()) {
      return extract(it);
    } else {
      return {};
    }
  }

  // TODO: merge methods.

  [[nodiscard]] Representation&& ExtractRep() && { return std::move(rep_); }

  template <typename KeyArg = key_type>
  size_type count(key_arg_t<KeyArg> const& key) const {
    return std::binary_search(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  iterator find(key_arg_t<KeyArg> const& key) {
    auto const it = std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
    if (it == rep_.end() || comp_(key, *it)) {
      return rep_.end();
    } else {
      return it;
    }
  }

  template <typename KeyArg = key_type>
  const_iterator find(key_arg_t<KeyArg> const& key) const {
    auto const it = std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
    if (it == rep_.end() || comp_(key, *it)) {
      return rep_.end();
    } else {
      return it;
    }
  }

  template <typename KeyArg = key_type>
  bool contains(key_arg_t<KeyArg> const& key) const {
    return std::binary_search(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  std::pair<iterator, iterator> equal_range(key_arg_t<KeyArg> const& key) {
    return std::equal_range(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  std::pair<const_iterator, const_iterator> equal_range(key_arg_t<KeyArg> const& key) const {
    return std::equal_range(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  iterator lower_bound(key_arg_t<KeyArg> const& key) {
    return std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  const_iterator lower_bound(key_arg_t<KeyArg> const& key) const {
    return std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  iterator upper_bound(key_arg_t<KeyArg> const& key) {
    return std::upper_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename KeyArg = key_type>
  const_iterator upper_bound(key_arg_t<KeyArg> const& key) const {
    return std::upper_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  key_compare key_comp() const { return comp_; }
  value_compare value_comp() const { return comp_; }

 private:
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Compare comp_;
  Representation rep_;
};

template <typename T, typename Compare = std::less<std::decay_t<T>>, size_t const N>
constexpr flat_set<T, Compare, std::array<T, N>> fixed_flat_set_of(std::array<T, N> array,
                                                                   Compare&& comp = Compare()) {
  internal::ConstexprSort(array, comp);
  internal::ConstexprCheckDuplications(array, comp);
  return flat_set<T, Compare, std::array<T, N>>(kSortedDeduplicatedContainer, std::move(array),
                                                std::forward<Compare>(comp));
}

template <typename T, typename Compare = std::less<std::decay_t<T>>, size_t const N>
constexpr flat_set<T, Compare, std::array<T, N>> fixed_flat_set_of(T const (&values)[N],
                                                                   Compare&& comp = Compare()) {
  return fixed_flat_set_of<T, Compare, N>(to_array(values), std::forward<Compare>(comp));
}

}  // namespace common
}  // namespace tsdb2

namespace std {

template <typename Key, typename Compare, typename Representation>
void swap(::tsdb2::common::flat_set<Key, Compare, Representation>& lhs,
          ::tsdb2::common::flat_set<Key, Compare, Representation>& rhs) {
  lhs.swap(rhs);
}

}  // namespace std

#endif  //__TSDB2_COMMON_FLAT_SET_H__
