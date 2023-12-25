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
    queue_.clear();
    tasks_.clear();
    state_ = State::STOPPED;
  }
}

absl::Status Scheduler::WaitUntilAllWorkersAsleep() const {
  absl::MutexLock lock{&mutex_};
  mutex_.Await(SimpleCondition([this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    return state_ != State::STARTED ||
           (absl::c_all_of(workers_, [](auto const &worker) { return worker->is_sleeping(); }) &&
            !task_due_);
  }));
  if (state_ > State::STARTED) {
    return absl::CancelledError("");
  } else {
    return absl::OkStatus();
  }
}

// Handles start from 1 because 0 is reserved as an invalid handle value.
SequenceNumber Scheduler::Task::handle_generator_{1};

void Scheduler::Worker::Run() {
  Task *task = nullptr;
  while (true) {
    auto const status_or_task = parent_->FetchTask(this, task);
    if (status_or_task.ok()) {
      task = status_or_task.value();
      task->Run();
    } else {
      return;
    }
  }
}

Scheduler::Handle Scheduler::ScheduleInternal(Callback callback, absl::Time const due_time,
                                              std::optional<absl::Duration> const period) {
  absl::MutexLock lock{&mutex_};
  auto const [it, _] = tasks_.emplace(std::move(callback), due_time, period);
  Task &task = const_cast<Task &>(*it);
  queue_.emplace_back(&task);
  std::push_heap(queue_.begin(), queue_.end(), CompareTasks());
  if (!task_due_) {
    task_due_ = task.due_time() <= clock_->TimeNow();
  }
  return task.handle();
}

bool Scheduler::CancelInternal(Handle const handle, bool const blocking) {
  absl::MutexLock lock{&mutex_};
  auto const it = tasks_.find(handle);
  if (it == tasks_.end()) {
    return false;
  }
  auto &task = const_cast<Task &>(*it);
  task.set_cancelled(true);
  auto *const ref = task.ref();
  if (ref) {
    size_t const i = ref - queue_.data();
    task.set_due_time(absl::InfinitePast());
    CompareTasks compare;
    std::push_heap(queue_.begin(), queue_.begin() + i + 1, compare);
    std::pop_heap(queue_.begin(), queue_.end(), compare);
    queue_.pop_back();
    tasks_.erase(handle);
    task_due_ = is_task_due();
    return true;
  } else {
    if (blocking) {
      mutex_.Await(SimpleCondition([this, handle]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
        return !tasks_.contains(handle);
      }));
    }
    return false;
  }
}

absl::StatusOr<Scheduler::Task *> Scheduler::FetchTask(Worker *const worker,
                                                       Task *const last_task) {
  absl::MutexLock lock{&mutex_};
  Worker::SleepScope worker_sleep_scope{worker};
  if (last_task) {
    if (!last_task->cancelled() && last_task->is_periodic()) {
      auto const period = last_task->period();
      auto const due_time = last_task->due_time();
      last_task->set_due_time(due_time +
                              std::max(absl::Ceil(clock_->TimeNow() - due_time, period), period));
      queue_.emplace_back(last_task);
      std::push_heap(queue_.begin(), queue_.end(), CompareTasks());
      task_due_ = is_task_due();
    } else {
      tasks_.erase(last_task->handle());
    }
  }
  auto const queue_not_empty_condition =
      SimpleCondition([this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
        return state_ > State::STARTED || !queue_.empty();
      });
  auto const task_due_condition = SimpleCondition(
      [this]() ABSL_SHARED_LOCKS_REQUIRED(mutex_) { return state_ > State::STARTED || task_due_; });
  while (true) {
    mutex_.Await(queue_not_empty_condition);
    if (state_ > State::STARTED) {
      return absl::AbortedError("");
    }
    clock_->AwaitWithDeadline(&mutex_, task_due_condition, queue_.front()->due_time());
    if (state_ > State::STARTED) {
      return absl::AbortedError("");
    }
    std::pop_heap(queue_.begin(), queue_.end(), CompareTasks());
    auto const task = queue_.back().Get();
    queue_.pop_back();
    task_due_ = is_task_due();
    if (task->cancelled()) {
      tasks_.erase(task->handle());
    } else {
      return task;
    }
  }
}

}  // namespace common
}  // namespace tsdb2
