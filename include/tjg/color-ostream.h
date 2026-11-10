// Formatting library for C++ - styled std::ostream output
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to LICENSE.

#ifndef TJG_COLOR_OSTREAM_H_
#define TJG_COLOR_OSTREAM_H_

#include "color.h"
#include "tree_format_specs.h"

#include <ostream>
#include <string_view>

#include <concepts>
#include <limits>

namespace tjg {

template <typename Char> class basic_styled_text;

namespace detail {
template <typename Char> struct styled_text_renderer;

inline void format_ostream_message(std::ostream& os, memory_buffer& buffer,
                                   text_style ts, std::string_view fmt,
                                   std::format_args args) {
  const auto locale = os.getloc();
  detail::vformat_to(buffer, ts, fmt, args, &locale);
}

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
template <typename Char, typename T>
  requires(Color<T> || std::same_as<T, no_color_t> || std::same_as<T, color_t> ||
           std::same_as<T, text_style>)
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
 * Prints styled text using the standard non-Unicode stream output path.
 * A nonempty style precedes the text with SGR sequences and appends a reset
 * to terminal defaults. An empty style emits no SGR sequences.
 */
inline void vprint_nonunicode(std::ostream& os, text_style ts,
                              std::string_view fmt, std::format_args args) {
  auto buffer = memory_buffer{};
  detail::format_ostream_message(os, buffer, ts, fmt, args);
  const auto text = std::string_view{buffer.data(), buffer.size()};
  std::vprint_nonunicode(os, "{}", std::make_format_args(text));
}

/// Prints styled text using the standard Unicode stream output path.
inline void vprint_unicode(std::ostream& os, text_style ts,
                           std::string_view fmt, std::format_args args) {
  auto buffer = memory_buffer{};
  detail::format_ostream_message(os, buffer, ts, fmt, args);
  const auto text = std::string_view{buffer.data(), buffer.size()};
  std::vprint_unicode(os, "{}", std::make_format_args(text));
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
  auto buffer = memory_buffer{};
  detail::format_ostream_message(os, buffer, ts, fmt.get(),
                                 std::make_format_args(args...));
  std::print(os, "{}", std::string_view{buffer.data(), buffer.size()});
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template <typename... T>
void println(std::ostream& os, text_style ts, std::format_string<T...> fmt,
             T&&... args) {
  auto buffer = memory_buffer{};
  detail::format_ostream_message(os, buffer, ts, fmt.get(),
                                 std::make_format_args(args...));
  std::println(os, "{}", std::string_view{buffer.data(), buffer.size()});
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
