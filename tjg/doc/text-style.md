# Text Styles

## `text_style`

::: text_style
    options:
      color_reference: true
      exclude_members:
        - get_foreground
        - get_background
        - get_emphasis
        - has_foreground
        - has_background
        - has_emphasis
        - strikethrough
        - is_strikethrough

### Readable style output

`text_style` supports `{}` and `{:?}` using the same compact notation. With
`<tjg/color-ostream.h>`, narrow and wide ostream insertion uses that notation
too. These operations describe the style without emitting SGR sequences.

Output starts with `=` if the style is absolute, immediately followed by the
characters for its set emphasis bits, in this order:

| Attribute   | Character |
| ----------- | --------- |
| `bold`      | `b`       |
| `faint`     | `f`       |
| `italic`    | `i`       |
| `underline` | `_`       |
| `blink`     | `!`       |
| `reverse`   | `r`       |
| `conceal`   | `?`       |
| `strike`    | `-`       |

If either color is specified, a single space separates it from any absolute or
emphasis characters. Colors use the
[readable color notation](colors.md#readable-color-output).
Foreground and background are separated by `/`. With foreground alone, omit
the slash; with background alone, begin with `/`. With neither, omit the color
component. An empty style displays `{}`; an absolute-only style displays `=`.

```cpp
std::format("{}", tjg::text_style{});                  // "{}"
std::format("{}", tjg::bg(tjg::gray_color{12}));        // "/gray12"
std::format("{:?}", tjg::absolute(tjg::bold().underline()
    .fg(tjg::term_color::bright_red).bg(tjg::rgb{26, 43, 60})));
// "=b_ bright_red/#1a2b3c"
```

String fill, alignment, width, and precision are supported, including dynamic
width and precision. Stream width, fill, and alignment behave as for an ordinary
string. A [styled-text debug view](styled.md#debug-output) omits empty node
styles.

With `<tjg/color.h>`, formatting an `emphasis` value with `{}` or `{:?}` lists
its set attribute names in declaration order, separated by spaces inside braces.
For example, `bold | faint | strike` displays
`{bold faint strike}`; an empty `emphasis{}` displays `{}`. The alias
`strikethrough` displays as `strike`. Narrow and wide formatting accepts string
fill, alignment, width, precision, optional `s` or `?`, and dynamic specifications.
With `<tjg/color-ostream.h>`, ostream insertion uses the same named form. Stream
width, fill, and alignment apply as for a string; precision is ignored. This
named form is separate from the compact characters used inside `text_style` output.

### Construction

Construct a style with colors first or emphasis first. Each color may be a
[`color_t`](colors.md#color-variants), or a named RGB constant, an `rgb`, a
`term_color`, `cube_color`, `gray_color`, or `no_color` implicitly converted
to `color_t`.

The color-first constructor requires a foreground color; background and
emphasis default to unspecified and empty. The emphasis-first constructor
accepts optional foreground and background colors, both defaulting to
`no_color`. Its emphasis defaults to `emphasis{}`, so `text_style{}` creates
an empty style. An emphasis value alone creates an emphasis-only style.

```cpp
auto foreground = tjg::text_style{tjg::color::red};
auto style = tjg::text_style{tjg::color::red, tjg::term_color::black,
                             tjg::emphasis::bold};
auto background = tjg::text_style{tjg::no_color, tjg::color::blue};
auto attributes_first = tjg::text_style{tjg::emphasis::bold, tjg::color::red,
                                        tjg::term_color::black};
auto underlined_background = tjg::text_style{
    tjg::emphasis::underline, tjg::no_color, tjg::color::blue};
auto palette = tjg::text_style{tjg::cube_color{1, 2, 3}, tjg::gray_color{12}};
```

### Absolute styles

Styles are relative by default: unspecified colors and unset emphasis leave
the terminal's existing attributes unchanged when emitted. Call `absolute()`
to reset the terminal before applying a style. Unspecified colors then select
terminal defaults, and unset emphasis means normal text.

```cpp
auto reset = tjg::text_style{}.absolute();
auto underlined = tjg::underline().absolute();
auto red = tjg::fg(tjg::color::red).absolute();
red.absolute(false); // Return to relative styling, retaining the foreground.
```

The free function `absolute(style)` returns an absolute copy, preserving both
colors and emphasis and leaving the input unchanged. It is `noexcept` and
supports constant evaluation under the same conditions as the member setter.
For example, `auto red = tjg::absolute(tjg::fg(tjg::color::red));`.

The member `absolute(bool enable = true)` returns `text_style&`; `is_absolute()`
queries the separately stored Boolean flag. Both are `noexcept` and support
constant evaluation. Absolute styles can still be explicitly modified using
setters, overlays, and underlays.

An absolute-only style is nonempty and emits `"\x1b[0m"`. Compact SGR output
combines the reset and attributes, such as `"\x1b[0;4m"`; noncompact output
emits the reset separately before the attribute sequences. Neither form
depends on a previously active terminal style.

### Overlaying styles

`overlay(other)` updates a style with another style's specified colors and
returns `text_style&` for chaining. The incoming foreground and background
replace their matching channels independently, preserving each color's kind.
An incoming `no_color` leaves the existing channel unchanged; a specified black
color replaces it. Emphasis bits combine by OR, so an overlay can add attributes
but cannot remove them. An empty overlay leaves the style unchanged.

The result is absolute if either input is absolute. This preserves the reset
requirement while allowing explicit changes to an absolute style's attributes.

```cpp
auto style = tjg::text_style{tjg::term_color::red, tjg::gray_color{12},
                             tjg::emphasis::bold};
const auto other = tjg::text_style{tjg::cube_color{0}, tjg::no_color,
                                   tjg::emphasis::underline};
style.overlay(other);
// Foreground: cube_color{0}; background: gray_color{12}.
// Emphasis: bold and underline. The value of other is unchanged.
```

The operation is `noexcept` and supports constant evaluation in C++23 and later
on supported compilers. It changes the style value without emitting ANSI output.
Use component setters to clear a color or remove attributes.

The free function `tjg::overlay(base, overrides)` returns a new `text_style`
with the same precedence rules, leaving both inputs unchanged. The second
argument supplies the overriding colors; reversing the arguments can change
the result. Like the member, it is `noexcept` and supports constant evaluation
in C++23 and later on supported compilers.

```cpp
const auto base = tjg::text_style{tjg::term_color::red, tjg::gray_color{12},
                                  tjg::emphasis::bold};
const auto overrides = tjg::text_style{tjg::cube_color{0}, tjg::no_color,
                                       tjg::emphasis::underline};
auto combined = tjg::overlay(base, overrides);
// combined has cube foreground, gray background, bold, and underline.
// base and overrides retain their original values.
```

::: overlay(text_style, text_style)
    options:
      color_reference: true

### Underlaying styles

`style.underlay(defaults)` fills unspecified foreground and background channels
from `defaults`, preserving colors already specified in `style`. Emphasis bits
combine by OR, and the result is absolute if either input is absolute. This
explicitly modifies a style value; it performs no inheritance or ANSI output.
The member is `noexcept` and returns `text_style&` for chaining.

The free `tjg::underlay(base, defaults)` returns the combined style without
modifying either input. Both forms support constant evaluation in C++23 and
later on supported compilers.

```cpp
constexpr auto base = tjg::fg(tjg::color::yellow);
constexpr auto defaults = tjg::fg(tjg::color::white).bg(tjg::color::blue);
auto combined = tjg::underlay(base, defaults);
// Yellow foreground is preserved; the missing background becomes blue.
```

For style values, `underlay(base, defaults)` equals `overlay(defaults, base)`.
For structured text, see
[group defaults and overrides](styled.md#use-underlay-to-supply-group-defaults).

::: underlay(text_style, text_style)
    options:
      color_reference: true

### Combining styles

Use constructors or fluent setters to select foreground, background, and emphasis.
Use `overlay` to replace specified colors, `underlay` to fill unspecified colors,
and `blend` to average them. These named operations combine emphasis bits by OR.
`text_style` does not provide `operator|` or `operator|=`; those operators remain
available for [color values](colors.md#composition-comparisons-and-numeric-representation)
and [emphasis masks](#combining-emphasis-attributes).

```cpp
auto style = tjg::fg(tjg::cube_color{1, 2, 3}).bg(tjg::gray_color{12});
// Foreground remains cube_color; background remains gray_color.
style.overlay(tjg::fg(tjg::gray_color{12}));
// Foreground becomes gray_color{12}; background is unchanged.
```

### Blending styles

::: blend(First, Second, Styles...)
    options:
      color_reference: true

Pass two or more `text_style` values to `tjg::blend` to return a new style.
Foreground colors are averaged independently of background colors, using
[color blending](colors.md#blending-color-variants). Unspecified colors are
ignored separately in each channel. Emphasis bits from every input are ORed.

Each channel remains unspecified if no input supplies a color, preserves its
color kind if exactly one input supplies it, and holds an RGB average if two
or more inputs supply colors. Palette colors use their fixed RGB equivalents.
All specified colors in a channel have equal weight; division occurs after
summation, with fractional results rounded down.

```cpp
constexpr auto first = tjg::text_style{tjg::term_color::red, tjg::no_color,
                                       tjg::emphasis::bold};
constexpr auto second = tjg::text_style{tjg::gray_color{12},
                                        tjg::cube_color{1, 2, 3},
                                        tjg::emphasis::underline};
constexpr auto third = tjg::text_style{tjg::rgb{}};
constexpr auto mixed = tjg::blend(first, second, third);
// Foreground: rgb{111, 42, 42}; background: cube_color{1, 2, 3}.
// Emphasis: bold and underline.
```

The function leaves its inputs unchanged, is `noexcept`, and supports constant
evaluation in C++23 and later. The result is always relative. Blending only
empty styles returns an empty style. All arguments must be styles; use a
`text_style` constructor to combine colors and emphasis before blending.

Each absolute input must specify both foreground and background colors: a
terminal-default color has an unknown RGB value. `assert` checks this
precondition for every input. With assertions disabled, unspecified channels
contribute nothing and the result still discards the absolute flag.

### Empty styles and clearing

`empty` returns `true` when both colors are unspecified, no emphasis is set,
and absolute is disabled. A specified black color or an absolute-only reset
makes a style nonempty.
The query is `const`, `noexcept`, and `constexpr` in C++23 and later.

`clear` removes both colors, all emphasis, and the absolute flag, restoring the
default-constructed state. It is `noexcept` and returns `text_style&` for
chaining. It supports constant evaluation in C++23 and later on supported
compilers. Clearing a style changes the value; it does not emit an ANSI reset.

```cpp
auto style = tjg::text_style{tjg::color::red, tjg::term_color::black,
                             tjg::emphasis::bold};
assert(!style.empty());
style.clear();
assert(style.empty());
style.clear().fg(tjg::gray_color{12}).underline();
```

### Component accessors

Each component has a getter, a replacement setter, and a presence query:

| Component    | Getter     | Setter               | Presence query   |
| ------------ | ---------- | -------------------- | ---------------- |
| Foreground   | `fg()`     | `fg(color)`          | `has_fg()`       |
| Background   | `bg()`     | `bg(color)`          | `has_bg()`       |
| Emphasis     | `emph()`   | `emph(attributes)`   | `has_emph()`     |

All these members are `noexcept`. Getters are `const` and return values.  The
color getters return the public type [`tjg::color_t`](colors.md#color-variants),
preserving the RGB, terminal, cube, gray, or unspecified alternative. The result
can be passed to either color setter, either style constructor, or the free
`tjg::fg` and `tjg::bg` functions. Use `std::holds_alternative`, `std::get`,
`std::get_if`, or `std::visit` to inspect its alternative. The emphasis getter
returns `emphasis`.

Getters accept empty components: color getters return an unspecified color,
and `emph()` returns `emphasis{}`. Presence checks are optional.

The `has_fg`, `has_bg`, and `has_emph` queries return `bool`, preserve
the style, and are `const`, `constexpr`, and `noexcept`. An empty style
returns `false` for all three. RGB black and terminal black are specified
colors, so their channel queries return `true`. `has_emph` returns `true`
when any emphasis bit is set, including faint or strike alone.

Each setter replaces its entire component, preserves the other two, and
returns `text_style&` for chaining. Color setters accept a named RGB constant,
an `rgb`, a `term_color`, a `cube_color`, a `gray_color`, or `no_color`.
Passing `no_color` clears that channel to unspecified; it does not store a
terminal-reset instruction.
Passing `emphasis{}` clears all attributes.

```cpp
auto style = tjg::text_style{tjg::color::coral};
style.bg(tjg::term_color::black).emph(tjg::emphasis::bold);
style.fg(tjg::no_color);  // Background and bold remain.
// has_fg() is false; has_bg() and has_emph() are true.

const auto source = style;
auto copy = tjg::text_style{source.fg(), source.bg(), source.emph()};

auto foreground = source.fg();
auto has_foreground = source.has_fg(); // false
auto unspecified = std::holds_alternative<tjg::no_color_t>(foreground); // true
```

!!! note "Backward compatibility"

    The original `get_foreground`, `get_background`, and `get_emphasis`
    names are retained as aliases for `fg`, `bg`, and `emph`, respectively.
    Likewise, `has_foreground`, `has_background`, and
    `has_emphasis` are retained as aliases for `has_fg`, `has_bg`, and
    `has_emph`, respectively.

!!! note "Changed behavior"

    Previously, the foreground, background, and emphasis getters required their
    components to be present. They can now be called when those components are
    absent.

!!! note "Changed behavior"

    Foreground and background values now have the public type `tjg::color_t`.
    Their type was previously unspecified and named `detail::color_type`
    internally. Both the short getters and the original names return
    `color_t` by value.

### Attribute queries

```cpp
bool is_bold()      const noexcept;
bool is_faint()     const noexcept;
bool is_italic()    const noexcept;
bool is_underline() const noexcept;
bool is_blink()     const noexcept;
bool is_reverse()   const noexcept;
bool is_conceal()   const noexcept;
bool is_strike()    const noexcept;
```

Each query returns whether its attribute is enabled. Unset attributes return
`false`, including every attribute of an empty style. Queries preserve the
style. These queries and the component getters support constant evaluation
in C++23 and later.

### Attribute setters

```cpp
text_style& bold(bool enable = true) noexcept;
text_style& faint(bool enable = true) noexcept;
text_style& italic(bool enable = true) noexcept;
text_style& underline(bool enable = true) noexcept;
text_style& blink(bool enable = true) noexcept;
text_style& reverse(bool enable = true) noexcept;
text_style& conceal(bool enable = true) noexcept;
text_style& strike(bool enable = true) noexcept;
```

Each setter enables its attribute by default, or disables it when passed
`false`. It changes only that bit, preserves all other attributes and colors,
and returns `text_style&` for chaining. Bold and faint are independent:
enabling or disabling either leaves the other unchanged. Rendering emits both
when both are enabled; their combined appearance depends on the terminal.
Attribute and component setters support constant evaluation in C++23 and
later on supported compilers.

The aliases `strikethrough` and `is_strikethrough` forward to `strike`
and `is_strike`, respectively. Either setter changes the same attribute bit,
and either query reports its state. `strikethrough(bool enable = true)` returns
`text_style&` for chaining.

Use `emph(attributes)` to set exactly a group of attributes, then individual
setters to change selected bits. In C++20 and later, `using enum` makes the
attribute names available without repeating their type:

```cpp
using enum tjg::emphasis;

auto style = tjg::text_style{tjg::color::coral};
style.emph(bold | blink | strike);  // Exactly these three attributes.
style.blink(false).underline();     // Bold, underline, strike.
```

In older language modes, qualify names with `tjg::emphasis::` or use a type
alias such as `using em = tjg::emphasis;` and write `em::bold`.

### Attribute functions returning a new style

The free functions `bold`, `faint`, `italic`, `underline`, `blink`,
`reverse`, `conceal`, and `strike` return `text_style` values. Each has a
Boolean factory overload and an overload that derives a copy from a base style.
All are `noexcept`.

#### Creating an attribute-only style

Call a function without arguments, or with `true`, to create a style containing
only that emphasis attribute. Both color channels are unspecified. Passing
`false` returns an empty style with no colors or attributes. The Boolean
parameter defaults to `true`; these factories support constant evaluation in
C++23 and later.

```cpp
constexpr auto heading = tjg::bold();
constexpr auto plain   = tjg::bold(false); // Empty style.
bool use_bold = true;                      // For example, a user preference.
std::println("{}", tjg::styled(42, tjg::bold(use_bold)));
std::println("{}", tjg::styled("Heading", tjg::bold()));
```

::: bold(bool)
    options:
      color_reference: true

::: faint(bool)
    options:
      color_reference: true

::: italic(bool)
    options:
      color_reference: true

::: underline(bool)
    options:
      color_reference: true

::: blink(bool)
    options:
      color_reference: true

::: reverse(bool)
    options:
      color_reference: true

::: conceal(bool)
    options:
      color_reference: true

::: strike(bool)
    options:
      color_reference: true

#### Deriving a style from a base

Pass a style as the first argument to return a modified copy. The original
style is unchanged. Each function enables its attribute by default; pass
`false` as the second argument to disable it. All other attributes and both
color channels retain their values and kinds.

```cpp
const auto theme = tjg::text_style{tjg::color::coral, tjg::term_color::black};
auto underlined = tjg::underline(theme);
auto bold_underlined = tjg::bold(underlined);
auto bold_only = tjg::underline(bold_underlined, false);
// theme still has no emphasis attributes.
```

Use these overloads when deriving a style from a reusable base. The member
call `style.underline()` modifies `style`; `tjg::underline(style)` returns a
new value. The style-copy overloads support constant evaluation in C++23 and
later on supported compilers.

::: bold(text_style, bool)
    options:
      color_reference: true

::: faint(text_style, bool)
    options:
      color_reference: true

::: italic(text_style, bool)
    options:
      color_reference: true

::: underline(text_style, bool)
    options:
      color_reference: true

::: blink(text_style, bool)
    options:
      color_reference: true

::: reverse(text_style, bool)
    options:
      color_reference: true

::: conceal(text_style, bool)
    options:
      color_reference: true

::: strike(text_style, bool)
    options:
      color_reference: true

### Comparisons

`text_style` stores its absolute flag, emphasis, foreground, and background as
separate members. Equality compares all four components. Ordering compares the
absolute flag first, then emphasis, background, and foreground, preserving the
previous ordering without packing the style. Different color kinds remain
distinct even when their RGB equivalents are equal. Styles can be used as keys
in `std::map` and `std::set`; `operator<=>` returns `std::strong_ordering`.

`std::hash<tjg::text_style>` combines the four components, allowing styles to be
keys in `std::unordered_map` and `std::unordered_set`. Equal styles have equal
hashes. Hash values are not a stable serialization format.

### Integer representation

`text_style` stores its components directly. No integer representation or
`to_underlying(text_style)` conversion is provided at present.

## Combining emphasis attributes

`tjg::emphasis` values are independent bit masks. `operator|` returns their
union without modifying either operand. `operator|=` adds the right operand's
attributes to the left operand and returns `emphasis&` referring to that left
operand, allowing chaining. Existing attributes remain set; combining with
`emphasis{}` leaves the value unchanged.

```cpp
auto attributes = tjg::emphasis::bold;
attributes |= tjg::emphasis::underline;
(attributes |= tjg::emphasis::italic) |= tjg::emphasis::strike;
auto style = tjg::text_style{attributes};
```

Both operators are `noexcept` and support constant evaluation.

## Foreground and background

`fg` and `bg` provide a convenient way to create a `text_style` without
calling a constructor directly. When passed a color value from namespace `tjg`,
argument-dependent lookup (ADL) finds these functions, so you can omit the
`std::` prefix: for example, `fg(tjg::color::red)` instead of
`tjg::text_style{tjg::color::red}`.

::: fg(color_t)
    options:
      color_reference: true

::: bg(color_t)
    options:
      color_reference: true

Pass `tjg::no_color` to either function to produce an empty style. This
specifies no color; combining it with an existing style does not clear that
style's color or emit a terminal reset.

See [Colors](colors.md) for the accepted color types and
[Text style rendering](index.md#TextStyleRendering) for formatting behavior.

