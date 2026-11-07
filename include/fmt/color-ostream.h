// Formatting library for C++ - styled std::ostream output
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COLOR_OSTREAM_H_
#define FMT_COLOR_OSTREAM_H_

#include "color.h"
#include "ostream.h"
#include "xchar.h"

#ifndef FMT_MODULE
#  include <limits>
#endif

namespace tjg {
FMT_BEGIN_EXPORT

template <typename Char> class basic_styled_text;

namespace detail {

template <typename Char> struct color_ostream_types {
  using string_view = fmt::basic_string_view<Char>;
  using format_args = fmt::basic_format_args<fmt::buffered_context<Char>>;
};

inline void write_color_buffer(std::ostream& os,
                               fmt::detail::buffer<char>& buf) {
  fmt::print(os, FMT_STRING("{}"), fmt::string_view(buf.data(), buf.size()));
}

template <typename Char>
void write_color_buffer(std::basic_ostream<Char>& os,
                        fmt::detail::buffer<Char>& buf) {
  write_buffer(os, buf);
}

}  // namespace detail

/**
 * Inserts set emphasis names in declaration order, separated by spaces and
 * enclosed in braces. Empty emphasis displays `{}`. Width, fill, and alignment
 * apply as for ordinary string insertion.
 */
template <typename Char>
auto operator<<(std::basic_ostream<Char>& os, emphasis value)
    -> std::basic_ostream<Char>& {
  auto buffer = fmt::basic_memory_buffer<Char, 64>{};
  detail::write_emphasis<Char>(fmt::basic_appender<Char>{buffer}, value);
  return os << std::basic_string<Char>{buffer.data(), buffer.size()};
}

/**
 * Inserts a color or text style using its formatter's readable notation.
 * Stream width, fill, and alignment apply as for ordinary string insertion.
 */
template <typename Char, typename T,
          FMT_ENABLE_IF(detail::is_color<T>::value ||
                        std::is_same<T, no_color_t>::value ||
                        std::is_same<T, color_t>::value ||
                        std::is_same<T, text_style>::value)>
auto operator<<(std::basic_ostream<Char>& os, const T& value)
    -> std::basic_ostream<Char>& {
  auto buffer = fmt::basic_memory_buffer<Char, 64>{};
  detail::write_style_value<Char>(fmt::basic_appender<Char>{buffer}, value);
  return os << std::basic_string<Char>{buffer.data(), buffer.size()};
}

/**
 * Inserts styled text using the formatter's ANSI rendering machinery.
 * Stream width, fill, and alignment control padding. Stream precision is
 * ignored, as for string insertion. Internal alignment behaves as right
 * alignment; only left places padding after text.
 * Field width is consumed; other stream settings are preserved.
 * Include `fmt/styled_text.h` as well as this header.
 */
template <typename Char>
auto operator<<(std::basic_ostream<Char>& os,
                const basic_styled_text<Char>& value)
    -> std::basic_ostream<Char>& {
  typename std::basic_ostream<Char>::sentry sentry{os};
  if (!sentry) return os;
  auto specs = fmt::format_specs{};
  const auto width = os.width();
  const auto max_int = (std::numeric_limits<int>::max)();
  if (width > max_int)
    fmt::report_error("styled text stream width is too large");
  if (width > 0) specs.width = static_cast<int>(width);
  const auto fill = os.fill();
  specs.set_fill(fmt::basic_string_view<Char>{&fill, 1});
  specs.set_align((os.flags() & std::ios_base::adjustfield) ==
                          std::ios_base::left
                      ? fmt::align::left
                      : fmt::align::right);
  os.width(0);
  auto buffer = fmt::basic_memory_buffer<Char>{};
  auto ctx = fmt::buffered_context<Char>{fmt::basic_appender<Char>{buffer}, {}};
  fmt::formatter<basic_styled_text<Char>, Char>::format_with_specs(value, ctx,
                                                                   specs);
  fmt::detail::write_buffer(os, buffer);
  return os;
}

/**
 * Prints formatted text to `os` using `ts`.
 * A nonempty style precedes the text with SGR sequences and appends a reset
 * to terminal defaults. An empty style emits no SGR sequences.
 */
template <typename Char>
void vprint(std::basic_ostream<Char>& os, text_style ts,
            typename detail::color_ostream_types<Char>::string_view fmt,
            typename detail::color_ostream_types<Char>::format_args args) {
  auto buffer = fmt::basic_memory_buffer<Char>();
  detail::vformat_to(buffer, ts, fmt, args);
  detail::write_color_buffer(os, buffer);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * The newline follows the terminal reset for a nonempty style.
 */
template <typename Char>
void vprintln(std::basic_ostream<Char>& os, text_style ts,
              typename detail::color_ostream_types<Char>::string_view fmt,
              typename detail::color_ostream_types<Char>::format_args args) {
  auto buffer = fmt::basic_memory_buffer<Char>();
  detail::vformat_to(buffer, ts, fmt, args);
  buffer.push_back(static_cast<Char>('\n'));
  detail::write_color_buffer(os, buffer);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a terminal reset for
 * a nonempty style. Reset does not restore a previously active style.
 *
 * **Example**:
 *
 *     tjg::print(os, tjg::text_style{tjg::emphasis::bold}, "Result: {}", 42);
 */
template <typename... T>
void print(std::ostream& os, text_style ts, fmt::format_string<T...> fmt,
           T&&... args) {
  tjg::vprint(os, ts, fmt.get(), fmt::make_format_args(args...));
}

/// Prints styled formatted text to a wide stream.
template <typename... T>
void print(std::wostream& os, text_style ts, fmt::wformat_string<T...> fmt,
           T&&... args) {
  tjg::vprint(os, ts, fmt.get(), fmt::make_wformat_args(args...));
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template <typename... T>
void println(std::ostream& os, text_style ts, fmt::format_string<T...> fmt,
             T&&... args) {
  tjg::vprintln(os, ts, fmt.get(), fmt::make_format_args(args...));
}

/// Prints styled formatted text to a wide stream, followed by a newline.
template <typename... T>
void println(std::wostream& os, text_style ts, fmt::wformat_string<T...> fmt,
             T&&... args) {
  tjg::vprintln(os, ts, fmt.get(), fmt::make_wformat_args(args...));
}

namespace detail {

template <typename T, typename Char>
auto operator<<(std::basic_ostream<Char>& os, const styled_arg<T>& arg)
    -> std::basic_ostream<Char>& {
  if (arg.style.empty()) return os << arg.value;

  const auto sgr = to_sgr<Char>(arg.style);
  os.write(sgr.data(), static_cast<std::streamsize>(sgr.size()));
  constexpr auto reset = sgr_reset<Char>();
#if FMT_USE_EXCEPTIONS
  try {
#endif
    os << arg.value;
#if FMT_USE_EXCEPTIONS
  } catch (...) {
    try {
      os.write(reset.data(), static_cast<std::streamsize>(reset.size()));
    } catch (...) {
      // Reset is best-effort; preserve the original insertion exception.
    }
    throw;
  }
#endif
  return os.write(reset.data(), static_cast<std::streamsize>(reset.size()));
}

}  // namespace detail

FMT_END_EXPORT
}  // namespace tjg

#endif  // FMT_COLOR_OSTREAM_H_
