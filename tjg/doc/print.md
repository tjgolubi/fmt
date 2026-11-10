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

## Runtime print

The runtime overloads take an argument pack created with
`std::make_format_args`. The arguments must remain alive while the pack is used.
The Unicode and non-Unicode overloads use the corresponding standard output
paths. Overloads without a destination write to stdout.

```cpp
int result = 42;
auto args = std::make_format_args(result);
tjg::vprint_unicode(stdout, tjg::fg(tjg::color::green), "Result: {}", args);
```

::: vprint_unicode(FILE*, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

::: vprint_unicode(text_style, std::string_view, std::format_args)
    options:
      color_reference: true

::: vprint_nonunicode(FILE*, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

::: vprint_nonunicode(text_style, std::string_view, std::format_args)
    options:
      color_reference: true

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
reset. The formatted-output functions use the stream's locale for localized
format specifications and leave settings such as field width and numeric base
alone.

::: print(std::ostream&, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::ostream&, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

::: vprint_unicode(std::ostream&, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

::: vprint_nonunicode(std::ostream&, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

For wide streams, use `tjg::format` with a wide format string and insert the
result, or use the styled stream insertion described below.

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

A nonempty style emits an [opening SGR sequence](format.md#to_sgr), followed
by the value and a reset to terminal defaults. An empty style simply inserts
the value. Inserting an empty string with a nonempty style still emits the
opening sequence and reset. A reset does not restore an enclosing style.

If inserting the value throws and exceptions are enabled, the operator attempts
to write a reset, ignores any exception from that recovery write, and rethrows
the original exception. A failed stream may prevent the reset from being
written. On the normal path, stream failures follow the stream's exception
mask, including failures while writing the opening sequences or final reset.
The operator also supports builds with exceptions disabled.

`styled` stores a reference to the value and a copy of the style. Immediate
insertion of a temporary value is valid for that expression. If you save the
wrapper, its referenced value must remain alive until insertion is complete.
