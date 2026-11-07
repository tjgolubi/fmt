# Styled Output

Include `<tjg/color.h>` to use these output overloads. See
[Text style rendering](index.md#TextStyleRendering) for the shared styling rules.

## print to a stream

::: print(FILE*, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## print to stdout

::: print(text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## println to a stream

::: println(FILE*, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## println to stdout

::: println(text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## vprint

::: vprint(FILE*, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

## vprintln

::: vprintln(FILE*, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `std::make_format_args`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result = 42;
auto args = std::make_format_args(result);
tjg::vprintln(stdout, tjg::fg(tjg::color::green), "Result: {}", args);
```

## Styled ostream output

Include `<tjg/color-ostream.h>` to use the styled `std::ostream` overloads.
This bridge includes `<tjg/color.h>` and standard ostream support. It does not
include the {fmt} engine.

```cpp
#include <tjg/color-ostream.h>
#include <sstream>

std::ostringstream os;
tjg::print(os, tjg::text_style{tjg::emphasis::bold}, "Result: {}", 42);
tjg::println(os, tjg::fg(tjg::term_color::red), "Status: {}", "failed");
```

The style applies to the entire formatted message. An empty style emits no
SGR sequences. A nonempty style ends with a reset to terminal defaults, which
does not restore an enclosing style. `println` places its newline after the
reset. Stream state and exception handling follow the existing ostream output
functions. These overloads require C++23 and standard-library formatting support. `print` and `println`
provide separate overloads for `std::ostream&` and `std::wostream&`;
`vprint` and `vprintln` take `std::basic_ostream<Char>&`.
The format string and argument pack must match the stream character type.
Use narrow format strings for narrow streams and wide format strings for wide
streams. Wide formatting is available without `<fmt/xchar.h>`.


```cpp
std::wostringstream wide;
const auto style = tjg::text_style{tjg::emphasis::bold};
tjg::print(wide, style, L"Result: {:04x}", 42);
tjg::println(wide, style, L"Status: {}", L"done");

int result = 42;
auto args = std::make_wformat_args(result);
tjg::vprint(wide, style, L"Result: {}", args);
tjg::vprintln(wide, style, L"Result: {}", args);
```

The formatted-output functions use standard format specifications and leave stream
formatting settings such as field width and numeric base unchanged. Wide and
narrow output follow the same styling, newline, and reset rules.

::: print(std::ostream&, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

::: print(std::wostream&, text_style, std::wformat_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::ostream&, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::wostream&, text_style, std::wformat_string<T...>, T&&...)
    options:
      color_reference: true

::: vprint(std::basic_ostream<Char>&, text_style, typename detail::color_ostream_types<Char>::string_view, typename detail::color_ostream_types<Char>::format_args)
    options:
      color_reference: true

::: vprintln(std::basic_ostream<Char>&, text_style, typename detail::color_ostream_types<Char>::string_view, typename detail::color_ostream_types<Char>::format_args)
    options:
      color_reference: true

## Styled stream insertion

With `<tjg/color-ostream.h>`, insert the result of `tjg::styled(value, style)`
directly into a stream. The operator returns the same stream by reference for
chaining and supports both `std::ostream` and `std::wostream` in C++23 and later.
The value must support insertion into that stream; a standard formatter is not
required.

```cpp
#include <tjg/color-ostream.h>
#include <iomanip>
#include <sstream>

const auto style = tjg::text_style{tjg::emphasis::bold};
std::ostringstream os;
os << "Result: " << std::hex << std::setfill('0') << std::setw(4)
   << tjg::styled(42, style) << '\n';
// "Result: \x1b[1m002a\x1b[0m\n"

std::wostringstream wide;
wide << tjg::styled(L"message", style) << L'\n';
```

The underlying value uses normal stream formatting, including numeric base,
precision, fill, alignment, and field width. SGR sequences do not consume the
field width; any padding produced by the value's insertion is inside the
styled span. Stream formatting settings follow the value's ordinary insertion
behavior. Strings are inserted literally, without interpreting braces as format
fields.

A nonempty style emits opening SGR sequences selected by
[`TJG_COLOR_COMPACT_SGR`](format.md#to_sgr), followed by the value and a reset
to terminal defaults. An empty style simply inserts the value and emits no
reset. Inserting an empty string with a nonempty style still emits the opening
sequences and reset. A reset does not restore an enclosing style.

If inserting the value throws and exceptions are enabled, the operator attempts
to write a reset, ignores any exception from that recovery write, and rethrows
the original exception. A failed stream may prevent the reset from being
written. On the normal path, stream failures follow the stream's exception
mask, including failures while writing the opening sequences or final reset.
The operator also supports builds with exceptions disabled.

`styled` stores a reference to the value and a copy of the style. Immediate
insertion of a temporary value is valid for that expression. If you save the
wrapper, its referenced value must remain alive until insertion is complete.
