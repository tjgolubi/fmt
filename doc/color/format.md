# Styled Formatting

The formatting overloads produce text containing styling sequences.
See [Text style rendering](index.md#TextStyleRendering) for the shared rules.

## `to_sgr`

::: to_sgr
    options:
      color_reference: true

`fmt::to_sgr<Char>(style)` renders a style as a `std::basic_string<Char>`
containing ANSI SGR escape sequences. `Char` defaults to `char`, so
`fmt::to_sgr(style)` returns `std::string`; `fmt::to_sgr<wchar_t>(style)`
returns `std::wstring`. It leaves the style unchanged and includes no text or
trailing reset. An empty style produces an empty string. Unspecified color
channels emit no parameters; they do not reset the terminal's existing colors.

```cpp
const auto style = fmt::text_style{fmt::emphasis::bold,
                                   fmt::rgb{1, 2, 3},
                                   fmt::term_color::black};
auto sgr      = fmt::to_sgr(style);
auto wide_sgr = fmt::to_sgr<wchar_t>(style);
```

The output depends on the `FMT_COLOR_COMPACT_SGR` compiler definition:

| Definition | Output for the example                  |
| ---------- | --------------------------------------- |
| `0`        | `\x1b[1m\x1b[38;2;001;002;003m\x1b[40m` |
| `1`        | `\x1b[1;38;2;1;2;3;40m`                 |

`0` is the default.

Here `\x1b` denotes the actual ESC character. Set the definition consistently
when compiling the application and its libraries. Both modes support C++11
and later. The function can allocate and is not `noexcept`.

Use `to_sgr` when managing terminal state yourself. Use the styled formatting
or printing functions to wrap text in styling followed by a terminal reset.

### Transitions from a known style

`fmt::to_sgr(final_style, initial_style)` returns the SGR needed to change from
the known initial style to the final style, without a trailing reset. The
second argument is required. Unspecified final foreground and background
inherit their respective initial colors. Final emphasis replaces initial
emphasis, so attributes can be turned off as well as on. Unspecified initial
colors represent terminal defaults. Neither input is modified.

```cpp
const auto initial = fmt::bold(fmt::fg(fmt::term_color::red));
const auto final_style = fmt::underline(fmt::fg(fmt::term_color::red));
auto transition = fmt::to_sgr(final_style, initial);
// "\x1b[22;4m": clear intensity, enable underline, retain red.
```

The overload compares selective changes with reset-and-rebuild and chooses
the shorter encoding; ties favor selective changes. Reset-and-rebuild restores
inherited colors. Bold and faint share SGR 22 for removal, so a surviving
intensity attribute is reapplied when necessary. Identical states emit nothing.
Colors are compared by kind and value, preserving terminal palette semantics.

To minimize output, transitions always use one combined sequence and unpadded
parameters, independently of `FMT_COLOR_COMPACT_SGR`. The one-argument overload
retains its existing output modes. Both overloads support C++11 and wide
characters; transitions can allocate and are not `noexcept`.

## `sgr_reset`

::: sgr_reset
    options:
      color_reference: true

`fmt::sgr_reset<Char>` returns a `fmt::basic_string_view<Char>` containing
the four-character sequence `\x1b[0m`. `Char` defaults to `char`; specify
`wchar_t` for a wide-character view. The view refers to static storage and
remains valid for the lifetime of the program. The function allocates no
memory, is `noexcept`, and supports constant evaluation in C++11 and later.
Its output is the same in both SGR modes.

Use the reset after manually styled text to restore terminal-default colors
and emphasis. It does not restore a previously active enclosing style.

```cpp
auto message     = fmt::to_sgr(style) + "message";
const auto reset = fmt::sgr_reset();
message.append(reset.data(), reset.size());

auto wide_message     = fmt::to_sgr<wchar_t>(style) + L"message";
const auto wide_reset = fmt::sgr_reset<wchar_t>();
wide_message.append(wide_reset.data(), wide_reset.size());
```

## `wrap_sgr`

`fmt::wrap_sgr(style, text)` returns a string containing the style's opening
SGR sequences, the text, and a terminal reset. The style comes first. Unlike
`format`, it copies the text literally: braces, embedded nulls, and escape
sequences are preserved without interpretation.

```cpp
const auto style = fmt::bold();
auto message     = fmt::wrap_sgr(style, "Result: {42}");
// "\x1b[1mResult: {42}\x1b[0m" in either mode.
```

Empty text produces an empty string, even with a nonempty style. An empty
style returns only the text. Otherwise, the opening sequences follow the
selected `FMT_COLOR_COMPACT_SGR` mode, and the trailing reset restores terminal
defaults rather than an enclosing style. The function leaves its inputs
unchanged, can allocate, and is not `noexcept`.

The character template accepts `fmt::basic_string_view<Char>` and returns
`std::basic_string<Char>`. A narrow convenience overload accepts arguments
convertible to `fmt::string_view`, including string literals and `std::string`.
For wide text, supply a view or specify the character type explicitly:

```cpp
auto wide_message = fmt::wrap_sgr<wchar_t>(style, L"Result: {42}");
auto wide_view    = fmt::basic_string_view<wchar_t>{L"message"};
auto another      = fmt::wrap_sgr(style, wide_view);
```

## SGR length constants

Both constants are `constexpr std::size_t` values available in C++11 and later:

| Constant                | Value | Meaning                                                                     |
| ----------------------- | ----- | --------------------------------------------------------------------------- |
| `fmt::sgr_max_length`   | `56`  | Upper bound on the opening SGR output for one style, in either output mode. |
| `fmt::sgr_reset_length` | `4`   | Length of the reset sequence returned by `sgr_reset`.                       |

Lengths count `Char` elements, including ESC, brackets, parameters, and the
final `m`. They exclude any terminating null. `sgr_max_length` excludes the
text and trailing reset and is a capacity bound, not the actual output length.
Use the returned string's `size` for the actual opening length.

## `format`

::: format(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## `format_to`

::: format_to(OutputIt, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## `vformat`

::: vformat(text_style, string_view, format_args)
    options:
      color_reference: true

## `vformat_to`

::: vformat_to(OutputIt, text_style, string_view, format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `fmt::make_format_args`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result   = 42;
auto args    = fmt::make_format_args(result);
auto message = fmt::vformat(fmt::fg(fmt::color::green),
                            "Result: {}", args);
```
