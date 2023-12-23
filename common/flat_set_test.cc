#include "common/flat_set.h"

#include <cstddef>
#include <deque>
#include <functional>

#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::flat_set;

struct TestKey {
  int field;
};

struct TestCompare {
  bool operator()(TestKey const& lhs, TestKey const& rhs) const { return lhs.field < rhs.field; }
};

struct AnotherKey {
  int field;
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
      (std::is_same_v<typename flat_set::const_iterator, typename flat_set::iterator const>));
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
      (std::is_same_v<typename flat_set::const_iterator, typename flat_set::iterator const>));
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
      (std::is_same_v<typename flat_set::const_iterator, typename flat_set::iterator const>));
}

TEST(FlatSetTest, Construct) { flat_set<TestKey, TestCompare, TestRepresentation>(); }

TEST(FlatSetTest, ConstructWithCompare) {
  flat_set<TestKey, TestCompare, TestRepresentation> fs{TestCompare()};
}

// TODO

}  // namespace
