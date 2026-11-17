/// @file
/// Standard formatting for emphasis attributes.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include "emphasis.hpp"
#include "format_support.hpp"
#include <tjg/inline_buffer.hpp>
#include <tjg/utility.hpp>

#include <format>
#include <string_view>
#include <array>
#include <iterator>
#include <utility>

namespace tjg::color {

/**
 * Writes the names of the set attributes in enum order, separated by spaces
 * and enclosed in braces. Writes `{}` for `emphasis::none`.
 * @param out   Destination output iterator.
 * @param value Attributes to write.
 * @return The advanced output iterator.
 */
template<std::output_iterator<char> OutputIt>
auto write_value(OutputIt out, emphasis value) -> OutputIt {
  using namespace std::string_view_literals;
  static constexpr auto names = std::array{
    "bold"sv , "faint"sv  , "italic"sv , "underline"sv,
    "blink"sv, "reverse"sv, "conceal"sv, "strike"sv
  };
  *out++ = '{';
  const unsigned bits = std::to_underlying(value);
  auto first = true;
  for (int bit = 0; bit < std::ssize(names); ++bit) {
    if ((bits & (1u << bit)) == 0) continue;
    if (!first) *out++ = ' ';
    out = detail::write_ascii(std::move(out), names[bit]);
    first = false;
  }
  *out++ = '}';
  return out;
} // write_value emphasis

/**
 * Writes one character per set attribute in enum order, with no separators.
 * The characters `bfi_!r?-` represent bold, faint, italic, underline, blink,
 * reverse, conceal, and strike, respectively. Writes `{}` for `emphasis::none`.
 * @param out   Destination output iterator.
 * @param value Attributes to write.
 * @return The advanced output iterator.
 */
template<std::output_iterator<char> OutputIt>
auto write_debug_value(OutputIt out, emphasis value) -> OutputIt {
  if (value == emphasis::none) return detail::write_ascii(std::move(out), "{}");
  const auto bits = std::to_underlying(value);
  for (int bit = 0; bit < 8; ++bit) {
    if ((bits & (1u << bit)) != 0) *out++ = "bfi_!r?-"[bit];
  }
  return out;
} // write_debug_value

} // tjg::color

/**
 * Formats emphasis values in narrow and wide format strings.
 * Supports fill, alignment, width, and precision, including dynamic width and
 * precision. The default and `s` presentations write a braced list of attribute
 * names, such as `{bold underline}`. The `?` presentation writes compact codes,
 * such as `b_` for the same attributes. All presentations write `{}` for
 * `emphasis::none` before applying width and precision.
 */
template<tjg::CharType Char>
struct std::formatter<tjg::color::emphasis, Char>
  : tjg::color::detail::standard_value_formatter<tjg::color::emphasis, Char> { };
