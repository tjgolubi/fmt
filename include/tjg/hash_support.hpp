// Adapted from Boost.ContainerHash 1.85.0, hash.hpp and detail/hash_mix.hpp.
// Copyright 2005-2014 Daniel James.
// Copyright 2021, 2022 Peter Dimov.
// Distributed under the Boost Software License, Version 1.0.
// See https://www.boost.org/LICENSE_1_0.txt.

#pragma once

#include <functional>
#include <limits>
#include <type_traits>
#include <utility>

#include <cstddef>
#include <cstdint>

namespace tjg {

using hash_t = std::size_t;

namespace detail {

// Jon Maiga's mx3 mixer, as used by Boost.ContainerHash.
constexpr auto hash_mix(std::uint64_t x) noexcept -> std::uint64_t {
  constexpr std::uint64_t m = 0xe9846af9b1a615d;
  x ^= x >> 32;
  x *= m;
  x ^= x >> 32;
  x *= m;
  x ^= x >> 28;
  return x;
} // hash_mix uint64_t

// The "best xmxmx" mixer from skeeto/hash-prospector, used by Boost.
constexpr auto hash_mix(std::uint32_t x) noexcept -> std::uint32_t {
  x ^= x >> 16;
  x *= 0x21f0aaad;
  x ^= x >> 15;
  x *= 0x735a2d97;
  x ^= x >> 15;
  return x;
} // has_mix uint32_t

} // detail

template<typename T>
[[nodiscard]] auto hash_value(const T& value)
  noexcept(noexcept(std::hash<T>{}(value)))
  -> hash_t
{ return std::hash<T>{}(value); }

/// Combines the hash of value into seed. Hash values are not stable storage.
/// Uses Boost.ContainerHash's mixing algorithm with std::hash<T>.
template <typename T>
inline void hash_combine(hash_t& seed, const T& value)
    noexcept(noexcept(hash_value(value)))
{
  constexpr auto bits = std::numeric_limits<hash_t>::digits;
  static_assert(bits == 32 || bits == 64);
  using mix_type = std::conditional_t<bits == 32, std::uint32_t, std::uint64_t>;
  const auto sum = seed + hash_t{0x9e3779b9} + hash_value(value);
  seed = detail::hash_mix(static_cast<mix_type>(sum));
}

/// Combines the values in argument order, starting with a zero seed.
template<typename... T>
[[nodiscard]] auto hash_values(const T&... values)
  noexcept((noexcept(tjg::hash_combine(std::declval<hash_t&>(), values))
            && ...))
  -> hash_t
{
  hash_t seed{0};
  (tjg::hash_combine(seed, values), ...);
  return seed;
}

} // tjg
