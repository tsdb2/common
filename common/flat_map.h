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
#include <utility>

#include "absl/base/attributes.h"

namespace tsdb2 {
namespace common {

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Representation = std::vector<std::pair<const Key, T>>>
class flat_map {
 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
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

    bool operator()(value_type const& lhs, value_type consy& rhs) const {
      return comp_(lhs.first, rhs.first);
    }

   private:
    Compare const& comp_;
  };

  flat_map() : flat_map(Compare()) {}

  explicit flat_map(Compare const& comp) : comp_(comp) {}

  // TODO

 private:
  ABSL_ATTRIBUTE_NO_UNIQUE_ADDRESS Compare comp_;
  Representation rep_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FLAT_MAP_H__
