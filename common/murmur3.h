#ifndef __TSDB2_COMMON_MURMUR3_H__
#define __TSDB2_COMMON_MURMUR3_H__

#include <cstddef>
#include <cstdint>

namespace tsdb2 {
namespace common {

// Calculates a 32-bit MurmurHash3. See https://en.wikipedia.org/wiki/MurmurHash#Algorithm.
//
// Objects of this class expect zero or more `Add` calls and one final `Finish` call.
//
// Usage:
//
//   MurmurHash3_32 hasher{/*seed=*/ 0x12345678};
//   hasher.Add(buffer1, length1);
//   hasher.Add(buffer2, length2);
//   ...
//   hash = hasher.Finish();
//
//
// WARNING: Calling `Add` after `Finish` or calling `Finish` multiple times leads to undefined
// behavior. It is okay to call `Finish` without calling `Add` at all.
//
// WARNING: this class is not thread-safe, only thread-friendly.
class MurmurHash3_32 final {
 public:
  // Hashes the content of the provided buffer using the provided seed.
  static uint32_t Hash(void const* const buffer, size_t const length, uint32_t const seed) {
    MurmurHash3_32 hasher{seed};
    hasher.Add(buffer, length);
    return hasher.Finish();
  }

  explicit MurmurHash3_32(uint32_t const seed) : hash_(seed) {}

  MurmurHash3_32(MurmurHash3_32 const&) = default;
  MurmurHash3_32& operator=(MurmurHash3_32 const&) = default;
  MurmurHash3_32(MurmurHash3_32&&) noexcept = default;
  MurmurHash3_32& operator=(MurmurHash3_32&&) noexcept = default;

  // Adds the provided data to the hash calculation. The buffer can be discarded when `Add` returns.
  void Add(void const* buffer, size_t length);

  // Finishes the calculation and returns the resulting hash.
  uint32_t Finish();

 private:
  uint8_t ReadByte(void const* const buffer, size_t const i) {
    if (i < remainder_.size) {
      return remainder_.bytes[i];
    } else {
      return static_cast<uint8_t const*>(buffer)[i - remainder_.size];
    }
  }

  static uint32_t Scramble(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    return k;
  }

  // The hash computed so far.
  uint32_t hash_;

  // The number of bytes hashed so far. Doesn't count `remainder_.size`.
  uint32_t length_ = 0;

  // `remainder_` stores at most 3 bytes left over from the previous `Add` call. They'll be either
  // used along with the next buffer or consumed by `Finish`.
  struct {
    uint8_t size = 0;
    uint8_t bytes[3] = {};
  } remainder_;
};

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_MURMUR3_H__
