# Terminal Colors and Text Styles

Include `<tjg/color.hpp>` to combine foreground and background
colors with emphasis, style individual arguments, and produce formatted strings
or output.

The library API lives in namespace `tjg::color` and targets C++23.
Text utilities and SGR helpers use `std::basic_string_view`, including
`std::string_view` and `std::wstring_view`. Formatting and printing use `std::format` and `std::print`; the extension
uses `<tjg/...>` headers and requires no {fmt} headers or library.
Color and styling APIs live in `tjg::color`; general utilities remain in `tjg`.
Use `using namespace tjg::color;` locally for concise styled-text expressions.
The repository's root CMake project provides the installable `tjg::tjg` target.
The library implements styled-text format specs, UTF-8 decoding, width
estimates, and debug escaping. `tjg::color::color_t` stores an unspecified color,
an RGB value, or a terminal, cube, or gray palette color in one public type.

Convenience is a major design goal. The API deliberately provides several ways
to express common styling operations: constructors for complete styles, fluent
member functions for updating values, and free functions for creating or
deriving styles. These overlapping forms let you choose the clearest expression
for each use, from concise one-off styles to attributes controlled by user
configuration.

```cpp
#include <tjg/color.hpp>
#include <print>

int main() {
  auto style = tjg::color::fg(tjg::color::rgb::forest_green).bold();
  tjg::color::println(style, "The result is {}.", 42);
  std::println("The result is {}.",
               tjg::color::styled(42, tjg::color::fg(tjg::color::term_color::red)));
}
```

<pre class="color-output"><span style="color: #228B22; font-weight: bold">The result is 42.</span>
The result is <span style="color: #CD0000">42</span>.</pre>

Illustrative output using Xterm's default red (`#CD0000`) for the terminal color.

The first call styles the entire message. The second styles only `42`.

## Headers

| Header                        | Provides                                           |
| ----------------------------- | -------------------------------------------------- |
| `<tjg/color.hpp>`             | The full library, including formatting and streams |
| `<tjg/color/colors.hpp>`      | Color values, named colors, and `color_t`          |
| `<tjg/color/text_style.hpp>`  | Color values, emphasis, and `text_style`           |
| `<tjg/color/styled_text.hpp>` | Owning styled-text trees and their composition     |
| `<tjg/backslash.hpp>`         | Backslash escaping and unescaping for strings      |
| `<tjg/strip_ansi.hpp>`        | ANSI escape removal from strings                   |
| `<tjg/inline_buffer.hpp>`     | Fixed-capacity character buffers                   |
| `<tjg/hash_support.hpp>`      | Hash values and hash combination                   |
| `<tjg/utility.hpp>`           | Character-type concept and cast helpers            |

The narrower headers can be used without formatting, printing, or
stream support. Headers under `tjg/color/bits/` are implementation headers.

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
auto style = tjg::color::text_style{tjg::color::rgb{30, 144, 255},
                             tjg::color::term_color::black,
                             tjg::color::emphasis::bold |
                             tjg::color::emphasis::underline};
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
auto red = tjg::color::to_rgb(tjg::color::term_color::bright_red);
auto blue = tjg::color::to_rgb(tjg::color::term_color::blue);
auto averaged = tjg::color::blend(red, blue); // RGB {127, 0, 119}.
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

A shared terminal style carries the enclosing style through replacement fields.
Relative styled arguments and structured text inherit missing colors and combine
emphasis with that style. Each field restores the enclosing style afterward:

```cpp
tjg::color::print(tjg::color::emphasis::underline,
           "The result is {} after rounding.",
           tjg::color::styled(42, tjg::color::fg(tjg::color::term_color::bright_red)));
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

The library keeps one shared `text_style`, initially set to terminal defaults.
Styled formatting saves and restores that value, including on exceptions.
Nested `format` and `format_to` calls share the active style even when they
construct a new formatting context. Formatting contexts carry no terminal state.

This implementation is single-threaded, with no mutex or thread-local state.
Styled formatting runs at runtime, including with compiled format strings;
it cannot be constant-evaluated because it accesses mutable static storage.
Raw SGR embedded in strings and external terminal styling are not tracked.

The [output functions](print.md) emit escape sequences even when output is
redirected to a file. The [formatting functions](format.md) store those
sequences in their result. For format string syntax, see the
[C++23 format specifiers](https://timsong-cpp.github.io/cppwp/n4950/format.string.std).

[SGR]: https://en.wikipedia.org/wiki/ANSI_escape_code#Select_Graphic_Rendition_parameters


## Standard formatting

Include `<tjg/color.hpp>` to use colors, `color_t`, `text_style`, `emphasis`,
and borrowed `styled` arguments with `std::format` and `std::format_to`.
Both narrow and wide formatting are supported.

```cpp
#include <format>
#include <tjg/color.hpp>

auto color = std::format("{}", tjg::color::rgb::red); // "#ff0000"
auto style = std::format("{:?}", tjg::color::bold());  // "b"
auto names = std::format("{}", tjg::color::emphasis::bold); // "{bold}"
auto text = std::format("{:04}", tjg::color::styled(42, tjg::color::bold()));
// Bold "0042", followed by restoration of the enclosing style.
```

Colors retain the same unquoted notation for `{}` and `{:?}`. Emphasis uses
names with `{}` and compact codes with `{:?}`. A `text_style` emits SGR with
`{}` and uses readable notation with `{:?}`. String fill, alignment, width,
and precision apply to the readable forms, including dynamic arguments.
`styled` uses its underlying type's standard formatter, including padding and
numeric specifications. Nested styled fields inherit and restore the shared
terminal state; absolute fields temporarily replace it. The wrapper still
borrows its value, so stored wrappers need a surviving referent.

The same header enables standard formatting of owning `styled_text` and
`wstyled_text` trees, including nested styles, padding,
truncation, dynamic specifications, and `?` tree inspection. The `?`
presentation rejects width and precision. Formatting reports invalid runtime
specifications through `std::format_error`.

## Inline buffer

`<tjg/inline_buffer.hpp>` provides `tjg::basic_inline_buffer<Char, Size>` and
its `char` alias, `tjg::inline_buffer`. The buffer stores up to `Size`
characters inline, with a default capacity of 256. SGR construction uses a
60-character buffer.

Use `view()` to read the contents, `push_back` or `append` to write, and
`clear()` to reuse the buffer. The caller supplies enough capacity for each
write. Use the view or the buffer's size to determine the end of the output.

Runtime construction leaves the character storage uninitialized. Copies copy
only the populated characters. `resize(size, fill)` fills newly exposed
characters with `fill`, which defaults to zero. Use
`resize_and_overwrite(limit, op)` to let a producer such as `std::fread` write
directly into the buffer. The callable receives a pointer and the limit, and
returns the number of initialized characters to keep.

::: tjg::basic_inline_buffer
    options:
      color_reference: true
