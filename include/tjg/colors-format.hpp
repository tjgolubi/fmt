/// @file
/// Formatting for colors.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include "colors.hpp"

#include "format_support.hpp"
#include "inline_buffer.hpp"
#include "utility.hpp"

#include <format>
#include <string_view>

#include <algorithm>
#include <array>
#include <variant>
#include <concepts>
#include <utility>

#include <cstdint>

namespace tjg {

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, no_color_t) -> OutIter
{ return detail::write_ascii(std::move(out), "no_color"); }

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, const rgb& value) -> OutIter {
  const auto packed = static_cast<std::uint32_t>(value);
  *out++ = '#';
  for (int shift = 20; shift >= 0; shift -= 4)
    *out++ = ("0123456789abcdef"[(packed >> shift) & 0x0f]);
  return out;
}

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, term_color value) -> OutIter
{ return detail::write_ascii(std::move(out), tjg::name_of(value)); }

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, const cube_color& value) -> OutIter {
  out = detail::write_ascii(std::move(out), "cube");
  *out++ = static_cast<char>('0' + value.r());
  *out++ = static_cast<char>('0' + value.g());
  *out++ = static_cast<char>('0' + value.b());
  return out;
}

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, const gray_color& value) -> OutIter {
  out = detail::write_ascii(std::move(out), "gray");
  const auto index = value.index();
  if (index >= 10)
    *out++ = static_cast<char>('0' + index / 10);
  *out++ = static_cast<char>('0' + index % 10);
  return out;
}

template<std::output_iterator<char> OutIter>
[[nodiscard]] constexpr
auto write_value(OutIter out, const color_t& value) -> OutIter {
  auto format_color = [&]<typename T>(const T& color) {
    return write_value(std::move(out), color);
  };
  if (value.valueless_by_exception()) return format_color(no_color);
  return std::visit(format_color, value);
}

} // tjg

namespace std {

template<tjg::CharType Char>
struct formatter<tjg::no_color_t, Char>
  : tjg::detail::standard_value_formatter<tjg::no_color_t, Char> { };

template<tjg::CharType Char>
struct formatter<tjg::rgb, Char>
  : tjg::detail::standard_value_formatter<tjg::rgb, Char> { };

template<tjg::CharType Char>
struct formatter<tjg::term_color, Char>
  : tjg::detail::standard_value_formatter<tjg::term_color, Char> { };

template<tjg::CharType Char>
struct formatter<tjg::cube_color, Char>
  : tjg::detail::standard_value_formatter<tjg::cube_color, Char> { };

template<tjg::CharType Char>
struct formatter<tjg::gray_color, Char>
  : tjg::detail::standard_value_formatter<tjg::gray_color, Char> { };

template<tjg::CharType Char>
struct formatter<tjg::color_t, Char>
  : tjg::detail::standard_value_formatter<tjg::color_t, Char> { };

} // std
