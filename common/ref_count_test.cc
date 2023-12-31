#include "common/ref_count.h"

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::RefCount;
using ::tsdb2::common::RefCounted;
using ::tsdb2::common::SimpleRefCounted;

TEST(RefCountTest, RefUnref) {
  RefCount rc;
  rc.Ref();
  EXPECT_TRUE(rc.Unref());
}

TEST(RefCountTest, RefRefUnrefUnref) {
  RefCount rc;
  rc.Ref();
  rc.Ref();
  EXPECT_FALSE(rc.Unref());
  EXPECT_TRUE(rc.Unref());
}

class TestRefCounted : public RefCounted {
 public:
  explicit TestRefCounted(bool *const flag) : flag_(flag) {}

 protected:
  void OnLastUnref() override { *flag_ = true; }

 private:
  bool *const flag_;
};

class RefCountedTest : public ::testing::Test {
 protected:
  bool flag_ = false;
  TestRefCounted rc_{&flag_};
};

TEST_F(RefCountedTest, Initial) { EXPECT_FALSE(flag_); }

TEST_F(RefCountedTest, RefUnref) {
  rc_.Ref();
  EXPECT_FALSE(flag_);
  rc_.Unref();
  EXPECT_TRUE(flag_);
}

TEST_F(RefCountedTest, RefRefUnrefUnref) {
  rc_.Ref();
  rc_.Ref();
  EXPECT_FALSE(flag_);
  rc_.Unref();
  EXPECT_FALSE(flag_);
  rc_.Unref();
  EXPECT_TRUE(flag_);
}

class TestSimpleRefCounted : public SimpleRefCounted {
 public:
  explicit TestSimpleRefCounted(bool *const flag) : flag_(flag) {}
  ~TestSimpleRefCounted() { *flag_ = true; }

 private:
  bool *const flag_;
};

class SimpleRefCountedTest : public ::testing::Test {
 protected:
  bool flag_ = false;
};

TEST_F(SimpleRefCountedTest, RefUnref) {
  {
    auto rc = std::make_unique<TestSimpleRefCounted>(&flag_);
    EXPECT_FALSE(flag_);
  }
  EXPECT_TRUE(flag_);
}

TEST_F(SimpleRefCountedTest, RefRefUnrefUnref) {
  {
    auto rc1 = std::make_shared<TestSimpleRefCounted>(&flag_);
    {
      auto rc2 = rc1;
      EXPECT_FALSE(flag_);
    }
    EXPECT_FALSE(flag_);
  }
  EXPECT_TRUE(flag_);
}

}  // namespace
