#ifndef __TSDB2_COMMON_OVERRIDABLE_H__
#define __TSDB2_COMMON_OVERRIDABLE_H__

#include <atomic>
#include <optional>
#include <utility>

#include "absl/base/optimization.h"
#include "absl/log/check.h"
#include "absl/synchronization/mutex.h"

namespace tsdb2 {
namespace common {

// Allows overriding a value of type T for testing purposes, e.g. replacing with a mock
// implementation.
//
// Note that `Overridable` makes `T` non-copyable and non-movable because it encapsulates an atomic
// and a mutex, and also because `ScopedOverride` objects will retain pointers to it.
//
// The internal mutex is used only to prevent concurrent constructions from `Override()` calls, but
// retrieving the regular value in the absence of an override does not block and is very fast.
template <typename T>
class Overridable {
 public:
  template <typename... Args>
  explicit Overridable(Args&&... args) : value_(std::forward<Args>(args)...) {}

  // TEST ONLY: replace the wrapped value with a different one.
  template <typename... Args>
  void Override(Args&&... args) ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    override_.emplace(std::forward<Args>(args)...);
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: replace the wrapped value with a different one, checkfailing if a different override
  // is already in place.
  template <typename... Args>
  void OverrideOrDie(Args&&... args) ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    CHECK(!override_);
    override_.emplace(std::forward<Args>(args)...);
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: restore the original value and destroy the override, if any.
  void Restore() ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    override_.reset();
    overridden_.store(false, std::memory_order_release);
  }

  // Retrieve the wrapped value.
  T* Get() const ABSL_LOCKS_EXCLUDED(mutex_) {
    if (ABSL_PREDICT_FALSE(overridden_.load(std::memory_order_acquire))) {
      absl::MutexLock lock{&mutex_};
      if (override_) {
        return const_cast<T*>(&*override_);
      } else {
        return &value_;
      }
    } else {
      return &value_;
    }
  }

  T* operator->() const { return Get(); }
  T& operator*() const { return *(Get()); }

 private:
  Overridable(Overridable const&) = delete;
  Overridable& operator=(Overridable const&) = delete;
  Overridable(Overridable&&) = delete;
  Overridable& operator=(Overridable&&) = delete;

  T mutable value_;

  std::atomic<bool> overridden_{false};
  absl::Mutex mutable mutex_;
  std::optional<T> override_ ABSL_GUARDED_BY(mutex_) = std::nullopt;
};

// Scoped object to call `Override` on construction and `Restore` on destruction on a given
// `Overridable`.
//
// `ScopedOverride` is movable but not copyable.
//
// WARNING: `ScopedOverride` does NOT support nesting. It uses `Overridable::OverrideOrDie`, so it
// will check-fail if two instances are nested in scope.
template <typename T>
class ScopedOverride {
 public:
  template <typename... Args>
  explicit ScopedOverride(Overridable<T>* const overridable, Args&&... args)
      : overridable_(overridable) {
    overridable_->OverrideOrDie(std::forward<Args>(args)...);
  }

  ScopedOverride(ScopedOverride&& other) noexcept : overridable_(other.overridable_) {
    other.overridable_ = nullptr;
  }

  ScopedOverride& operator=(ScopedOverride&& other) noexcept {
    MaybeRestore();
    overridable_ = other.overridable_;
    other.overridable_ = nullptr;
    return *this;
  }

  ~ScopedOverride() { MaybeRestore(); }

 private:
  ScopedOverride(ScopedOverride const&) = delete;
  ScopedOverride& operator=(ScopedOverride const&) = delete;

  void MaybeRestore() {
    if (overridable_) {
      overridable_->Restore();
    }
  }

  Overridable<T>* overridable_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_OVERRIDABLE_H__
