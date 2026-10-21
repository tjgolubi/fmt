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
