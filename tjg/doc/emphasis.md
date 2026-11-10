# Emphasis

Use emphasis values as styles or combine them with colors through
[`text_style`](text-style.md).

## Attributes

`tjg::emphasis::strike` selects crossed-out text (SGR 9).

::: emphasis
    options:
      color_reference: true
      enum_value_placeholder: unspecified value

!!! note "Backward compatibility"

    `tjg::emphasis::strikethrough` remains an alias for `tjg::emphasis::strike`.
    Both names select the same attribute bit and produce the same SGR sequence.

## Combining attributes

::: operator|(emphasis, emphasis)
    options:
      color_reference: true

The result is an `emphasis` value containing both sets of attribute bits.
An `emphasis` value converts implicitly to `text_style`.

```cpp
auto style = tjg::emphasis::bold
           | tjg::emphasis::underline;
tjg::println(style, "Important: {}", 42);
```

!!! note "Changed behavior"

    Combining two emphasis values now returns `emphasis`.
    Previously, it returned `text_style`.

```cpp
tjg::text_style style = tjg::emphasis::bold | tjg::emphasis::underline;
if (style.has_emph())
  tjg::println(style, "Important");
```

Bold and faint remain independent bits: combining them preserves both and
emits both SGR attributes. Their combined appearance depends on the terminal.

## Integer representation

The underlying representation of `tjg::emphasis` is `uint8_t`.

```cpp
using emphasis_t = std::underlying_type_t<tjg::emphasis>;
constexpr auto bold_bit
                   = std::to_underlying(tjg::emphasis::bold); // 1
constexpr auto attributes = tjg::emphasis::bold
                          | tjg::emphasis::faint;
constexpr auto bits = std::to_underlying(attributes);         // 3
static_assert(std::is_same_v<const emphasis_t, decltype(bits)>);
static_assert(bold_bit == 1);
static_assert(bits     == 3);
```
