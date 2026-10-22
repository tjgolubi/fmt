# Terminal Colors and Text Styles

Include `<fmt/color.h>` to combine foreground and background colors with
emphasis attributes, style individual formatting arguments, and produce
formatted strings or output.

```cpp
#include <fmt/color.h>

int main() {
  auto style = fmt::fg(fmt::color::forest_green) | fmt::emphasis::bold;
  fmt::println(style, "The result is {}.", 42);
  fmt::println("The result is {}.",
               fmt::styled(42, fmt::fg(fmt::term_color::red)));
}
```

<pre class="color-output"><span style="color: #228B22; font-weight: bold">The result is 42.</span>
The result is <span style="color: #CD0000">42</span>.</pre>

Illustrative output using Xterm's default red (`#CD0000`) for the terminal color.

The first call styles the entire message. The second styles only `42`.

## Reference

| Page                           | Contents |
| ------------------------------ | --- |
| [Colors](colors.md)            | Named RGB colors, terminal palette colors, `rgb`, `no_color`, and integer representations |
| [Emphasis](emphasis.md)        | Text attributes and their combination |
| [Text styles](text-style.md)   | `text_style`, its members and operators, and `fg()`/`bg()` |
| [Styled arguments](styled.md)  | `styled()`, format specifications, and lifetimes |
| [Styled output](print.md)      | `print()`, `println()`, `vprint()`, and `vprintln()` |
| [Styled formatting](format.md) | `format()`, `format_to()`, and their `v` variants |

## Choosing and combining styles

Use constants in the `color` namespace for named RGB values, `rgb` for explicit
RGB components, and `term_color` for colors from the terminal's palette. Pass
a color to `fg()` or `bg()` to create a foreground or background style.

```cpp
auto style = fmt::fg(fmt::rgb{30, 144, 255})
           | fmt::bg(fmt::term_color::black)
           | fmt::emphasis::bold | fmt::emphasis::underline;
```

A default-constructed `text_style` has no colors or emphasis. Combine styles
with `|` or `|=`. Colors in different channels compose independently, and
emphasis bits are combined. Two RGB colors in the same channel combine by
bitwise OR. A same-channel conflict involving a terminal color throws
`format_error`, including when both terminal colors are identical.

## Text style rendering

<a id="TextStyleRendering"></a>

If a style is not empty, the formatted text is preceded by [SGR] (Select Graphic
Rendition) sequences for its specified foreground, background, and emphasis,
and followed by an SGR reset to terminal defaults. An empty style adds no
sequences of its own. `println()` and `vprintln()` append their newline after
the reset.

Styles apply to subsequent text until another sequence changes or resets them.
A styled argument can therefore affect the rest of an enclosing styled message:

```cpp
fmt::print(fmt::emphasis::underline, "The result is {} after rounding.",
           fmt::styled(42, fmt::fg(fmt::term_color::red)));
```

<pre class="color-output"><span style="text-decoration: underline">The result is <span style="color: #CD0000">42</span></span> after rounding.</pre>

Illustrative output using Xterm's default red (`#CD0000`).

| Text               | Appearance         |
| ------------------ | ------------------ |
| `The result is `   | Underlined         |
| `42`               | Red and underlined |
| ` after rounding.` | Terminal defaults  |

The reset after `42` clears the underline as well as the red foreground.
The enclosing underline is not restored. The outer call also emits its own
reset at the end.

The [output functions](print.md) emit escape sequences even when output is
redirected to a file. The [formatting functions](format.md) store those sequences
in their result. For format string syntax, see the [syntax reference](../syntax.md).

[SGR]: https://en.wikipedia.org/wiki/ANSI_escape_code#Select_Graphic_Rendition_parameters
