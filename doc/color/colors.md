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
auto unspecified    = fmt::fg(fmt::no_color);          // No foreground.
auto rgb_black      = fmt::fg(fmt::rgb{});             // Black foreground.
auto terminal_black = fmt::fg(fmt::term_color::black); // Black foreground.
```

Both `fmt::fg(fmt::no_color)` and `fmt::bg(fmt::no_color)` produce empty styles.
They do not clear a color already present in another style when combined with
`|` or `|=`, and they do not emit a terminal reset.

## Terminal palette colors

::: term_color
    options:
      color_reference: true

!!! note "Backward compatibility"

    `fmt::terminal_color` remains an alias for `fmt::term_color`.
    Both names denote the same type and provide the same color constants.

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
auto style = fmt::fg(fmt::term_color::black);
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
static_assert(std::is_same<const packed_t, decltype(packed)>::value,
              "packed RGB type");
static_assert(fmt::rgb{packed} == coral, "packed RGB round trip");
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

## Color variants

`fmt::color_t` is a public value type for storing different kinds of color in
one variable. It is available in C++11 and later and uses its own union-based
implementation in every supported language mode; it does not require
`std::variant`.

::: color_t
    options:
      color_reference: true

### Construction and assignment

Default construction holds `no_color_t`. Construction from `no_color`, an
`rgb` (including a named RGB constant), or a `term_color` is implicit and
`noexcept`. Assignment replaces the active alternative and is also `noexcept`.
Copying or moving a `color_t` preserves its kind and value.

```cpp
fmt::color_t accent;                    // Unspecified.
accent = fmt::color::coral;             // Holds rgb.
accent = fmt::term_color::red;          // Holds term_color.
auto style = fmt::text_style{accent};
style.bg(fmt::color_t{fmt::color::blue});
accent = fmt::no_color;                 // Unspecified again; style is unchanged.
```

`text_style` constructors, color setters, and free `fg()`/`bg()` accept
`color_t`. The style's `fg()` and `bg()` getters return independent `color_t`
values. Changing a stored color or an extracted color leaves the style
unchanged; use a style setter to replace a channel.

### Alternatives and indices

`index()` returns the active alternative's index as `std::size_t`:

| Index | `color_t::kind` | Alternative  | Meaning                |
| ----: | :-------------: | -----------  | ---------------------- |
|    0  |     `none`      | `no_color_t` | Unspecified color      |
|    1  |     `rgb`       | `rgb`        | Three RGB channels     |
|    2  |     `rsvd`      | `rsvd_color` | Reserved; invalid      |
|    3  |     `term`      | `term_color` | Terminal palette color |

RGB black and terminal black remain specified alternatives, distinct from
`no_color_t`.

### Accessing a color

::: holds_alternative
    options:
      color_reference: true

::: get(color_t&)
    options:
      color_reference: true

::: get_if(color_t*)
    options:
      color_reference: true

`holds_alternative<T>(color)` reports whether the color holds `T`.
`get<T>(color)` returns a reference to that alternative and throws
`format_error` if another alternative is active. `get_if<T>(&color)` returns
a pointer to the alternative, or `nullptr` for a different kind or a null
input pointer. Classification and pointer access are `noexcept`.

Both access functions also accept an index: `get<1>(color)` and
`get_if<1>(&color)` access RGB. `T` must be one of the listed alternatives;
an index must be in the range 0–3. An unsupported type or index is a compile-time
error. The reserved alternative never matches a valid color.

The `get` overloads preserve the argument's constness and value category:

| Color argument    | Type-based result | Index-based result |
| ----------------- | ----------------- | ------------------ |
| `color_t&`        | `T&`              | Reference to the indexed alternative |
| `const color_t&`  | `const T&`        | Const reference to the indexed alternative |
| `color_t&&`       | `T&&`             | Rvalue reference to the indexed alternative |
| `const color_t&&` | `const T&&`       | Const rvalue reference to the indexed alternative |

`get_if` returns `T*` for a mutable color pointer and `const T*` for a const
color pointer. Mutable references and pointers allow changes to the active
color value.

```cpp
fmt::color_t accent = fmt::color::coral;
if (auto* channels = fmt::get_if<fmt::rgb>(&accent))
  channels->g = 128;

bool is_rgb = fmt::holds_alternative<fmt::rgb>(accent); // true
constexpr auto rgb_index = fmt::enum_cast<std::size_t>(fmt::color_t::kind::rgb);
auto red = fmt::get<rgb_index>(accent).r;
```

References and pointers refer to storage inside the `color_t`; they must not
outlive it or an alternative change. Style color getters return temporary
values, so copy an alternative when extracting it directly:

```cpp
auto style = fmt::text_style{fmt::color::coral};
auto channels = fmt::get<fmt::rgb>(style.fg()); // Copies RGB before the temporary dies.
```

Const access, classification, and `index()` support constant evaluation in
C++11. Mutable access and alternative assignment support it in C++14 and
later on supported compilers.

### Visitation

`fmt::visit(visitor, color)` calls the visitor with the active alternative of
one `color_t`. It preserves the color argument's constness and value category
and forwards the visitor. The visitor must accept all four alternatives,
including `fmt::rsvd_color`, with exactly the same return type. Value,
reference, and `void` results are supported; visitor exceptions propagate.

A function object with overloads works in C++11:

```cpp
struct describe_color {
  const char* operator()(fmt::no_color_t) const { return "unspecified";      }
  const char* operator()(fmt::rgb)        const { return "RGB";              }
  const char* operator()(fmt::term_color) const { return "terminal palette"; }
  const char* operator()(fmt::rsvd_color) const { return "reserved";         }
};

fmt::color_t accent = fmt::color::coral;
const char* description = fmt::visit(describe_color{}, accent); // "RGB"
```

The reserved overload is required for compilation but is never called for a
valid color. A visitor can instead use a templated call operator that accepts
every alternative. Visitation supports C++11 constant evaluation when the
visitor and color access do.

### Composition, comparisons, and numeric representation

`color_t` supports `|` and `|=`. An unspecified color takes the other operand's
value; two RGB colors combine their channel bits by OR. A conflict involving
a terminal color throws `format_error`, even for identical terminal colors.
Failed composition leaves the left operand unchanged.

Equality and ordering include both kind and payload: unspecified, RGB, and
terminal colors remain distinct. Ordering follows the unsigned packed value,
with unspecified colors before RGB and RGB before terminal colors. C++20
and later provide `std::strong_ordering` through `operator<=>`.
`std::hash<fmt::color_t>` permits use as an unordered-container key.

`color_t::value_type` and `underlying_type<color_t>::type` are `uint32_t`.
`fmt::to_underlying(color)` returns the packed representation used by
`text_style`, including the alternative's discriminator and payload. This
encoding and its hash values are not stable serialization formats.
