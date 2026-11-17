/// Emphasis attributes.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/utility.hpp>

#include <utility>
#include <cstdint>

namespace tjg::color {

/**
 * Independent text attributes that can be combined with `operator|`.
 * `emphasis::none` specifies no attributes.
 *
 * The enum values are bit masks.
 */
enum class emphasis : std::uint8_t {
  none      = 0,      ///< No attributes.
  bold      = 1 << 0, ///< Increased intensity (SGR 1).
  faint     = 1 << 1, ///< Decreased intensity (SGR 2).
  italic    = 1 << 2, ///< Italic text         (SGR 3).
  underline = 1 << 3, ///< Underlined text     (SGR 4).
  blink     = 1 << 4, ///< Blinking text       (SGR 5).
  reverse   = 1 << 5, ///< Reversed fg and bg  (SGR 7).
  conceal   = 1 << 6, ///< Concealed text      (SGR 8).
  strike    = 1 << 7, ///< Crossed-out text    (SGR 9).
};  // emphasis

/// Union of two emphasis attributes.
[[nodiscard]]
constexpr inline auto operator|(emphasis lhs, emphasis rhs) noexcept -> emphasis
{
  return enum_cast<emphasis>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

/// Add emphasis attributes.
constexpr inline auto operator|=(emphasis& lhs, emphasis rhs) noexcept
  -> emphasis&
{ return lhs = lhs | rhs; }

} // tjg::color
