# Styled Formatting

Include `<tjg/color.hpp>` for the formatting overloads and SGR helpers.
Formatting produces text containing styling sequences.
See [Text style rendering](index.md#TextStyleRendering) for the shared rules.

## `to_sgr`

::: to_sgr
    options:
      color_reference: true

`tjg::color::to_sgr(style)` returns a `std::string` containing the ASCII bytes of
the ANSI SGR escape sequence. The result contains only the opening sequence.
An empty relative style produces an empty string. Unspecified
color channels emit no parameters. A relative style leaves those colors
unchanged; an absolute style begins with a reset, selecting terminal defaults
for them. An absolute-only style emits `"\x1b[0m"`.

```cpp
const auto style = tjg::color::text_style{tjg::color::emphasis::bold,
                                   tjg::color::rgb{1, 2, 3},
                                   tjg::color::term_color::black};
auto sgr = tjg::color::to_sgr(style);
```

The example emits one SGR sequence with unpadded parameters,
`\x1b[1;38;2;1;2;3;40m`. Here `\x1b` denotes the ESC character.
The function can allocate and is not `noexcept`.

Use `to_sgr` when managing terminal state yourself. Use the styled formatting
or printing functions to wrap text in styling followed by a terminal reset.

### Transitions from a known style

`tjg::color::to_sgr(final_style, initial_style)` returns the SGR needed to change from
the known initial style to the final style, without a trailing reset. The
second argument is required. Both arguments describe complete terminal states:
an empty foreground or background means the corresponding terminal-default
color, and absent emphasis means off. No inheritance occurs here. Resolve any
enclosing style before calling this overload. The absolute flag on the initial
style is ignored. Neither input is modified.

An absolute final style always emits a full reset followed by its own
attributes, even when the initial state has the same attributes. This case
delegates to `tjg::color::to_sgr(final_style)`.

```cpp
const auto initial = tjg::color::bold(tjg::color::fg(tjg::color::term_color::red));
const auto final_style = tjg::color::underline(tjg::color::fg(tjg::color::term_color::red));
auto transition = tjg::color::to_sgr(final_style, initial);
// "\x1b[22;4m" leaves red text underlined and clears bold.
```

For a relative final style, identical complete states emit nothing. When the
state changes, the formatter emits one sequence with unpadded parameters. It
chooses the shorter of selective changes and a reset followed by the target
attributes, preferring selective changes on a tie.

Selective changes can restore default foreground or background with
SGR `39` or `49`. Bold and faint share SGR `22` for removal, so a surviving
intensity attribute is reapplied when necessary. Colors are compared by kind
and value, preserving terminal palette semantics.

Both overloads return `std::string`. Constructing the result can allocate.

## `sgr_reset`

::: sgr_reset
    options:
      color_reference: true

`tjg::color::sgr_reset<Char>` returns a `std::basic_string_view<Char>` containing
the four-character sequence `\x1b[0m`. `Char` defaults to `char`; specify
`wchar_t` for a wide-character view. The view refers to static storage and
remains valid for the lifetime of the program. The function allocates no
memory, is `noexcept`, and supports constant evaluation in C++23 and later.

Use the reset after manually styled text to restore terminal-default colors
and emphasis. It does not restore a previously active enclosing style.

```cpp
auto message     = tjg::color::to_sgr(style) + "message";
const auto reset = tjg::color::sgr_reset();
message.append(reset.data(), reset.size());

auto wide_message = tjg::color::wrap_sgr<wchar_t>(style, L"message");
```

## `wrap_sgr`

`tjg::color::wrap_sgr(style, text)` returns a string containing the style's opening
SGR sequence, the text, and a terminal reset. The style comes first. Unlike
`format`, it copies the text literally: braces, embedded nulls, and escape
sequences are preserved without interpretation.

```cpp
const auto style = tjg::color::bold();
auto message     = tjg::color::wrap_sgr(style, "Result: {42}");
// "\x1b[1mResult: {42}\x1b[0m".
```

Empty text produces an empty string, even with a nonempty style. An empty
style returns only the text. Otherwise, the trailing reset restores terminal
defaults. The function leaves its inputs
unchanged, can allocate, and is not `noexcept`.

The character template accepts `std::basic_string_view<Char>` and returns
`std::basic_string<Char>`. A narrow convenience overload accepts arguments
convertible to `std::string_view`, including string literals and `std::string`.
For wide text, supply a view or specify the character type explicitly:

```cpp
auto wide_message = tjg::color::wrap_sgr<wchar_t>(style, L"Result: {42}");
auto wide_view    = std::basic_string_view<wchar_t>{L"message"};
auto another      = tjg::color::wrap_sgr(style, wide_view);
```

## SGR length constants

Both constants are `constexpr std::size_t` values available in C++23 and later:

| Constant                | Value | Meaning                                                                     |
| ----------------------- | ----- | --------------------------------------------------------------------------- |
| `tjg::color::sgr_max_length`   | `60`  | Capacity bound for generating SGR output.                                   |
| `tjg::color::sgr_reset_length` | `4`   | Length of the reset sequence returned by `sgr_reset`.                       |

Lengths count sequence characters, including ESC, brackets, parameters, and the
final `m`. They exclude any terminating null. `sgr_max_length` excludes the
text and trailing reset and is a capacity bound, not the actual output length.
Use the returned string's `size` for the actual opening length.

## `format`

::: format(text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## `format_to`

::: format_to(OutputIt, text_style, std::format_string<T...>, T&&...)
    options:
      color_reference: true

## `vformat`

::: vformat(text_style, std::string_view, std::format_args)
    options:
      color_reference: true

## `vformat_to`

::: vformat_to(OutputIt, text_style, std::string_view, std::format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `std::make_format_args`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result   = 42;
auto args    = std::make_format_args(result);
auto message = tjg::color::vformat(tjg::color::fg(tjg::color::rgb::green),
                            "Result: {}", args);
```
