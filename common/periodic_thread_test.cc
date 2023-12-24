#include "common/periodic_thread.h"

#include "absl/time/time.h"
#include "common/mock_clock.h"
#include "common/testing.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MockClock;
using ::tsdb2::common::PeriodicClosure;

class PeriodicThreadTest : public ::testing::Test {
 protected:
  MockClock clock_;
};

TEST_F(PeriodicThreadTest, NotStarted) {
  PeriodicClosure pc{PeriodicClosure::Options{
                         .period = absl::Seconds(10),
                         .clock = &clock_,
                     },
                     [] { FAIL(); }};
  EXPECT_EQ(pc.state(), PeriodicClosure::State::IDLE);
  clock_.AdvanceTime(absl::Seconds(11));
  ASSERT_OK(pc.WaitUntilAsleep());
}

// TODO

}  // namespace
