/// @file
/// Styled std::ostream output
/// @copyright 2026 Terry Golubiewski, all rights reserved.
#pragma once

#include "styled_text-format.hpp"
#include "text_style-format.hpp"
#include "colors-format.hpp"
#include "emphasis-format.hpp"
#include "sgr_support.hpp"
#include <tjg/inline_buffer.hpp>
#include <tjg/utility.hpp>

#include <ostream>
#include <string_view>
#include <iterator>
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>

namespace tjg::color {

/**
 * Inserts color values and emphasis using their value notation.
 * Emphasis names are enclosed in braces; empty emphasis displays `{}`.
 * Width, fill, and alignment apply as for ordinary string insertion.
 */
template<CharType Char, typename T>
  requires (Color<T>
            || std::same_as<std::remove_cvref_t<T>, color_t>
            || std::same_as<std::remove_cvref_t<T>, no_color_t>
            || std::same_as<std::remove_cvref_t<T>, text_style>
            || std::same_as<std::remove_cvref_t<T>, emphasis>)
        && requires(char* out, const T& value)
           { (void) tjg::color::write_value(out, value); }
auto operator<<(std::basic_ostream<Char>& os, const T& value)
  -> std::basic_ostream<Char>&
{
  auto buf = basic_inline_buffer<Char>{};
  (void) write_value(std::back_inserter(buf), value);
  return os << buf.view();
}

/**
 * Writes a style's SGR sequence without a trailing reset. An empty style
 * writes a reset. Pending stream width and other formatting settings are
 * preserved. This raw output does not update the tracked formatting style.
 */
template<CharType Char>
auto operator<<(std::basic_ostream<Char>& os, const text_style& style)
  -> std::basic_ostream<Char>&
{
  auto buffer = basic_sgr_buffer<Char>{};
  detail::append_sgr(buffer, style.empty() ? absolute(style) : style);
  return os.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
}

namespace detail {

template<typename T, CharType Char>
auto operator<<(std::basic_ostream<Char>& os, const styled_arg<T>& arg)
  -> std::basic_ostream<Char>&
{
  if (arg.style.empty())
    return os << arg.value;

  constexpr auto reset = sgr_reset<Char>();

  auto write_view = [&os](std::basic_string_view<Char> sgr) -> decltype(os)
    { return os.write(sgr.data(), static_cast<std::streamsize>(sgr.size())); };

  auto sgr = basic_sgr_buffer<Char>{};
  append_sgr(sgr, arg.style);
  write_view(sgr.view());
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    os << arg.value;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch (...) {
    // Reset is best-effort; preserve the original insertion exception.
    try { write_view(reset); }
    catch (...) { }
    throw;
  }
#endif
  return write_view(reset);
} // << styled_arg

} // detail

#define TJG_SGR_OSTREAM_SCOPE(stream, style) \
  auto writer = [&](const char* data, std::size_t size) \
    { stream.write(data, static_cast<std::streamsize>(size)); }; \
  TJG_SGR_SCOPE(writer, style)

/**
 * Prints styled text using the standard non-Unicode stream output path.
 * A nonempty style precedes the text with SGR sequences and appends a reset
 * to terminal defaults. An empty style emits no SGR sequences.
 */
inline void vprint_nonunicode(std::ostream& os, text_style ts,
                              std::string_view fmt, std::format_args args)
{
  TJG_SGR_OSTREAM_SCOPE(os, ts);
  std::vprint_nonunicode(os, fmt, args);
}

/// Prints styled text using the standard Unicode stream output path.
inline void vprint_unicode(std::ostream& os, text_style ts,
                           std::string_view fmt, std::format_args args)
{
  TJG_SGR_OSTREAM_SCOPE(os, ts);
  std::vprint_unicode(os, fmt, args);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a terminal reset for
 * a nonempty style. Reset does not restore a previously active style.
 *
 * **Example**:
 *
 *     tjg::color::print(os, tjg::color::text_style{tjg::color::emphasis::bold}, "Result: {}", 42);
 */
template<typename... T>
void print(std::ostream& os, text_style ts, std::format_string<T...> fmt,
           T&&... args)
{
  TJG_SGR_OSTREAM_SCOPE(os, ts);
  std::print(os, fmt, std::forward<T>(args)...);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template<typename... T>
void println(std::ostream& os, text_style ts, std::format_string<T...> fmt,
             T&&... args)
{
  tjg::color::print(os, ts, fmt, std::forward<T>(args)...);
  os.put('\n');
}

/**
 * Inserts styled text using the formatter's ANSI rendering machinery.
 * Stream width, fill, and alignment control padding. Stream precision is
 * ignored, as for string insertion. Internal alignment behaves as right
 * alignment; only left places padding after text.
 * Field width is consumed; other stream settings are preserved.
 */
template<CharType Char>
auto operator<<(std::basic_ostream<Char>& os,
                const basic_styled_text<Char>& value)
  -> std::basic_ostream<Char>&
{
  auto sentry = typename std::basic_ostream<Char>::sentry{os};
  if (!sentry)
    return os;

  auto specs = detail::string_format_specs<Char>{};
  const auto width = os.width();
  if (width > static_cast<decltype(width)>(std::numeric_limits<int>::max()))
    detail::standard_format_error("styled text stream width is too large");
  if (width > 0) specs.width = static_cast<std::size_t>(width);
  const auto fill = os.fill();
  specs.set_fill(std::basic_string_view<Char>{&fill, 1});
  specs.alignment =
      ((os.flags() & std::ios_base::adjustfield) == std::ios_base::left) ? '<'
                                                                         : '>';
  os.width(0);

  auto output = std::ostreambuf_iterator<Char>{os};

  bool failed;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  try {
#endif
    auto renderer = detail::styled_text_renderer<Char>{specs};
    failed = renderer.format(value, output).failed();
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  } catch (...) {
    const bool rethrow = (os.exceptions() & std::ios_base::badbit) != 0;
    // Keep the stream buffer's exception if setstate() also throws.
    try { os.setstate(std::ios_base::badbit); }
    catch (...) { }
    if (rethrow) throw;
    return os;
  }
#endif

  if (failed)
    os.setstate(std::ios_base::badbit);
  return os;
} // << styled_text

} // tjg::color

#undef TJG_SGR_OSTREAM_SCOPE
