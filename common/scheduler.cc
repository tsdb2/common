#include "common/scheduler.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/sequence_number.h"

namespace tsdb2 {
namespace common {

void Scheduler::Start() {
  // TODO
}

void Scheduler::Stop() {
  // TODO
}

void Scheduler::Worker::Run() {
  // TODO
}

Scheduler::Handle Scheduler::ScheduleInternal(Callback callback, absl::Time const due_time,
                                              std::optional<absl::Duration> const period) {
  absl::MutexLock lock{&mutex_};
  auto const [it, _] = events_.emplace(this, std::move(callback), due_time, period);
  Event &event = const_cast<Event &>(*it);
  queue_.emplace_back(&event);
  std::push_heap(queue_.begin(), queue_.end(), CompareEvents());
  return event.handle();
}

// Handles start from 1 because 0 is reserved as an invalid handle value.
SequenceNumber Scheduler::handle_generator_{1};

}  // namespace common
}  // namespace tsdb2
