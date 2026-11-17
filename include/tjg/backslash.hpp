/// @file
/// Escape control characters for inspection and decode backslash escapes.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/color/bits/error_support.hpp>
#include <tjg/utility.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <iterator>
#include <utility>

#include <cstdint>

namespace tjg {

namespace detail {

template<CharType Char>
constexpr auto backslash_hex_digit(Char c) -> int {
  if (c >= Char('0') && c <= Char('9')) return c - Char('0');
  if (c >= Char('a') && c <= Char('f')) return c - Char('a') + 10;
  if (c >= Char('A') && c <= Char('F')) return c - Char('A') + 10;
  return -1;
}

// The caller has already validated the scalar value.
template<CharType Char, std::output_iterator<Char> OutputIt>
constexpr void write_backslash_scalar(OutputIt& out, std::uint32_t value) {
  if constexpr (std::same_as<Char, char>) {
    auto continuation = [&](std::uint32_t bits) {
      *out++ = static_cast<char>(0x80u | (bits & 0x3fu));
    };
    if (value < 0x80u) {
      *out++ = static_cast<char>(value);
      return;
    }
    if (value < 0x800u) {
      *out++ = static_cast<char>(0xc0u | (value >>  6));
    } else if (value < 0x10000u) {
      *out++ = static_cast<char>(0xe0u | (value >> 12));
      continuation(value >> 6);
    } else {
      *out++ = static_cast<char>(0xf0u | (value >> 18));
      continuation(value >> 12);
      continuation(value >>  6);
    }
    continuation(value);
  } else {
    if constexpr (sizeof(Char) == sizeof(std::uint16_t)) {
      if (value >= 0x10000u) {
        value -= 0x10000u;
        *out++ = static_cast<Char>(0xd800u + (value >> 10));
        *out++ = static_cast<Char>(0xdc00u + (value & 0x3ffu));
        return;
      }
    }
    *out++ = static_cast<Char>(value);
  }
} // write_backslash_scalar

} // detail

/**
 * Writes text with ASCII controls escaped to out and returns the advanced
 * iterator. Uses `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, and `\v` for their
 * corresponding controls, and exactly two lowercase hexadecimal digits after
 * `\x` for remaining C0 controls and DEL. Backslashes and double quotes are
 * escaped too. Other code units are preserved verbatim; no surrounding quotes
 * are added. Supports embedded NUL through string views.
 * The output must not invalidate or overwrite unread input.
 */
template<CharType Char, std::output_iterator<Char> OutputIt>
constexpr
auto backslash_escape_to(OutputIt out, std::basic_string_view<Char> input)
  -> OutputIt
{
  for (auto c : input) {
    auto escape = char{0};
    switch (c) {
      case Char{'\a'  }: escape = 'a' ; break;
      case Char{'\b'  }: escape = 'b' ; break;
      case Char{'\x1b'}: escape = 'e' ; break;
      case Char{'\f'  }: escape = 'f' ; break;
      case Char{'\n'  }: escape = 'n' ; break;
      case Char{'\r'  }: escape = 'r' ; break;
      case Char{'\t'  }: escape = 't' ; break;
      case Char{'\v'  }: escape = 'v' ; break;
      case Char{'\\'  }: escape = '\\'; break;
      case Char{'"'   }: escape = '"' ; break;
    }
    if (escape != char{0}) {
      *out++ = Char{'\\'};
      *out++ = static_cast<Char>(escape);
    } else if ((c >= Char{0} && c < Char{' '}) || c == Char{127}) {
      const auto value = static_cast<int>(c);
      const char* hex = "0123456789abcdef";
      *out++ = Char{'\\'};
      *out++ = Char{'x'};
      *out++ = static_cast<Char>(hex[value / 16]);
      *out++ = static_cast<Char>(hex[value % 16]);
    } else {
      *out++ = c;
    }
  }
  return out;
}  // backslash_escape_to

/**
 * Writes decoded text to out and returns the advanced iterator.
 * Decodes `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\"`, `\'`,
 * `\?`, and `\x` followed by exactly two hex digits (either case).
 * Further digits are ordinary text. Also decodes one to three octal digits,
 * with values limited to 0 through 255. Unicode escapes `\u` and `\U` take
 * exactly four and eight hex digits, respectively. They emit UTF-8 for `char`,
 * UTF-16 for 16-bit `wchar_t`, and UTF-32 for 32-bit `wchar_t`.
 * Output written before an error is retained. Decoding is a single pass:
 * decoded backslashes do not introduce further escapes. Surrounding quotes
 * are preserved.
 * The output must not invalidate or overwrite unread input.
 * @throws std::invalid_argument An escape is unknown or malformed, or a
 *         Unicode escape names a surrogate.
 * @throws std::out_of_range An octal escape exceeds 255 or a Unicode escape
 *         exceeds U+10FFFF.
 */
template<CharType Char, std::output_iterator<Char> OutputIt>
constexpr
auto backslash_unescape_to(OutputIt out, std::basic_string_view<Char> input)
  -> OutputIt
{
  while (!input.empty()) {
    if (input[0] != Char('\\')) {
      *out++ = input[0];
      input.remove_prefix(1);
      continue;
    }
    if (input.size() < 2)
      TJG_THROW(std::invalid_argument("incomplete backslash escape"));
    auto c = Char{0};
    auto length = std::size_t{2};
    if (input[1] >= Char('0') && input[1] <= Char('7')) {
      auto value = static_cast<int>(input[1] - Char('0'));
      while (length < 4 && length < input.size()
          && input[length] >= Char('0') && input[length] <= Char('7'))
      {
        value = value * 8 + static_cast<int>(input[length] - Char('0'));
        ++length;
      }
      if (value > 255)
        TJG_THROW(std::out_of_range("octal escape is out of range"));
      *out++ = static_cast<Char>(value);
      input.remove_prefix(length);
      continue;
    }
    switch (input[1]) {
      case Char{'a' }: c = Char{'\a'  }; break;
      case Char{'b' }: c = Char{'\b'  }; break;
      case Char{'e' }: c = Char{'\x1b'}; break;
      case Char{'f' }: c = Char{'\f'  }; break;
      case Char{'n' }: c = Char{'\n'  }; break;
      case Char{'r' }: c = Char{'\r'  }; break;
      case Char{'t' }: c = Char{'\t'  }; break;
      case Char{'v' }: c = Char{'\v'  }; break;
      case Char{'\\'}: c = Char{'\\'  }; break;
      case Char{'"' }: c = Char{'"'   }; break;
      case Char{'\''}: c = Char{'\''  }; break;
      case Char{'?' }: c = Char{'?'   }; break;
      case Char{'x' }: {
        const auto high =
            input.size() >= 4 ? detail::backslash_hex_digit(input[2]) : -1;
        const auto low =
            input.size() >= 4 ? detail::backslash_hex_digit(input[3]) : -1;
        if (high < 0 || low < 0)
          TJG_THROW(std::invalid_argument("invalid hexadecimal escape"));
        c = static_cast<Char>(high * 16 + low);
        length = 4;
        break;
      }
      case Char{'u' }:
      case Char{'U' }: {
        length = input[1] == Char{'u'} ? 6 : 10;
        if (input.size() < length)
          TJG_THROW(std::invalid_argument("incomplete Unicode escape"));
        auto value = std::uint32_t{0};
        for (std::size_t i = 2; i < length; ++i) {
          const auto digit = detail::backslash_hex_digit(input[i]);
          if (digit < 0)
            TJG_THROW(std::invalid_argument("invalid Unicode escape"));
          value = (value << 4) | static_cast<std::uint32_t>(digit);
        }
        if (value > 0x10ffffu)
          TJG_THROW(std::out_of_range("Unicode escape is out of range"));
        if (value >= 0xd800u && value <= 0xdfffu)
          TJG_THROW(std::invalid_argument("Unicode escape names a surrogate"));
        detail::write_backslash_scalar<Char>(out, value);
        input.remove_prefix(length);
        continue;
      }
      default: TJG_THROW(std::invalid_argument("unknown backslash escape"));
    }
    *out++ = c;
    input.remove_prefix(length);
  }
  return out;
}  // backslash_unescape_to

// Fixed character-type overloads allow string literals and std::basic_string
// arguments without explicit template arguments.
template<std::output_iterator<char> OutputIt>
constexpr
auto backslash_escape_to(OutputIt out, std::string_view input) -> OutputIt
{ return backslash_escape_to<char>(std::move(out), input); }

template<std::output_iterator<wchar_t> OutputIt>
constexpr
auto backslash_escape_to(OutputIt out, std::basic_string_view<wchar_t> input)
  -> OutputIt
{ return backslash_escape_to<wchar_t>(std::move(out), input); }

template<std::output_iterator<char> OutputIt>
constexpr
auto backslash_unescape_to(OutputIt out, std::string_view input) -> OutputIt
{ return backslash_unescape_to<char>(std::move(out), input); }

template<std::output_iterator<wchar_t> OutputIt>
constexpr
auto backslash_unescape_to(OutputIt out, std::basic_string_view<wchar_t> input)
  -> OutputIt
{ return backslash_unescape_to<wchar_t>(std::move(out), input); }

/// Returns an owning string using the same escaping as backslash_escape_to.
template<CharType Char>
[[nodiscard]] constexpr
auto backslash_escape(std::basic_string_view<Char> input)
  -> std::basic_string<Char>
{
  auto result = std::basic_string<Char>{};
  backslash_escape_to(std::back_inserter(result), input);
  return result;
}

/// Returns an owning string using the same decoding as backslash_unescape_to.
template<CharType Char>
[[nodiscard]] constexpr
auto backslash_unescape(std::basic_string_view<Char> input)
  -> std::basic_string<Char>
{
  auto result = std::basic_string<Char>{};
  backslash_unescape_to(std::back_inserter(result), input);
  return result;
}

[[nodiscard]] constexpr inline
auto backslash_escape(std::string_view input) -> std::string
{ return backslash_escape<char>(input); }

[[nodiscard]] constexpr inline
auto backslash_escape(std::basic_string_view<wchar_t> input) -> std::wstring
{ return backslash_escape<wchar_t>(input); }

[[nodiscard]] constexpr inline
auto backslash_unescape(std::string_view input) -> std::string
{ return backslash_unescape<char>(input); }

[[nodiscard]] constexpr inline
auto backslash_unescape(std::basic_string_view<wchar_t> input) -> std::wstring
{ return backslash_unescape<wchar_t>(input); }

} // tjg
