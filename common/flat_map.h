// This header provides `flat_map`, a drop-in replacement for std::map backed by a specified data
// structure. When backed by an std::vector, flat_map behaves like a sorted array and is well suited
// for read-mostly use cases and/or small-ish data structures. In those cases, being allocated in a
// single heap block makes the data much more cache-friendly and efficient. For any uses cases with
// large (but still read-mostly) datasets, you may want to use std::deque instead of std::vector.

#ifndef __TSDB2_COMMON_FLAT_MAP_H__
#define __TSDB2_COMMON_FLAT_MAP_H__

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "common/flat_container_internal.h"
#include "common/to_array.h"

namespace tsdb2 {
namespace common {

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Representation = std::vector<std::pair<Key, T>>>
class flat_map {
 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using representation_type = Representation;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = Compare;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = typename Representation::pointer;
  using const_pointer = typename Representation::const_pointer;
  using iterator = typename Representation::iterator const;
  using const_iterator = typename Representation::const_iterator;
  using reverse_iterator = typename Representation::reverse_iterator const;
  using const_reverse_iterator = typename Representation::const_reverse_iterator;

  class node {
   public:
    using key_type = Key;
    using mapped_type = T;

    constexpr node() noexcept : value_(std::nullopt) {}
    explicit node(value_type&& value) : value_(std::move(value)) {}

    node(node&&) noexcept = default;
    node& operator=(node&&) noexcept = default;

    bool empty() const noexcept { return !value_.has_value(); }
    explicit operator bool() const noexcept { return value_.has_value(); }

    key_type& key() const { return const_cast<key_type&>(value_->first); }
    mapped_type& mapped() const { return const_cast<mapped_type&>(value_->second); }

    value_type& value() const& { return const_cast<value_type&>(*value_); }
    value_type&& value() && { return *std::move(value_); }

    void swap(node& other) noexcept { value_.swap(other.value_); }
    friend void swap(node& lhs, node& rhs) noexcept { lhs.swap(rhs); }

   private:
    node(node const&) = delete;
    node& operator=(node const&) = delete;

    std::optional<value_type> value_;
  };

  using node_type = node;

  struct insert_return_type {
    iterator position;
    bool inserted;
    node_type node;
  };

  template <typename Arg>
  using key_arg_t = typename internal::key_arg<Compare>::template type<key_type, Arg>;

  class value_compare {
   public:
    explicit constexpr value_compare(Compare const& comp) : comp_(comp) {}

    template <typename LHS, typename RHS>
    constexpr bool operator()(LHS&& lhs, RHS&& rhs) const {
      return comp_(to_key(std::forward<LHS>(lhs)), to_key(std::forward<RHS>(rhs)));
    }

   private:
    static constexpr key_type const& to_key(value_type const& value) { return value.first; }

    template <typename KeyArg = key_type>
    static constexpr auto to_key(key_arg_t<KeyArg> const& key) {
      return key;
    }

    Compare const& comp_;
  };

  constexpr flat_map() : flat_map(Compare()) {}

  explicit constexpr flat_map(SortedDeduplicatedContainer, Representation rep,
                              Compare const& comp = Compare())
      : comp_(comp), rep_(std::move(rep)) {}

  explicit constexpr flat_map(Compare const& comp) : comp_(comp) {}

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

  template <typename H>
  friend H AbslHashValue(H h, flat_map const& fm) {
    return H::combine(std::move(h), fm.rep_);
  }

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

  template <typename Mapped>
  std::pair<iterator, bool> insert_or_assign(Key const& key, Mapped&& mapped) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      return insert(value_type(key, std::forward<Mapped>(mapped)));
    } else {
      it->second = std::move(mapped);
      return {it, false};
    }
  }

  template <typename Mapped>
  std::pair<iterator, bool> insert_or_assign(Key&& key, Mapped&& mapped) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      return insert(value_type(std::move(key), std::forward<Mapped>(mapped)));
    } else {
      it->second = std::move(mapped);
      return {it, false};
    }
  }

  template <typename Mapped>
  iterator insert_or_assign(const_iterator const hint, Key const& key, Mapped&& mapped) {
    return insert_or_assign(key, std::forward<Mapped>(mapped));
  }

  template <typename Mapped>
  iterator insert_or_assign(const_iterator const hint, Key&& key, Mapped&& mapped) {
    return insert_or_assign(std::move(key), std::forward<Mapped>(mapped));
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return insert(value_type(std::forward<Args>(args)...));
  }

  template <typename... Args>
  iterator emplace_hint(const_iterator const hint, Args&&... args) {
    auto [it, unused] = emplace(std::forward<Args>(args)...);
    return it;
  }

  template <typename... Args>
  std::pair<iterator, bool> try_emplace(Key const& key, Args&&... args) {
    value_compare comp{comp_};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      it = rep_.emplace(it, std::piecewise_construct, std::forward_as_tuple(key),
                        std::forward_as_tuple(std::forward<Args>(args)...));
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
      it = rep_.emplace(it, std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                        std::forward_as_tuple(std::forward<Args>(args)...));
      return std::make_pair(it, true);
    } else {
      return std::make_pair(it, false);
    }
  }

  template <typename... Args>
  iterator try_emplace(const_iterator const hint, Key const& key, Args&&... args) {
    auto [it, unused] = try_emplace(key, std::forward<Args>(args)...);
    return it;
  }

  template <typename... Args>
  iterator try_emplace(const_iterator const hint, Key&& key, Args&&... args) {
    auto [it, unused] = try_emplace(std::move(key), std::forward<Args>(args)...);
    return it;
  }

  iterator erase(iterator const pos) { return rep_.erase(pos); }
  iterator erase(const_iterator const pos) { return rep_.erase(pos); }

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

  void swap(flat_map& other) noexcept {
    std::swap(comp_, other.comp_);
    std::swap(rep_, other.rep_);
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

  Representation const& rep() const { return rep_; }

  Representation&& ExtractRep() && { return std::move(rep_); }

  template <typename KeyArg = key_type>
  size_t count(key_arg_t<KeyArg> const& key) const {
    return std::binary_search(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  iterator find(key_arg_t<KeyArg> const& key) {
    value_compare comp{comp_};
    auto const it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      return rep_.end();
    } else {
      return it;
    }
  }

  template <typename KeyArg = key_type>
  const_iterator find(key_arg_t<KeyArg> const& key) const {
    value_compare comp{comp_};
    auto const it = std::lower_bound(rep_.begin(), rep_.end(), key, comp);
    if (it == rep_.end() || comp(key, it->first)) {
      return rep_.end();
    } else {
      return it;
    }
  }

  template <typename KeyArg = key_type>
  bool contains(key_arg_t<KeyArg> const& key) const {
    return std::binary_search(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  std::pair<iterator, iterator> equal_range(key_arg_t<KeyArg> const& key) {
    return std::equal_range(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  std::pair<const_iterator, const_iterator> equal_range(key_arg_t<KeyArg> const& key) const {
    return std::equal_range(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  iterator lower_bound(key_arg_t<KeyArg> const& key) {
    return std::lower_bound(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  const_iterator lower_bound(key_arg_t<KeyArg> const& key) const {
    return std::lower_bound(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  iterator upper_bound(key_arg_t<KeyArg> const& key) {
    return std::upper_bound(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  template <typename KeyArg = key_type>
  const_iterator upper_bound(key_arg_t<KeyArg> const& key) const {
    return std::upper_bound(rep_.begin(), rep_.end(), key, value_compare(comp_));
  }

  key_compare key_comp() const { return comp_; }
  value_compare value_comp() const { return value_compare(comp_); }

 private:
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Compare comp_;
  Representation rep_;
};

template <typename Key, typename T, typename Compare = std::less<Key>, size_t const N>
constexpr auto fixed_flat_map_of(std::array<std::pair<Key, T>, N> array,
                                 Compare&& comp = Compare()) {
  typename flat_map<Key, T, Compare, std::array<std::pair<Key, T>, N>>::value_compare value_comp{
      comp};
  internal::ConstexprSort(array, value_comp);
  internal::ConstexprCheckDuplications(array, value_comp);
  return flat_map<Key, T, Compare, std::array<std::pair<Key, T>, N>>(
      kSortedDeduplicatedContainer, std::move(array), std::forward<Compare>(comp));
}

template <typename Key, typename T, typename Compare = std::less<Key>, size_t const N>
constexpr auto fixed_flat_map_of(std::pair<Key, T> const (&values)[N],  // NOLINT(*-avoid-c-arrays)
                                 Compare&& comp = Compare()) {
  return fixed_flat_map_of<Key, T, Compare, N>(to_array(values), std::forward<Compare>(comp));
}

template <typename Key, typename T, typename Compare = std::less<Key>>
constexpr auto fixed_flat_map_of(internal::EmptyInitializerList, Compare&& comp = Compare()) {
  return flat_map<Key, T, Compare, std::array<std::pair<Key, T>, 0>>(std::forward<Compare>(comp));
}

}  // namespace common
}  // namespace tsdb2

namespace std {

template <typename Key, typename T, typename Compare, typename Representation>
void swap(::tsdb2::common::flat_map<Key, T, Compare, Representation>& lhs,
          ::tsdb2::common::flat_map<Key, T, Compare, Representation>& rhs) {
  lhs.swap(rhs);
}

}  // namespace std

#endif  // __TSDB2_COMMON_FLAT_MAP_H__
