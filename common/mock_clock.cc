#include "common/mock_clock.h"

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

namespace tsdb2 {
namespace common {

absl::Time MockClock::TimeNow() const {
  absl::MutexLock lock{&mutex_};
  return current_time_;
}

void MockClock::AdvanceTime(absl::Duration const delta) {
  absl::MutexLock lock{&mutex_};
  current_time_ += delta;
}

}  // namespace common
}  // namespace tsdb2
