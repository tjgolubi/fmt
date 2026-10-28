# Styled Arguments

Style an individual replacement field with `styled()`. Its format
specifications still apply to the underlying value.

::: styled(const T&, text_style)
    options:
      color_reference: true

```cpp
fmt::println("Amount: {:>8.2f}",
             fmt::styled(1.23, fmt::fg(fmt::color::green)));
```

The width and precision apply to the number, including padding inside the styled
span. For interaction with enclosing styles, see
[Text style rendering](index.md#TextStyleRendering).

To insert a styled value directly into an ostream, include
`<fmt/color-ostream.h>` and use `os << fmt::styled(value, style)`.
[Styled stream insertion](print.md#styled-stream-insertion) describes stream
formatting, wide streams, and the wrapper's reference lifetime.
