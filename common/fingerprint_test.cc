#include "common/fingerprint.h"

#include <string>
#include <string_view>

#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::FingerprintOf;

TEST(FingerprintTest, Integrals) {
  EXPECT_EQ(FingerprintOf(42), FingerprintOf(42));
  EXPECT_NE(FingerprintOf(42), FingerprintOf(43));
}

TEST(FingerprintTest, Floats) {
  double constexpr pi = 3.141;
  double constexpr e = 2.73;
  EXPECT_EQ(FingerprintOf(pi), FingerprintOf(pi));
  EXPECT_NE(FingerprintOf(pi), FingerprintOf(e));
}

TEST(FingerprintTest, Booleans) {
  EXPECT_EQ(FingerprintOf(true), FingerprintOf(true));
  EXPECT_NE(FingerprintOf(true), FingerprintOf(false));
}

TEST(FingerprintTest, Strings) {
  EXPECT_EQ(FingerprintOf(std::string("lorem ipsum")), FingerprintOf(std::string("lorem ipsum")));
  EXPECT_NE(FingerprintOf(std::string("lorem ipsum")), FingerprintOf(std::string("dolor amet")));
}

TEST(FingerprintTest, StringViews) {
  EXPECT_EQ(FingerprintOf(std::string_view("lorem ipsum")),
            FingerprintOf(std::string_view("lorem ipsum")));
  EXPECT_NE(FingerprintOf(std::string_view("lorem ipsum")),
            FingerprintOf(std::string_view("dolor amet")));
}

TEST(FingerprintTest, CharacterArrays) {
  EXPECT_EQ(FingerprintOf("lorem ipsum"), FingerprintOf("lorem ipsum"));
  EXPECT_NE(FingerprintOf("lorem ipsum"), FingerprintOf("dolor amet"));
}

// TODO

}  // namespace
