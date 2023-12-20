#include "common/fingerprint.h"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

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

TEST(FungerprintTest, Pointers) {
  std::string const s1 = "foo";
  std::string const s2 = "bar";
  std::string const* const p = nullptr;
  int constexpr i = 42;
  EXPECT_EQ(FingerprintOf(&s1), FingerprintOf(&s1));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(nullptr));
  EXPECT_EQ(FingerprintOf(p), FingerprintOf(nullptr));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(&s2));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(&i));
}

TEST(FingerprintTest, Tuples) {
  std::string const s = "foobar";
  int constexpr i = 42;
  bool constexpr b = true;
  double constexpr f = 3.14;
  EXPECT_EQ(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(s, i, b, f)));
  EXPECT_NE(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(i, s, b, f)));
  EXPECT_NE(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(s, i, b)));
}

TEST(FungerprintTest, Optionals) {
  std::optional<std::string> const s1 = "foo";
  std::optional<std::string> const s2 = "bar";
  std::optional<std::string> const s3 = std::nullopt;
  std::optional<int> const i = 42;
  EXPECT_EQ(FingerprintOf(s1), FingerprintOf(s1));
  EXPECT_NE(FingerprintOf(s1), FingerprintOf(std::nullopt));
  EXPECT_EQ(FingerprintOf(s3), FingerprintOf(std::nullopt));
  EXPECT_NE(FingerprintOf(s1), FingerprintOf(s2));
  EXPECT_NE(FingerprintOf(s1), FingerprintOf(i));
}

// TODO

}  // namespace
