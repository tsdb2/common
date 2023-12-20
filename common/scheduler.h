#ifndef __TSDB2_COMMON_SCHEDULER_H__
#define __TSDB2_COMMON_SCHEDULER_H__

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "absl/container/node_hash_set.h"
#include "absl/functional/any_invocable.h"
#include "absl/functional/bind_front.h"
#include "absl/hash/hash.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/clock.h"
#include "common/sequence_number.h"

namespace tsdb2 {
namespace common {

// Manages the scheduling of generic runnable events. Supports both blocking and non-blocking event
// cancellation, as well as recurring (aka periodic) events that are automatically rescheduled after
// every run.
//
// Under the hood this class uses a fixed (configurable) number of worker threads that wait on the
// event queue and run each event as soon as it's due.
class Scheduler {
 public:
  struct Options {
    // The number of worker threads. Must be > 0. At most 65535 workers are supported (but you
    // definitely shouldn't use that many; each worker is a system thread).
    uint16_t num_workers = 2;

    // Clock used to schedule actions. `nullptr` means the scheduler uses the `RealClock`.
    Clock const *clock = nullptr;

    // If `true` the constructor calls `Start()` right away, otherwise you need to call it manually.
    // You need to set this to `false` e.g. when instantiating a Scheduler in global scope, so that
    // it doesn't spin up its worker threads right away.
    bool start_now = false;
  };

  using Callback = absl::AnyInvocable<void()>;
  using Handle = uintptr_t;

  explicit Scheduler(Options options)
      : options_(std::move(options)),
        clock_(options_.clock ? options_.clock : RealClock::GetInstance()) {
    if (options_.start_now) {
      Start();
    }
  }

  ~Scheduler() { Stop(); }

  void Start();

  void Stop();

  // Schedules an event to be executed ASAP. `callback` is the function to execute. The returned
  // `Key` can be used to cancel the event using `Cancel`.
  Handle ScheduleNow(Callback callback) {
    return ScheduleInternal(std::move(callback), clock_->TimeNow(), std::nullopt);
  }

  // Schedules an event to be executed at the specified time.
  Handle ScheduleAt(Callback callback, absl::Time const due_time) {
    return ScheduleInternal(std::move(callback), due_time, std::nullopt);
  }

  // Schedules an event to be executed at now+`delay`.
  Handle ScheduleIn(Callback callback, absl::Duration const delay) {
    return ScheduleInternal(std::move(callback), clock_->TimeNow() + delay, std::nullopt);
  }

  // Schedules a recurring event to be executed once every `period`, starting ASAP.
  Handle ScheduleRecurring(Callback callback, absl::Duration const period) {
    return ScheduleInternal(std::move(callback), clock_->TimeNow(), period);
  }

  // Schedules a recurring event to be executed once every `period`, starting at `due_time`.
  Handle ScheduleRecurringAt(Callback callback, absl::Time const due_time,
                             absl::Duration const period) {
    return ScheduleInternal(std::move(callback), due_time, period);
  }

  // Schedules a recurring event to be executed once every `period`, starting at now+`delay`.
  Handle ScheduleRecurringIn(Callback callback, absl::Duration const delay,
                             absl::Duration const period) {
    return ScheduleInternal(std::move(callback), clock_->TimeNow() + delay, period);
  }

  // TODO

 private:
  class EventRef;

  // Represents a scheduled event. This class is NOT thread-safe. Thread safety must be guaranteed
  // by `Scheduler::mutex_`.
  class Event final {
   public:
    // Custom hash functor to store Event objects in a node_hash_set.
    struct Hash {
      using is_transparent = void;
      size_t operator()(Event const &event) const { return absl::HashOf(event.handle()); }
      size_t operator()(Handle const &handle) const { return absl::HashOf(handle); }
    };

    // Custom equals functor to store Event objects in a node_hash_set.
    struct Equals {
      using is_transparent = void;

      static Handle ToHandle(Event const &event) { return event.handle(); }
      static Handle ToHandle(Handle const &handle) { return handle; }

      template <typename LHS, typename RHS>
      bool operator()(LHS const &lhs, RHS const &rhs) const {
        return ToHandle(lhs) == ToHandle(rhs);
      }
    };

    explicit Event(Scheduler *const parent, Callback callback, absl::Time const due_time,
                   std::optional<absl::Duration> const period)
        : handle_(parent->handle_generator_.GetNext()),
          callback_(std::move(callback)),
          due_time_(due_time),
          period_(period) {}

    Handle handle() const { return handle_; }
    EventRef const &ref() const { return *ref_; }
    void set_ref(EventRef const *const ref) { ref_ = ref; }
    absl::Time due_time() const { return due_time_; }
    void set_due_time(absl::Time const value) { due_time_ = value; }

    void Run() { callback_(); }

   private:
    Event(Event const &) = delete;
    Event &operator=(Event const &) = delete;
    Event(Event &&) = delete;
    Event &operator=(Event &&) = delete;

    Handle const handle_;
    EventRef const *ref_ = nullptr;
    Callback callback_;
    absl::Time due_time_;
    std::optional<absl::Duration> period_;
  };

  // This class acts as a smart pointer to an `Event` object and maintains a backlink to itself
  // inside the referenced Event. The priority queue of the scheduler is a min-heap backed by an
  // array of `EventRef` objects. The heap swap operations move the `EventRef`s which in turn update
  // the backlinks of their respective events, and thanks to the backlinks an `Event` can determine
  // its current index in the priority queue array. The index is in turn used for cancellation.
  //
  // An EventRef can be empty, in which case no event backlinks to it. An event's backlink may also
  // be null, in which case it means the event is not in the priority queue (i.e. it's being
  // processed by a worker).
  class EventRef final {
   public:
    explicit EventRef(Event *const event = nullptr) : event_(event) {
      if (event_) {
        event_->set_ref(this);
      }
    }

    ~EventRef() {
      if (event_) {
        event_->set_ref(nullptr);
      }
    }

    EventRef(EventRef &&other) noexcept : event_(other.event_) {
      other.event_ = nullptr;
      event_->set_ref(this);
    }

    EventRef &operator=(EventRef &&other) noexcept {
      if (event_) {
        event_->set_ref(nullptr);
      }
      event_ = other.event_;
      other.event_ = nullptr;
      event_->set_ref(this);
      return *this;
    }

    explicit operator bool() const { return event_ != nullptr; }
    Event *operator->() const { return event_; }
    Event &operator*() const { return *event_; }

   private:
    EventRef(EventRef const &) = delete;
    EventRef &operator=(EventRef const &) = delete;

    Event *event_;
  };

  // This functor is used to index the events by due time in the `queue_` min-heap. We use
  // `operator>` to do the comparison because the STL algorithms implement a max-heap if `operator<`
  // is used.
  struct CompareEvents {
    bool operator()(EventRef const &lhs, EventRef const &rhs) const {
      return lhs->due_time() > rhs->due_time();
    }
  };

  class Worker final {
   public:
    explicit Worker(Scheduler *const parent, size_t const index)
        : parent_(parent), index_(index), thread_(absl::bind_front(&Worker::Run, this)) {}

   private:
    void Run();

    Scheduler *const parent_;
    size_t const index_;

    std::thread thread_;
  };

  // Describe the state of the scheduler.
  enum class State {
    // Constructed but not yet started.
    IDLE = 0,

    // Started. The worker threads are processing the events.
    STARTED = 1,

    // Stopped. All workers joined, no more events will be executed.
    STOPPED = 2,
  };

  Scheduler(Scheduler const &) = delete;
  Scheduler &operator=(Scheduler const &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  Handle ScheduleInternal(Callback callback, absl::Time due_time,
                          std::optional<absl::Duration> period) ABSL_LOCKS_EXCLUDED(mutex_);

  Options const options_;
  Clock const *const clock_;

  static SequenceNumber handle_generator_;

  absl::Mutex mutable mutex_;
  absl::node_hash_set<Event, Event::Hash, Event::Equals> events_ ABSL_GUARDED_BY(mutex_);
  std::vector<EventRef> queue_ ABSL_GUARDED_BY(mutex_);

  absl::Mutex mutable worker_mutex_;
  State state_ ABSL_GUARDED_BY(worker_mutex_) = State::IDLE;
  std::vector<std::unique_ptr<Worker>> workers_ ABSL_GUARDED_BY(worker_mutex_);
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SCHEDULER_H__
