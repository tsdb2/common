#ifndef __TSDB2_COMMON_MOCK_CLOCK_H__
#define __TSDB2_COMMON_MOCK_CLOCK_H__

#include <memory>
#include <utility>

#include "absl/base/attributes.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/clock.h"

namespace tsdb2 {
namespace common {

// This `Clock` implementation allows creating test scenarios with simulated time. It never relies
// on the real time: instead, it encapsulates a fake time that only changes in response to `SetTime`
// or `AdvanceTime` calls.
//
// Advancements of the fake time are correctly reflected in all public methods; as such, they may
// unblock `Sleep*` and `Await*` calls from other threads.
//
// This class is thread-safe.
class MockClock : public Clock {
 public:
  explicit MockClock(absl::Time const current_time = absl::UnixEpoch())
      : current_time_{current_time} {}

  absl::Time TimeNow() const override ABSL_LOCKS_EXCLUDED(mutex_);

  void SleepFor(absl::Duration duration) const override ABSL_LOCKS_EXCLUDED(mutex_);

  void SleepUntil(absl::Time wakeup_time) const override ABSL_LOCKS_EXCLUDED(mutex_);

  bool AwaitWithTimeout(absl::Mutex* mutex, absl::Condition const& condition,
                        absl::Duration timeout) const override ABSL_SHARED_LOCKS_REQUIRED(mutex);

  bool AwaitWithDeadline(absl::Mutex* mutex, absl::Condition const& condition,
                         absl::Time deadline) const override ABSL_SHARED_LOCKS_REQUIRED(mutex);

  // Sets the fake time to the specified value. Checkfails if `time` is not greater than or equal to
  // the current fake time.
  void SetTime(absl::Time time) ABSL_LOCKS_EXCLUDED(mutex_);

  // Advances the fake time by the specified amount.
  void AdvanceTime(absl::Duration delta) ABSL_LOCKS_EXCLUDED(mutex_);

 private:
  class TimeListener final {
   public:
    explicit TimeListener(absl::Mutex* const mutex, absl::Time const deadline,
                          absl::Time const current_time, absl::Condition const& condition)
        : mutex_(mutex), deadline_(deadline), current_time_(current_time), condition_(condition) {}

    void Run(absl::Time const current_time) ABSL_LOCKS_EXCLUDED(mutex_) {
      absl::MutexLock{mutex_};
      current_time_ = current_time;
    }

    bool Eval() const { return current_time_ >= deadline_ || condition_.Eval(); }

   private:
    absl::Mutex* const mutex_;
    absl::Time const deadline_;
    absl::Time current_time_;
    absl::Condition const& condition_;
  };

  using ListenerSet = absl::flat_hash_set<std::shared_ptr<TimeListener>>;

  class ListenerHandle final {
   public:
    ~ListenerHandle() { MaybeRemove(); }

    ListenerHandle(ListenerHandle&& other) noexcept
        : parent_(other.parent_), listener_(std::move(other.listener_)) {
      other.parent_ = nullptr;
    }

    ListenerHandle& operator=(ListenerHandle&& other) noexcept {
      MaybeRemove();
      parent_ = other.parent_;
      listener_ = std::move(other.listener_);
      other.parent_ = nullptr;
      other.listener_ = nullptr;
      return *this;
    }

    bool Eval() const { return listener_ && listener_->Eval(); }

   private:
    friend class MockClock;

    explicit ListenerHandle(MockClock* const parent, std::shared_ptr<TimeListener> listener)
        : parent_(parent), listener_(std::move(listener)) {}

    ListenerHandle(ListenerHandle const&) = delete;
    ListenerHandle& operator=(ListenerHandle const&) = delete;

    void MaybeRemove() {
      if (parent_) {
        parent_->RemoveListener(listener_);
        parent_ = nullptr;
        listener_ = nullptr;
      }
    }

    MockClock* parent_;
    std::shared_ptr<TimeListener> listener_;
  };

  MockClock(MockClock const&) = delete;
  MockClock& operator=(MockClock const&) = delete;
  MockClock(MockClock&&) = delete;
  MockClock& operator=(MockClock&&) = delete;

  ListenerHandle AddListener(absl::Mutex* mutex, absl::Time deadline,
                             absl::Condition const& condition) ABSL_LOCKS_EXCLUDED(mutex_);

  void RemoveListener(std::shared_ptr<TimeListener> const& listener) ABSL_LOCKS_EXCLUDED(mutex_);

  absl::Mutex mutable mutex_;
  absl::Time current_time_ ABSL_GUARDED_BY(mutex_);
  ListenerSet listeners_ ABSL_GUARDED_BY(mutex_);
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_MOCK_CLOCK_H__
