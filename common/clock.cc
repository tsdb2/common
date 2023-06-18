#include "common/clock.h"

#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace tsdb2 {
namespace common {

RealClock const* RealClock::GetInstance() {
  static constexpr RealClock kInstance;
  return &kInstance;
}

absl::Time RealClock::TimeNow() const { return absl::Now(); }

}  // namespace common
}  // namespace tsdb2
