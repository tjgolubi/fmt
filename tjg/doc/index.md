# Terminal Colors and Text Styles

Include `<tjg/color.h>` to combine foreground and background colors with
emphasis attributes, style individual formatting arguments, and produce
formatted strings or output.

On `ref/CPP20`, the extension API lives in namespace `tjg` and targets C++23.
Text utilities and SGR helpers use `std::basic_string_view`, including
`std::string_view` and `std::wstring_view`. Formatting and printing use `std::format` and `std::print`; the extension
uses `<tjg/...>` headers and requires no {fmt} headers or library.
The standalone `tjg/` CMake project provides the installable `tjg::tjg` target. Owning-tree specifications,
UTF-8 decoding, width estimates, and debug escaping are extension-owned. `tjg::color_t` stores an
unspecified color, an RGB value, or a terminal, cube, or gray palette color
in one public type.

Convenience is a major design goal. The API deliberately provides several ways
to express common styling operations: constructors for complete styles, fluent
member functions for updating values, and free functions for creating or
deriving styles. These overlapping forms let you choose the clearest expression
for each use, from concise one-off styles to attributes controlled by user
configuration.

```cpp
#include <tjg/color.h>

int main() {
  auto style = tjg::fg(tjg::color::forest_green).bold();
  tjg::println(style, "The result is {}.", 42);
  std::println("The result is {}.",
               tjg::styled(42, tjg::fg(tjg::term_color::red)));
}
```

<pre class="color-output"><span style="color: #228B22; font-weight: bold">The result is 42.</span>
The result is <span style="color: #CD0000">42</span>.</pre>

Illustrative output using Xterm's default red (`#CD0000`) for the terminal color.

The first call styles the entire message. The second styles only `42`.

## Reference

| Page                           | Contents                                                                                                                           |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------- |
| [Colors](colors.md)            | RGB, terminal, cube, and gray colors; `no_color`; public `color_t` values, access and visitation, and integer representations      |
| [Emphasis](emphasis.md)        | Text attributes and their combination                                                                                              |
| [Text styles](text-style.md)   | `text_style`, named style operations, and `fg`/`bg`                                                                                |
| [Styled arguments](styled.md)  | Structured styled text, SGR parsing, ANSI stripping, backslash escaping/unescaping, `styled`, format specifications, and lifetimes |
| [Styled output](print.md)      | `print`, `println`, `vprint_unicode`, and `vprint_nonunicode`                                                                                    |
| [Styled formatting](format.md) | `format`, `format_to`, and their `v` variants                                                                                      |

## Choosing and combining styles

Use constants in the [`color`](colors.md#named-rgb-colors) namespace for named
RGB values, [`rgb`](colors.md#rgb-values) for explicit RGB components, and
[`term_color`](colors.md#terminal-palette-colors) for the terminal's 16 named
palette colors.  Use [`cube_color`](colors.md#cube-palette-colors) for its
6×6×6 cube and [`gray_color`](colors.md#gray-palette-colors) for its 24 gray
levels.  Pass a color to [`fg`](text-style.md#foreground-and-background) or
[`bg`](text-style.md#foreground-and-background) to create a foreground or
background style.

```cpp
auto style = tjg::text_style{tjg::rgb{30, 144, 255},
                             tjg::term_color::black,
                             tjg::emphasis::bold |
                             tjg::emphasis::underline};
```

A default-constructed [`text_style`](text-style.md#text_style) has no colors or
emphasis. Combine styles with [`blend`](text-style.md#blending-styles).
Foreground and background are handled independently, unspecified colors are
ignored, and emphasis bits are ORed. A channel specified by only one input
preserves its color and kind.  When multiple inputs specify the same channel,
their colors convert to RGB and combine by averaging the color channels; that
channel then holds RGB.

[RGB `operator|`](colors.md#combining-rgb-values) and `operator|=` combine
the corresponding channel bits. OR cannot lower a channel value. Use `blend`
to average colors.

These examples convert terminal colors to RGB using the fixed Xterm-default
palette, which may differ from the user's terminal theme.

```cpp
auto red = tjg::to_rgb(tjg::term_color::bright_red);
auto blue = tjg::to_rgb(tjg::term_color::blue);
auto averaged = tjg::blend(red, blue); // RGB {127, 0, 119}.
auto combined = red | blue;           // RGB {255, 0, 238}.
```

<table class="color-comparison">
<thead><tr><th>Inputs (<a href="colors.html#terminal-palette-colors"><code>term_color</code></a>)</th><th><a href="colors.html#averaging-rgb-values"><code>blend()</code></a> — average</th><th><a href="colors.html#combining-rgb-values"><code>operator|</code></a> — bitwise OR</th></tr></thead>
<tbody>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#FF0000;color:#000000"><a href="colors.html#terminal-palette-colors"><code>bright_red</code></a><small>255, 0, 0</small></span><span class="color-chip" style="background-color:#0000EE;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>blue</code></a><small>0, 0, 238</small></span></div></td>
<td><span class="color-chip" style="background-color:#7F0077;color:#FFFFFF"><code>rgb{127, 0, 119}</code><small>#7F0077</small></span></td>
<td><span class="color-chip" style="background-color:#FF00EE;color:#000000"><code>rgb{255, 0, 238}</code><small>#FF00EE</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#FF0000;color:#000000"><a href="colors.html#terminal-palette-colors"><code>bright_red</code></a><small>255, 0, 0</small></span><span class="color-chip" style="background-color:#5C5CFF;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>bright_blue</code></a><small>92, 92, 255</small></span></div></td>
<td><span class="color-chip" style="background-color:#AD2E7F;color:#FFFFFF"><code>rgb{173, 46, 127}</code><small>#AD2E7F</small></span></td>
<td><span class="color-chip" style="background-color:#FF5CFF;color:#000000"><code>rgb{255, 92, 255}</code><small>#FF5CFF</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#CD0000;color:#FFFFFF"><code>red</code><small>205, 0, 0</small></span><span class="color-chip" style="background-color:#0000EE;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>blue</code></a><small>0, 0, 238</small></span></div></td>
<td><span class="color-chip" style="background-color:#660077;color:#FFFFFF"><code>rgb{102, 0, 119}</code><small>#660077</small></span></td>
<td><span class="color-chip" style="background-color:#CD00EE;color:#000000"><code>rgb{205, 0, 238}</code><small>#CD00EE</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#CD0000;color:#FFFFFF"><code>red</code><small>205, 0, 0</small></span><span class="color-chip" style="background-color:#5C5CFF;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>bright_blue</code></a><small>92, 92, 255</small></span></div></td>
<td><span class="color-chip" style="background-color:#942E7F;color:#FFFFFF"><code>rgb{148, 46, 127}</code><small>#942E7F</small></span></td>
<td><span class="color-chip" style="background-color:#DD5CFF;color:#000000"><code>rgb{221, 92, 255}</code><small>#DD5CFF</small></span></td>
</tr>
</tbody>
</table>

## Text style rendering

<a id="TextStyleRendering"></a>

If a style is not empty, the formatted text is preceded by [SGR] (Select Graphic
Rendition) sequences for its specified foreground, background, and emphasis,
and followed by an SGR reset to terminal defaults. An empty style adds no
sequences of its own. `println` appends its newline after
the reset.

The active formatting style carries the enclosing style through replacement fields.
Relative styled arguments and structured text inherit missing colors and combine
emphasis with that style. Each field restores the enclosing style afterward:

```cpp
tjg::print(tjg::emphasis::underline,
           "The result is {} after rounding.",
           tjg::styled(42, tjg::fg(tjg::term_color::bright_red)));
```

<pre class="color-output"><span style="text-decoration: underline">The result is <span style="color: #FF0000">42</span> after rounding.</span></pre>

Illustrative output using our fixed terminal-to-RGB palette: bright red is
`rgb{255, 0, 0}` (`#FF0000`). The terminal theme may display it differently.

| Text               | Appearance                |
| ------------------ | ------------------------- |
| `The result is `   | Underlined                |
| `42`               | Bright red and underlined |
| ` after rounding.` | Underlined                |

After `42`, the foreground returns to its enclosing value and underline remains
active. The outer call resets to terminal defaults at the end. An absolute
replacement field temporarily uses its own complete style, then also restores
the enclosing style.

The same rules apply to
[structured styled text](styled.md#structured-styled-text):
concatenate complete spans with `+` or `+=`, and use `underlay` to supply group
defaults while preserving explicit colors. Use `overlay` to replace specified
colors throughout relative nodes; absolute scopes are protected.

See the [table demonstration](styled.md#table-demonstration) for a complete
example combining column defaults, row emphasis, absolute spans, UTF-8 display
widths, and fixed-width formatting, with build and run instructions.

`get_terminal_state` returns the separately tracked terminal style. String and
iterator formatting use their own active state, whose outermost value is reset.
Each nested `format`, `vformat`, `format_to`, or `vformat_to` call inherits and
restores its enclosing formatting style, including on exceptions. Standard
formatting contexts carry no added style state.

This implementation is single-threaded, with no mutex or thread-local state.
Styled formatting runs at runtime, including with compiled format strings;
it cannot be constant-evaluated because it accesses mutable static storage.
Raw SGR embedded in strings and external terminal styling are not tracked.

The [output functions](print.md) emit escape sequences even when output is
redirected to a file. The [formatting functions](format.md) store those
sequences in their result. For format string syntax, see the
[syntax reference](../../doc/syntax.md).

[SGR]: https://en.wikipedia.org/wiki/ANSI_escape_code#Select_Graphic_Rendition_parameters


## Standard formatting

Include `<tjg/color.h>` to use colors, `color_t`, `text_style`, `emphasis`,
and borrowed `styled` arguments with `std::format` and `std::format_to`.
Both narrow and wide formatting are supported.

```cpp
#include <format>
#include <tjg/color.h>

auto color = std::format("{}", tjg::color::red); // "#ff0000"
auto style = std::format("{:?}", tjg::bold());  // "b"
auto names = std::format("{}", tjg::emphasis::bold); // "{bold}"
auto text = std::format("{:04}", tjg::styled(42, tjg::bold()));
// Bold "0042", followed by restoration of the enclosing style.
```

Readable values retain the same unquoted notation for `{}` and `{:?}`.
Standard string fill, alignment, width, precision, and dynamic arguments apply.
`styled` uses its underlying type's standard formatter, including padding and
numeric specifications. Nested styled fields inherit and restore the shared
formatting state; absolute fields temporarily replace it. The wrapper still
borrows its value, so stored wrappers need a surviving referent.

Include `<tjg/styled_text.h>` for standard formatting of owning `styled_text`
and `wstyled_text` trees, including nested styles, padding, truncation, dynamic
specifications, and `?` tree inspection. The extension uses standard formatting
throughout; old {fmt} adapters are removed. Formatting reports specification
errors through `std::format_error`.

## Memory buffer

`<tjg/memory_buffer.h>` provides `tjg::basic_memory_buffer<Char, SIZE>` and
its `char` alias, `tjg::memory_buffer`, using only standard C++23 facilities.
It stores `SIZE` characters inline (500 by default) and grows dynamically when
needed. SGR construction uses an explicit 60-character inline buffer.

The interface consists of `data`, `size`, `clear`, `push_back`, pointer-range
`append`, and `value_type` for `std::back_inserter`. Output is not null-terminated.
Growth invalidates pointers into the buffer; `clear` retains storage for reuse.
The buffer is neither copyable nor movable. Appending a range of its own output
is supported, including across growth.

::: basic_memory_buffer
    options:
      color_reference: true
