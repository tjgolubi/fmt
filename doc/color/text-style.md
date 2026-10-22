# Text Styles

## text_style

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

### Construction

Construct a style with colors first or emphasis first. Each color may be a
named color, an `rgb`, a `terminal_color`, or `no_color`.

The color-first constructor requires a foreground color; background and
emphasis default to unspecified and empty. The emphasis-first constructor
accepts optional foreground and background colors, both defaulting to
`no_color`. Its emphasis defaults to `emphasis{}`, so `text_style{}` creates
an empty style. An emphasis value alone creates an emphasis-only style.

```cpp
auto foreground = fmt::text_style{fmt::color::red};
auto style = fmt::text_style{fmt::color::red, fmt::terminal_color::black,
                             fmt::emphasis::bold};
auto background = fmt::text_style{fmt::no_color, fmt::color::blue};
auto attributes_first = fmt::text_style{fmt::emphasis::bold, fmt::color::red,
                                        fmt::terminal_color::black};
auto underlined_background = fmt::text_style{
    fmt::emphasis::underline, fmt::no_color, fmt::color::blue};
```

### Component accessors

Each component has a getter, a replacement setter, and a presence query:

| Component  | Getter   | Setter             | Presence query |
|------------|----------|--------------------|----------------|
| Foreground | `fg()`   | `fg(color)`        | `has_fg()`     |
| Background | `bg()`   | `bg(color)`        | `has_bg()`     |
| Emphasis   | `emph()` | `emph(attributes)` | `has_emph()`   |

All these members are `noexcept`. Getters are `const` and return values.
The color getters return an *unspecified-type* that preserves the RGB or
terminal color kind. Use `auto` to store the result; it can be passed to
either color setter, either style constructor, or the free `fmt::fg()` and
`fmt::bg()` functions. The emphasis getter returns `emphasis`.

Getters accept empty components: color getters return an unspecified color,
and `emph()` returns `emphasis{}`. Presence checks are optional.

The `has_fg()`, `has_bg()`, and `has_emph()` queries return `bool`, preserve
the style, and are `const`, `constexpr`, and `noexcept`. An empty style
returns `false` for all three. RGB black and terminal black are specified
colors, so their channel queries return `true`. `has_emph()` returns `true`
when any emphasis bit is set, including faint or strikethrough alone.

Each setter replaces its entire component, preserves the other two, and
returns `text_style&` for chaining. Color setters accept a named RGB constant,
an `rgb`, a `terminal_color`, or `no_color`. Passing `no_color` clears that
channel to unspecified; it does not store a terminal-reset instruction.
Passing `emphasis{}` clears all attributes.

```cpp
auto style = fmt::text_style{fmt::color::coral};
style.bg(fmt::terminal_color::black).emph(fmt::emphasis::bold);
style.fg(fmt::no_color);  // Background and bold remain.
// has_fg() is false; has_bg() and has_emph() are true.

const auto source = style;
auto copy = fmt::text_style{source.fg(), source.bg(), source.emph()};
```

!!! note "Backward compatibility"

    The older `get_foreground()`, `get_background()`, and `get_emphasis()`
    names are retained for backward compatibility with `fg()`, `bg()`, and
    `emph()`. Likewise, `has_foreground()`, `has_background()`, and
    `has_emphasis()` are retained as aliases for `has_fg()`, `has_bg()`, and
    `has_emph()`, respectively.

!!! note "Changed behavior"

    Previously, the foreground, background, and emphasis getters required their
    components to be present. They can now be called when those components are
    absent.

### Attribute queries

```cpp
bool is_bold()          const noexcept;
bool is_faint()         const noexcept;
bool is_italic()        const noexcept;
bool is_underline()     const noexcept;
bool is_blink()         const noexcept;
bool is_reverse()       const noexcept;
bool is_conceal()       const noexcept;
bool is_strikethrough() const noexcept;
```

Each query returns whether its attribute is enabled. Unset attributes return
`false`, including every attribute of an empty style. Queries preserve the
style. These queries and the component getters support constant evaluation
in C++11 and later.

### Attribute setters

```cpp
text_style& bold(bool enable = true) noexcept;
text_style& faint(bool enable = true) noexcept;
text_style& italic(bool enable = true) noexcept;
text_style& underline(bool enable = true) noexcept;
text_style& blink(bool enable = true) noexcept;
text_style& reverse(bool enable = true) noexcept;
text_style& conceal(bool enable = true) noexcept;
text_style& strikethrough(bool enable = true) noexcept;
```

Each setter enables its attribute by default, or disables it when passed
`false`. It changes only that bit, preserves all other attributes and colors,
and returns `text_style&` for chaining. Bold and faint are independent:
enabling or disabling either leaves the other unchanged. Rendering emits both
when both are enabled; their combined appearance depends on the terminal.
Attribute and component setters support constant evaluation in C++14 and
later on supported compilers.

Use `emph(attributes)` to set exactly a group of attributes, then individual
setters to change selected bits. In C++20 and later, `using enum` makes the
attribute names available without repeating their type:

```cpp
using enum fmt::emphasis;

auto style = fmt::text_style{fmt::color::coral};
style.emph(bold | blink | strikethrough);  // Exactly these three attributes.
style.blink(false).underline();            // Bold, underline, strikethrough.
```

In older language modes, qualify names with `fmt::emphasis::` or use a type
alias such as `using em = fmt::emphasis;` and write `em::bold`.

### Comparisons

`text_style` values support equality and ordering by their packed `uint64_t`
representations, including color kinds, color values, and emphasis bits.
An empty style sorts before any nonempty style. RGB and terminal colors
remain distinct even when they display the same color. Styles can be used
as keys in `std::map` and `std::set`. In C++20 and later, `operator<=>`
returns `std::strong_ordering`.

`std::hash<fmt::text_style>` hashes the packed `uint64_t` representation,
allowing styles to be keys in `std::unordered_map` and `std::unordered_set`.
Equal styles have equal hashes. Hash values are not a stable serialization
format.

### Integer representation

`fmt::underlying_type<fmt::text_style>::type` is `uint64_t`.
`fmt::to_underlying(style)` returns the packed foreground, background, and
emphasis representation.  An empty style produces zero.

```cpp
constexpr auto empty = fmt::to_underlying(fmt::text_style{});  // 0
auto style  = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
auto packed = fmt::to_underlying(style);  // uint64_t
```

The packed encoding is not a stable serialization format.

## Foreground and background

Pass `fmt::no_color` to either function to produce an empty style. This
specifies no color; combining it with an existing style does not clear that
style's color or emit a terminal reset.

::: fg(detail::color_type)
    options:
      color_reference: true

::: bg(detail::color_type)
    options:
      color_reference: true

See [Colors](colors.md) for the accepted color types and
[Text style rendering](index.md#TextStyleRendering) for formatting behavior.
