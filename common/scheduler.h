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
#include "absl/log/check.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
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
//
// This class is fully thread-safe.
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

  // Describe the state of the scheduler.
  enum class State {
    // Constructed but not yet started.
    IDLE = 0,

    // Started. The worker threads are processing the events.
    STARTED = 1,

    // Stop started. Waiting for current events, no more events will be executed.
    STOPPING = 2,

    // Stopped. All workers joined, no more events will be executed.
    STOPPED = 3,
  };

  // Type of the callback functions that can be scheduled in the scheduler.
  using Callback = absl::AnyInvocable<void()>;

  // Handles are unique event IDs.
  using Handle = uintptr_t;

  static Handle constexpr kInvalidHandle = 0;

  explicit Scheduler(Options options)
      : options_(std::move(options)),
        clock_(options_.clock ? options_.clock : RealClock::GetInstance()) {
    DCHECK_GT(options_.num_workers, 0) << "Scheduler must have at least 1 worker thread";
    if (options_.start_now) {
      Start();
    }
  }

  // The destructor calls `Stop`, meaning that it implicitly stops and joins all workers.
  ~Scheduler() { Stop(); }

  // Returns the current state of the scheduler. See the `State` enum for more details.
  State state() const ABSL_LOCKS_EXCLUDED(mutex_) {
    absl::MutexLock lock{&mutex_};
    return state_;
  }

  // Starts the workers. Has no effect if the scheduler is in any other state than `IDLE`. The
  // scheduler is guaranteed to be in state `STARTED` when `Start` returns. If this method is called
  // concurrently, the workers are initialized and started only once.
  void Start() ABSL_LOCKS_EXCLUDED(mutex_);

  // Stops and joins all workers.
  //
  // The scheduler will be in state `STOPPING` through the execution of this method. Upon `Stop`
  // returning, the scheduler is guaranteed to be in state `STOPPED`.
  //
  // `Start` had never been called (i.e. the scheduler is in state `IDLE` when `Stop` is called)
  // this method will still make the scheduler transition to the `STOPPED` state, preventing it from
  // being able to ever run any events.
  //
  // If `Stop is called concurrently multiple times, all calls block until the workers are joined.
  void Stop() ABSL_LOCKS_EXCLUDED(mutex_);

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

  // Cancels the event with the specified handle. Does nothing if the handle is invalid for any
  // reason, e.g. if a past event with this handle has already finished running.
  //
  // If the event has already started running but hasn't yet finished when this method is invoked,
  // the event will finish running normally. In any case this method returns immediately.
  //
  // The returned boolean indicates whether actual cancellation happened: it is true iff the event
  // was in the queue and hadn't started yet.
  bool Cancel(Handle const handle) { return CancelInternal(handle, /*blocking=*/false); }

  // Cancels the event with the specified handle. Does nothing if the handle is invalid for any
  // reason, e.g. if a past event with this handle has already finished running.
  //
  // If the event has already started running but hasn't yet finished when this method is invoked,
  // the event will finish running normally and this method will block until then.
  //
  // The returned boolean indicates whether actual cancellation happened: it is true iff the event
  // was in the queue and hadn't started yet.
  bool BlockingCancel(Handle const handle) { return CancelInternal(handle, /*blocking=*/true); }

  // TEST ONLY: wait until all due events have been processed and all workers are asleep. This
  // method only makes sense in testing scenarios with a `MockClock`, otherwise if more events are
  // scheduled in the queue and close enough in the future there's no guarantee that the workers
  // won't wake up again by the time this method returns. The time of a `MockClock` on the other
  // hand will only advance as decided in the test, so the workers are guaranteed to remain asleep
  // until then.
  absl::Status WaitUntilAllWorkersAsleep() ABSL_LOCKS_EXCLUDED(mutex_);

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

    explicit Event(Callback callback, absl::Time const due_time,
                   std::optional<absl::Duration> const period)
        : callback_(std::move(callback)), due_time_(due_time), period_(period) {}

    Handle handle() const { return handle_; }

    EventRef const *ref() const { return ref_; }
    void set_ref(EventRef const *const ref) { ref_ = ref; }

    void CheckRef(EventRef const *const ref) const {
      DCHECK_EQ(ref_, ref) << "invalid event backlink!";
    }

    absl::Time due_time() const { return due_time_; }
    void set_due_time(absl::Time const value) { due_time_ = value; }

    bool cancelled() const { return cancelled_; }
    void set_cancelled(bool const value) { cancelled_ = value; }

    void Run() { callback_(); }

   private:
    Event(Event const &) = delete;
    Event &operator=(Event const &) = delete;
    Event(Event &&) = delete;
    Event &operator=(Event &&) = delete;

    // Generates event handles.
    static SequenceNumber handle_generator_;

    // Unique ID for the event.
    Handle const handle_ = handle_generator_.GetNext();

    Callback callback_;
    absl::Time due_time_;
    std::optional<absl::Duration> period_;

    // Backlink to the `EventRef` of this Event. The `EventRef` move constructor and assignment
    // operator take care of keeping this up to date.
    EventRef const *ref_ = nullptr;

    bool cancelled_ = false;
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

    Event *Get() const { return event_; }
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

  // Worker thread implementation.
  //
  // NOTE: the worker's sleeping flag is guarded by the Scheduler's mutex. Unfortunately we can't
  // note that via thread annotalysis because the compiler can't track that `parent_` points to the
  // parent `Scheduler`.
  class Worker final {
   public:
    // Used by `Scheduler::FetchEvent` to manage the sleeping flag of the calling worker.
    class SleepScope final {
     public:
      explicit SleepScope(Worker *const worker) : worker_(worker) { worker_->set_sleeping(true); }
      ~SleepScope() { worker_->set_sleeping(false); }

     private:
      SleepScope(SleepScope const &) = delete;
      SleepScope &operator=(SleepScope const &) = delete;
      SleepScope(SleepScope &&) = delete;
      SleepScope &operator=(SleepScope &&) = delete;

      Worker *const worker_;
    };

    explicit Worker(Scheduler *const parent)
        : parent_(parent), thread_(absl::bind_front(&Worker::Run, this)) {}

    // Indicates whether the worker is waiting for event.
    bool is_sleeping() const { return sleeping_; }

    // Sets the worker's sleeping flag.
    void set_sleeping(bool const value) { sleeping_ = value; }

    void Join() { thread_.join(); }

   private:
    void Run();

    Scheduler *const parent_;

    bool sleeping_ = false;
    std::thread thread_;
  };

  Scheduler(Scheduler const &) = delete;
  Scheduler &operator=(Scheduler const &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  // Checks the state of the queue and determines whether an event is due. The returned value is
  // used to update the `event_due_` flag below.
  bool is_event_due() const ABSL_SHARED_LOCKS_REQUIRED(mutex_) {
    if (queue_.empty()) {
      return false;
    }
    auto &ref = queue_.front();
    return !ref->cancelled() && ref->due_time() <= clock_->TimeNow();
  }

  Handle ScheduleInternal(Callback callback, absl::Time due_time,
                          std::optional<absl::Duration> period) ABSL_LOCKS_EXCLUDED(mutex_);

  bool CancelInternal(Handle handle, bool blocking) ABSL_LOCKS_EXCLUDED(mutex_);

  absl::StatusOr<Event *> FetchEvent(Worker *worker, Event *last_event) ABSL_LOCKS_EXCLUDED(mutex_);

  Options const options_;
  Clock const *const clock_;

  absl::Mutex mutable mutex_;
  absl::node_hash_set<Event, Event::Hash, Event::Equals> events_ ABSL_GUARDED_BY(mutex_);
  std::vector<EventRef> queue_ ABSL_GUARDED_BY(mutex_);

  // Indicates whether the event at the top of queue is due. This flag MUST be updated every time
  // `queue_` is modified, as it's used for conditional locking to wake up a worker to run the
  // event. The value of this flag can be calculated by calling `is_event_due()`.
  //
  // NOTE: we need to cache the value of `is_event_due()` in this flag for use in conditional
  // locking because locking conditions can only access state guarded by the mutex and must
  // otherwise be pure, so fetching the clock is not allowed. `is_event_due()` needs to fetch the
  // clock to determine whether and event is due, so it cannot be invoked directly in locking
  // conditions.
  bool event_due_ ABSL_GUARDED_BY(mutex_) = false;

  State state_ ABSL_GUARDED_BY(mutex_) = State::IDLE;
  std::vector<std::unique_ptr<Worker>> workers_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SCHEDULER_H__
