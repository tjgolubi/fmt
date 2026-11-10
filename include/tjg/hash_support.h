// Adapted from Boost.ContainerHash 1.85.0, hash.hpp and detail/hash_mix.hpp.
// Copyright 2005-2014 Daniel James.
// Copyright 2021, 2022 Peter Dimov.
// Distributed under the Boost Software License, Version 1.0.
// See tjg/LICENSE-Boost or https://www.boost.org/LICENSE_1_0.txt.

#ifndef TJG_HASH_SUPPORT_H_
#define TJG_HASH_SUPPORT_H_

#include <functional>
#include <limits>

#include <cstddef>
#include <cstdint>

namespace tjg {
namespace detail {

// Jon Maiga's mx3 mixer, as used by Boost.ContainerHash.
constexpr auto hash_mix(uint64_t x) noexcept -> uint64_t {
  constexpr uint64_t m = 0xe9846af9b1a615d;
  x ^= x >> 32;
  x *= m;
  x ^= x >> 32;
  x *= m;
  x ^= x >> 28;
  return x;
}

// The "best xmxmx" mixer from skeeto/hash-prospector, used by Boost.
constexpr auto hash_mix(uint32_t x) noexcept -> uint32_t {
  x ^= x >> 16;
  x *= 0x21f0aaad;
  x ^= x >> 15;
  x *= 0x735a2d97;
  x ^= x >> 15;
  return x;
}

}  // namespace detail

/// Combines the hash of value into seed. Hash values are not stable storage.
/// Uses Boost.ContainerHash's mixing algorithm with std::hash<T>.
template <typename T>
inline void hash_combine(std::size_t& seed, const T& value)
    noexcept(noexcept(std::hash<T>{}(value))) {
  static_assert(std::numeric_limits<std::size_t>::digits == 32 ||
                std::numeric_limits<std::size_t>::digits == 64);
  seed = detail::hash_mix(seed + std::size_t{0x9e3779b9} +
                          std::hash<T>{}(value));
}

}  // namespace tjg

#endif  // TJG_HASH_SUPPORT_H_
