#ifndef __TSDB2_COMMON_RAW_FLAT_SET_H__
#define __TSDB2_COMMON_RAW_FLAT_SET_H__

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>

#include "absl/base/attributes.h"

namespace tsdb2 {
namespace common {
namespace internal {

// Internal implementation of `flat_set` and `flat_map`.
template <typename Traits, typename Compare, typename Representation>
class raw_flat_set {
 public:
  using key_type = typename Traits::key_type;
  using value_type = typename Traits::value_type;
  using representation_type = Representation;
  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using key_compare = Compare;
  using value_compare = Compare;
  using allocator_type = typename Representation::allocator_type;
  using reference = value_type&;
  using const_reference = value_type const&;
  using pointer = typename std::allocator_traits<allocator_type>::pointer;
  using const_pointer = typename std::allocator_traits<allocator_type>::const_pointer;

  class iterator {
   public:
    // TODO

   private:
    friend class raw_flat_set;

    explicit iterator(typename representation_type::iterator it) : it_(std::move(it)) {}

    typename Representation::iterator it_;
  };

  using const_iterator = iterator const;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  raw_flat_set() : raw_flat_set(Compare()) {}

  explicit raw_flat_set(Compare const& comp, allocator_type const& alloc = allocator_type())
      : comp_(comp), rep_(alloc) {}

  explicit raw_flat_set(allocator_type const& alloc) : rep_(alloc) {}

  template <typename InputIt>
  explicit raw_flat_set(InputIt first, InputIt last, Compare const& comp,
                        allocator_type const& alloc)
      : comp_(comp), rep_(alloc) {
    for (; first != last; ++first) {
      emplace(*first);
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
      emplace(value);
    }
  }

  raw_flat_set(std::initializer_list<value_type> const init, allocator_type const& alloc)
      : raw_flat_set(init, Compare(), alloc) {}

  raw_flat_set& operator=(std::initializer_list<value_type> const init) {
    for (auto& value : init) {
      emplace(value);
    }
  }

  allocator_type get_allocator() const noexcept { return rep_.get_allocator(); }

  iterator begin() noexcept { return iterator(rep_.begin()); }
  const_iterator begin() const noexcept { return iterator(rep_.begin()); }
  const_iterator cbegin() const noexcept { return iterator(rep_.cbegin()); }
  iterator end() noexcept { return iterator(rep_.end()); }
  const_iterator end() const noexcept { return iterator(rep_.end()); }
  const_iterator cend() const noexcept { return iterator(rep_.cend()); }

  // TODO

  bool empty() const noexcept { return rep_.empty(); }
  size_type size() const noexcept { return rep_.size(); }

  // TODO

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    key_type key{std::forward<Args>(args)...};
    auto it = std::lower_bound(rep_.begin(), rep_.end(), key, comp_);
    if (comp_(key, *it)) {
      it = rep_.insert(it, std::move(key));
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

}  // namespace internal
}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_RAW_FLAT_SET_H__
