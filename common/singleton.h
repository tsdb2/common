#ifndef __TSDB2_COMMON_SINGLETON_H__
#define __TSDB2_COMMON_SINGLETON_H__

#include <atomic>
#include <utility>

#include "absl/base/call_once.h"
#include "absl/base/optimization.h"
#include "absl/functional/any_invocable.h"
#include "absl/log/check.h"
#include "absl/synchronization/mutex.h"

namespace tsdb2 {
namespace common {

template <typename T>
class Singleton {
 public:
  template <typename... Args>
  explicit Singleton(Args&&... args) : construct_([=] { Construct(args...); }) {}

  // TEST ONLY: replace the wrapped value with a different one.
  void Override(T* const override) ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    override_ = override;
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: replace the wrapped value with a different one, checkfailing if a different override
  // is already in place.
  void OverrideOrDie(T* const override) ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    CHECK(!override_);
    override_ = override;
    overridden_.store(true, std::memory_order_release);
  }

  // TEST ONLY: restore the original value and destroy the override, if any.
  void Restore() ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    override_ = nullptr;
    overridden_.store(false, std::memory_order_release);
  }

  // Retrieve the wrapped value.
  T* Get() const ABSL_LOCKS_EXCLUDED(mutex_) {
    if (ABSL_PREDICT_FALSE(overridden_.load(std::memory_order_relaxed))) {
      absl::MutexLock lock{&mutex_};
      if (override_) {
        return override_;
      }
    }
    absl::call_once(once_, construct_);
    return reinterpret_cast<T*>(storage_);
  }

  T* operator->() const { return Get(); }
  T& operator*() const { return *Get(); }

 private:
  Singleton(Singleton const&) = delete;
  Singleton& operator=(Singleton const&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;

  template <typename... Args>
  void Construct(Args&&... args) {
    new (storage_) T(std::forward<Args>(args)...);
  }

  alignas(T) char mutable storage_[sizeof(T)];

  absl::once_flag mutable once_;
  absl::AnyInvocable<void()> mutable construct_;

  absl::Mutex mutable mutex_;
  std::atomic<bool> overridden_ = false;
  T* override_ ABSL_GUARDED_BY(mutex_) = nullptr;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SINGLETON_H__
