# Styled Output

Include `<fmt/color.h>` to use these output overloads. See
[Text style rendering](index.md#TextStyleRendering) for the shared styling rules.

## print to a stream

::: print(FILE*, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## print to stdout

::: print(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## println to a stream

::: println(FILE*, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## println to stdout

::: println(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## vprint

::: vprint(FILE*, text_style, string_view, format_args)
    options:
      color_reference: true

## vprintln

::: vprintln(FILE*, text_style, string_view, format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `fmt::make_format_args()`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result = 42;
auto args = fmt::make_format_args(result);
fmt::vprintln(stdout, fmt::fg(fmt::color::green), "Result: {}", args);
```

## Styled ostream output

Include `<fmt/color-ostream.h>` to use the styled `std::ostream` overloads.
This bridge includes both `<fmt/color.h>` and `<fmt/ostream.h>`; the two
existing headers remain independent.

```cpp
#include <fmt/color-ostream.h>
#include <sstream>

std::ostringstream os;
fmt::print(os, fmt::text_style{fmt::emphasis::bold}, "Result: {}", 42);
fmt::println(os, fmt::fg(fmt::term_color::red), "Status: {}", "failed");
```

The style applies to the entire formatted message. An empty style emits no
SGR sequences. A nonempty style ends with a reset to terminal defaults, which
does not restore an enclosing style. `println()` places its newline after the
reset. Stream state and exception handling follow the existing ostream output
functions. These overloads support C++11 and later and take `std::ostream&`.

::: print(std::ostream&, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::ostream&, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

::: vprint(std::ostream&, text_style, string_view, format_args)
    options:
      color_reference: true

::: vprintln(std::ostream&, text_style, string_view, format_args)
    options:
      color_reference: true
