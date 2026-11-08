// String specifications shared by TJG styled text and readable values.
#ifndef TJG_TREE_FORMAT_SPECS_H_
#define TJG_TREE_FORMAT_SPECS_H_
#include "text_support.h"

#include <format>

#include <iterator>

#include <concepts>
#include <utility>

namespace tjg::detail {
template <typename Char> struct tree_format_specs {
  std::size_t width = 0;
  std::size_t precision = (std::numeric_limits<std::size_t>::max)();
  int width_arg = -1, precision_arg = -1;
  char alignment = '<';
  Char fill[4] = {Char(' ')};
  std::size_t fill_size = 1;
  bool debug = false;
  constexpr void set_fill(std::basic_string_view<Char> value) {
    fill_size = value.size();
    std::copy(value.begin(), value.end(), fill);
  }
  template <typename Context, typename Fail>
  constexpr auto parse(Context& ctx, Fail fail) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    const auto end = ctx.end();
    auto align = [](Char c) {
      return c == Char('<') || c == Char('>') || c == Char('^');
    };
    if (it == end || *it == Char('}')) return it;
    auto next = it + 1;
    if constexpr (std::same_as<Char, char>) {
      const auto bytes = std::string_view{it, end};
      next = it + static_cast<std::ptrdiff_t>(decode_utf8(bytes).size);
    }
    if (next != end && align(*next)) {
      if (*it == Char('{') || *it == Char('}')) fail("invalid fill character");
      set_fill(std::basic_string_view<Char>{it, next});
      alignment = static_cast<char>(*next);
      it = next + 1;
    } else if (align(*it)) {
      alignment = static_cast<char>(*it++);
    }
    auto number = [&]() constexpr -> std::size_t {
      std::size_t value = 0;
      if (it == end || *it < Char('0') || *it > Char('9'))
        fail("expected a number");
      while (it != end && *it >= Char('0') && *it <= Char('9')) {
        const auto digit = static_cast<unsigned>(*it++ - Char('0'));
        if (value >
            (static_cast<unsigned>((std::numeric_limits<int>::max)()) - digit) /
                10)
          fail("format specification is too large");
        value = value * 10 + digit;
      }
      return value;
    };
    auto argument = [&]() constexpr -> int {
      ++it;
      std::size_t id = 0;
      if (it != end && *it == Char('}'))
        id = static_cast<std::size_t>(ctx.next_arg_id());
      else if (it != end && *it >= Char('0') && *it <= Char('9')) {
        id = number();
        ctx.check_arg_id(static_cast<int>(id));
      } else {
        fail("invalid dynamic argument index");
      }
      if (it == end || *it != Char('}')) fail("invalid dynamic specification");
      ++it;
      if (id > static_cast<unsigned>((std::numeric_limits<int>::max)()))
        fail("argument index is too large");
      return static_cast<int>(id);
    };
    if (it != end && *it == Char('0')) fail("invalid string format flag");
    if (it != end && *it >= Char('1') && *it <= Char('9'))
      width = number();
    else if (it != end && *it == Char('{'))
      width_arg = argument();
    if (it != end && *it == Char('.')) {
      ++it;
      if (it != end && *it == Char('{'))
        precision_arg = argument();
      else
        precision = number();
    }
    if (it != end && (*it == Char('s') || *it == Char('?')))
      debug = *it++ == Char('?');
    if (it != end && *it != Char('}'))
      fail("invalid string format specification");
    return it;
  }
  template <typename Context, typename Fail>
  auto resolve(Context& ctx, Fail fail) const -> tree_format_specs {
    auto result = *this;
    auto get = [&](int id) -> std::size_t {
      auto visitor = [&](auto value) -> std::size_t {
        using T = decltype(value);
        if constexpr (std::integral<T> && !std::same_as<T, bool> &&
                      !std::same_as<T, char> && !std::same_as<T, wchar_t>) {
          if (std::cmp_less(value, 0) ||
              std::cmp_greater(value, (std::numeric_limits<int>::max)()))
            fail("invalid dynamic width or precision");
          return static_cast<std::size_t>(value);
        } else {
          fail("dynamic width or precision is not an integer");
          return 0;
        }
      };
      auto visit = [&](auto arg) -> std::size_t {
        if constexpr (requires { arg.visit(visitor); })
          return arg.visit(visitor);
        else
          return std::visit_format_arg(visitor, arg);
      };
      return visit(ctx.arg(id));
    };
    if (width_arg >= 0) result.width = get(width_arg);
    if (precision_arg >= 0) result.precision = get(precision_arg);
    return result;
  }
  template <typename OutputIt>
  auto pad(OutputIt out, std::size_t count) const -> OutputIt {
    for (std::size_t i = 0; i < count; ++i)
      out = std::copy_n(fill, fill_size, out);
    return out;
  }
};
}  // namespace tjg::detail
#endif
