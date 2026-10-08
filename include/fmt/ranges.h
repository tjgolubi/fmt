// Formatting library for C++23, based on {fmt}.
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors.
// Distributed under the MIT license; see LICENSE.
#ifndef FMT_RANGES_H_
#define FMT_RANGES_H_
#include "format.h"
#include <ranges>
#if !defined(__cpp_lib_format_ranges) || __cpp_lib_format_ranges < 202207L
#  error "fmt/ranges.h requires C++23 standard range formatting; no fallback is provided"
#endif

namespace fmt { inline namespace v12 {
using std::range_format;
using std::range_formatter;
using std::format_kind;
namespace detail {
template <typename It, typename Sentinel, typename Char> struct join_view {
  It begin;
  Sentinel end;
  std::basic_string_view<Char> separator;
};
}
template <std::input_iterator It, std::sentinel_for<It> Sentinel>
auto join(It begin, Sentinel end, string_view separator)
    -> detail::join_view<It, Sentinel, char> {
  return {begin, end, separator};
}
template <std::ranges::input_range Range>
auto join(Range&& range, string_view separator) {
  return fmt::join(std::ranges::begin(range), std::ranges::end(range), separator);
}
} }
template <typename It, typename Sentinel, typename Char>
struct std::formatter<fmt::detail::join_view<It, Sentinel, Char>, Char>
    : std::formatter<std::remove_cvref_t<std::iter_reference_t<It>>, Char> {
  using base = std::formatter<std::remove_cvref_t<std::iter_reference_t<It>>, Char>;
  template <typename Context>
  auto format(const fmt::detail::join_view<It, Sentinel, Char>& value,
              Context& ctx) const -> decltype(ctx.out()) {
    auto out = ctx.out();
    auto it = value.begin;
    if (it == value.end) return out;
    out = base::format(*it, ctx);
    while (++it != value.end) {
      out = std::copy(value.separator.begin(), value.separator.end(), out);
      ctx.advance_to(out);
      out = base::format(*it, ctx);
    }
    return out;
  }
};
#endif
