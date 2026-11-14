/// Common formatting failure handling for the TJG extension.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include "error_support.hpp"
#include "inline_buffer.hpp"
#include "text_support.hpp"
#include "utility.hpp"

#include <format>
#include <iterator>
#include <concepts>
#include <utility>

namespace tjg::detail {

[[noreturn]] inline void standard_format_error(const char* message)
{ TJG_THROW(std::format_error(message)); }

template<typename OutputIt>
[[nodiscard]] constexpr
auto write_ascii(OutputIt out, const char* text) -> OutputIt {
  while (*text)
    *out++ = *text++;
  return out;
}

template<typename OutputIt>
[[nodiscard]] constexpr
auto write_ascii(OutputIt out, std::string_view text) -> OutputIt {
  for (auto ch : text)
    *out++ = ch;
  return out;
}

template<CharType Char>
struct string_format_specs {
  int width      =  0;
  int precision  = -1;
  int width_arg     = -1;
  int precision_arg = -1;
  char alignment = '<';
  bool debug     = false;
  basic_inline_buffer<Char, 4> _fill;

  constexpr string_format_specs() noexcept { _fill.push_back(Char{' '}); }

  constexpr void set_fill(std::basic_string_view<Char> value) {
    auto fill = basic_inline_buffer<Char, 4>{};
    if (!value.empty())
      fill.append(value.data(), value.data() + value.size());
    _fill = fill;
  }

  template<typename Context, typename Fail>
  constexpr auto parse(Context& ctx, Fail fail) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    const auto end = ctx.end();

    auto align = [](Char c) {
      return c == Char('<') || c == Char('>') || c == Char('^');
    }; // align

    if (it == end || *it == Char('}'))
      return it;

    auto next = it + 1;
    if constexpr (std::same_as<Char, char>) {
      const auto bytes = std::string_view{it, end};
      next = it + static_cast<std::ptrdiff_t>(decode_utf8(bytes).size);
    } else if constexpr (sizeof(Char) == 2) {
      if (next != end) {
        const auto lead  = static_cast<char16_t>(*it);
        const auto trail = static_cast<char16_t>(*next);
        if (   lead  >= 0xd800 && lead  <= 0xdbff
            && trail >= 0xdc00 && trail <= 0xdfff)
          ++next;
      }
    }
    if (next != end && align(*next)) {
      if (*it == Char('{') || *it == Char('}')) {
        fail("invalid fill character");
        return end;
      }
      set_fill(std::basic_string_view<Char>{it, next});
      alignment = static_cast<char>(*next);
      it = next + 1;
    } else if (align(*it)) {
      alignment = static_cast<char>(*it++);
    }

    auto number = [&]() constexpr -> int {
      int value = 0;
      if (it == end || *it < Char('0') || *it > Char('9')) {
        fail("expected a number");
        return -1;
      }
      while (it != end && *it >= Char('0') && *it <= Char('9')) {
        const auto digit = static_cast<int>(*it++ - Char('0'));
        if (value > (std::numeric_limits<int>::max() - digit) / 10) {
          fail("format specification is too large");
          return -1;
        }
        value = value * 10 + digit;
      }
      return value;
    }; // number

    auto argument = [&]() constexpr -> int {
      ++it; // Skip leading '{'.
      int id = 0;
      if (it != end && *it == Char('}')) {
        std::size_t arg = ctx.next_arg_id();
        if (arg > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
          fail("argument index is too large");
          return -1;
        }
        id = static_cast<int>(arg);
      }
      else if (it != end && *it >= Char('0') && *it <= Char('9')) {
        if (*it == Char('0')) {
          ++it;
        } else {
          id = number();
          if (id < 0)
            return -1;
        }
        ctx.check_arg_id(static_cast<std::size_t>(id));
      } else {
        fail("invalid dynamic argument index");
        return -1;
      }
      if (it == end || *it != Char('}')) {
        fail("invalid dynamic specification");
        return -1;
      }
      ++it;
      return id;
    }; // argument

    if (it != end && *it == Char('0')) {
      fail("invalid string format flag");
      return end;
    }
    if (it != end && *it >= Char('1') && *it <= Char('9')) {
      width = number();
      if (width < 0)
        return end;
    }
    else if (it != end && *it == Char('{')) {
      width_arg = argument();
      if (width_arg < 0)
        return end;
    }
    if (it != end && *it == Char('.')) {
      ++it;
      if (it != end && *it == Char('{')) {
        precision_arg = argument();
        if (precision_arg < 0)
          return end;
      }
      else {
        precision = number();
        if (precision < 0)
          return end;
      }
    }
    if (it != end && (*it == Char('s') || *it == Char('?')))
      debug = *it++ == Char('?');
    if (it != end && *it != Char('}')) {
      fail("invalid string format specification");
      return end;
    }
    return it;
  } // parse

  template<typename Context, typename Fail>
  auto resolve(Context& ctx, Fail fail) const -> string_format_specs {
    auto result = *this;

    auto get = [&](int id) -> int {

      auto visitor = [&](auto value) -> int {
        using T = decltype(value);
        if constexpr (!std::integral<T> || std::same_as<T, bool> || CharType<T>)
        {
          fail("dynamic width or precision is not an integer");
          return 0;
        }
        else {
          if (  std::cmp_less(value, 0)
             || std::cmp_greater(value, std::numeric_limits<int>::max()))
          {
            fail("invalid dynamic width or precision");
            return 0;
          }
          return static_cast<int>(value);
        }
      }; // visitor

      auto visit = [&](auto arg) {
        if constexpr (requires { arg.visit(visitor); })
          return arg.visit(visitor);
        else
          return std::visit_format_arg(visitor, arg);
      }; // visit

      return visit(ctx.arg(static_cast<std::size_t>(id)));
    }; // get

    if (width_arg >= 0)     result.width     = get(width_arg);
    if (precision_arg >= 0) result.precision = get(precision_arg);
    return result;
  } // resolve

  template<typename OutputIt>
  auto pad(OutputIt out, int count) const -> OutputIt {
    for (int i = 0; i < count; ++i)
      out = std::copy_n(_fill.data(), _fill.size(), out);
    return out;
  }
}; // string_format_specs

// Parse the original context so argument IDs and its implementation-provided
// checking information are preserved.
template<CharType Char>
class standard_value_specs {
  string_format_specs<Char> _specs{};

 public:
  constexpr auto parse(std::basic_format_parse_context<Char>& ctx)
    -> decltype(ctx.begin())
  { return _specs.parse(ctx, standard_format_error); }

 protected:
  [[nodiscard]] constexpr auto debug_format() const noexcept -> bool
  { return _specs.debug; }

  template<typename FormatContext>
  auto write(std::basic_string_view<Char> text, FormatContext& ctx) const
    -> decltype(ctx.out())
  {
    const auto specs = _specs.resolve(ctx, standard_format_error);
    // Color/style notations contain only ASCII, so code units equal columns.
    const auto text_size = static_cast<int>(std::ssize(text));
    const auto count   = (specs.precision < 0) ? text_size
                     : std::min(text_size, specs.precision);
    const auto padding = (specs.width > count) ? specs.width - count : 0;
    const auto left    = (specs.alignment == '>') ? padding
                       : (specs.alignment == '^') ? padding / 2 : 0;
    auto out = specs.pad(ctx.out(), left);
    out = std::copy_n(text.begin(), static_cast<std::size_t>(count), out);
    return specs.pad(out, padding - left);
  }
}; // standard_value_specs

template<typename OutputIt, typename T>
[[nodiscard]] constexpr
auto write_formatted_value(OutputIt out, const T& value, bool debug) -> OutputIt
{
  if constexpr (requires { write_debug_value(std::move(out), value); }) {
    if (debug) return write_debug_value(std::move(out), value);
  }
  return write_value(std::move(out), value);
} // write_formatted_value

/// Formats short fields whose full representation fits in the default inline
/// buffer (256 code units), even when precision truncates the output.
template<typename T, CharType Char>
struct standard_value_formatter : standard_value_specs<Char> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto buffer = basic_inline_buffer<Char>{};
    (void) write_formatted_value(
                      std::back_inserter(buffer), value, this->debug_format());
    return this->write(buffer.view(), ctx);
  }
}; // standard_value_formatter

} // tjg::detail
