#include "common/reffed_ptr.h"

#include <utility>

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

class Derived : public RefCounted {
 public:
  explicit Derived() : field_(0) {}
  explicit Derived(int const field) : field_(field) {}

  int field() const { return field_; }

 private:
  int field_;
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
  reffed_ptr<RefCounted> ptr{&rc};
  EXPECT_EQ(ptr.get(), &rc);
  EXPECT_TRUE(ptr.operator bool());
  EXPECT_EQ(rc.ref_count(), 1);
}

TEST(ReffedPtrTest, CopyConstructor) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2{ptr1};
  EXPECT_EQ(rc.ref_count(), 2);
}

TEST(ReffedPtrTest, AssignableCopyConstructor) {
  Derived rc;
  reffed_ptr<Derived> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2{ptr1};
  EXPECT_EQ(rc.ref_count(), 2);
}

TEST(ReffedPtrTest, MoveConstructor) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2{std::move(ptr1)};
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 1);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_FALSE(ptr1.operator bool());
}

TEST(ReffedPtrTest, AssignableMoveConstructor) {
  Derived rc;
  reffed_ptr<Derived> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2{std::move(ptr1)};
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 1);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_FALSE(ptr1.operator bool());
}

TEST(ReffedPtrTest, Destructor) {
  RefCounted rc;
  {
    reffed_ptr<RefCounted> ptr{&rc};
    ASSERT_EQ(ptr.get(), &rc);
    ASSERT_TRUE(ptr.operator bool());
    ASSERT_EQ(rc.ref_count(), 1);
  }
  EXPECT_EQ(rc.ref_count(), 0);
}

TEST(ReffedPtrTest, Destructors) {
  RefCounted rc;
  {
    reffed_ptr<RefCounted> ptr1{&rc};
    ASSERT_EQ(ptr1.get(), &rc);
    ASSERT_TRUE(ptr1.operator bool());
    ASSERT_EQ(rc.ref_count(), 1);
    {
      reffed_ptr<RefCounted> ptr2{ptr1};
      ASSERT_EQ(ptr2.get(), &rc);
      ASSERT_TRUE(ptr2.operator bool());
      EXPECT_EQ(rc.ref_count(), 2);
    }
    EXPECT_EQ(rc.ref_count(), 1);
  }
  EXPECT_EQ(rc.ref_count(), 0);
}

TEST(ReffedPtrTest, CopyAssignmentOperator) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2;
  ptr2 = ptr1;
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 2);
}

TEST(ReffedPtrTest, AssignableCopyAssignmentOperator) {
  Derived rc;
  reffed_ptr<Derived> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2;
  ptr2 = ptr1;
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 2);
}

TEST(ReffedPtrTest, MoveOperator) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2;
  ptr2 = std::move(ptr1);
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 1);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_FALSE(ptr1.operator bool());
}

TEST(ReffedPtrTest, AssignableMoveOperator) {
  Derived rc;
  reffed_ptr<Derived> ptr1{&rc};
  reffed_ptr<RefCounted> ptr2;
  ptr2 = std::move(ptr1);
  EXPECT_EQ(ptr2.get(), &rc);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc.ref_count(), 1);
  EXPECT_EQ(ptr1.get(), nullptr);
  EXPECT_FALSE(ptr1.operator bool());
}

TEST(ReffedPtrTest, Release) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr{&rc};
  auto const released = ptr.release();
  EXPECT_EQ(released, &rc);
  EXPECT_EQ(rc.ref_count(), 1);
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(ptr.operator bool());
}

TEST(ReffedPtrTest, Reset) {
  RefCounted rc;
  reffed_ptr<RefCounted> ptr{&rc};
  ptr.reset();
  EXPECT_EQ(ptr.get(), nullptr);
  EXPECT_FALSE(ptr.operator bool());
  EXPECT_EQ(rc.ref_count(), 0);
}

TEST(ReffedPtrTest, ResetWith) {
  RefCounted rc1;
  reffed_ptr<RefCounted> ptr{&rc1};
  Derived rc2;
  ptr.reset(&rc2);
  EXPECT_EQ(ptr.get(), &rc2);
  EXPECT_TRUE(ptr.operator bool());
  EXPECT_EQ(rc1.ref_count(), 0);
  EXPECT_EQ(rc2.ref_count(), 1);
}

TEST(ReffedPtrTest, Swap) {
  RefCounted rc1;
  reffed_ptr<RefCounted> ptr1{&rc1};
  RefCounted rc2;
  reffed_ptr<RefCounted> ptr2{&rc2};
  ptr1.swap(ptr2);
  EXPECT_EQ(ptr1.get(), &rc2);
  EXPECT_TRUE(ptr1.operator bool());
  EXPECT_EQ(ptr2.get(), &rc1);
  EXPECT_TRUE(ptr2.operator bool());
  EXPECT_EQ(rc1.ref_count(), 1);
  EXPECT_EQ(rc2.ref_count(), 1);
}

TEST(ReffedPtrTest, Dereference) {
  Derived rc{42};
  reffed_ptr<Derived> ptr{&rc};
  EXPECT_EQ((*ptr).field(), 42);
  EXPECT_EQ(ptr->field(), 42);
}

TEST(ReffedPtrTest, ComparisonOperators) {
  RefCounted rc1;
  reffed_ptr<RefCounted> ptr1{&rc1};
  reffed_ptr<RefCounted> ptr2{&rc1};
  RefCounted rc2;
  reffed_ptr<RefCounted> ptr3{&rc2};
  EXPECT_TRUE(ptr1 == ptr2);
  EXPECT_FALSE(ptr1 != ptr2);
  EXPECT_FALSE(ptr1 == ptr3);
  EXPECT_TRUE(ptr1 != ptr3);
  EXPECT_FALSE(ptr2 == ptr3);
  EXPECT_TRUE(ptr2 != ptr3);
}

}  // namespace
