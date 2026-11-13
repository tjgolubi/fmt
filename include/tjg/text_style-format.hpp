/// @file
/// `text_style` formatting
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include "text_style.hpp"
#include "sgr_support.hpp"
#include "format_support.hpp"

#include <print>
#include <format>
#include <locale>
#include <cstdint>

namespace tjg {

namespace detail {

class style_scope {
  text_style _saved;

 public:
  style_scope() noexcept : _saved{get_terminal_state()} {}

  // Restore shared state on exceptions; partial output may remain.
  ~style_scope() { get_terminal_state() = _saved; }

  style_scope(const style_scope&) = delete;
  void operator=(const style_scope&) = delete;
};  // style_scope

template<CharType Char>
using format_context =
    std::conditional_t<std::same_as<Char, char>, std::format_context,
                                                 std::wformat_context>;
template<CharType Char>
using format_args = std::basic_format_args<format_context<Char>>;

template<typename Buffer, CharType Char = typename Buffer::value_type>
void vformat_to(Buffer& buf, text_style ts,
                std::basic_string_view<Char> pattern, format_args<Char> args,
                const std::locale* locale = nullptr) {
  style_scope scope{};
  append_sgr(buf, ts);
  get_terminal_state() = text_style{ts}.absolute(false);
  if (locale)
    std::vformat_to(std::back_inserter(buf), *locale, pattern, args);
  else
    std::vformat_to(std::back_inserter(buf), pattern, args);
  if (ts != text_style()) reset_color(buf);
}  // vformat_to

} // detail

/**
 * The colored analog of
 * [std::vprint_nonunicode()](https://en.cppreference.com/w/cpp/io/vprint_nonunicode.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline void vprint_nonunicode(FILE* f, text_style ts, std::string_view fmt,
                              std::format_args args)
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  const auto text = std::string_view{buf.data(), buf.size()};
  std::vprint_nonunicode(f, "{}", std::make_format_args(text));
}

/// Prints a styled message to stdout without Unicode-specific handling.
inline void vprint_nonunicode(text_style ts, std::string_view fmt,
                              std::format_args args)
{ tjg::vprint_nonunicode(stdout, ts, fmt, args); }

/// Prints a styled message using the standard Unicode output path.
inline void vprint_unicode(FILE* f, text_style ts, std::string_view fmt,
                           std::format_args args)
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  const auto text = std::string_view{buf.data(), buf.size()};
  std::vprint_unicode(f, "{}", std::make_format_args(text));
}

/// Prints a styled message to stdout using the standard Unicode output path.
inline void vprint_unicode(text_style ts, std::string_view fmt,
                           std::format_args args)
{ tjg::vprint_unicode(stdout, ts, fmt, args); }

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     tjg::print(stderr, tjg::bold().fg(tjg::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(FILE* f, text_style ts, std::format_string<T...> fmt, T&&... args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt.get(), std::make_format_args(args...));
  std::print(f, "{}", std::string_view{buf.data(), buf.size()});
}

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     tjg::print(tjg::bold().fg(tjg::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(text_style ts, std::format_string<T...> fmt, T&&... args)
{ return print(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     tjg::println(stderr, tjg::bold().fg(tjg::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(FILE* f, text_style ts, std::format_string<T...> fmt,
             T&&... args)
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt.get(), std::make_format_args(args...));
  std::println(f, "{}", std::string_view{buf.data(), buf.size()});
}

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     tjg::println(tjg::bold().fg(tjg::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(text_style ts, std::format_string<T...> fmt, T&&... args)
{ return println(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * The colored analog of
 * [std::vformat()](https://en.cppreference.com/w/cpp/utility/format/vformat.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline auto vformat(text_style ts, std::string_view fmt, std::format_args args)
  -> std::string
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  return {buf.data(), buf.size()};
}

/**
 * The colored analog of
 * [std::format()](https://en.cppreference.com/w/cpp/utility/format/format.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 * Reset does not restore a previously active style.
 *
 * **Example**:
 *
 * ```
 * #include <tjg/color.h>
 * std::string message = tjg::format(tjg::bold().fg(tjg::color::red),
 *                                   "The answer is {}", 42);
 * ```
 */
template<typename... T>
inline auto format(text_style ts, std::format_string<T...> fmt, T&&... args)
  -> std::string
{ return tjg::vformat(ts, fmt.get(), std::make_format_args(args...)); }

/**
 * The colored analog of
 * [std::vformat_to()](https://en.cppreference.com/w/cpp/utility/format/vformat_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
template<std::output_iterator<const char&> OutputIt>
auto vformat_to(OutputIt out, text_style ts, std::string_view pattern,
                std::format_args args)
  -> OutputIt
{
  detail::style_scope scope{};
  auto sgr = basic_inline_buffer<char, sgr_max_length>{};
  detail::append_sgr(sgr, ts);
  out = std::copy(sgr.begin(), sgr.end(), out);
  get_terminal_state() = text_style{ts}.absolute(false);
  out = std::vformat_to(std::move(out), pattern, args);
  if (ts != text_style()) {
    const auto reset = sgr_reset();
    out = std::copy(reset.begin(), reset.end(), out);
  }
  return out;
}

/**
 * The colored analog of
 * [std::format_to()](https://en.cppreference.com/w/cpp/utility/format/format_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     std::vector<char> out;
 *     tjg::format_to(std::back_inserter(out),
 *                    tjg::bold().fg(tjg::color::red), "{}", 42);
 */
template<std::output_iterator<const char&> OutputIt, typename... T>
inline auto format_to(OutputIt out, text_style ts, std::format_string<T...> fmt,
                      T&&... args)
  -> OutputIt
{ return vformat_to(out, ts, fmt.get(), std::make_format_args(args...)); }

inline auto vformat(text_style ts, std::wstring_view pattern,
                    std::wformat_args args)
  -> std::wstring
{
  auto buf = basic_memory_buffer<wchar_t>{};
  detail::vformat_to(buf, ts, pattern, args);
  return {buf.data(), buf.size()};
}

template<typename... T>
inline auto format(text_style ts, std::wformat_string<T...> pattern,
                   T&&... args)
 -> std::wstring
{ return tjg::vformat(ts, pattern.get(), std::make_wformat_args(args...)); }

template<std::output_iterator<const wchar_t&> OutputIt>
auto vformat_to(OutputIt out, text_style ts, std::wstring_view pattern,
                std::wformat_args args)
  -> OutputIt
{
  detail::style_scope scope{};
  auto sgr = basic_inline_buffer<wchar_t, sgr_max_length>{};
  detail::append_sgr(sgr, ts);
  out = std::copy(sgr.begin(), sgr.end(), out);
  get_terminal_state() = text_style{ts}.absolute(false);
  out = std::vformat_to(std::move(out), pattern, args);
  if (ts != text_style()) {
    const auto reset = sgr_reset<wchar_t>();
    out = std::copy(reset.begin(), reset.end(), out);
  }
  return out;
}

template<std::output_iterator<const wchar_t&> OutputIt, typename... T>
auto format_to(OutputIt out, text_style ts, std::wformat_string<T...> pattern,
               T&&... args)
  -> OutputIt
{
  return tjg::vformat_to(std::move(out), ts, pattern.get(),
                         std::make_wformat_args(args...));
}

namespace detail {

template<typename OutputIt, CharType Char = char>
auto write_debug(OutputIt out, text_style value) -> OutputIt {
  if (value.empty()) {
    *out++ = Char{'{'};
    *out++ = Char{'}'};
    return out;
  }
  if (value.is_absolute()) *out++ = Char{'='};
  const auto bits = std::to_underlying(value.emph());
  for (int bit = 0; bit < 8; ++bit) {
    if (bits & (1u << bit)) *out++ = Char{"bfi_!r?-"[bit]};
  }
  if (!value.has_fg() && !value.has_bg()) return out;
  if (value.is_absolute() || bits != 0) *out++ = Char{' '};
  if (value.has_fg()) out = write_color<Char>(out, value.fg());
  if (value.has_bg()) {
    *out++ = Char{'/'};
    out = write_color<Char>(out, value.bg());
  }
  return out;
} // write_debug text_style

template<CharType Char, typename OutputIt>
auto write_style_value(OutputIt out, emphasis value) -> OutputIt {
  return write_emphasis<Char>(out, value);
}

template<typename T, CharType Char>
struct standard_style_value_formatter : standard_value_specs<Char> {
  template<typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto buffer = basic_inline_buffer<Char, 64>{};
    write_style_value<Char>(std::back_inserter(buffer), value);
    return this->write(buffer.view(), ctx);
  }
};

template<typename T>
struct styled_arg {
  const T& value;
  text_style style;
  constexpr styled_arg(const T& v, text_style s) : value{v}, style{s} {}
};  // styled_arg

/// Formats a styled argument using the underlying type's format specifications.
template<typename T, CharType Char, typename Formatter>
struct basic_styled_arg_formatter : Formatter {
  template<typename FormatContext>
  auto format(const detail::styled_arg<T>& arg, FormatContext& ctx) const
    -> decltype(ctx.out())
  {
    const auto initial = get_terminal_state();
    const auto effective =
        arg.style.is_absolute() ? arg.style : tjg::overlay(initial, arg.style);
    detail::style_scope scope{};
    auto sgr = basic_inline_buffer<Char, sgr_max_length>{};
    detail::append_sgr_transition(sgr, effective, initial);
    auto out = std::copy(sgr.begin(), sgr.end(), ctx.out());
    ctx.advance_to(out);
    get_terminal_state() = text_style{effective}.absolute(false);
    out = Formatter::format(arg.value, ctx);
    sgr.clear();
    detail::append_sgr_transition(sgr, initial, get_terminal_state());
    return std::copy(sgr.begin(), sgr.end(), out);
  }
};  // basic_styled_arg_formatter

} // detail

/** Returns a formatting argument that applies text style `ts` to `value` using
 * ANSI escape sequences. The underlying type's format specifications are
 * preserved, including width, alignment, and precision. Padding produced by the
 * underlying formatter is inside the styled span.
 *
 * The result borrows `value` and stores a copy of `ts`. It does not copy or own
 * `value`, which must remain alive whenever the result is formatted. A
 * temporary value can be used in an immediate formatting call, but storing the
 * result does not extend that temporary's lifetime.
 *
 * Relative styles inherit the shared terminal state's missing colors
 * and combine emphasis. Absolute styles use terminal defaults for unspecified
 * components. After the value, including padding, the enclosing style is
 * restored. Shared state initially assumes terminal defaults; styling already
 * active outside this formatting operation and raw escapes are not tracked.
 *
 * **Example**:
 *
 *     std::print("Elapsed time: {0:.2f} seconds",
 *                tjg::styled(1.23, tjg::fg(tjg::color::green)
 *                                      .bg(tjg::color::midnight_blue)));
 */
template<typename T>
constexpr auto styled(const T& value, text_style ts)
  -> detail::styled_arg<std::remove_cvref_t<T>>
{ return detail::styled_arg<std::remove_cvref_t<T>>{value, ts}; }

namespace std {

template<typename T, CharType Char>
  requires formattable<decay_t<const T>, Char>
struct formatter<tjg::detail::styled_arg<T>, Char>
    : tjg::detail::basic_styled_arg_formatter<
          T, Char, formatter<decay_t<const T>, Char>> {};

} // std
