#ifndef __TSDB2_COMMON_MOCK_CLOCK_H__
#define __TSDB2_COMMON_MOCK_CLOCK_H__

#include <algorithm>
#include <cstddef>
#include <utility>

namespace tsdb2 {
namespace common {

// `reffed_ptr` is a smart pointer class that behaves almost identically to `std::shared_ptr` except
// that it defers all reference counting to the wrapped object rather than implementing its own. As
// such it has an intrusive API requiring that the wrapped object provides two methods called `Ref`
// and `Unref`.
//
// This has three benefits:
//
// * Unlike `std::shared_ptr` there's no risk of keeping multiple separate reference counts because
//   the target containing the reference count value is managed by the wrapped object rather than by
//   `reffed_ptr`.
// * `reffed_ptr` doesn't need to allocate a separate memory block for the target / reference count.
// * `reffed_ptr` allows implementing custom reference counting schemes, e.g. the destructor of the
//   wrapped object may block until the reference count drops to zero (see `BlockingRefCounted`).
template <typename T>
class reffed_ptr {
 public:
  constexpr reffed_ptr() : ptr_(nullptr) {}

  constexpr reffed_ptr(std::nullptr_t) : ptr_(nullptr) {}

  template <typename U>
  explicit reffed_ptr(U* const ptr) : ptr_(ptr) {
    MaybeRef();
  }

  reffed_ptr(reffed_ptr const& other) : ptr_(other.ptr_) { MaybeRef(); }

  template <typename U>
  reffed_ptr(reffed_ptr<U> const& other) : ptr_(other.ptr_) {
    MaybeRef();
  }

  reffed_ptr(reffed_ptr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

  template <typename U>
  reffed_ptr(reffed_ptr<U>&& other) noexcept : ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  ~reffed_ptr() { MaybeUnref(); }

  reffed_ptr& operator=(reffed_ptr const& other) {
    reset(other.ptr_);
    return *this;
  }

  template <typename U>
  reffed_ptr& operator=(reffed_ptr<U> const& other) {
    reset(other.ptr_);
    return *this;
  }

  reffed_ptr& operator=(reffed_ptr&& other) noexcept {
    MaybeUnref();
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }

  template <typename U>
  reffed_ptr& operator=(reffed_ptr<U>&& other) noexcept {
    MaybeUnref();
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
    return *this;
  }

  void reset() {
    MaybeUnref();
    ptr_ = nullptr;
  }

  template <typename U>
  void reset(U* const ptr) {
    MaybeUnref();
    ptr_ = ptr;
    MaybeRef();
  }

  void swap(reffed_ptr& other) { std::swap(ptr_, other.ptr_); }

  T* get() const { return ptr_; }

  T& operator*() const { return *ptr_; }
  T* operator->() const { return ptr_; }

  explicit operator bool() const { return !!ptr_; }

  template <typename U, typename V>
  friend bool operator==(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ == rhs.ptr_;
  }

  template <typename U, typename V>
  friend bool operator!=(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ != rhs.ptr_;
  }

  template <typename U, typename V>
  friend bool operator<(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ < rhs.ptr_;
  }

  template <typename U, typename V>
  friend bool operator>(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ > rhs.ptr_;
  }

  template <typename U, typename V>
  friend bool operator<=(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ <= rhs.ptr_;
  }

  template <typename U, typename V>
  friend bool operator>=(reffed_ptr<U> const& lhs, reffed_ptr<V> const& rhs) {
    return lhs.ptr_ >= rhs.ptr_;
  }

 private:
  void MaybeRef() {
    if (ptr_) {
      ptr_->Ref();
    }
  }

  void MaybeUnref() {
    if (ptr_) {
      ptr_->Unref();
    }
  }

  T* ptr_;
};

// Constructs a new object of type T and wraps it in a `reffed_ptr<T>`.
template <typename T, typename... Args>
reffed_ptr<T> MakeReffed(Args&&... args) {
  return reffed_ptr<T>(new T(std::forward<Args>(args)...));
}

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_MOCK_CLOCK_H__