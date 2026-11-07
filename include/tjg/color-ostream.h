// Formatting library for C++ - styled std::ostream output
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to LICENSE.

#ifndef TJG_COLOR_OSTREAM_H_
#define TJG_COLOR_OSTREAM_H_

#include <limits>
#include <ostream>
#include <string_view>

#include "color.h"
#include "tree_format_specs.h"

namespace tjg {

template <typename Char> class basic_styled_text;

namespace detail {
template <typename Char> struct styled_text_renderer;

template <typename Char> struct color_ostream_types {
  using string_view = std::basic_string_view<Char>;
  using format_args = detail::format_args<Char>;
};

template <typename Char, std::size_t SIZE>
void write_color_buffer(std::basic_ostream<Char>& os,
                        const basic_memory_buffer<Char, SIZE>& buf) {
  auto remaining = buf.size();
  auto data = buf.data();
  constexpr auto max_size =
      static_cast<std::size_t>((std::numeric_limits<std::streamsize>::max)());
  do {
    const auto count = (std::min)(remaining, max_size);
    os.write(data, static_cast<std::streamsize>(count));
    data += count;
    remaining -= count;
  } while (remaining != 0);
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
  auto buffer = basic_memory_buffer<Char, 64>{};
  detail::write_emphasis<Char>(std::back_inserter(buffer), value);
  return os << std::basic_string<Char>{buffer.data(), buffer.size()};
}

/**
 * Inserts a color or text style using its formatter's readable notation.
 * Stream width, fill, and alignment apply as for ordinary string insertion.
 */
template <typename Char, typename T,
          std::enable_if_t<
              detail::is_color<T>::value || std::is_same_v<T, no_color_t> ||
                  std::is_same_v<T, color_t> || std::is_same_v<T, text_style>,
              int> = 0>
auto operator<<(std::basic_ostream<Char>& os, const T& value)
    -> std::basic_ostream<Char>& {
  auto buffer = basic_memory_buffer<Char, 64>{};
  detail::write_style_value<Char>(std::back_inserter(buffer), value);
  return os << std::basic_string<Char>{buffer.data(), buffer.size()};
}

/**
 * Inserts styled text using the formatter's ANSI rendering machinery.
 * Stream width, fill, and alignment control padding. Stream precision is
 * ignored, as for string insertion. Internal alignment behaves as right
 * alignment; only left places padding after text.
 * Field width is consumed; other stream settings are preserved.
 * Include `tjg/styled_text.h` as well as this header.
 */
template <typename Char>
auto operator<<(std::basic_ostream<Char>& os,
                const basic_styled_text<Char>& value)
    -> std::basic_ostream<Char>& {
  typename std::basic_ostream<Char>::sentry sentry{os};
  if (!sentry) return os;
  auto specs = detail::tree_format_specs<Char>{};
  const auto width = os.width();
  const auto max_int = (std::numeric_limits<int>::max)();
  if (width > max_int)
    detail::standard_format_error("styled text stream width is too large");
  if (width > 0) specs.width = static_cast<int>(width);
  const auto fill = os.fill();
  specs.set_fill(std::basic_string_view<Char>{&fill, 1});
  specs.alignment =
      (os.flags() & std::ios_base::adjustfield) == std::ios_base::left ? '<'
                                                                       : '>';
  os.width(0);
  auto buffer = basic_memory_buffer<Char>{};
  auto output = std::back_inserter(buffer);
  struct output_context {
    decltype(output) iterator;
    auto out() { return iterator; }
  };
  auto ctx = output_context{output};
  detail::styled_text_renderer<Char>::format_with_specs(value, ctx, specs);
  detail::write_color_buffer(os, buffer);
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
  auto buffer = basic_memory_buffer<Char>();
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
  auto buffer = basic_memory_buffer<Char>();
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
void print(std::ostream& os, text_style ts, std::format_string<T...> fmt,
           T&&... args) {
  tjg::vprint(os, ts, fmt.get(), std::make_format_args(args...));
}

/// Prints styled formatted text to a wide stream.
template <typename... T>
void print(std::wostream& os, text_style ts, std::wformat_string<T...> fmt,
           T&&... args) {
  tjg::vprint(os, ts, fmt.get(), std::make_wformat_args(args...));
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template <typename... T>
void println(std::ostream& os, text_style ts, std::format_string<T...> fmt,
             T&&... args) {
  tjg::vprintln(os, ts, fmt.get(), std::make_format_args(args...));
}

/// Prints styled formatted text to a wide stream, followed by a newline.
template <typename... T>
void println(std::wostream& os, text_style ts, std::wformat_string<T...> fmt,
             T&&... args) {
  tjg::vprintln(os, ts, fmt.get(), std::make_wformat_args(args...));
}

namespace detail {

template <typename T, typename Char>
auto operator<<(std::basic_ostream<Char>& os, const styled_arg<T>& arg)
    -> std::basic_ostream<Char>& {
  if (arg.style.empty()) return os << arg.value;

  const auto sgr = to_sgr<Char>(arg.style);
  os.write(sgr.data(), static_cast<std::streamsize>(sgr.size()));
  constexpr auto reset = sgr_reset<Char>();
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    os << arg.value;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
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

}  // namespace tjg

#endif  // TJG_COLOR_OSTREAM_H_
