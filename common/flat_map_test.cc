#include "common/flat_map.h"

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "common/flat_container_testing.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::_;
using ::tsdb2::common::fixed_flat_map_of;
using ::tsdb2::common::flat_map;
using ::tsdb2::testing::TestCompare;
using ::tsdb2::testing::TestKey;
using ::tsdb2::testing::TestValue;
using ::tsdb2::testing::TestValueRepresentation;
using ::tsdb2::testing::TransparentTestCompare;

template <typename FlatMap, typename... Inner>
class TestValuesMatcher;

template <typename Key, typename Value, typename Compare, typename Representation>
class TestValuesMatcher<flat_map<Key, Value, Compare, Representation>>
    : public ::testing::MatcherInterface<flat_map<Key, Value, Compare, Representation> const&> {
 public:
  using is_gtest_matcher = void;

  using FlatMap = flat_map<Key, Value, Compare, Representation>;

  explicit TestValuesMatcher() = default;
  ~TestValuesMatcher() override = default;

  bool MatchAndExplain(FlatMap const& value,
                       ::testing::MatchResultListener* const listener) const override {
    return MatchAndExplainInternal(value, value.begin(), listener);
  }

  void DescribeTo(std::ostream* const os) const override { *os << "is an empty flat_map"; }

 protected:
  virtual bool MatchAndExplainInternal(FlatMap const& value,
                                       typename FlatMap::const_iterator const it,
                                       ::testing::MatchResultListener* const listener) const {
    return it == value.end();
  }

  virtual void DescribeToInternal(std::ostream* const os) const {}
};

template <typename Key, typename Value, typename Compare, typename Representation,
          typename KeyMatcher, typename ValueMatcher, typename... Rest>
class TestValuesMatcher<flat_map<Key, Value, Compare, Representation>, KeyMatcher, ValueMatcher,
                        Rest...>
    : public TestValuesMatcher<flat_map<Key, Value, Compare, Representation>, Rest...> {
 public:
  using is_gtest_matcher = void;

  using FlatMap = flat_map<Key, Value, Compare, Representation>;
  using Base = TestValuesMatcher<FlatMap, Rest...>;

  explicit TestValuesMatcher(KeyMatcher&& key, ValueMatcher&& value, Rest&&... rest)
      : Base(std::forward<Rest>(rest)...),
        key_matcher_(std::move(key)),
        value_matcher_(std::move(value)) {}

  ~TestValuesMatcher() override = default;

  void DescribeTo(std::ostream* const os) const override {
    *os << "is a flat_map with:";
    DescribeToInternal(os);
  }

 protected:
  bool MatchAndExplainInternal(FlatMap const& value, typename FlatMap::const_iterator it,
                               ::testing::MatchResultListener* const listener) const override {
    return ::testing::MatcherCast<Key>(key_matcher_).MatchAndExplain(it->first, listener) &&
           ::testing::MatcherCast<Value>(value_matcher_).MatchAndExplain(it->second, listener) &&
           Base::MatchAndExplainInternal(value, ++it, listener);
  }

  void DescribeToInternal(std::ostream* const os) const override {
    *os << " {";
    ::testing::MatcherCast<Key>(key_matcher_).DescribeTo(os);
    *os << ", ";
    ::testing::MatcherCast<Value>(value_matcher_).DescribeTo(os);
    *os << "},";
    Base::DescribeToInternal(os);
  }

 private:
  KeyMatcher key_matcher_;
  ValueMatcher value_matcher_;
};

template <typename Representation, typename... Inner>
auto TestValuesAre(Inner&&... inner) {
  return TestValuesMatcher<flat_map<TestKey, std::string, TestCompare, Representation>, Inner...>(
      std::forward<Inner>(inner)...);
}

template <typename Representation, typename... Inner>
auto TransparentTestValuesAre(Inner&&... inner) {
  return TestValuesMatcher<flat_map<TestKey, std::string, TransparentTestCompare, Representation>,
                           Inner...>(std::forward<Inner>(inner)...);
}

TEST(FlatMapTest, Traits) {
  using flat_map = flat_map<TestKey, std::string, TestCompare, TestValueRepresentation>;
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::value_type, TestValue>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::representation_type, TestValueRepresentation>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reference, TestValue&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reference, TestValue const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::pointer, TestValue*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_pointer, TestValue const*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::iterator,
                              typename TestValueRepresentation::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_iterator,
                              typename TestValueRepresentation::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reverse_iterator,
                              typename TestValueRepresentation::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reverse_iterator,
                              typename TestValueRepresentation::const_reverse_iterator>));
}

TEST(FlatMapTest, DefaultRepresentation) {
  using flat_map = flat_map<TestKey, std::string, TestCompare>;
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::value_type, TestValue>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::representation_type, std::vector<TestValue>>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reference, TestValue&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reference, TestValue const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::pointer, TestValue*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_pointer, TestValue const*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::iterator,
                              typename std::vector<TestValue>::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_iterator,
                              typename std::vector<TestValue>::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reverse_iterator,
                              typename std::vector<TestValue>::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reverse_iterator,
                              typename std::vector<TestValue>::const_reverse_iterator>));
}

TEST(FlatMapTest, DefaultComparator) {
  using flat_map = flat_map<TestKey, std::string>;
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::node_type::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::value_type, TestValue>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::representation_type, std::vector<TestValue>>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_compare, std::less<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reference, TestValue&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reference, TestValue const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::pointer, TestValue*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_pointer, TestValue const*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::iterator,
                              typename std::vector<TestValue>::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_iterator,
                              typename std::vector<TestValue>::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reverse_iterator,
                              typename std::vector<TestValue>::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reverse_iterator,
                              typename std::vector<TestValue>::const_reverse_iterator>));
}

template <typename Representation>
class FlatMapWithRepresentationTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(FlatMapWithRepresentationTest);

TYPED_TEST_P(FlatMapWithRepresentationTest, Construct) {
  flat_map<TestKey, std::string, TestCompare, TypeParam> fm1{TestCompare()};
  EXPECT_THAT(fm1, TestValuesAre<TypeParam>());
  flat_map<TestKey, std::string, TestCompare, TypeParam> fm2;
  EXPECT_THAT(fm2, TestValuesAre<TypeParam>());
}

TYPED_TEST_P(FlatMapWithRepresentationTest, ConstructWithInitializerList) {
  flat_map<TestKey, std::string, TestCompare, TypeParam> fm{
      {-2, "lorem"}, {-3, "ipsum"},      {4, "dolor"},    {-1, "sit"},
      {-2, "amet"},  {1, "consectetur"}, {5, "adipisci"}, {-3, "elit"},
  };
  EXPECT_THAT(fm, TestValuesAre<TypeParam>(-3, "ipsum", -2, "lorem", -1, "sit", 1, "consectetur", 4,
                                           "dolor", 5, "adipisci"));
}

// TODO

REGISTER_TYPED_TEST_SUITE_P(FlatMapWithRepresentationTest, Construct, ConstructWithInitializerList);

using RepresentationElement = std::pair<TestKey, std::string>;
using RepresentationTypes =
    ::testing::Types<std::vector<RepresentationElement>, std::deque<RepresentationElement>>;
INSTANTIATE_TYPED_TEST_SUITE_P(FlatMapWithRepresentationTest, FlatMapWithRepresentationTest,
                               RepresentationTypes);

template <typename Key, typename T, typename Compare = std::less<Key>, typename... Inner>
auto FixedValuesAre(Inner&&... inner) {
  return TestValuesMatcher<flat_map<Key, T, Compare, std::array<std::pair<Key, T>, 3>>, Inner...>(
      std::forward<Inner>(inner)...);
}

TEST(FixedFlatMapTest, ConstructIntKeys) {
  auto constexpr fm = fixed_flat_map_of<int, std::string_view>({
      {1, "lorem"},
      {2, "ipsum"},
      {3, "dolor"},
  });
  EXPECT_THAT(fm, (FixedValuesAre<int, std::string_view>(1, "lorem", 2, "ipsum", 3, "dolor")));
}

TEST(FixedFlatMapTest, ConstructStringKeys) {
  auto constexpr fm = fixed_flat_map_of<std::string_view, std::string_view>({
      {"lorem", "ipsum"},
      {"dolor", "amet"},
      {"consectetur", "adipisci"},
  });
  EXPECT_THAT(fm, (FixedValuesAre<std::string_view, std::string_view>(
                      "consectetur", "adipisci", "dolor", "amet", "lorem", "ipsum")));
}

TEST(FixedFlatMapTest, SortInts) {
  auto constexpr fm = fixed_flat_map_of<int, std::string_view>({
      {1, "lorem"},
      {3, "ipsum"},
      {2, "dolor"},
  });
  EXPECT_THAT(fm, (FixedValuesAre<int, std::string_view>(1, "lorem", 2, "dolor", 3, "ipsum")));
}

TEST(FixedFlatMapTest, SortIntsReverse) {
  auto constexpr fm = fixed_flat_map_of<int, std::string_view, std::greater<int>>({
      {1, "lorem"},
      {3, "ipsum"},
      {2, "dolor"},
  });
  EXPECT_THAT(fm, (FixedValuesAre<int, std::string_view, std::greater<int>>(3, "ipsum", 2, "dolor",
                                                                            1, "lorem")));
}

TEST(FixedFlatMapTest, SortStringsReverse) {
  auto constexpr fm =
      fixed_flat_map_of<std::string_view, std::string_view, std::greater<std::string_view>>({
          {"lorem", "ipsum"},
          {"dolor", "amet"},
          {"consectetur", "adipisci"},
      });
  EXPECT_THAT(fm,
              (FixedValuesAre<std::string_view, std::string_view, std::greater<std::string_view>>(
                  "lorem", "ipsum", "dolor", "amet", "consectetur", "adipisci")));
}

TEST(FixedFlatSetDeathTest, ConstructIntsWithDuplicates) {
  EXPECT_DEATH((fixed_flat_map_of<int, std::string_view>({
                   {1, "lorem"},
                   {2, "ipsum"},
                   {1, "dolor"},
                   {3, "amet"},
               })),
               _);
}

TEST(FixedFlatSetDeathTest, ConstructStringsWithDuplicates) {
  EXPECT_DEATH((fixed_flat_map_of<std::string_view, std::string_view>({
                   {"a", "lorem"},
                   {"b", "ipsum"},
                   {"a", "dolor"},
                   {"c", "amet"},
               })),
               _);
}

}  // namespace
