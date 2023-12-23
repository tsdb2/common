// This header provides `flat_set`, a drop-in replacement for std::set backed by a specified data
// structure. When backed by an std::vector, flat_set behaves like a sorted array.

#ifndef __TSDB2_COMMON_FLAT_SET_H__
#define __TSDB2_COMMON_FLAT_SET_H__

#include <functional>
#include <utility>
#include <vector>

#include "common/raw_flat_set.h"

namespace tsdb2 {
namespace common {

template <typename Key>
struct flat_set_traits {
  using key_type = Key;
  using value_type = Key;
};

template <typename Key, typename Compare = std::less<Key>,
          typename Representation = std::vector<Key>>
class flat_set : private internal::raw_flat_set<flat_set_traits<Key>, Compare, Representation> {
 public:
  using raw_flat_set = internal::raw_flat_set<flat_set_traits<Key>, Compare, Representation>;

  using key_type = typename raw_flat_set::key_type;
  using value_type = typename raw_flat_set::value_type;
  using representation_type = typename raw_flat_set::representation_type;
  using size_type = typename raw_flat_set::size_type;
  using difference_type = typename raw_flat_set::difference_type;
  using key_compare = typename raw_flat_set::key_compare;
  using value_compare = typename raw_flat_set::value_compare;
  using allocator_type = typename raw_flat_set::allocator_type;
  using reference = typename raw_flat_set::reference;
  using const_reference = typename raw_flat_set::const_reference;
  using pointer = typename raw_flat_set::pointer;
  using const_pointer = typename raw_flat_set::const_pointer;
  using iterator = typename raw_flat_set::iterator;
  using const_iterator = typename raw_flat_set::const_iterator;
  using reverse_iterator = typename raw_flat_set::reverse_iterator;
  using const_reverse_iterator = typename raw_flat_set::const_reverse_iterator;

  flat_set() : raw_flat_set() {}

  explicit flat_set(Compare const& compare, allocator_type const& alloc = allocator_type())
      : raw_flat_set(compare, alloc) {}

  explicit flat_set(allocator_type const& alloc) : raw_flat_set(alloc) {}

  template <typename InputIt>
  explicit flat_set(InputIt first, InputIt last, Compare const& compare,
                    allocator_type const& alloc)
      : raw_flat_set(first, last, compare, alloc) {}

  template <typename InputIt>
  explicit flat_set(InputIt first, InputIt last, allocator_type const& alloc)
      : raw_flat_set(first, last, alloc) {}

  flat_set(std::initializer_list<value_type> const init, Compare const& comp = Compare(),
           allocator_type const& alloc = allocator_type())
      : raw_flat_set(init, comp, alloc) {}

  flat_set(std::initializer_list<value_type> const init, allocator_type const& alloc)
      : raw_flat_set(init, alloc) {}

  flat_set(flat_set const&) = default;
  flat_set& operator=(flat_set const&) = default;
  flat_set(flat_set&&) noexcept = default;
  flat_set& operator=(flat_set&&) noexcept = default;

  flat_set& operator=(std::initializer_list<value_type> const init) {
    return raw_flat_set::operator=(init);
  }

  using raw_flat_set::get_allocator;

  using raw_flat_set::begin;
  using raw_flat_set::cbegin;
  using raw_flat_set::cend;
  using raw_flat_set::end;

  // TODO

  using raw_flat_set::empty;
  using raw_flat_set::size;

  // TODO

  using raw_flat_set::emplace;

  // TODO
};

}  // namespace common
}  // namespace tsdb2

#endif  //__TSDB2_COMMON_FLAT_SET_H__
