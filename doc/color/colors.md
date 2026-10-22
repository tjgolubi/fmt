# Colors

Use these color values with [`fg()` and `bg()`](text-style.md#foreground-and-background).
See the [overview](index.md) for examples of combining colors and emphasis.

## Unspecified color

::: no_color_t
    options:
      color_reference: true

::: no_color
    options:
      color_reference: true

An unspecified color adds no foreground or background styling. It is distinct
from the color black.

```cpp
auto unspecified    = fmt::fg(fmt::no_color);              // No foreground.
auto rgb_black      = fmt::fg(fmt::rgb{});                 // Black foreground.
auto terminal_black = fmt::fg(fmt::terminal_color::black); // Black foreground.
```

Both `fmt::fg(fmt::no_color)` and `fmt::bg(fmt::no_color)` produce empty styles.
They do not clear a color already present in another style when combined with
`|` or `|=`, and they do not emit a terminal reset.

## Terminal palette colors

::: terminal_color
    options:
      color_reference: true

Terminal colors use the terminal's palette rather than fixed RGB values.
Their enum values are contiguous palette indices, separate from the ANSI
SGR codes emitted for foreground and background colors:

| Color            | Value | FG SGR | BG SGR |
| -----            | ----: | -----: | -----: |
| `black`          |   0   |   30   |   40   |
| `red`            |   1   |   31   |   41   |
| `green`          |   2   |   32   |   42   |
| `yellow`         |   3   |   33   |   43   |
| `blue`           |   4   |   34   |   44   |
| `magenta`        |   5   |   35   |   45   |
| `cyan`           |   6   |   36   |   46   |
| `white`          |   7   |   37   |   47   |
| `bright_black`   |   8   |   90   |  100   |
| `bright_red`     |   9   |   91   |  101   |
| `bright_green`   |  10   |   92   |  102   |
| `bright_yellow`  |  11   |   93   |  103   |
| `bright_blue`    |  12   |   94   |  104   |
| `bright_magenta` |  13   |   95   |  105   |
| `bright_cyan`    |  14   |   96   |  106   |
| `bright_white`   |  15   |   97   |  107   |

```cpp
auto style = fmt::fg(fmt::terminal_color::black);
bool specified = style.has_fg(); // true
```

!!! note "Changed behavior"

    Previously, `terminal_color` values were foreground SGR codes 30-37 and
    90-97.  They are now palette indices 0-15. Symbolic names and emitted SGR
    sequences are unchanged, but numeric conversions and stored terminal-color
    values differ.

Use symbolic terminal-color names in styling code. Their numeric values and
the packed style encoding are not stable serialization formats.

## RGB values

::: rgb
    options:
      color_reference: true

### Combining RGB values

`operator|` returns a new `rgb` by taking the bitwise OR of corresponding red,
green, and blue components. `operator|=` updates its left operand and returns
a reference to it. These operations do not average colors or perform alpha
compositing. Both operations are `noexcept`.

```cpp
auto magenta = fmt::rgb{255, 0, 0} | fmt::rgb{0, 0, 255};  // {255, 0, 255}
auto mixed   = fmt::rgb{0xC0F000};
mixed |= fmt::rgb{0x000FEE};  // {192, 255, 238}, packed 0x00C0FFEE
```

### Averaging RGB values

::: blend
    options:
      color_reference: true

`fmt::blend()` takes one or more colors and returns a new `rgb` containing
their equally weighted channel averages. Fractional channel values are
rounded down after all arguments are summed. A single argument is returned
unchanged. This averages the stored RGB components without gamma correction
or alpha compositing.

```cpp
auto gray = fmt::blend(fmt::rgb{255,   0,   0},
                       fmt::rgb{  0, 255,   0},
                       fmt::rgb{  0,   0, 255});  // {85, 85, 85}
auto light_gray = fmt::blend(fmt::rgb{},
                             fmt::rgb{255, 255, 255},
                             fmt::rgb{255, 255, 255});  // {170, 170, 170}
```

The free function leaves its arguments unchanged. The `rgb::blend()` member
averages its object and one other color in place.

### Named RGB colors

::: color
    options:
      color_reference: true

See [Named Colors](named-colors.md) for the available names and their colors.

Each name denotes a constexpr `fmt::rgb` value. Use `fmt::rgb` to store named
and custom colors in the same variable:

```cpp
auto accent = fmt::color::coral;
accent = fmt::rgb{30, 144, 255};
auto style = fmt::fg(accent);
```

### Comparisons

`rgb` values support equality and ordering by their packed `0xRRGGBB` values:
red first, then green, then blue. This is numeric ordering, rather than an
ordering by brightness. They can be used as keys in `std::map` and `std::set`.
In C++20 and later, `operator<=>` returns `std::strong_ordering`.

`std::hash<fmt::rgb>` hashes the packed `uint32_t` representation, allowing RGB
values to be keys in `std::unordered_map` and `std::unordered_set`. Equal RGB
values have equal hashes. Hash values are not a stable serialization format.

!!! note "Changed behavior"

    `fmt::color` was previously an enum type. It is now a namespace containing
    RGB constants. The color names and RGB values are unchanged; `fmt::color`
    is no longer a type, and its named values are no longer enum values.

### Integer representation

`fmt::underlying_type<fmt::rgb>::type` names the integer representation type.
`fmt::to_underlying(fmt::rgb value)` returns that representation:

```cpp
using packed_t = fmt::underlying_type<fmt::rgb>::type;
constexpr auto coral  = fmt::color::coral;
constexpr auto packed = fmt::to_underlying(coral);  // 0x00FF7F50
static_assert(std::is_same_v<const packed_t, decltype(packed)>);
static_assert(fmt::rgb{packed} == coral);
```
Packed integer conversions are explicit in both directions. Direct construction
accepts `0xRRGGBB` and ignores the high byte; conversion back produces
`0x00RRGGBB`:

```cpp
auto sample = fmt::rgb{0xab123456};
auto packed = static_cast<uint32_t>(sample); // 0x00123456
auto named  = static_cast<uint32_t>(fmt::color::coral); // 0x00FF7F50
```

!!! note "Changed behavior"

    Packed integer construction of `rgb` was previously implicit. It is now
    explicit; conversion from `rgb` to a packed integer is also explicit.
