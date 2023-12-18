#include "common/overridable.h"

#include <string>
#include <string_view>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::testing::_;
using ::tsdb2::common::Overridable;
using ::tsdb2::common::ScopedOverride;

class TestClass {
 public:
  explicit TestClass(std::string_view const label) : label_(label) {}

  std::string_view label() const { return label_; }

 private:
  std::string const label_;
};

TEST(OverridableTest, NotOverridden) {
  Overridable<TestClass> instance{"foo"};
  EXPECT_EQ(instance->label(), "foo");
}

TEST(OverridableTest, Overridden) {
  Overridable<TestClass> instance{"foo"};
  instance.Override("bar");
  EXPECT_EQ(instance->label(), "bar");
}

TEST(OverridableTest, OverriddenAgain) {
  Overridable<TestClass> instance{"foo"};
  instance.Override("bar");
  instance.Override("baz");
  EXPECT_EQ(instance->label(), "baz");
}

TEST(OverridableTest, Restored) {
  Overridable<TestClass> instance{"foo"};
  instance.Override("bar");
  instance.Restore();
  EXPECT_EQ(instance->label(), "foo");
}

TEST(OverridableTest, ScopedOverridable) {
  Overridable<TestClass> instance{"foo"};
  {
    ScopedOverride so{&instance, "bar"};
    EXPECT_EQ(instance->label(), "bar");
  }
  EXPECT_EQ(instance->label(), "foo");
}

TEST(OverridableDeathTest, NestedScopedOverridable) {
  Overridable<TestClass> instance{"foo"};
  ScopedOverride so{&instance, "bar"};
  EXPECT_DEATH(ScopedOverride(&instance, "baz"), _);
}

TEST(OverridableTest, MoveConstructScopedOverridable) {
  Overridable<TestClass> instance{"foo"};
  ScopedOverride so1{&instance, "bar"};
  {
    ScopedOverride so2{std::move(so1)};
    EXPECT_EQ(instance->label(), "bar");
  }
  EXPECT_EQ(instance->label(), "foo");
}

TEST(OverridableTest, MoveScopedOverridable) {
  Overridable<TestClass> instance{"foo"};
  ScopedOverride so1{&instance, "bar"};
  {
    Overridable<TestClass> dummy{""};
    ScopedOverride so2{&dummy, ""};
    so2 = std::move(so1);
    EXPECT_EQ(instance->label(), "bar");
  }
  EXPECT_EQ(instance->label(), "foo");
}

}  // namespace
