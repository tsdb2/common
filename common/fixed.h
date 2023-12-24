#ifndef __TSDB2_COMMON_FIXED_H__
#define __TSDB2_COMMON_FIXED_H__

namespace tsdb2 {
namespace common {

template <typename T, typename Unused>
struct Fixed {
  using type = T;
  static T value(T&& t) { return t; }
};

template <typename T, typename Unused>
using FixedT = typename Fixed<T, Unused>::type;

template <typename Unused, typename T>
T FixedV(T&& t) {
  return Fixed<T, Unused>::value(std::forward<T>(t));
}

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FIXED_H__
