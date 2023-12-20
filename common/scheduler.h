#ifndef __TSDB2_COMMON_SCHEDULER_H__
#define __TSDB2_COMMON_SCHEDULER_H__

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/clock.h"
#include "common/sequence_number.h"

namespace tsdb2 {
namespace common {

class Scheduler {
 public:
  struct Options {
    // The number of worker threads. Must be > 0. At most 65535 workers are supported (but you
    // definitely shouldn't use that many; each worker is a system thread).
    uint16_t num_workers = 2;

    // Clock used to schedule actions. `nullptr` means the scheduler uses the `RealClock`.
    Clock *clock = nullptr;

    // If `true` the constructor calls `Start()` right away, otherwise you need to call it manually.
    // You need to set this to `false` e.g. when instantiating a Scheduler in global scope, so that
    // it doesn't spin up its worker threads right away.
    bool start_now = true;
  };

  using Callback = absl::AnyInvocable<void()>;
  using Handle = uintptr_t;

  explicit Scheduler(Options options) : options_(std::move(options)) {
    if (options_.start_now) {
      Start();
    }
  }

  ~Scheduler() { Stop(); }

  void Start();

  void Stop();

 private:
  class EventRef;

  // Represents a scheduled event. This class is NOT thread-safe. Thread safety must be guaranteed
  // by `Scheduler::mutex_`.
  class Event final {
   public:
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

  Scheduler(Scheduler const &) = delete;
  Scheduler &operator=(Scheduler const &) = delete;
  Scheduler(Scheduler &&) = delete;
  Scheduler &operator=(Scheduler &&) = delete;

  Handle ScheduleInternal(Callback callback, absl::Time due_time,
                          std::optional<absl::Duration> period) ABSL_LOCKS_EXCLUDED(mutex_);

  Options const options_;

  static SequenceNumber handle_generator_;

  absl::Mutex mutable mutex_;
  std::vector<EventRef> queue_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SCHEDULER_H__
