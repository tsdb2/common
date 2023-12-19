#ifndef __TSDB2_COMMON_FINGERPRINT_H__
#define __TSDB2_COMMON_FINGERPRINT_H__

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace tsdb2 {
namespace common {

uint32_t constexpr kSeed = 71104;

namespace internal {

// A 32-bit Murmur3 hasher that assumes the content to hash is always made up of 32-bit words.
// Thanks to that assumption the implementation is very fast because it doesn't have to fetch
// unaligned words, take care of endianness, etc.
//
// This implementation uses the fixed seed value defined above and is only suitable for
// fingerprinting, not for general-purpose hashing.
//
// Objects of this class expect zero or more `Add` calls and one final `Finish` call. The `Hash*`
// methods are shorthands for a series of `Add` calls followed by a `Finish`.
//
// WARNING: Calling `Add` after `Finish` or calling `Finish` multiple times leads to undefined
// behavior. It is okay to call `Finish` without calling `Add` at all.
class FingerprintBase {
 public:
  // Adds the provided 32-bit word to the hash calculation.
  constexpr FingerprintBase& Add(uint32_t const k) {
    AddInternal(k);
    ++length_;
    return *this;
  }

  // Adds the provided 32-bit words to the hash calculation.
  constexpr FingerprintBase& Add(uint32_t const* const ks, size_t const count) {
    for (size_t i = 0; i < count; ++i) {
      AddInternal(ks[i]);
    }
    length_ += count;
    return *this;
  }

  // Finishes the hash calculation and returns the calculated value.
  constexpr uint32_t Finish() {
    hash_ ^= length_;
    hash_ ^= hash_ >> 16;
    hash_ *= 0x85ebca6b;
    hash_ ^= hash_ >> 13;
    hash_ *= 0xc2b2ae35;
    hash_ ^= hash_ >> 16;
    return hash_;
  }

  // Shorthand for `Add(k).Finish()`.
  constexpr uint32_t Hash(uint32_t const k) { return Add(k).Finish(); }

  // Shorthand for `Add(ks, count).Finish()`.
  constexpr uint32_t Hash(uint32_t const* const ks, size_t const count) {
    return Add(ks, count).Finish();
  }

  // Hashes a text string.
  uint32_t HashString(std::string_view const value) {
    Add(value.size());
    auto const data = reinterpret_cast<uint8_t const*>(value.data());
    auto const main_chunk = value.size() >> 2;
    Add(reinterpret_cast<uint32_t const*>(data), main_chunk);
    auto const remainder = value.size() % 4;
    if (remainder > 0) {
      uint32_t last = 0;
      std::memcpy(&last, data + (main_chunk << 2), remainder);
      Add(last);
    }
    return Finish();
  }

 private:
  constexpr void AddInternal(uint32_t k) {
    k *= 0xcc9e2d51;
    k = (k << 15) | (k >> 17);
    k *= 0x1b873593;
    hash_ ^= k;
    hash_ = (hash_ << 13) | (hash_ >> 19);
    hash_ = hash_ * 5 + 0xe6546b64;
  }

  uint32_t hash_ = kSeed;
  uint32_t length_ = 0;
};

}  // namespace internal

template <typename T, typename Enable = void>
struct Fingerprint;

template <typename T>
constexpr uint32_t FingerprintOf(T&& value) {
  return Fingerprint<std::decay_t<T>>{}(std::forward<T>(value));
}

template <typename T>
struct Fingerprint<T, std::enable_if_t<std::is_integral_v<T>>> : private internal::FingerprintBase {
  constexpr uint32_t operator()(T const value) {
    if constexpr (sizeof(T) > sizeof(uint32_t)) {
      static_assert(sizeof(T) % sizeof(uint32_t) == 0, "unsupported type");
      return Hash(reinterpret_cast<uint32_t const*>(&value), sizeof(T) / sizeof(uint32_t));
    } else {
      return Hash(value);
    }
  }
};

uint32_t constexpr kFalseFingerprint = Fingerprint<uint32_t>{}(false);
uint32_t constexpr kTrueFingerprint = Fingerprint<uint32_t>{}(true);

template <>
struct Fingerprint<bool> : private internal::FingerprintBase {
  constexpr uint32_t operator()(bool const value) {
    return value ? kTrueFingerprint : kFalseFingerprint;
  }
};

template <typename T>
struct Fingerprint<T, std::enable_if_t<std::is_floating_point_v<T>>>
    : private internal::FingerprintBase {
  constexpr uint32_t operator()(T const value) {
    if constexpr (sizeof(T) > sizeof(uint32_t)) {
      static_assert(sizeof(T) % sizeof(uint32_t) == 0, "unsupported type");
      return Hash(reinterpret_cast<uint32_t const*>(&value), sizeof(T) / sizeof(uint32_t));
    } else {
      return Hash(value);
    }
  }
};

template <>
struct Fingerprint<std::string> : private internal::FingerprintBase {
  uint32_t operator()(std::string_view const value) { return HashString(value); }
};

template <>
struct Fingerprint<std::string_view> : private internal::FingerprintBase {
  uint32_t operator()(std::string_view const value) { return HashString(value); }
};

template <>
struct Fingerprint<char const*> : private internal::FingerprintBase {
  uint32_t operator()(std::string_view const value) { return HashString(value); }
};

// TODO: inductive types.

}  // namespace common
}  // namespace tsdb2

#endif  // __TSDB2_COMMON_FINGERPRINT_H__
