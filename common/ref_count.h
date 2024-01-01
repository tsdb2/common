#ifndef __TSDB2_COMMON_REF_COUNT_H__
#define __TSDB2_COMMON_REF_COUNT_H__

#include <atomic>

#include "absl/log/check.h"

namespace tsdb2 {
namespace common {

// Implements fast thread-safe reference counting.
//
// The reference count is initialized to 0.
//
// See
// https://www.boost.org/doc/libs/1_84_0/libs/atomic/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_reference_counters
// for an explanation of how the memory barriers work.
class RefCount {
 public:
  // Increments the reference count.
  void Ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  // Decrements the reference count and returns true iff it has reached 0.
  bool Unref() { return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1; }

 private:
  std::atomic<intptr_t> ref_count_ = 0;
};

// Implements a reference-counted object that performs a custom action when the reference count
// reaches 0. The custom action is executed by `OnLastUnref`, which is a pure virtual method. Users
// can inherit this class and implement `OnLastUnref` by e.g. triggering `delete this`.
//
// Note that the reference count is initialized to 0.
class RefCounted {
 public:
  virtual ~RefCounted() { DCHECK_EQ(ref_count_.load(std::memory_order_acquire), 0); }

  // Increments the reference count.
  void Ref() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  // Decrements the reference count and triggers `OnLastUnref` the count reaches 0.
  void Unref() {
    if (ref_count_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      OnLastUnref();
    }
  }

 protected:
  // Invoked by `Unref` when the reference count reaches 0.
  virtual void OnLastUnref() = 0;

 private:
  std::atomic<intptr_t> ref_count_ = 0;
};

// Simple `RefCounted` implementation that runs `delete this` on the last unref. `SimpleRefCounted`
// objects MUST be allocated on the heap.
class SimpleRefCounted : public RefCounted {
 protected:
  void OnLastUnref() override { delete this; }
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_REF_COUNT_H__
