#ifndef __TSDB2_COMMON_SINGLETON_H__
#define __TSDB2_COMMON_SINGLETON_H__

#include <atomic>
#include <utility>

#include "absl/base/call_once.h"
#include "absl/base/optimization.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/check.h"
#include "absl/synchronization/mutex.h"
#include "common/no_destructor.h"

namespace tsdb2 {
namespace common {

// Holds a singleton instance of a class, providing the following functionalities:
//
//  * lazy construction;
//  * trivial destruction (the destructor of the wrapped object is never executed);
//  * overriding the instance in tests.
//
// Lazy construction and trivial destruction make `Singleton` suitable for use in global scope and
// avoid initialization order fiascos.
//
// `Singleton` is fully thread-safe and is conceptually similar to the localized static
// initialization pattern:
//
//   MyClass GetInstance() {
//     static MyClass *const kInstance = new MyClass();
//     return kInstance;
//   }
//
// However such pattern doesn't allow overriding in tests.
//
// Retrieving the instance wrapped in a `Singleton` is very fast in production, as the check for
// possible overrides only performs a single relaxed atomic load when there's no override.
template <typename T>
class Singleton {
 public:
  template <typename... Args>
  explicit Singleton(Args&&... args) : construct_([=] { Construct(args...); }) {}

  // TEST ONLY: replace the wrapped value with a different one.
  void Override(T* const value) {
    absl::MutexLock lock{mutex_.Get()};
    override_ = value;
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: replace the wrapped value with a different one, checkfailing if a different override
  // is already in place.
  void OverrideOrDie(T* const value) {
    absl::MutexLock lock{mutex_.Get()};
    CHECK(!override_);
    override_ = value;
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: restore the original value and destroy the override, if any.
  void Restore() {
    absl::MutexLock lock{mutex_.Get()};
    override_ = nullptr;
    overridden_.store(false, std::memory_order_release);
  }

  // Retrieves the wrapped value.
  T* Get() { return GetInternal(); }
  T const* Get() const { return GetInternal(); }

  T* operator->() { return Get(); }
  T const* operator->() const { return Get(); }

  T& operator*() { return *Get(); }
  T const& operator*() const { return *Get(); }

 private:
  Singleton(Singleton const&) = delete;
  Singleton& operator=(Singleton const&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;

  template <typename... Args>
  void Construct(Args&&... args) {
    new (storage_) T(std::forward<Args>(args)...);
  }

  T* GetInternal() const {
    if (ABSL_PREDICT_FALSE(overridden_.load(std::memory_order_relaxed))) {
      absl::MutexLock lock{mutex_.Get()};
      if (override_) {
        return override_;
      }
    }
    absl::call_once(once_, *construct_);
    return reinterpret_cast<T*>(storage_);
  }

  alignas(T) char mutable storage_[sizeof(T)];

  absl::once_flag mutable once_;
  NoDestructor<absl::AnyInvocable<void()>> mutable construct_;

  NoDestructor<absl::Mutex> mutable mutex_;
  std::atomic<bool> overridden_ = false;
  T* override_ = nullptr;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SINGLETON_H__
