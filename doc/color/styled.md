# Styled Arguments

## Structured styled text

Include `<fmt/styled-text.h>` to format an owned `styled_text` tree with `{}`.
For wide text use `wstyled_text` and include `<fmt/xchar.h>` for wide formatting
entry points. Concatenation preserves child styles; a style on the group provides
the enclosing style for its children.

```cpp
auto phrase = fmt::styled_text{"Hello "} + fmt::underline("world");
phrase.overlay(fmt::bold());
auto output = fmt::format("{}!", phrase);
// Both words are bold; only "world" is underlined. The exclamation mark
// follows the return to terminal defaults.
```

Relative nodes inherit missing colors and combine emphasis with the enclosing
style. Specified child colors replace the matching enclosing color, preserving
its color kind. An absolute node starts from terminal defaults: missing colors
remain default and absent emphasis is off. Its descendants inherit from that
node normally. After each child, output restores the enclosing complete style;
after the root, it returns to defaults. Absolute boundaries force a reset even
when their attributes match the enclosing state.

The formatter writes ordinary ANSI SGR directly while traversing the tree;
it leaves the tree unchanged and does not use extended push/pop sequences.
Empty text emits nothing. Leaf contents, including braces, embedded nulls, and
escapes, are copied verbatim. Embedded escapes are not interpreted or tracked;
they can change terminal state outside the formatter's model. The replacement
field assumes terminal defaults on entry and does not capture external terminal
state or a style applied by an outer formatting call.

Only `{}` and the equivalent `{:}` are currently supported. Width, alignment,
precision, and other format specifications report `format_error` through the
normal format-string checking rules. Deferred blending and visible-width
formatting remain future work.

## Individual styled arguments

Style an individual replacement field with `styled`. Its format
specifications still apply to the underlying value.

::: styled(const T&, text_style)
    options:
      color_reference: true

Expected output for the example above (without a trailing newline):

<pre class="color-output">Elapsed time: <span style="color: #008000; background-color: #191970">1.23</span> seconds</pre>

```cpp
fmt::println("Amount: {:>8.2f}",
             fmt::styled(1.23, fmt::fg(fmt::color::green)));
```

Expected output (followed by a newline):

<pre class="color-output">Amount: <span style="color: #008000">    1.23</span></pre>

The width and precision apply to the number, including padding inside the styled
span. For interaction with enclosing styles, see
[Text style rendering](index.md#TextStyleRendering).

To insert a styled value directly into an ostream, include
`<fmt/color-ostream.h>` and use `os << fmt::styled(value, style)`.
[Styled stream insertion](print.md#styled-stream-insertion) describes stream
formatting, wide streams, and the wrapper's reference lifetime.

```cpp
auto style = fmt::fg(fmt::color::green).bold();
std::cout << "Amount: " << std::fixed << std::setprecision(2)
          << fmt::styled(1.23, style) << '\n';
```

Expected output (followed by a newline):

<pre class="color-output">Amount: <span style="color: #008000; font-weight: bold">1.23</span></pre>

Only `1.23` is styled in bold green. The value uses the ostream's formatting
settings.
