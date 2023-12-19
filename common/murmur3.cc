#include "common/murmur3.h"

#include <cstddef>
#include <cstdint>

namespace tsdb2 {
namespace common {

void MurmurHash3_32::Add(void const* const buffer, size_t const length) {
  auto const total_bytes = remainder_.size + length;
  size_t i = 0;
  for (; i + 3 < total_bytes; i += 4) {
    uint32_t k = ReadByte(buffer, i + 3);
    k = (k << 8) + ReadByte(buffer, i + 2);
    k = (k << 8) + ReadByte(buffer, i + 1);
    k = (k << 8) + ReadByte(buffer, i);
    hash_ ^= Scramble(k);
    hash_ = (hash_ << 13) | (hash_ >> 19);
    hash_ = hash_ * 5 + 0xe6546b64;
  }
  length_ += length;
  size_t const j = i;
  for (; i < total_bytes; ++i) {
    remainder_.bytes[i - j] = ReadByte(buffer, i);
  }
  remainder_.size = total_bytes - j;
}

uint32_t MurmurHash3_32::Finish() {
  uint32_t k = 0;
  for (size_t i = remainder_.size; i; --i) {
    k = (k << 8) + remainder_.bytes[i - 1];
    ++length_;
  }
  hash_ ^= Scramble(k);
  hash_ ^= length_;
  hash_ ^= hash_ >> 16;
  hash_ *= 0x85ebca6b;
  hash_ ^= hash_ >> 13;
  hash_ *= 0xc2b2ae35;
  hash_ ^= hash_ >> 16;
  return hash_;
}

}  // namespace common
}  // namespace tsdb2
