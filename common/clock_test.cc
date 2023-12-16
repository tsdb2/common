#include "common/clock.h"

#include "absl/time/clock.h"
#include "gtest/gtest.h"

namespace {

using ::tsdb2::common::RealClock;

TEST(ClockTest, TimeNow) {
  auto const now = absl::Now();
  EXPECT_GE(RealClock::GetInstance()->TimeNow(), now);
}

}  // namespace
