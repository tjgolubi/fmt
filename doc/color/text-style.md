# Text Styles

## text_style

::: text_style
    options:
      color_reference: true

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

### Color getters

The color getters return an *unspecified-type*. Use `auto` to store the
result and pass it to `fg()` or `bg()`.

```cpp
auto style = fmt::fg(fmt::color::red);
if (style.has_foreground()) {
  auto color = style.get_foreground();
  auto background = fmt::bg(color);
  fmt::println(background, "Red background");
}
```

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

### Getters for empty components

!!! note "Changed behavior"

    Previously, the foreground, background, and emphasis getters required their
    components to be present. They can now be called when those components are
    absent

`get_foreground()`, `get_background()`, and `get_emphasis()` can be called on
an empty style. An unspecified color remains unspecified when passed to
`fg()` or `bg()`, and `get_emphasis()` returns `emphasis{}` when no attributes
are set. Presence checks are optional before calling the getters.

This allows copying the components of any style, including an empty one:

```cpp
fmt::text_style source;
auto copy = fmt::fg(source.get_foreground())
          | fmt::bg(source.get_background())
          | source.get_emphasis();
```

Use `has_foreground()` and `has_background()` when you need to distinguish an
unspecified channel from a specified color.

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
