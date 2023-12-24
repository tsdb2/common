#ifndef __TSDB2_COMMON_RAW_FLAT_SET_H__
#define __TSDB2_COMMON_RAW_FLAT_SET_H__

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

#include "absl/base/attributes.h"

namespace tsdb2 {
namespace common {
namespace internal {

template <typename KeyType, typename KeyArg, typename Compare, typename IsTransparent = void>
struct key_arg {
  using type = KeyType;
};

template <typename KeyType, typename KeyArg, typename Compare>
struct key_arg<KeyType, KeyArg, Compare, std::void_t<typename Compare::is_transparent>> {
  using type = KeyArg;
};

// Internal implementation of `flat_set` and `flat_map`.
template <typename Traits, typename Compare, typename Representation>
class raw_flat_set {
 public:
  using key_type = typename Traits::key_type;
  using value_type = typename Traits::value_type;
  using representation_type = Representation;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = typename Traits::key_compare;
  using value_compare = typename Traits::value_compare;
  using allocator_type = typename Representation::allocator_type;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;
  using iterator = typename Traits::iterator;
  using const_iterator = typename Traits::const_iterator;
  using reverse_iterator = typename Traits::reverse_iterator;
  using const_reverse_iterator = typename Traits::const_reverse_iterator;

  using node_type = typename Traits::node;

  struct insert_return_type {
    iterator position;
    bool inserted;
    node_type node;
  };

  template <typename Arg>
  using key_arg_t = typename key_arg<key_type, Arg, Compare>::type;

  raw_flat_set() : raw_flat_set(Compare()) {}

  explicit raw_flat_set(Compare const& comp, allocator_type const& alloc = allocator_type())
      : comp_(comp), rep_(alloc) {}

  explicit raw_flat_set(allocator_type const& alloc) : rep_(alloc) {}

  template <typename InputIt>
  explicit raw_flat_set(InputIt first, InputIt last, Compare const& comp = Compare(),
                        allocator_type const& alloc = allocator_type())
      : comp_(comp), rep_(alloc) {
    for (; first != last; ++first) {
      insert(*first);
    }
  }

  template <typename InputIt>
  explicit raw_flat_set(InputIt first, InputIt last, allocator_type const& alloc)
      : raw_flat_set(first, last, Compare(), alloc) {}

  raw_flat_set(raw_flat_set const& other) : rep_(other.rep_) {}

  raw_flat_set& operator=(raw_flat_set const& other) {
    rep_ = other.rep_;
    return *this;
  }

  raw_flat_set(raw_flat_set&& other) noexcept : rep_(std::move(other.rep_)) {}

  raw_flat_set& operator=(raw_flat_set&& other) noexcept {
    rep_ = std::move(other.rep_);
    return *this;
  }

  raw_flat_set(std::initializer_list<value_type> const init, Compare const& comp = Compare(),
               allocator_type const& alloc = allocator_type())
      : comp_(comp), rep_(alloc) {
    for (auto& value : init) {
      insert(value);
    }
  }

  raw_flat_set(std::initializer_list<value_type> const init, allocator_type const& alloc)
      : raw_flat_set(init, Compare(), alloc) {}

  raw_flat_set& operator=(std::initializer_list<value_type> const init) {
    for (auto& value : init) {
      insert(value);
    }
    return *this;
  }

  friend bool operator==(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ == rhs.rep_;
  }

  friend bool operator!=(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ != rhs.rep_;
  }

  friend bool operator<(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ < rhs.rep_;
  }

  friend bool operator<=(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ <= rhs.rep_;
  }

  friend bool operator>(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ > rhs.rep_;
  }

  friend bool operator>=(raw_flat_set const& lhs, raw_flat_set const& rhs) {
    return lhs.rep_ >= rhs.rep_;
  }

  allocator_type get_allocator() const noexcept { return rep_.get_allocator(); }

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

  iterator erase(iterator pos) { rep_.erase(pos); }
  iterator erase(const_iterator pos) { rep_.erase(pos); }
  iterator erase(const_iterator first, const_iterator last) { rep_.erase(first, last); }

  template <typename Key = key_type>
  size_type erase(key_arg_t<Key>&& key) {
    return rep_.erase(std::binary_search(rep_.begin(), rep_.end(), key, comp_));
  }

  void swap(raw_flat_set& other) noexcept {
    std::swap(comp_, other.comp_);
    rep_.swap(other.rep_);
  }

  friend void swap(raw_flat_set& lhs, raw_flat_set& rhs) noexcept { lhs.swap(rhs); }

  node_type extract(iterator position) {
    node_type node{std::move(*position)};
    rep_.erase(position);
    return node;
  }

  node_type extract(const_iterator position) {
    node_type node{std::move(*position)};
    rep_.erase(position);
    return node;
  }

  template <typename Key = key_type>
  node_type extract(key_arg_t<Key>&& key) {
    return extract(std::binary_search(rep_.begin(), rep_.end(), key, comp_));
  }

  // TODO: merge methods.

  [[nodiscard]] Representation&& ExtractRep() && { return rep_; }

  template <typename Key = key_type>
  size_type count(key_arg_t<Key> const& key) const {
    auto const it = std::binary_search(rep_.begin(), rep_.end(), key, comp_);
    return it != rep_.end();
  }

  template <typename Key = key_type>
  iterator find(key_arg_t<Key> const& key) {
    return std::binary_search(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  const_iterator find(key_arg_t<Key> const& key) const {
    return std::binary_search(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  bool contains(key_arg_t<Key> const& key) const {
    auto const it = std::binary_search(rep_.begin(), rep_.end(), key, comp_);
    return it != rep_.end();
  }

  template <typename Key = key_type>
  std::pair<iterator, iterator> equal_range(key_arg_t<Key> const& key) {
    return std::equal_range(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  std::pair<const_iterator, const_iterator> equal_range(key_arg_t<Key> const& key) const {
    return std::equal_range(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  std::pair<iterator, iterator> lower_bound(key_arg_t<Key> const& key) {
    return std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  std::pair<const_iterator, const_iterator> lower_bound(key_arg_t<Key> const& key) const {
    return std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  std::pair<iterator, iterator> upper_bound(key_arg_t<Key> const& key) {
    return std::upper_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  template <typename Key = key_type>
  std::pair<const_iterator, const_iterator> upper_bound(key_arg_t<Key> const& key) const {
    return std::upper_bound(rep_.begin(), rep_.end(), key, comp_);
  }

  key_compare key_comp() const { return comp_; }

  value_compare value_comp() const { return comp_; }

 private:
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Compare comp_;
  Representation rep_;
};

}  // namespace internal
}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_RAW_FLAT_SET_H__
