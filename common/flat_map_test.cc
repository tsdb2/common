#include "common/flat_map.h"

#include <cstddef>
#include <deque>
#include <string>
#include <type_traits>
#include <utility>

#include "common/flat_container_testing.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::flat_map;
using ::tsdb2::testing::TestCompare;
using ::tsdb2::testing::TestKey;
using ::tsdb2::testing::TestValue;
using ::tsdb2::testing::TestValueRepresentation;

TEST(FlatMapTest, Traits) {
  using flat_map = flat_map<TestKey, std::string, TestCompare, TestValueRepresentation>;
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_type, TestKey>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::mapped_type, std::string>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::value_type, TestValue>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::representation_type, std::deque<TestValue>>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::size_type, size_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::difference_type, ptrdiff_t>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::key_compare, TestCompare>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reference, TestValue&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reference, TestValue const&>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::pointer, TestValue*>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_pointer, TestValue const*>));
  EXPECT_TRUE(
      (std::is_same_v<typename flat_map::iterator, typename TestValueRepresentation::iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_iterator,
                              typename TestValueRepresentation::const_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::reverse_iterator,
                              typename TestValueRepresentation::reverse_iterator>));
  EXPECT_TRUE((std::is_same_v<typename flat_map::const_reverse_iterator,
                              typename TestValueRepresentation::const_reverse_iterator>));
}

TEST(FlatMapTest, Construct) {
  flat_map<TestKey, std::string, TestCompare> fm{
      {-2, "lorem"}, {-3, "ipsum"},      {4, "dolor"},   {-1, "sit"},
      {-2, "amet"},  {1, "consectetur"}, {5, "adpisci"}, {-3, "elit"},
  };
  // TODO
}

}  // namespace
