#include "common/scheduler.h"

#include "common/sequence_number.h"

namespace tsdb2 {
namespace common {

void Scheduler::Start() {
  // TODO
}

void Scheduler::Stop() {
  // TODO
}

// Handles start from 1 because 0 is reserved as an invalid handle value.
SequenceNumber Scheduler::handle_generator_{1};

}  // namespace common
}  // namespace tsdb2
