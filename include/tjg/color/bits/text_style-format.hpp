/// @file
/// Formatting for `text_style`.
/// @copyright 2026 Terry Golubiewski, all rights reserved.
#pragma once

#include <tjg/color/text_style.hpp>
#include "colors-format.hpp"
#include "emphasis-format.hpp"
#include "format_support.hpp"
#include "sgr_support.hpp"
#include <tjg/inline_buffer.hpp>
#include "error_support.hpp"
#include <tjg/utility.hpp>

#include <print>
#include <format>
#include <locale>
#include <string>
#include <string_view>

#include <algorithm>
#include <iterator>
#include <system_error>
#include <exception>
#include <concepts>
#include <type_traits>
#include <utility>

#include <cstdio>
#include <cstddef>

namespace tjg::color::detail {

[[nodiscard]] inline auto get_style_state() -> text_style& {
  static text_style _state{};
  return _state;
}

class style_scope {
  text_style _saved;

 public:
  style_scope() noexcept : _saved{get_style_state()} {}

  // Restore shared state on exceptions; partial output may remain.
  ~style_scope() { get_style_state() = _saved; }

  style_scope(const style_scope&) = delete;
  void operator=(const style_scope&) = delete;
}; // style_scope

template<std::output_iterator<char> OutputIt> inline
auto write_reset(OutputIt out) -> OutputIt
{ return detail::write_ascii(std::move(out), tjg::color::sgr_reset()); }

template<std::output_iterator<char> OutputIt> inline
auto write_debug_value(OutputIt out, text_style value) -> OutputIt {
  if (value.empty()) {
    *out++ = '{';
    *out++ = '}';
    return out;
  }
  if (value.is_absolute()) *out++ = '=';
  if (value.emph() != emphasis::none)
    out = tjg::color::write_debug_value(std::move(out), value.emph());
  if (!value.has_fg() && !value.has_bg()) return out;
  if (value.is_absolute() || value.emph() != emphasis::none) *out++ = ' ';
  if (value.has_fg()) out = tjg::color::write_value(std::move(out), value.fg());
  if (value.has_bg()) {
    *out++ = '/';
    out = tjg::color::write_value(std::move(out), value.bg());
  }
  return out;
} // write_debug_value text_style

} // tjg::color::detail

namespace std {

template<tjg::CharType Char>
struct formatter<tjg::color::text_style, Char>
  : tjg::color::detail::standard_value_specs<Char>
{
  template<typename FormatContext>
  auto format(const tjg::color::text_style& value, FormatContext& ctx) const
    -> decltype(ctx.out())
  {
    if (this->debug_format()) {
      auto buffer = tjg::basic_inline_buffer<Char>{};
      (void) tjg::color::detail::write_debug_value(std::back_inserter(buffer), value);
      return this->write(buffer.view(), ctx);
    }

    if (value.empty())
      return ctx.out();

    auto sgr = tjg::color::sgr_buffer{};
    tjg::color::detail::append_sgr(sgr, value);
    return std::copy(sgr.begin(), sgr.end(), ctx.out());
  }
}; // text_style formatter

} // std

namespace tjg::color {

namespace detail {

template<typename Fn>
    requires requires(Fn& writer, const char* data, std::size_t size) {
      writer(data, size);
    }
class sgr_scope {
  Fn _writer;
  text_style& _state;
  text_style _saved;
  int _exceptions = std::uncaught_exceptions();
  sgr_buffer _sgr;
  bool _finished = false;

  void transition(text_style target) {
    _sgr.clear();
    append_sgr_transition(_sgr, target, _state);
    if (!_sgr.empty()) _writer(_sgr.data(), _sgr.size());
    _state = target.absolute(false);
  }

 public:
  sgr_scope(Fn writer_, text_style style)
      : _writer{std::move(writer_)}, _state{get_style_state()}, _saved{_state} {
    if (!style.is_absolute()) style.underlay(_saved);
    transition(style);
  }

  void finish() {
    transition(_saved);
    _finished = true;
  }

  ~sgr_scope() {
    if (_finished) return;
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
#endif
      // A throwing nested formatter may have emitted SGR before restoring its
      // logical state. Rebuild the entry style when unwinding in that case.
      transition(text_style{_saved}.absolute(
          std::uncaught_exceptions() > _exceptions));
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    } catch (...) {
      // Closing SGR is best effort.
    }
#endif
    _state = _saved;
  }

  sgr_scope(const sgr_scope&) = delete;
  void operator=(const sgr_scope&) = delete;
}; // sgr_scope

#define TJG_SGR_SCOPE(writer, style) \
  TJG_ASSERT(detail::get_style_state().empty(), " non-default style state"); \
  auto scope = detail::sgr_scope{writer, style}

template<CharType Char>
using format_context =
    std::conditional_t<std::same_as<Char, char>, std::format_context,
                                                 std::wformat_context>;
template<CharType Char>
using format_args = std::basic_format_args<format_context<Char>>;

template<CharType Char, std::output_iterator<Char> OutputIt>
auto vformat_to(OutputIt out, text_style style,
                std::basic_string_view<Char> fmt, format_args<Char> args,
                const std::locale* locale = nullptr)
  -> OutputIt
{
  bool can_write = true;

  auto writer = [&](const char* data, std::size_t size) {
    if (!can_write) return;
    can_write = false;
    for (std::size_t i = 0; i < size; ++i)
      *out++ = static_cast<Char>(data[i]);
    can_write = true;
  }; // writer

  TJG_SGR_SCOPE(writer, style);

  can_write = false;
  if (locale)
    out = std::vformat_to(std::move(out), *locale, fmt, args);
  else
    out = std::vformat_to(std::move(out), fmt, args);
  can_write = true;

  scope.finish();
  return out;
}  // vformat_to

} // detail

/**
 * The colored analog of
 * [std::vformat_to()](https://en.cppreference.com/w/cpp/utility/format/vformat_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
template<std::output_iterator<const char&> OutputIt>
auto vformat_to(OutputIt out, text_style style,
                std::string_view fmt, std::format_args args)
  -> OutputIt
{ return detail::vformat_to(std::move(out), style, fmt, args); }

template<std::output_iterator<const wchar_t&> OutputIt>
auto vformat_to(OutputIt out, text_style style,
                std::wstring_view fmt, std::wformat_args args)
  -> OutputIt
{ return detail::vformat_to(std::move(out), style, fmt, args); }

template<std::output_iterator<const char&> OutputIt>
auto vformat_to(OutputIt out, const std::locale& loc, text_style style,
                std::string_view fmt, std::format_args args)
  -> OutputIt
{ return detail::vformat_to(std::move(out), style, fmt, args, &loc); }

template<std::output_iterator<const wchar_t&> OutputIt>
auto vformat_to(OutputIt out, const std::locale& loc, text_style style,
                std::wstring_view fmt, std::wformat_args args)
  -> OutputIt
{ return detail::vformat_to(std::move(out), style, fmt, args, &loc); }

namespace detail {

inline void write_file(FILE* f, const char* data, std::size_t size)
{
  auto count = std::fwrite(data, 1, size, f);
  if (count == size)
    return;
  const auto ec = std::make_error_code(std::io_errc::stream);
  const auto x =std::system_error{ec, "writing SGR"};
  TJG_THROW(x);
}

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
  auto writer = [&](const char* data, std::size_t size)
                { detail::write_file(f, data, size); };

  TJG_SGR_SCOPE(writer, ts);
  std::vprint_nonunicode(f, fmt, args);
}  // vprint_nonunicode

/// Prints a styled message to stdout without Unicode-specific handling.
inline void vprint_nonunicode(text_style ts, std::string_view fmt,
                              std::format_args args)
{ vprint_nonunicode(stdout, ts, fmt, args); }

/// Prints a styled message using the standard Unicode output path.
inline void vprint_unicode(FILE* f, text_style ts, std::string_view fmt,
                           std::format_args args)
{
  auto writer = [&](const char* data, std::size_t size)
                { detail::write_file(f, data, size); };

  TJG_SGR_SCOPE(writer, ts);
  std::vprint_unicode(f, fmt, args);
}

/// Prints a styled message to stdout using the standard Unicode output path.
inline void vprint_unicode(text_style ts, std::string_view fmt,
                           std::format_args args)
{ vprint_unicode(stdout, ts, fmt, args); }

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
 *     tjg::color::print(stderr, tjg::color::bold().fg(tjg::color::rgb::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(FILE* f, text_style ts, std::format_string<T...> fmt, T&&... args)
{
  auto writer = [&](const char* data, std::size_t size)
                { detail::write_file(f, data, size); };

  TJG_SGR_SCOPE(writer, ts);
  std::print(f, fmt, std::forward<T>(args)...);
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
 *     tjg::color::print(tjg::color::bold().fg(tjg::color::rgb::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(text_style ts, std::format_string<T...> fmt, T&&... args)
{ print(stdout, ts, fmt, std::forward<T>(args)...); }

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
 *     tjg::color::println(stderr, tjg::color::bold().fg(tjg::color::rgb::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(FILE* f, text_style ts, std::format_string<T...> fmt, T&&... args)
{
  auto writer = [&](const char* data, std::size_t size)
                { detail::write_file(f, data, size); };

  print(f, ts, fmt, std::forward<T>(args)...);
  writer("\n", 1);
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
 *     tjg::color::println(tjg::color::bold().fg(tjg::color::rgb::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(text_style ts, std::format_string<T...> fmt, T&&... args)
{ println(stdout, ts, fmt, std::forward<T>(args)...); }

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
  auto result = std::string{};
  auto writer = [&](const char* data, std::size_t size)
                { result.append(data, size); };

  TJG_SGR_SCOPE(writer, ts);
  std::vformat_to(std::back_inserter(result), fmt, args);
  scope.finish();
  return result;
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
 * std::string message = tjg::color::format(tjg::color::bold().fg(tjg::color::rgb::red),
 *                                   "The answer is {}", 42);
 * ```
 */
template<typename... T>
inline auto format(text_style ts, std::format_string<T...> fmt, T&&... args)
  -> std::string
{ return vformat(ts, fmt.get(), std::make_format_args(args...)); }

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
 *     tjg::color::format_to(std::back_inserter(out),
 *                    tjg::color::bold().fg(tjg::color::rgb::red), "{}", 42);
 */
template<std::output_iterator<const char&> OutputIt, typename... T>
inline auto format_to(OutputIt out, text_style ts, std::format_string<T...> fmt,
                      T&&... args)
  -> OutputIt
{
  return vformat_to(std::move(out), ts, fmt.get(),
                    std::make_format_args(args...));
}

inline auto vformat(text_style ts, std::wstring_view pattern,
                    std::wformat_args args)
  -> std::wstring
{
  auto result = std::wstring{};
  auto writer = [&](const char* data, std::size_t size)
                { result.append(data, data + size); };

  TJG_SGR_SCOPE(writer, ts);
  std::vformat_to(std::back_inserter(result), pattern, args);
  scope.finish();
  return result;
}

template<typename... T>
inline auto format(text_style ts, std::wformat_string<T...> pattern,
                   T&&... args)
  -> std::wstring
{ return vformat(ts, pattern.get(), std::make_wformat_args(args...)); }

template<std::output_iterator<const wchar_t&> OutputIt, typename... T>
auto format_to(OutputIt out, text_style ts, std::wformat_string<T...> pattern,
               T&&... args)
  -> OutputIt
{
  return vformat_to(std::move(out), ts, pattern.get(),
                    std::make_wformat_args(args...));
}

namespace detail {

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
    auto out = ctx.out();
    auto can_write = true;

    auto writer = [&](const char* data, std::size_t size) {
      if (!can_write) return;
      can_write = false;
      for (auto i = std::size_t{0}; i < size; ++i)
        *out++ = static_cast<Char>(data[i]);
      can_write = true;
    }; // writer

    auto scope = sgr_scope{writer, arg.style};

    can_write = false;
    ctx.advance_to(std::move(out));
    out = Formatter::format(arg.value, ctx);
    can_write = true;

    scope.finish();
    return out;
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
 *                tjg::color::styled(1.23, tjg::color::fg(tjg::color::rgb::green)
 *                                      .bg(tjg::color::rgb::midnight_blue)));
 */
template<typename T>
constexpr auto styled(const T& value, text_style ts)
  -> detail::styled_arg<std::remove_cvref_t<T>>
{ return detail::styled_arg<std::remove_cvref_t<T>>{value, ts}; }

} // tjg::color

namespace std {

template<typename T, tjg::CharType Char>
  requires formattable<decay_t<const T>, Char>
struct formatter<tjg::color::detail::styled_arg<T>, Char>
  : tjg::color::detail::basic_styled_arg_formatter<
                                    T, Char, formatter<decay_t<const T>, Char>>
{};

} // std
