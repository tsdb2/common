#ifndef __TSDB2_COMMON_CLOCK_H__
#define __TSDB2_COMMON_CLOCK_H__

#include "absl/time/time.h"

namespace tsdb2 {
namespace common {

class Clock {
 public:
  virtual absl::Time TimeNow() const = 0;
};

class RealClock : public Clock {
 public:
  static RealClock const* GetInstance();

  absl::Time TimeNow() const override;

 private:
  constexpr explicit RealClock() = default;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_CLOCK_H__
