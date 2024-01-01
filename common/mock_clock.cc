#include "common/mock_clock.h"

#include <memory>
#include <utility>

#include "absl/log/check.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/simple_condition.h"

namespace tsdb2 {
namespace common {

absl::Time MockClock::TimeNow() const {
  absl::MutexLock lock{&mutex_};
  return current_time_;
}

void MockClock::SleepFor(absl::Duration const duration) const {
  absl::MutexLock lock{&mutex_};
  auto const wakeup_time = current_time_ + duration;
  mutex_.Await(SimpleCondition([this, wakeup_time]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    return current_time_ >= wakeup_time;
  }));
}

void MockClock::SleepUntil(absl::Time const wakeup_time) const {
  absl::MutexLock lock{&mutex_};
  mutex_.Await(SimpleCondition([this, wakeup_time]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    return current_time_ >= wakeup_time;
  }));
}

bool MockClock::AwaitWithTimeout(absl::Mutex* const mutex, absl::Condition const& condition,
                                 absl::Duration const timeout) const {
  return AwaitWithDeadline(mutex, condition, TimeNow() + timeout);
}

bool MockClock::AwaitWithDeadline(absl::Mutex* const mutex, absl::Condition const& condition,
                                  absl::Time const deadline) const {
  absl::Time current_time = TimeNow();
  auto listener = [mutex, &current_time](absl::Time const new_time) ABSL_LOCKS_EXCLUDED(mutex) {
    absl::MutexLock lock{mutex};
    current_time = new_time;
  };
  auto const handle = const_cast<MockClock*>(this)->AddListener(std::move(listener));
  mutex->Await(SimpleCondition([&] { return current_time >= deadline || condition.Eval(); }));
  return condition.Eval();
}

void MockClock::SetTime(absl::Time const time) {
  ListenerSet listeners;
  {
    absl::MutexLock lock{&mutex_};
    CHECK_GE(time, current_time_) << "MockClock's time cannot move backward!";
    current_time_ = time;
    listeners_.swap(listeners);
  }
  for (auto const& listener : listeners) {
    listener->Run(time);
  }
}

void MockClock::AdvanceTime(absl::Duration const delta) {
  absl::Time current_time;
  ListenerSet listeners;
  {
    absl::MutexLock lock{&mutex_};
    current_time = current_time_ += delta;
    listeners_.swap(listeners);
  }
  for (auto const& listener : listeners) {
    listener->Run(current_time);
  }
}

MockClock::ListenerHandle MockClock::AddListener(TimeCallback callback) {
  absl::MutexLock lock{&mutex_};
  auto const [it, inserted] =
      listeners_.emplace(std::make_shared<TimeListener>(std::move(callback)));
  return ListenerHandle(this, *it);
}

void MockClock::RemoveListener(std::shared_ptr<TimeListener> const& listener) {
  absl::MutexLock lock{&mutex_};
  listeners_.erase(listener);
}

}  // namespace common
}  // namespace tsdb2
