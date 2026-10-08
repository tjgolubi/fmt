// Formatting library for C++23, based on {fmt}.
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors.
// Distributed under the MIT license; see LICENSE.
#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_
#include "format.h"
#include <ostream>
#include <sstream>
namespace fmt { inline namespace v12 {
namespace detail {
template <typename T> struct streamed_view { const T& value; };
}
template <typename T>
constexpr auto streamed(const T& value) -> detail::streamed_view<T> {
  return {value};
}
template <typename Char>
struct basic_ostream_formatter : std::formatter<std::basic_string_view<Char>, Char> {
  template <typename T, typename Context>
  auto format(const T& value, Context& ctx) const -> decltype(ctx.out()) {
    std::basic_ostringstream<Char> stream;
    stream.imbue(std::locale::classic());
    stream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    stream << value;
    const auto text = stream.str();
    return std::formatter<std::basic_string_view<Char>, Char>::format(text, ctx);
  }
};
using ostream_formatter = basic_ostream_formatter<char>;
inline void vprint(std::ostream& stream, string_view s, format_args args) {
  const auto text = std::vformat(s, args);
  stream.write(text.data(), static_cast<std::streamsize>(text.size()));
}
template <typename... T>
void print(std::ostream& stream, format_string<T...> s, T&&... args) {
  fmt::vprint(stream, s.get(), std::make_format_args(args...));
}
template <typename... T>
void println(std::ostream& stream, format_string<T...> s, T&&... args) {
  fmt::print(stream, s, std::forward<T>(args)...);
  stream.put('\n');
}
} }
template <typename T, typename Char>
struct std::formatter<fmt::detail::streamed_view<T>, Char>
    : fmt::basic_ostream_formatter<Char> {
  template <typename Context>
  auto format(fmt::detail::streamed_view<T> value, Context& ctx) const
      -> decltype(ctx.out()) {
    return fmt::basic_ostream_formatter<Char>::format(value.value, ctx);
  }
};
#endif
