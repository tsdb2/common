#ifndef __TSDB2_COMMON_FLAT_SET_INTERNAL_H__
#define __TSDB2_COMMON_FLAT_SET_INTERNAL_H__

#include <array>
#include <cstddef>
#include <cstdlib>
#include <type_traits>
#include <utility>

namespace tsdb2 {
namespace common {

struct SortedDeduplicatedContainer {};
inline SortedDeduplicatedContainer constexpr kSortedDeduplicatedContainer;

namespace internal {

template <typename KeyType, typename KeyArg, typename Compare, typename IsTransparent = void>
struct key_arg {
  using type = KeyType;
};

template <typename KeyType, typename KeyArg, typename Compare>
struct key_arg<KeyType, KeyArg, Compare, std::void_t<typename Compare::is_transparent>> {
  using type = KeyArg;
};

template <typename T>
constexpr void ConstexprSwap(T& lhs, T& rhs) {
  T temp = std::move(lhs);
  lhs = std::move(rhs);
  rhs = std::move(temp);
}

template <typename T, size_t N, typename Compare>
constexpr void ConstexprSort(std::array<T, N>& array, Compare const& cmp) {
  for (size_t i = 0; i < N - 1; ++i) {
    size_t j = i;
    for (size_t k = i + 1; k < N; ++k) {
      if (cmp(array[k], array[j])) {
        j = k;
      }
    }
    if (j != i) {
      ConstexprSwap(array[i], array[j]);
    }
  }
}

template <typename T, size_t N, typename Compare>
constexpr void ConstexprCheckDuplications(std::array<T, N> const& array, Compare const& cmp) {
  for (size_t i = 1; i < N; ++i) {
    if (!cmp(array[i - 1], array[i])) {
      std::abort();
    }
  }
}

}  // namespace internal
}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FLAT_SET_INTERNAL_H__
