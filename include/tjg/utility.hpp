/// Utilities for {color} support.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <concepts>

namespace tjg {

/// Character types supported by the formatting helpers.
template<typename T>
concept CharType = std::same_as<T, char> || std::same_as<T, wchar_t>;

/// A narrowing static_cast.
template<typename T, typename U>
constexpr inline auto narrow_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

/// An enum-related static_cast.
template<typename T, typename U>
constexpr inline auto enum_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

} // tjg
