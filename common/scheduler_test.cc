#include "common/scheduler.h"

#include <cstdint>
#include <utility>

#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "common/mock_clock.h"
#include "common/simple_condition.h"
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

TEST_P(SchedulerStateTest, Stopping) {
  scheduler_.Start();
  absl::Mutex mutex;
  bool started = false;
  bool stopping = false;
  bool exit = false;
  scheduler_.ScheduleNow([&] {
    absl::MutexLock lock{&mutex};
    started = true;
    mutex.Await(absl::Condition(&exit));
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPING);
  });
  std::thread thread{[&] {
    {
      absl::MutexLock lock{&mutex};
      mutex.Await(absl::Condition(&started));
      stopping = true;
    }
    scheduler_.Stop();
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
  }};
  {
    absl::MutexLock lock{&mutex};
    mutex.Await(absl::Condition(&stopping));
    exit = true;
  }
  thread.join();
}

TEST_P(SchedulerStateTest, Stopped) {
  scheduler_.Start();
  scheduler_.Stop();
  EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
}

TEST_P(SchedulerStateTest, ConcurrentStops) {
  scheduler_.Start();
  absl::Mutex mutex;
  bool started = false;
  int stopping = 0;
  bool exit = false;
  scheduler_.ScheduleNow([&] {
    absl::MutexLock lock{&mutex};
    started = true;
    mutex.Await(absl::Condition(&exit));
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPING);
  });
  auto const stopper = [&] {
    {
      absl::MutexLock lock{&mutex};
      mutex.Await(absl::Condition(&started));
      ++stopping;
    }
    scheduler_.Stop();
    EXPECT_EQ(scheduler_.state(), Scheduler::State::STOPPED);
  };
  std::thread thread1{stopper};
  std::thread thread2{stopper};
  std::thread thread3{stopper};
  {
    absl::MutexLock lock{&mutex};
    mutex.Await(SimpleCondition([&] { return stopping == 3; }));
    exit = true;
  }
  thread3.join();
  thread2.join();
  thread1.join();
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

  Scheduler::Handle Schedule(absl::Duration const due_time, Scheduler::Callback callback) {
    return scheduler_.ScheduleAt(std::move(callback), absl::UnixEpoch() + due_time);
  }

  Scheduler::Handle Schedule(absl::Duration const due_time, Scheduler::Callback callback,
                             absl::Duration const period) {
    return scheduler_.ScheduleRecurringAt(std::move(callback), absl::UnixEpoch() + due_time,
                                          period);
  }
};

TEST_P(SchedulerEventTest, SmokeTest) {
  // TODO
}

// TODO

INSTANTIATE_TEST_SUITE_P(SchedulerEventTest, SchedulerEventTest, ::testing::Range(1, 10));

}  // namespace
