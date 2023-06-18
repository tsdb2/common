#include "common/mock_clock.h"

#include "absl/base/attributes.h"
#include "absl/base/const_init.h"
#include "absl/time/time.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::MockClock;

ABSL_CONST_INIT MockClock test_global_instance{absl::kConstInit};

TEST(MockClockTest, InitialTime) {
  MockClock clock;
  EXPECT_EQ(clock.TimeNow(), absl::UnixEpoch());
}

TEST(MockClockTest, GlobalInstance) {
  auto const delta = absl::Seconds(123);
  test_global_instance.AdvanceTime(delta);
  EXPECT_EQ(test_global_instance.TimeNow(), absl::UnixEpoch() + delta);
}

// TODO

}  // namespace
