#ifndef __TSDB2_COMMON_SIMPLE_CONDITION_H__
#define __TSDB2_COMMON_SIMPLE_CONDITION_H__

#include <utility>

#include "absl/functional/any_invocable.h"
#include "absl/synchronization/mutex.h"

namespace tsdb2 {
namespace common {

class SimpleCondition : public absl::Condition {
 public:
  explicit SimpleCondition(absl::AnyInvocable<bool() const> callback)
      : absl::Condition(this, &SimpleCondition::Run), callback_(std::move(callback)) {}

 private:
  bool Run() const { return callback_(); }

  absl::AnyInvocable<bool() const> callback_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_SIMPLE_CONDITION_H__
