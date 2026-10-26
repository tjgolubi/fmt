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

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

/**
 * Prints formatted text to `os` using `ts`.
 * A nonempty style precedes the text with SGR sequences and appends a reset
 * to terminal defaults. An empty style emits no SGR sequences.
 */
inline void vprint(std::ostream& os, text_style ts, string_view fmt,
                   format_args args)
{
  auto buffer = memory_buffer();
  detail::vformat_to(buffer, ts, fmt, args);
  fmt::print(os, FMT_STRING("{}"), string_view(buffer.data(), buffer.size()));
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * The newline follows the terminal reset for a nonempty style.
 */
inline void vprintln(std::ostream& os, text_style ts, string_view fmt,
                     format_args args)
{
  auto buffer = memory_buffer();
  detail::vformat_to(buffer, ts, fmt, args);
  buffer.push_back('\n');
  fmt::print(os, FMT_STRING("{}"), string_view(buffer.data(), buffer.size()));
}

/**
 * Prints formatted text to `os` using `ts`, followed by a terminal reset for
 * a nonempty style. Reset does not restore a previously active style.
 *
 * **Example**:
 *
 *     fmt::print(os, fmt::text_style{fmt::emphasis::bold}, "Result: {}", 42);
 */
template <typename... T>
void print(std::ostream& os, text_style ts, format_string<T...> fmt,
           T&&... args)
{ fmt::vprint(os, ts, fmt.str, vargs<T...>{{args...}}); }

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template <typename... T>
void println(std::ostream& os, text_style ts, format_string<T...> fmt,
             T&&... args)
{ fmt::vprintln(os, ts, fmt.str, vargs<T...>{{args...}}); }

FMT_END_EXPORT
FMT_END_NAMESPACE

#endif  // FMT_COLOR_OSTREAM_H_
