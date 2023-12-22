#include "common/scheduler.h"

#include <cstdint>

#include "common/mock_clock.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MockClock;
using ::tsdb2::common::Scheduler;

class SchedulerTest : public ::testing::Test {
 protected:
  MockClock clock_;
};

class DefaultSchedulerTest : public SchedulerTest, public ::testing::WithParamInterface<uint16_t> {
 protected:
  Scheduler scheduler_{{
      .num_workers = GetParam(),
      .clock = &clock_,
      .start_now = true,
  }};
};

TEST_F(SchedulerTest, Idle) {
  Scheduler scheduler{{
      .clock = &clock_,
      .start_now = false,
  }};
  EXPECT_EQ(scheduler.state(), Scheduler::State::IDLE);
}

TEST_F(SchedulerTest, Started) {
  Scheduler scheduler{{
      .clock = &clock_,
      .start_now = false,
  }};
  scheduler.Start();
  EXPECT_EQ(scheduler.state(), Scheduler::State::STARTED);
}

// TODO

TEST_P(DefaultSchedulerTest, SmokeTest) {
  // TODO
}

// TODO

INSTANTIATE_TEST_SUITE_P(ManyWorkers, DefaultSchedulerTest,
                         ::testing::Values(1, 2, 3, 4, 5, 6, 7, 8, 9, 10));

}  // namespace
