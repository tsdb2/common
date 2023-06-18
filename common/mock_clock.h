#ifndef __TSDB2_COMMON_MOCK_CLOCK_H__
#define __TSDB2_COMMON_MOCK_CLOCK_H__

#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/clock.h"

namespace tsdb2 {
namespace common {

class MockClock : public Clock {
 public:
  explicit MockClock() = default;

  constexpr explicit MockClock(absl::ConstInitType const) : mutex_{absl::kConstInit} {}

  virtual absl::Time TimeNow() const override ABSL_LOCKS_EXCLUDED(mutex_);

  void AdvanceTime(absl::Duration delta) ABSL_LOCKS_EXCLUDED(mutex_);

 private:
  absl::Mutex mutable mutex_;
  absl::Time current_time_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_MOCK_CLOCK_H__
