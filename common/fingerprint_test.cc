#include "common/fingerprint.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

#include "gtest/gtest.h"

namespace foo {

class TestClass {
 public:
  explicit TestClass(std::string_view const x, int const y, bool const z) : x_(x), y_(y), z_(z) {}

  template <typename State>
  friend State Tsdb2FingerprintValue(State state, TestClass const& value) {
    return State::Combine(std::move(state), value.x_, value.y_, value.z_);
  }

 private:
  std::string x_;
  int y_;
  bool z_;
};

}  // namespace foo

namespace {

using ::tsdb2::common::FingerprintOf;

auto constexpr kInt8Fingerprint = FingerprintOf(int8_t{42});
auto constexpr kInt16Fingerprint = FingerprintOf(int16_t{42});
auto constexpr kInt32Fingerprint = FingerprintOf(int32_t{42});
auto constexpr kUint8Fingerprint = FingerprintOf(uint8_t{42});
auto constexpr kUint16Fingerprint = FingerprintOf(uint16_t{42});
auto constexpr kUint32Fingerprint = FingerprintOf(uint32_t{42});

auto constexpr kFloatFingerprint = FingerprintOf(3.141f);

auto constexpr kFalseFingerprint = FingerprintOf(false);
auto constexpr kTrueFingerprint = FingerprintOf(true);

auto constexpr kNullPointerFingerprint = FingerprintOf(nullptr);

int const kIntValue = 42;
auto constexpr kIntPointerFingerprint = FingerprintOf(&kIntValue);

float constexpr kFloatValue = 3.14;
auto constexpr kFloatPointerFingerprint = FingerprintOf(&kFloatValue);

bool const kBoolValue = true;
auto constexpr kBoolPointerFingerprint = FingerprintOf(&kBoolValue);

auto constexpr kTupleFingerprint = FingerprintOf(std::make_tuple(42, 3.14f, true));

TEST(FingerprintTest, Integrals) {
  EXPECT_EQ(FingerprintOf(int8_t{42}), FingerprintOf(int8_t{42}));
  EXPECT_NE(FingerprintOf(int8_t{42}), FingerprintOf(int8_t{43}));
  EXPECT_EQ(FingerprintOf(int8_t{42}), kInt8Fingerprint);
  EXPECT_EQ(FingerprintOf(int16_t{42}), FingerprintOf(int16_t{42}));
  EXPECT_NE(FingerprintOf(int16_t{42}), FingerprintOf(int16_t{43}));
  EXPECT_EQ(FingerprintOf(int16_t{42}), kInt16Fingerprint);
  EXPECT_EQ(FingerprintOf(int32_t{42}), FingerprintOf(int32_t{42}));
  EXPECT_NE(FingerprintOf(int32_t{42}), FingerprintOf(int32_t{43}));
  EXPECT_EQ(FingerprintOf(int32_t{42}), kInt32Fingerprint);
  EXPECT_EQ(FingerprintOf(int64_t{42}), FingerprintOf(int64_t{42}));
  EXPECT_NE(FingerprintOf(int64_t{42}), FingerprintOf(int64_t{43}));
  EXPECT_EQ(FingerprintOf(uint8_t{42}), FingerprintOf(uint8_t{42}));
  EXPECT_NE(FingerprintOf(uint8_t{42}), FingerprintOf(uint8_t{43}));
  EXPECT_EQ(FingerprintOf(uint8_t{42}), kUint8Fingerprint);
  EXPECT_EQ(FingerprintOf(uint16_t{42}), FingerprintOf(uint16_t{42}));
  EXPECT_NE(FingerprintOf(uint16_t{42}), FingerprintOf(uint16_t{43}));
  EXPECT_EQ(FingerprintOf(uint16_t{42}), kUint16Fingerprint);
  EXPECT_EQ(FingerprintOf(uint32_t{42}), FingerprintOf(uint32_t{42}));
  EXPECT_NE(FingerprintOf(uint32_t{42}), FingerprintOf(uint32_t{43}));
  EXPECT_EQ(FingerprintOf(uint32_t{42}), kUint32Fingerprint);
  EXPECT_EQ(FingerprintOf(uint64_t{42}), FingerprintOf(uint64_t{42}));
  EXPECT_NE(FingerprintOf(uint64_t{42}), FingerprintOf(uint64_t{43}));
}

TEST(FingerprintTest, Floats) {
  float constexpr pi1 = 3.141;
  double constexpr pi2 = 3.141;
  long double constexpr pi3 = 3.141;
  float constexpr e1 = 2.718;
  double constexpr e2 = 2.718;
  long double constexpr e3 = 2.718;
  EXPECT_EQ(FingerprintOf(pi1), FingerprintOf(pi1));
  EXPECT_NE(FingerprintOf(pi1), FingerprintOf(e1));
  EXPECT_EQ(FingerprintOf(pi1), kFloatFingerprint);
  EXPECT_EQ(FingerprintOf(pi2), FingerprintOf(pi2));
  EXPECT_NE(FingerprintOf(pi2), FingerprintOf(e2));
  EXPECT_EQ(FingerprintOf(pi3), FingerprintOf(pi3));
  EXPECT_NE(FingerprintOf(pi3), FingerprintOf(e3));
}

TEST(FingerprintTest, Booleans) {
  EXPECT_EQ(FingerprintOf(true), FingerprintOf(true));
  EXPECT_EQ(FingerprintOf(true), kTrueFingerprint);
  EXPECT_EQ(FingerprintOf(false), kFalseFingerprint);
  EXPECT_NE(FingerprintOf(true), FingerprintOf(false));
}

TEST(FingerprintTest, Strings) {
  EXPECT_EQ(FingerprintOf(std::string("lorem ipsum")), FingerprintOf(std::string("lorem ipsum")));
  EXPECT_NE(FingerprintOf(std::string("lorem ipsum")), FingerprintOf(std::string("dolor amet")));
  EXPECT_EQ(FingerprintOf(std::string_view("lorem ipsum")),
            FingerprintOf(std::string_view("lorem ipsum")));
  EXPECT_NE(FingerprintOf(std::string_view("lorem ipsum")),
            FingerprintOf(std::string_view("dolor amet")));
  EXPECT_EQ(FingerprintOf("lorem ipsum"), FingerprintOf("lorem ipsum"));
  EXPECT_NE(FingerprintOf("lorem ipsum"), FingerprintOf("dolor amet"));
}

TEST(FingerprintTest, Pointers) {
  std::string const s1 = "foo";
  std::string const s2 = "bar";
  std::string const* const p = nullptr;
  int constexpr i = 42;
  float constexpr f = 3.14;
  bool constexpr b = true;
  EXPECT_EQ(FingerprintOf(&s1), FingerprintOf(&s1));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(nullptr));
  EXPECT_EQ(FingerprintOf(p), FingerprintOf(nullptr));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(&s2));
  EXPECT_NE(FingerprintOf(&s1), FingerprintOf(&i));
  EXPECT_EQ(FingerprintOf(p), kNullPointerFingerprint);
  EXPECT_EQ(FingerprintOf(&i), kIntPointerFingerprint);
  EXPECT_EQ(FingerprintOf(&f), kFloatPointerFingerprint);
  EXPECT_EQ(FingerprintOf(&b), kBoolPointerFingerprint);
}

TEST(FingerprintTest, Tuples) {
  std::string const s = "foobar";
  int constexpr i = 42;
  bool constexpr b = true;
  float constexpr f = 3.14;
  EXPECT_EQ(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(s, i, b, f)));
  EXPECT_NE(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(i, s, b, f)));
  EXPECT_NE(FingerprintOf(std::tie(s, i, b, f)), FingerprintOf(std::tie(s, i, b)));
  EXPECT_EQ(FingerprintOf(std::make_tuple(i, f, b)), kTupleFingerprint);
}

TEST(FingerprintTest, Optionals) {
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

TEST(FingerprintTest, Variant) {
  using Variant = std::variant<std::string, int, bool>;
  Variant const v1 = std::string("foo");
  Variant const v2 = 123;
  Variant const v3 = true;
  EXPECT_EQ(FingerprintOf(v1), FingerprintOf(v1));
  EXPECT_NE(FingerprintOf(v1), FingerprintOf(v2));
  EXPECT_NE(FingerprintOf(v1), FingerprintOf(v3));
}

TEST(FingerprintTest, CustomObject) {
  ::foo::TestClass value{"foo", 42, true};
  EXPECT_EQ(FingerprintOf(value), FingerprintOf(::foo::TestClass("foo", 42, true)));
  EXPECT_NE(FingerprintOf(value), FingerprintOf(::foo::TestClass("bar", 43, false)));
  EXPECT_EQ(FingerprintOf(value), FingerprintOf(std::make_tuple(std::string("foo"), 42, true)));
  EXPECT_EQ(FingerprintOf(value), FingerprintOf(std::make_tuple("foo", 42, true)));
}

}  // namespace
