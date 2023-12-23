#include "common/scheduler.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>

#include "absl/algorithm/container.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/sequence_number.h"
#include "common/simple_condition.h"

namespace tsdb2 {
namespace common {

void Scheduler::Start() {
  absl::MutexLock lock{&mutex_};
  if (state_ != State::IDLE) {
    return;
  }
  size_t const num_workers = options_.num_workers;
  workers_.reserve(num_workers);
  for (size_t i = 0; i < num_workers; ++i) {
    workers_.emplace_back(std::make_unique<Worker>(this));
  }
  state_ = State::STARTED;
}

void Scheduler::Stop() {
  std::vector<std::unique_ptr<Worker>> workers;
  {
    absl::MutexLock lock{&mutex_};
    if (state_ < State::STARTED) {
      state_ = State::STOPPED;
      return;
    } else if (state_ > State::STARTED) {
      mutex_.Await(SimpleCondition(
          [this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) { return state_ == State::STOPPED; }));
      return;
    } else {
      workers_.swap(workers);
      state_ = State::STOPPING;
    }
  }
  for (auto &worker : workers) {
    worker->Join();
  }
  {
    absl::MutexLock lock{&mutex_};
    state_ = State::STOPPED;
  }
}

absl::Status Scheduler::WaitUntilAllWorkersAsleep() const {
  absl::MutexLock lock{&mutex_};
  mutex_.Await(SimpleCondition([this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    return state_ > State::STARTED ||
           (absl::c_all_of(workers_, [](auto const &worker) { return worker->is_sleeping(); }) &&
            !event_due_);
  }));
  if (state_ > State::STARTED) {
    return absl::CancelledError("");
  } else {
    return absl::OkStatus();
  }
}

// Handles start from 1 because 0 is reserved as an invalid handle value.
SequenceNumber Scheduler::Event::handle_generator_{1};

void Scheduler::Worker::Run() {
  Event *event = nullptr;
  while (true) {
    auto const status_or_event = parent_->FetchEvent(this, event);
    if (status_or_event.ok()) {
      event = status_or_event.value();
      event->Run();
    } else {
      return;
    }
  }
}

Scheduler::Handle Scheduler::ScheduleInternal(Callback callback, absl::Time const due_time,
                                              std::optional<absl::Duration> const period) {
  absl::MutexLock lock{&mutex_};
  auto const [it, _] = events_.emplace(std::move(callback), due_time, period);
  Event &event = const_cast<Event &>(*it);
  queue_.emplace_back(&event);
  std::push_heap(queue_.begin(), queue_.end(), CompareEvents());
  if (!event_due_) {
    event_due_ = event.due_time() <= clock_->TimeNow();
  }
  return event.handle();
}

bool Scheduler::CancelInternal(Handle const handle, bool const blocking) {
  absl::MutexLock lock{&mutex_};
  auto const it = events_.find(handle);
  if (it == events_.end()) {
    return false;
  }
  auto &event = const_cast<Event &>(*it);
  event.set_cancelled(true);
  auto *const ref = event.ref();
  if (ref) {
    size_t const i = ref - queue_.data();
    event.set_due_time(absl::InfinitePast());
    CompareEvents compare;
    std::push_heap(queue_.begin(), queue_.begin() + i + 1, compare);
    std::pop_heap(queue_.begin(), queue_.end(), compare);
    queue_.pop_back();
    events_.erase(handle);
    event_due_ = is_event_due();
    return true;
  } else {
    if (blocking) {
      mutex_.Await(SimpleCondition([this, handle]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
        return !events_.contains(handle);
      }));
    }
    return false;
  }
}

absl::StatusOr<Scheduler::Event *> Scheduler::FetchEvent(Worker *const worker,
                                                         Event *const last_event) {
  absl::MutexLock lock{&mutex_};
  Worker::SleepScope worker_sleep_scope{worker};
  if (last_event) {
    // TODO: handle periodic events.
    events_.erase(last_event->handle());
  }
  auto const queue_not_empty_condition =
      SimpleCondition([this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
        return state_ > State::STARTED || !queue_.empty();
      });
  auto const event_due_condition = SimpleCondition([this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    return state_ > State::STARTED || event_due_;
  });
  while (true) {
    mutex_.Await(queue_not_empty_condition);
    if (state_ > State::STARTED) {
      return absl::AbortedError("");
    }
    clock_->AwaitWithDeadline(&mutex_, event_due_condition, queue_.front()->due_time());
    if (state_ > State::STARTED) {
      return absl::AbortedError("");
    }
    std::pop_heap(queue_.begin(), queue_.end(), CompareEvents());
    auto const event = queue_.back().Get();
    queue_.pop_back();
    event_due_ = is_event_due();
    if (event->cancelled()) {
      events_.erase(event->handle());
    } else {
      return event;
    }
  }
}

}  // namespace common
}  // namespace tsdb2
