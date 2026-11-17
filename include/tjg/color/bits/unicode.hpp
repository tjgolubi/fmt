/**
 * @file
 * Unicode lookup, UTF-8 decoding, text measurement, and debug quoting.
 *
 * Adapted from {fmt}, copyright (c) 2012-present Victor Zverovich and {fmt}
 * contributors. Distributed under the repository's license.
 */

#pragma once

#include "EastAsianWidth.hpp"
#include "UnicodeData.hpp"

#include "error_support.hpp"
#include <tjg/utility.hpp>

#include <string_view>

#include <algorithm>
#include <iterator>

#include <concepts>
#include <type_traits>
#include <utility>
#include <limits>

#include <cstdint>
#include <cstddef>

namespace tjg::color::detail::unicode {

using std::size_t;
using std::uint16_t;
using std::uint32_t;

/**
 * Returns the estimated display width of a code point.
 * @return Two columns for code points in the generated wide ranges and one
 *         column for every other value.
 */
constexpr inline auto display_width_of(uint32_t cp) noexcept -> size_t {
  if (cp < 0x1100)
    return 1;
  size_t lo = 0;
  const auto& ranges = detail::wide_ranges;
  auto hi = std::size(ranges);
  while (lo < hi) {
    size_t mid = lo + (hi - lo) / 2;
    if (cp < ranges[mid].first)
      hi = mid;
    else if (cp > ranges[mid].last)
      lo = mid + 1;
    else
      return 2;
  }
  return 1;
} // display_width_of

namespace detail {

/// Looks up the low 16 bits of a code point in one plane's compressed table.
inline auto is_printable(uint16_t x, const singleton* singletons,
                         size_t singletons_size,
                         const unsigned char* singleton_lowers,
                         const unsigned char* normal, size_t normal_size)
  -> bool
{
  auto upper = x >> 8;
  auto lower_start = 0;
  for (size_t i = 0; i < singletons_size; ++i) {
    auto s = singletons[i];
    auto lower_end = lower_start + s.lower_count;
    if (upper < s.upper)
      break;
    if (upper == s.upper) {
      for (auto j = lower_start; j < lower_end; ++j) {
        if (singleton_lowers[j] == (x & 0xff))
          return false;
      }
    }
    lower_start = lower_end;
  }

  auto xsigned = static_cast<int>(x);
  auto current = true;
  for (size_t i = 0; i < normal_size; ++i) {
    auto v = static_cast<int>(normal[i]);
    auto len = ((v & 0x80) != 0) ? ((v & 0x7f) << 8) | normal[++i] : v;
    xsigned -= len;
    if (xsigned < 0)
      break;
    current = !current;
  }
  return current;
} // is_printable

} // detail

/**
 * Tests whether debug quoting treats a code point as printable.
 * The generated table marks ASCII space printable and excludes controls,
 * format characters, surrogates, private-use and unassigned code points, and
 * other separators. Values above U+10FFFF also return false.
 */
inline auto is_printable(uint32_t cp) -> bool {
  const auto lower = static_cast<uint16_t>(cp);
  if (cp < 0x10000) {
    return detail::is_printable(lower, detail::singletons0,
                                std::size(detail::singletons0),
                                detail::singletons0_lower, detail::normal0,
                                std::size(detail::normal0));
  }
  if (cp < 0x20000) {
    return detail::is_printable(lower, detail::singletons1,
                                std::size(detail::singletons1),
                                detail::singletons1_lower, detail::normal1,
                                std::size(detail::normal1));
  }
  for (const auto& range : detail::nonprintable_ranges) {
    if (range.first <= cp && cp < range.end)
      return false;
  }
  return cp < 0x110000;
} // is_printable(uint32_t)

/// The result of decoding one UTF-8 sequence.
struct utf8_character {
  uint32_t value; ///< Decoded Unicode scalar value, or `0xffffffffu` on error.
  size_t size;    ///< Number of input bytes consumed.
};

/**
 * Decodes the first UTF-8 sequence in a string view.
 * @pre The input is nonempty.
 * @return The decoded scalar value and its byte count. A malformed or
 *         truncated sequence returns `0xffffffffu` and consumes one byte.
 */
constexpr auto decode_utf8(std::string_view input) noexcept -> utf8_character {
  TJG_EXPECTS(!input.empty(), "unicode::decode_utf8: empty input");
  const auto first = static_cast<unsigned char>(input[0]);
  if (first < 0x80)
    return {first, 1};
  const size_t count = first >= 0xc2 && first <= 0xdf ? 2
                     : first >= 0xe0 && first <= 0xef ? 3
                     : first >= 0xf0 && first <= 0xf4 ? 4
                                                      : 0;
  if (count == 0 || input.size() < count)
    return {0xffffffffu, 1};
  uint32_t value = first & (0x7fu >> count);
  for (size_t i = 1; i < count; ++i) {
    const auto byte = static_cast<unsigned char>(input[i]);
    if ((byte & 0xc0u) != 0x80u)
      return {0xffffffffu, 1};
    value = (value << 6) | (byte & 0x3fu);
  }
  if ((count == 2 && value <    0x80) || (count == 3 && value < 0x800)
   || (count == 4 && value < 0x10000) || value > 0x10ffff
   || (value >= 0xd800 && value <= 0xdfff))
    return {0xffffffffu, 1};
  return {value, count};
} // decode_utf8

/**
 * Finds the longest prefix that fits within the width limit.
 * For `char` text, the limit is in estimated display columns. Each malformed
 * UTF-8 byte counts as one column. For other character types, the limit is
 * in code units. With a 16-bit character type, the prefix ends before any
 * surrogate pair that would cross the limit.
 * @return The prefix length in code units and its measured width, in that order.
 */
template<CharType Char>
auto text_prefix(std::basic_string_view<Char> text, size_t limit)
  -> std::pair<size_t, size_t>
{
  if constexpr (!std::is_same_v<Char, char>) {
    auto size = std::min(text.size(), limit);
    if constexpr (sizeof(Char) == sizeof(uint16_t)) {
      if (size != 0 && size < text.size()) {
        const auto before = static_cast<uint16_t>(text[size - 1]);
        const auto after  = static_cast<uint16_t>(text[size]);
        if (0xd800 <= before && before <= 0xdbff
         && 0xdc00 <= after  && after  <= 0xdfff)
          --size;
      }
    }
    return {size, size};
  } else {
    size_t size = 0, width = 0;
    while (size < text.size()) {
      const auto cp = decode_utf8(text.substr(size));
      const auto columns = display_width_of(cp.value);
      if (columns > limit - width)
        break;
      size  += cp.size;
      width += columns;
    }
    return {size, width};
  }
} // text_prefix

/**
 * Writes a double-quoted debug representation of text.
 * Escapes newlines, carriage returns, tabs, double quotes, and backslashes.
 * Other nonprintable code points use `\x`, `\u`, or `\U` with 2, 4, or 8
 * lowercase hexadecimal digits, respectively. For `char` text, UTF-8 is
 * decoded and each malformed byte is escaped individually as `\x`. Other
 * character types are processed one code unit at a time. A code unit above
 * U+10FFFF is escaped as `\x` followed by the two digits of its low byte.
 * @pre Writing to the output preserves unread input.
 * @return The advanced output iterator.
 */
template<CharType Char, std::output_iterator<Char> OutputIt>
auto quote_text(std::basic_string_view<Char> text, OutputIt out) -> OutputIt {
  *out++ = Char('"');
  for (size_t pos = 0; pos < text.size();) {
    auto cp = utf8_character{
      static_cast<uint32_t>(static_cast<std::make_unsigned_t<Char>>(text[pos])),
      1};
    if constexpr (std::is_same_v<Char, char>)
      cp = decode_utf8(text.substr(pos));
    const auto value = cp.value;
    if (value == '\n' || value == '\r' || value == '\t' || value == '"'
     || value == '\\')
    {
      *out++ = Char('\\');
      *out++ = Char(value == '\n' ? 'n'
                  : value == '\r' ? 'r'
                  : value == '\t' ? 't'
                                  : static_cast<char>(value));
    } else if (value < 0x20 || value == 0x7f || !is_printable(value)) {
      auto hex = [&](uint32_t number, int digits, char prefix) {
        *out++ = Char('\\');
        *out++ = Char(prefix);
        for (int shift = (digits - 1) * 4; shift >= 0; shift -= 4)
          *out++ = Char("0123456789abcdef"[(number >> shift) & 15]);
      };
      if (     value < 0x100)
        hex(value, 2, 'x');
      else if (value < 0x10000)
        hex(value, 4, 'u');
      else if (value < 0x110000)
        hex(value, 8, 'U');
      else {
        for (size_t i = 0; i < cp.size; ++i)
          hex(static_cast<uint32_t>(text[pos + i]) & 255u, 2, 'x');
      }
    } else {
      for (std::size_t i = 0; i < cp.size; ++i)
        *out++ = text[pos + i];
    }
    pos += cp.size;
  }
  *out++ = Char('"');
  return out;
} // quote_text

} // tjg::color::detail::unicode
