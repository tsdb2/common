#include "common/flat_set.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "common/fixed.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::FixedT;
using ::tsdb2::common::FixedV;
using ::tsdb2::common::flat_set;

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

struct TestCompare {
  bool operator()(TestKey const& lhs, TestKey const& rhs) const { return lhs.field < rhs.field; }
};

struct TestTransparentCompare {
  using is_transparent = void;
  template <typename RHS>
  bool operator()(TestKey const& lhs, RHS const& rhs) const {
    return lhs.field < rhs.field;
  }
};

using TestRepresentation = std::deque<TestKey>;
using TestAllocator = typename TestRepresentation::allocator_type;

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

  void DescribeNegationTo(std::ostream* const os) const override {
    ::testing::MatcherCast<int>(inner_).DescribeTo(os);
  }

 private:
  Inner inner_;
};

template <typename Inner>
TestKeyMatcher<std::decay_t<Inner>> TestKeyEq(Inner&& inner) {
  return TestKeyMatcher<std::decay_t<Inner>>(std::forward<Inner>(inner));
}

template <typename... Inner>
class TestKeysMatcher : public ::testing::MatcherInterface<
                            flat_set<TestKey, TestCompare, TestRepresentation> const&> {
 public:
  using is_gtest_matcher = void;

  using Tuple = std::tuple<FixedT<TestKey, Inner>...>;

  explicit TestKeysMatcher(Inner&&... inner) : inner_{std::forward<Inner>(inner)...} {}
  ~TestKeysMatcher() override = default;

  bool MatchAndExplain(flat_set<TestKey, TestCompare, TestRepresentation> const& value,
                       ::testing::MatchResultListener* const listener) const override {
    auto it = value.begin();
    Tuple values{TestKey(FixedV<Inner>(*it++))...};
    return ::testing::MatcherCast<Tuple>(inner_).MatchAndExplain(values, listener);
  }

  void DescribeTo(std::ostream* const os) const override {
    ::testing::MatcherCast<Tuple>(inner_).DescribeTo(os);
  }

  void DescribeNegationTo(std::ostream* const os) const override {
    ::testing::MatcherCast<Tuple>(inner_).DescribeTo(os);
  }

 private:
  std::tuple<Inner...> inner_;
};

template <typename... Inner>
TestKeysMatcher<std::decay_t<Inner>...> TestKeysAre(Inner&&... inner) {
  return TestKeysMatcher<std::decay_t<Inner>...>(std::forward<Inner>(inner)...);
}

TEST(FlatSetTest, Traits) {
  using flat_set = flat_set<TestKey, TestCompare, TestRepresentation>;
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::representation_type, std::deque<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::allocator_type, TestAllocator>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reference, TestKey&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reference, TestKey const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::pointer, TestKey*>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_pointer, TestKey const*>));
  EXPECT_TRUE(
      (std::is_same_v<typename flat_set::iterator, typename TestRepresentation::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_iterator,
                              typename TestRepresentation::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reverse_iterator,
                              typename TestRepresentation::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reverse_iterator,
                              typename TestRepresentation::const_reverse_iterator>));
}

TEST(FlatSetTest, DefaultRepresentation) {
  using flat_set = flat_set<TestKey, TestCompare>;
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::representation_type, std::vector<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::allocator_type,
                              typename std::vector<TestKey>::allocator_type>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reference, TestKey&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reference, TestKey const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::pointer, TestKey*>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_pointer, TestKey const*>));
  EXPECT_TRUE(
      (std::is_same_v<typename flat_set::iterator, typename std::vector<TestKey>::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_iterator,
                              typename std::vector<TestKey>::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reverse_iterator,
                              typename std::vector<TestKey>::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reverse_iterator,
                              typename std::vector<TestKey>::const_reverse_iterator>));
}

TEST(FlatSetTest, DefaultComparator) {
  using flat_set = flat_set<TestKey>;
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::representation_type, std::vector<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::key_compare, std::less<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::value_compare, std::less<TestKey>>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::allocator_type,
                              typename std::vector<TestKey>::allocator_type>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reference, TestKey&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reference, TestKey const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::pointer, TestKey*>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_pointer, TestKey const*>));
  EXPECT_TRUE(
      (std::is_same_v<typename flat_set::iterator, typename std::vector<TestKey>::iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_iterator,
                              typename std::vector<TestKey>::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::reverse_iterator,
                              typename std::vector<TestKey>::reverse_iterator const>));
  EXPECT_TRUE((std::is_same_v<typename flat_set::const_reverse_iterator,
                              typename std::vector<TestKey>::const_reverse_iterator>));
}

TEST(FlatSetTest, Construct) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{TestCompare(), TestAllocator()};
  EXPECT_THAT(fs1, TestKeysAre());
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{TestCompare()};
  EXPECT_THAT(fs2, TestKeysAre());
  flat_set<TestKey, TestCompare, TestRepresentation> fs3{TestAllocator()};
  EXPECT_THAT(fs3, TestKeysAre());
  flat_set<TestKey, TestCompare, TestRepresentation> fs4{};
  EXPECT_THAT(fs4, TestKeysAre());
}

TEST(FlatSetTest, ConstructWithIterators) {
  std::vector<TestKey> keys{-2, -3, 4, -1, -2, 1, 5, -3};

  flat_set<TestKey, TestCompare, TestRepresentation> fs1{keys.begin(), keys.end(), TestCompare(),
                                                         TestAllocator()};
  EXPECT_THAT(fs1, TestKeysAre(-3, -2, -1, 1, 4, 5));

  flat_set<TestKey, TestCompare, TestRepresentation> fs2{keys.begin(), keys.end(), TestCompare()};
  EXPECT_THAT(fs2, TestKeysAre(-3, -2, -1, 1, 4, 5));

  flat_set<TestKey, TestCompare, TestRepresentation> fs3{keys.begin(), keys.end(), TestAllocator()};
  EXPECT_THAT(fs3, TestKeysAre(-3, -2, -1, 1, 4, 5));

  flat_set<TestKey, TestCompare, TestRepresentation> fs4{keys.begin(), keys.end()};
  EXPECT_THAT(fs4, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, ConstructWithInitializerList) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, AssignInitializerList) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs;
  fs = {-2, -3, 4, -1, -2, 1, 5, -3};
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, Deduplication) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{-3, -2, -1, 1, 4, 5};
  EXPECT_EQ(fs1, fs2);
}

TEST(FlatSetTest, CompareEqual) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{-2, -3, 4, -1, -2, 1, 5, -3};
  EXPECT_TRUE(fs1 == fs2);
  EXPECT_FALSE(fs1 != fs2);
  EXPECT_FALSE(fs1 < fs2);
  EXPECT_TRUE(fs1 <= fs2);
  EXPECT_FALSE(fs1 > fs2);
  EXPECT_TRUE(fs1 >= fs2);
}

TEST(FlatSetTest, CompareLHSLessThanRHS) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{-3, 4, -1, 1, 5, -3};
  EXPECT_FALSE(fs1 == fs2);
  EXPECT_TRUE(fs1 != fs2);
  EXPECT_TRUE(fs1 < fs2);
  EXPECT_TRUE(fs1 <= fs2);
  EXPECT_FALSE(fs1 > fs2);
  EXPECT_FALSE(fs1 >= fs2);
}

TEST(FlatSetTest, CompareLHSGreaterThanRHS) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-3, 4, -1, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{-2, -3, 4, -1, -2, 1, 5, -3};
  EXPECT_FALSE(fs1 == fs2);
  EXPECT_TRUE(fs1 != fs2);
  EXPECT_FALSE(fs1 < fs2);
  EXPECT_FALSE(fs1 <= fs2);
  EXPECT_TRUE(fs1 > fs2);
  EXPECT_TRUE(fs1 >= fs2);
}

TEST(FlatSetTest, CopyConstruct) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{fs1};
  EXPECT_THAT(fs2, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, Copy) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2;
  fs2 = fs1;
  EXPECT_THAT(fs2, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, MoveConstruct) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2{std::move(fs1)};
  EXPECT_THAT(fs1, TestKeysAre());
  EXPECT_THAT(fs2, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, Move) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs1{-2, -3, 4, -1, -2, 1, 5, -3};
  flat_set<TestKey, TestCompare, TestRepresentation> fs2;
  fs2 = std::move(fs1);
  EXPECT_THAT(fs1, TestKeysAre());
  EXPECT_THAT(fs2, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, Empty) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs;
  ASSERT_THAT(fs, TestKeysAre());
  EXPECT_TRUE(fs.empty());
  EXPECT_EQ(fs.size(), 0);
}

TEST(FlatSetTest, NotEmpty) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  EXPECT_FALSE(fs.empty());
  EXPECT_EQ(fs.size(), 6);
}

TEST(FlatSetTest, Clear) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  fs.clear();
  EXPECT_THAT(fs, TestKeysAre());
  EXPECT_TRUE(fs.empty());
  EXPECT_EQ(fs.size(), 0);
}

TEST(FlatSetTest, Insert) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  TestKey const value{6};
  auto const [it, inserted] = fs.insert(value);
  EXPECT_EQ(value, *it);
  EXPECT_TRUE(inserted);
}

TEST(FlatSetTest, MoveInsert) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  TestKey value{6};
  auto const [it, inserted] = fs.insert(std::move(value));
  EXPECT_EQ(it->field, 6);
  EXPECT_TRUE(inserted);
}

TEST(FlatSetTest, InsertCollision) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  TestKey const value{5};
  auto const [it, inserted] = fs.insert(value);
  EXPECT_EQ(value, *it);
  EXPECT_FALSE(inserted);
}

TEST(FlatSetTest, MoveInsertCollision) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  TestKey value{5};
  auto const [it, inserted] = fs.insert(std::move(value));
  EXPECT_EQ(it->field, 5);
  EXPECT_FALSE(inserted);
}

TEST(FlatSetTest, InsertFromIterators) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1};
  std::vector<TestKey> v{-2, 1, 5, -3};
  fs.insert(v.begin(), v.end());
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, InsertFromInitializerList) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1};
  fs.insert({-2, 1, 5, -3});
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, InsertNode) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  auto node1 = fs.extract(TestKey{1});
  auto const [it, inserted, node2] = fs.insert(std::move(node1));
  EXPECT_EQ(it->field, 1);
  EXPECT_TRUE(inserted);
  EXPECT_TRUE(node2.empty());
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, InsertNodeCollision) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{-2, -3, 4, -1, -2, 1, 5, -3};
  auto node1 = fs.extract(TestKey{1});
  fs.insert(1);
  auto const [it, inserted, node2] = fs.insert(std::move(node1));
  EXPECT_EQ(it->field, 1);
  EXPECT_FALSE(inserted);
  EXPECT_FALSE(node2.empty());
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

TEST(FlatSetTest, InsertEmptyNode) {
  using flat_set = flat_set<TestKey, TestCompare, TestRepresentation>;
  flat_set fs{-2, -3, 4, -1, -2, 1, 5, -3};
  typename flat_set::node_type node1;
  EXPECT_TRUE(node1.empty());
  EXPECT_TRUE(!node1);
  auto const [it, inserted, node2] = fs.insert(std::move(node1));
  EXPECT_EQ(it, fs.end());
  EXPECT_FALSE(inserted);
  EXPECT_TRUE(node2.empty());
  EXPECT_THAT(fs, TestKeysAre(-3, -2, -1, 1, 4, 5));
}

// TODO

}  // namespace
