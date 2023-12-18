#include "common/murmur3.h"

#include <cstdint>
#include <string_view>
#include <utility>

#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MurmurHash3_32;

uint8_t constexpr kText[] = "Lorem ipsum dolor sit amet consectetur adipisci elit.";
size_t constexpr kTextLength = sizeof(kText) - 1;

class MurmurHash3SeededTest : public ::testing::TestWithParam<uint32_t> {};

uint32_t constexpr kSeed = 0x12345678;
uint32_t constexpr kConstexprHash = MurmurHash3_32::Hash(kText, kTextLength, kSeed);

TEST_P(MurmurHash3SeededTest, Hash) {
  MurmurHash3_32 hasher{GetParam()};
  hasher.Add(kText, kTextLength);
  EXPECT_EQ(hasher.Finish(), MurmurHash3_32::Hash(kText, kTextLength, GetParam()));
}

TEST(MurmurHash3Test, DifferentSeeds) {
  EXPECT_NE(MurmurHash3_32::Hash(kText, kTextLength, 12345),
            MurmurHash3_32::Hash(kText, kTextLength, 71104));
}

TEST(MurmurHash3Test, ConstexprHash) {
  MurmurHash3_32 hasher{kSeed};
  hasher.Add(kText, kTextLength);
  EXPECT_EQ(kConstexprHash, hasher.Finish());
}

TEST_P(MurmurHash3SeededTest, SingleAddCall) {
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, kTextLength);
  MurmurHash3_32 hasher2{GetParam()};
  hasher2.Add(kText, kTextLength);
  EXPECT_EQ(hasher1.Finish(), hasher2.Finish());
}

TEST_P(MurmurHash3SeededTest, TwoAddCalls) {
  size_t constexpr first_batch = 10;
  static_assert(kTextLength > first_batch, "please update this test");
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  hasher1.Add(kText + first_batch, kTextLength - first_batch);
  MurmurHash3_32 hasher2{GetParam()};
  hasher2.Add(kText, kTextLength);
  EXPECT_EQ(hasher1.Finish(), hasher2.Finish());
}

TEST_P(MurmurHash3SeededTest, ThreeAddCalls) {
  size_t constexpr first_batch = 9;
  size_t constexpr second_batch = 13;
  static_assert(kTextLength > first_batch + second_batch, "please update this test");
  size_t constexpr third_batch = kTextLength - first_batch - second_batch;
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  hasher1.Add(kText + first_batch, second_batch);
  hasher1.Add(kText + first_batch + second_batch, third_batch);
  MurmurHash3_32 hasher2{GetParam()};
  hasher2.Add(kText, kTextLength);
  EXPECT_EQ(hasher1.Finish(), hasher2.Finish());
}

TEST_P(MurmurHash3SeededTest, MoveConstructor) {
  size_t constexpr first_batch = 10;
  static_assert(kTextLength > first_batch, "please update this test");
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  MurmurHash3_32 hasher2{std::move(hasher1)};
  hasher2.Add(kText + first_batch, kTextLength - first_batch);
  MurmurHash3_32 hasher3{GetParam()};
  hasher3.Add(kText, kTextLength);
  EXPECT_EQ(hasher2.Finish(), hasher3.Finish());
}

TEST_P(MurmurHash3SeededTest, MoveAssignment) {
  size_t constexpr first_batch = 10;
  static_assert(kTextLength > first_batch, "please update this test");
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  MurmurHash3_32 hasher2{0};
  hasher2 = std::move(hasher1);
  hasher2.Add(kText + first_batch, kTextLength - first_batch);
  MurmurHash3_32 hasher3{GetParam()};
  hasher3.Add(kText, kTextLength);
  EXPECT_EQ(hasher2.Finish(), hasher3.Finish());
}

TEST_P(MurmurHash3SeededTest, CopyConstructor) {
  size_t constexpr first_batch = 10;
  static_assert(kTextLength > first_batch, "please update this test");
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  MurmurHash3_32 hasher2{hasher1};
  hasher1.Add(kText + first_batch, kTextLength - first_batch);
  hasher2.Add(kText + first_batch, kTextLength - first_batch);
  EXPECT_EQ(hasher1.Finish(), hasher2.Finish());
}

TEST_P(MurmurHash3SeededTest, CopyAssignment) {
  size_t constexpr first_batch = 10;
  static_assert(kTextLength > first_batch, "please update this test");
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  MurmurHash3_32 hasher2{0};
  hasher2 = hasher1;
  hasher1.Add(kText + first_batch, kTextLength - first_batch);
  hasher2.Add(kText + first_batch, kTextLength - first_batch);
  EXPECT_EQ(hasher1.Finish(), hasher2.Finish());
}

TEST_P(MurmurHash3SeededTest, Branching) {
  size_t constexpr first_batch = 9;
  size_t constexpr second_batch = 13;
  static_assert(kTextLength > first_batch + second_batch, "please update this test");
  size_t constexpr third_batch = kTextLength - first_batch - second_batch;
  MurmurHash3_32 hasher1{GetParam()};
  hasher1.Add(kText, first_batch);
  MurmurHash3_32 hasher2{hasher1};
  hasher1.Add(kText + first_batch, second_batch);
  hasher2.Add(kText + first_batch, third_batch);
  EXPECT_NE(hasher1.Finish(), hasher2.Finish());
}

INSTANTIATE_TEST_SUITE_P(SeededTest, MurmurHash3SeededTest, ::testing::Values(0, 12345, 71104));

}  // namespace
