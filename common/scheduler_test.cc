#include "common/scheduler.h"

#include "common/mock_clock.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MockClock;
using ::tsdb2::common::Scheduler;

class SchedulerTest : public ::testing::Test {
 protected:
  MockClock clock_;
};

TEST_F(SchedulerTest, SmokeTest) {
  Scheduler(Scheduler::Options{
      .clock = &clock_,
      .start_now = false,
  });
}

// TODO

}  // namespace
