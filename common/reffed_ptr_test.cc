#include "common/reffed_ptr.h"

#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::reffed_ptr;

class RefCounted {
 public:
  void Ref() { ++ref_count_; }
  void Unref() { --ref_count_; }

  intptr_t ref_count() const { return ref_count_; }

 private:
  intptr_t ref_count_ = 0;
};

TEST(ReffedPtrTest, DefaultConstructor) {
  reffed_ptr<RefCounted> ptr;
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(ptr.operator bool());
}

TEST(ReffedPtrTest, NullptrConstructor) {
  reffed_ptr<RefCounted> ptr{nullptr};
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(ptr.operator bool());
}

TEST(ReffedPtrTest, PointerConstructor) {
  RefCounted rc;
  {
    reffed_ptr<RefCounted> ptr{&rc};
    EXPECT_EQ(ptr.get(), &rc);
    EXPECT_TRUE(ptr.operator bool());
    EXPECT_EQ(rc.ref_count(), 1);
  }
  EXPECT_EQ(rc.ref_count(), 0);
}

// TODO

}  // namespace
