// This header provides `flat_set`, a drop-in replacement for std::set backed by a specified data
// structure. When backed by an std::vector, flat_set behaves like a sorted array and is well suited
// for read-mostly use cases and/or small-ish data structures. In those cases, being allocated in a
// single heap block makes the data much more cache-friendly and efficient. For any uses cases with
// large (but still read-mostly) datasets, you may want to use std::deque instead of std::vector.

#ifndef __TSDB2_COMMON_FLAT_MAP_H__
#define __TSDB2_COMMON_FLAT_MAP_H__

#include <functional>
#include <utility>

namespace tsdb2 {
namespace common {

template <typename Key, typename Value, typename Compare = std::less<Key>,
          typename Representation = std::vector<std::pair<Key, Value>>>
class flat_map {
 public:
  // TODO

 private:
  Representation rep_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FLAT_MAP_H__
