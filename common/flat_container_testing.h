#ifndef __TSDB2_COMMON_FLAT_CONTAINER_TESTING_H__
#define __TSDB2_COMMON_FLAT_CONTAINER_TESTING_H__

#include <deque>
#include <ostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "common/fixed.h"
#include "common/flat_map.h"
#include "common/flat_set.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace tsdb2 {
namespace testing {

struct TestKey {
  TestKey(int const field_value) : field(field_value) {}

  friend bool operator==(TestKey const& lhs, TestKey const& rhs) { return lhs.field == rhs.field; }
  friend bool operator!=(TestKey const& lhs, TestKey const& rhs) { return !operator==(lhs, rhs); }

  friend bool operator<(TestKey const& lhs, TestKey const& rhs) { return lhs.field < rhs.field; }
  friend bool operator<=(TestKey const& lhs, TestKey const& rhs) { return lhs.field <= rhs.field; }
  friend bool operator>(TestKey const& lhs, TestKey const& rhs) { return lhs.field > rhs.field; }
  friend bool operator>=(TestKey const& lhs, TestKey const& rhs) { return lhs.field >= rhs.field; }

  int field;
};

using TestValue = std::pair<TestKey, std::string>;

struct TestCompare {
  bool operator()(TestKey const& lhs, TestKey const& rhs) const { return lhs.field < rhs.field; }
};

struct ReverseTestCompare {
  bool operator()(TestKey const& lhs, TestKey const& rhs) const { return lhs.field > rhs.field; }
};

struct TransparentTestCompare {
  using is_transparent = void;
  template <typename RHS>
  bool operator()(TestKey const& lhs, RHS const& rhs) const {
    return lhs.field < rhs.field;
  }
};

using TestRepresentation = std::deque<TestKey>;
using TestValueRepresentation = std::deque<TestValue>;

template <typename Inner>
class TestKeyMatcher : public ::testing::MatcherInterface<TestKey const&> {
 public:
  using is_gtest_matcher = void;

  explicit TestKeyMatcher(Inner inner) : inner_(std::move(inner)) {}
  ~TestKeyMatcher() override = default;

  bool MatchAndExplain(TestKey const& value,
                       ::testing::MatchResultListener* const listener) const override {
    return ::testing::MatcherCast<int>(inner_).MatchAndExplain(value.field, listener);
  }

  void DescribeTo(std::ostream* const os) const override {
    ::testing::MatcherCast<int>(inner_).DescribeTo(os);
  }

 private:
  Inner inner_;
};

template <typename Inner>
TestKeyMatcher<std::decay_t<Inner>> TestKeyEq(Inner&& inner) {
  return TestKeyMatcher<std::decay_t<Inner>>(std::forward<Inner>(inner));
}

template <typename FlatSet, typename... Inner>
class TestKeysMatcher;

template <typename Key, typename Compare, typename Representation, typename... Inner>
class TestKeysMatcher<::tsdb2::common::flat_set<Key, Compare, Representation>, Inner...>
    : public ::testing::MatcherInterface<
          ::tsdb2::common::flat_set<Key, Compare, Representation> const&> {
 public:
  using is_gtest_matcher = void;

  using FlatSet = ::tsdb2::common::flat_set<Key, Compare, Representation>;
  using Tuple = std::tuple<::tsdb2::common::FixedT<TestKey, Inner>...>;

  explicit TestKeysMatcher(Inner&&... inner) : inner_{std::forward<Inner>(inner)...} {}
  ~TestKeysMatcher() override = default;

  bool MatchAndExplain(FlatSet const& value,
                       ::testing::MatchResultListener* const listener) const override {
    auto it = value.begin();
    Tuple values{TestKey(::tsdb2::common::FixedV<Inner>(*it++))...};
    return ::testing::MatcherCast<Tuple>(inner_).MatchAndExplain(values, listener);
  }

  void DescribeTo(std::ostream* const os) const override {
    ::testing::MatcherCast<Tuple>(inner_).DescribeTo(os);
  }

 private:
  std::tuple<Inner...> inner_;
};

template <typename Representation, typename... Inner>
TestKeysMatcher<::tsdb2::common::flat_set<TestKey, TestCompare, Representation>,
                std::decay_t<Inner>...>
TestKeysAre(Inner&&... inner) {
  return TestKeysMatcher<::tsdb2::common::flat_set<TestKey, TestCompare, Representation>,
                         std::decay_t<Inner>...>(std::forward<Inner>(inner)...);
}

template <typename Representation, typename... Inner>
TestKeysMatcher<::tsdb2::common::flat_set<TestKey, TransparentTestCompare, Representation>,
                std::decay_t<Inner>...>
TransparentTestKeysAre(Inner&&... inner) {
  return TestKeysMatcher<::tsdb2::common::flat_set<TestKey, TransparentTestCompare, Representation>,
                         std::decay_t<Inner>...>(std::forward<Inner>(inner)...);
}

}  // namespace testing
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FLAT_CONTAINER_TESTING_H__
