# Styled Formatting

These overloads produce formatted text containing styling sequences.
See [Text style rendering](index.md#TextStyleRendering) for the shared rules.

## format

::: format(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## format_to

::: format_to(OutputIt, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## vformat

::: vformat(text_style, string_view, format_args)
    options:
      color_reference: true

## vformat_to

::: vformat_to(OutputIt, text_style, string_view, format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `fmt::make_format_args()`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result = 42;
auto args = fmt::make_format_args(result);
auto message = fmt::vformat(fmt::fg(fmt::color::green), "Result: {}", args);
```
