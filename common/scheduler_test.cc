#include "common/scheduler.h"

#include <cstdint>
#include <utility>

#include "absl/synchronization/mutex.h"
#include "absl/synchronization/notification.h"
#include "absl/time/time.h"
#include "common/mock_clock.h"
#include "common/simple_condition.h"
#include "common/testing.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MockClock;
using ::tsdb2::common::Scheduler;
using ::tsdb2::common::SimpleCondition;

class SchedulerTest : public ::testing::TestWithParam<int> {
 protected:
  explicit SchedulerTest(bool const start_now)
      : scheduler_{{
            .num_workers = static_cast<uint16_t>(GetParam()),
            .clock = &clock_,
            .start_now = start_now,
        }} {}

  MockClock clock_;
  Scheduler scheduler_;
};

class SchedulerStateTest : public SchedulerTest {
 protected:
  explicit SchedulerStateTest() : SchedulerTest(/*start_now=*/false) {}
};

TEST_P(SchedulerStateTest, Idle) { EXPECT_EQ(scheduler_.state(), Scheduler::State::IDLE); }

TEST_P(SchedulerStateTest, Started) {
  scheduler_.Start();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STARTED);
}

TEST_P(SchedulerStateTest, StartedAgain) {
  scheduler_.Start();
  scheduler_.Start();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STARTED);
}

TEST_P(SchedulerStateTest, StartAfterScheduling) {
  absl::Notification done;
  scheduler_.ScheduleNow([&] {
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STARTED);
    done.Notify();
  });
  scheduler_.Start();
  done.WaitForNotification();
}

TEST_P(SchedulerStateTest, Stopped) {
  scheduler_.Start();
  scheduler_.Stop();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
}

TEST_P(SchedulerStateTest, Stop) {
  scheduler_.Start();
  absl::Mutex mutex;
  bool started = false;
  bool stopped = false;
  scheduler_.ScheduleNow([&] {
    while (scheduler_.state() == Scheduler::State::STARTED) {
      absl::MutexLock lock{&mutex};
      started = true;
    }
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPING);
    stopped = true;
  });
  {
    absl::MutexLock lock{&mutex};
    mutex.Await(absl::Condition(&started));
  }
  scheduler_.Stop();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
  EXPECT_TRUE(stopped);
}

TEST_P(SchedulerStateTest, StoppedButNotStarted) {
  scheduler_.Stop();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
}

INSTANTIATE_TEST_SUITE_P(SchedulerStateTest, SchedulerStateTest, ::testing::Range(1, 10));

class SchedulerEventTest : public SchedulerTest {
 protected:
  explicit SchedulerEventTest() : SchedulerTest(/*start_now=*/true) {
    clock_.AdvanceTime(absl::Seconds(12));
  }

  Scheduler::Handle ScheduleAt(absl::Duration const due_time, Scheduler::Callback callback) {
    return scheduler_.ScheduleAt(std::move(callback), absl::UnixEpoch() + due_time);
  }

  Scheduler::Handle ScheduleAt(absl::Duration const due_time, Scheduler::Callback callback,
                               absl::Duration const period) {
    return scheduler_.ScheduleRecurringAt(std::move(callback), absl::UnixEpoch() + due_time,
                                          period);
  }

  void WaitUntilAllWorkersAsleep() const { ASSERT_OK(scheduler_.WaitUntilAllWorkersAsleep()); }
};

TEST_P(SchedulerEventTest, ScheduleNow) {
  absl::Notification done;
  ScheduleAt(absl::Seconds(12), [&] { done.Notify(); });
  done.WaitForNotification();
}

TEST_P(SchedulerEventTest, SchedulePast) {
  absl::Notification done;
  ScheduleAt(absl::Seconds(10), [&] { done.Notify(); });
  done.WaitForNotification();
}

TEST_P(SchedulerEventTest, ScheduleFuture) {
  absl::Notification done;
  ScheduleAt(absl::Seconds(34), [&] { done.Notify(); });
  WaitUntilAllWorkersAsleep();
  EXPECT_FALSE(done.HasBeenNotified());
}

TEST_P(SchedulerEventTest, AdvanceTime) {
  absl::Notification done;
  ScheduleAt(absl::Seconds(34), [&] { done.Notify(); });
  clock_.AdvanceTime(absl::Seconds(22));
  WaitUntilAllWorkersAsleep();
  EXPECT_TRUE(done.HasBeenNotified());
}

// TODO

INSTANTIATE_TEST_SUITE_P(SchedulerEventTest, SchedulerEventTest, ::testing::Range(1, 10));

}  // namespace
