// Formatting library for C++ - color support
//
// Copyright (c) 2018 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to LICENSE.

// Currently being refactored by Terry Golubiewski.

#ifndef TJG_COLOR_H_
#define TJG_COLOR_H_

#include "tree_format_specs.h"
#include "memory_buffer.h"
#include "format_support.h"
#include "error_support.h"
#include "hash_support.h"

#include <print>
#include <format>
#include <locale>
#include <string>
#include <string_view>

#include <variant>
#include <functional>
#include <algorithm>
#include <iterator>

#include <compare>
#include <concepts>
#include <exception>
#include <type_traits>
#include <utility>
#include <limits>

#include <cstdio>
#include <cstdint>

namespace tjg {

/// A narrowing static_cast.
template <typename T, typename U>
constexpr inline auto narrow_cast(U x) noexcept -> T {
  return static_cast<T>(x);
}

/// An enum-related static_cast.
template <typename T, typename U>
constexpr inline auto enum_cast(U x) noexcept -> T {
  return static_cast<T>(x);
}

/**
 * Indicates an unspecified color, distinct from RGB or terminal black.
 *
 * Use `no_color` with `fg()` or `bg()` to construct an unspecified channel.
 */
struct no_color_t {
  constexpr auto operator<=>(const no_color_t&) const noexcept = default;

#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER <= 1951
 private:
  // MSVC cannot constexpr-assign an empty alternative in std::variant.
  char _dummy{'\0'};
#endif
};

/// The unspecified-color value.
inline constexpr no_color_t no_color{};

/**
 * The eight normal and eight bright colors of the terminal's palette.
 *
 * Pass a value to `fg()` or `bg()` to select a foreground or background color.
 * The actual RGB values depend on the terminal's palette and theme.
 *
 * Values are palette indices: 0-7 for normal colors and 8-15 for bright colors,
 * in declaration order.  Prefer symbolic names when selecting colors.
 */
enum class term_color : int8_t {
  black,
  red,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  white,
  bright_black,
  bright_red,
  bright_green,
  bright_yellow,
  bright_blue,
  bright_magenta,
  bright_cyan,
  bright_white
};  // term_color

/**
 * Independent text attributes that can be combined with `operator|`.
 * `emphasis::none` specifies no attributes.
 *
 * The enum values are bit masks. Attribute support and the appearance of
 * combinations depend on the terminal.
 */
enum class emphasis : uint8_t {
  none = 0,              ///< No attributes.
  bold = 1 << 0,          ///< Increased intensity (SGR 1).
  faint = 1 << 1,         ///< Decreased intensity (SGR 2).
  italic = 1 << 2,        ///< Italic text         (SGR 3).
  underline = 1 << 3,     ///< Underlined text     (SGR 4).
  blink = 1 << 4,         ///< Blinking text       (SGR 5).
  reverse = 1 << 5,       ///< Reversed fg and bg  (SGR 7).
  conceal = 1 << 6,       ///< Concealed text      (SGR 8).
  strike = 1 << 7,        ///< Crossed-out text    (SGR 9).
};  // emphasis

[[nodiscard]]
constexpr inline auto operator|(emphasis lhs, emphasis rhs) noexcept
    -> emphasis {
  return enum_cast<emphasis>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

constexpr inline auto operator|=(emphasis& lhs, emphasis rhs) noexcept
    -> emphasis& {
  return lhs = lhs | rhs;
}

/// An RGB color with red, green, and blue components in the range 0-255.
struct rgb {
  using value_type = uint32_t;
  using chan_type = uint8_t;

  chan_type r = 0;  ///< Red   component
  chan_type g = 0;  ///< Green component
  chan_type b = 0;  ///< Blue  component

 private:
  [[nodiscard]] constexpr auto value() const noexcept -> value_type {
    return (value_type{r} << 16) | (value_type{g} << 8) | (value_type{b} << 0);
  }

  [[nodiscard]] static constexpr auto check_arg(int v) noexcept -> chan_type {
    return (v >= 0 && v <= 255)
               ? narrow_cast<chan_type>(v)
               : (TJG_ASSERT(false, "rgb::rgb: channel out of range"),
                  narrow_cast<chan_type>(v < 0 ? 0 : 255));
  }

 public:
  /// Constructs black, with all three components set to zero.
  constexpr rgb() noexcept = default;

  /// Constructs channels in the range 0-255.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr rgb(int r_, int g_, int b_) noexcept
      : r{check_arg(r_)}, g{check_arg(g_)}, b{check_arg(b_)} {}

  /**
   * Constructs a color from the low 24 bits of `hex`, interpreted as
   * `0xRRGGBB`. The high byte is ignored.
   */
  constexpr explicit rgb(value_type hex) noexcept
      : r{narrow_cast<chan_type>(hex >> 16)},
        g{narrow_cast<chan_type>(hex >> 8)},
        b{narrow_cast<chan_type>(hex >> 0)} {}

  /// Converts a terminal color using the fixed Xterm-default RGB palette.
  constexpr rgb(term_color clr) noexcept;

  /// Returns a numeric value for the color as 0x00RRGGBB.
  [[nodiscard]]
  constexpr explicit operator value_type() const noexcept {
    return value();
  }

  [[nodiscard]] constexpr auto operator<=>(const rgb&) const noexcept
      -> std::strong_ordering = default;

  /// Combines another color.
  constexpr auto operator|=(rgb rhs) noexcept -> rgb& {
    r |= rhs.r;
    g |= rhs.g;
    b |= rhs.b;
    return *this;
  }

  /// Combines two colors.
  [[nodiscard]]
  friend constexpr auto operator|(rgb lhs, rgb rhs) noexcept -> rgb {
    return rgb{narrow_cast<chan_type>(lhs.r | rhs.r),
               narrow_cast<chan_type>(lhs.g | rhs.g),
               narrow_cast<chan_type>(lhs.b | rhs.b)};
  }

};  // rgb

namespace color {

// The agreed Xterm-default RGB mapping for terminal palette indices 0-15.
constexpr inline rgb term_black{0, 0, 0};
constexpr inline rgb term_red{205, 0, 0};
constexpr inline rgb term_green{0, 205, 0};
constexpr inline rgb term_yellow{205, 205, 0};
constexpr inline rgb term_blue{0, 0, 238};
constexpr inline rgb term_magenta{205, 0, 205};
constexpr inline rgb term_cyan{0, 205, 205};
constexpr inline rgb term_white{229, 229, 229};
constexpr inline rgb term_bright_black{127, 127, 127};
constexpr inline rgb term_bright_red{255, 0, 0};
constexpr inline rgb term_bright_green{0, 255, 0};
constexpr inline rgb term_bright_yellow{255, 255, 0};
constexpr inline rgb term_bright_blue{92, 92, 255};
constexpr inline rgb term_bright_magenta{255, 0, 255};
constexpr inline rgb term_bright_cyan{0, 255, 255};
constexpr inline rgb term_bright_white{255, 255, 255};

}  // namespace color

namespace detail {

inline constexpr rgb term_color_palette[16] = {
    // 0–7 normal
    color::term_black, color::term_red, color::term_green, color::term_yellow,
    color::term_blue, color::term_magenta, color::term_cyan, color::term_white,

    // 8–15 bright
    color::term_bright_black, color::term_bright_red, color::term_bright_green,
    color::term_bright_yellow, color::term_bright_blue,
    color::term_bright_magenta, color::term_bright_cyan,
    color::term_bright_white};  // term_color_palette

constexpr auto checked_term_index(term_color clr) noexcept -> int8_t {
  return (std::to_underlying(clr) >= 0 &&
          std::to_underlying(clr) <= std::to_underlying(term_color::bright_white))
             ? std::to_underlying(clr)
             : (TJG_ASSERT(false, "rgb::rgb: terminal color out of range"),
                std::to_underlying(clr));
}

}  // namespace detail

constexpr inline rgb::rgb(term_color clr) noexcept {
  *this = detail::term_color_palette[detail::checked_term_index(clr)];
}

/// A zero-based gray palette index from 0 to 23, rendered at palette
/// index + 232.
struct gray_color {
  using value_type = uint8_t;

 private:
  using chan_type = int8_t;
  chan_type _value = 0;

  static constexpr auto make_rgb(int level) noexcept -> rgb {
    return rgb{level, level, level};
  }

  [[nodiscard]] static constexpr chan_type check_arg(int v) noexcept {
    return (v >= 0 && v < 24)
               ? narrow_cast<chan_type>(v)
               : (TJG_ASSERT(false, "gray_color::gray_color: index out of range"),
                  narrow_cast<chan_type>(v < 0 ? 0 : 23));
  }

 public:
  /// Constructs gray index zero, whose RGB equivalent is {8, 8, 8}.
  constexpr gray_color() noexcept = default;
  constexpr gray_color(const gray_color&) noexcept = default;
  constexpr gray_color& operator=(const gray_color&) noexcept = default;

  [[nodiscard]] constexpr auto operator<=>(const gray_color&) const noexcept
      -> std::strong_ordering = default;

  /// Constructs a gray index in the range 0-23.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr explicit gray_color(int v) noexcept : _value{check_arg(v)} {}

  /// Returns the zero-based gray palette index, from 0 to 23.
  [[nodiscard]] constexpr auto index() const noexcept -> int { return _value; }

  /// Converts to RGB, with each channel equal to 8 + 10 * index.
  [[nodiscard]] constexpr operator rgb() const noexcept {
    return make_rgb(narrow_cast<rgb::chan_type>(10 * _value + 8));
  }

};  // gray_color

namespace detail {

inline constexpr rgb::chan_type cube_levels[6] = {0, 95, 135, 175, 215, 255};
}  // namespace detail

/// A 6x6x6 palette color with channel indices 0-5 and a combined index 0-215.
struct cube_color {
  using value_type = uint8_t;

 private:
  using chan_type = int8_t;
  chan_type _r = 0;
  chan_type _g = 0;
  chan_type _b = 0;

  [[nodiscard]]
  static constexpr auto check_arg(int v) noexcept -> chan_type {
    return (v >= 0 && v < 6)
               ? narrow_cast<chan_type>(v)
               : (TJG_ASSERT(false, "cube_color::cube_color: channel out of range"),
                  narrow_cast<chan_type>(v < 0 ? 0 : 5));
  }

  [[nodiscard]] static constexpr auto check_index(unsigned v) noexcept
      -> value_type {
    return v <= 215 ? narrow_cast<value_type>(v)
                    : (TJG_ASSERT(false,
                                  "cube_color::cube_color: index out of range"),
                       value_type{215});
  }

 public:
  /// Returns the zero-based cube palette index, from 0 to 215.
  [[nodiscard]] constexpr auto index() const noexcept -> int {
    return 36 * _r + 6 * _g + _b;
  }

  /// Returns the red channel index, from 0 to 5.
  [[nodiscard]] constexpr auto r() const noexcept -> int { return _r; };
  /// Returns the green channel index, from 0 to 5.
  [[nodiscard]] constexpr auto g() const noexcept -> int { return _g; };
  /// Returns the blue channel index, from 0 to 5.
  [[nodiscard]] constexpr auto b() const noexcept -> int { return _b; };

  /// Sets the red index; asserts on invalid input and clamps to 0-5.
  constexpr auto r(int v) noexcept -> cube_color& {
    _r = check_arg(v);
    return *this;
  }

  /// Sets the green index; asserts on invalid input and clamps to 0-5.
  constexpr auto g(int v) noexcept -> cube_color& {
    _g = check_arg(v);
    return *this;
  }

  /// Sets the blue index; asserts on invalid input and clamps to 0-5.
  constexpr auto b(int v) noexcept -> cube_color& {
    _b = check_arg(v);
    return *this;
  }

  /// Constructs cube index zero, with all channel indices zero.
  constexpr cube_color() noexcept = default;
  constexpr cube_color(const cube_color&) noexcept = default;
  constexpr cube_color& operator=(const cube_color&) noexcept = default;

  [[nodiscard]] constexpr auto operator<=>(const cube_color&) const noexcept
      -> std::strong_ordering = default;

  /// Constructs channel indices 0-5.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr cube_color(int r_, int g_, int b_) noexcept
      : _r{check_arg(r_)}, _g{check_arg(g_)}, _b{check_arg(b_)} {}

  /// Constructs a zero-based index 0-215.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr explicit cube_color(unsigned value) noexcept
      : cube_color{check_index(value) / 36, (check_index(value) / 6) % 6,
                   check_index(value) % 6} {}

  /// Converts channel indices to RGB levels {0, 95, 135, 175, 215, 255}.
  [[nodiscard]] constexpr operator rgb() const noexcept {
    return rgb{detail::cube_levels[_r], detail::cube_levels[_g],
               detail::cube_levels[_b]};
  }

};  // cube_color

/// Identifies types that can be used directly as color values.
template <typename T>
concept Color = std::same_as<std::remove_cvref_t<T>, rgb> ||
                std::same_as<std::remove_cvref_t<T>, term_color> ||
                std::same_as<std::remove_cvref_t<T>, cube_color> ||
                std::same_as<std::remove_cvref_t<T>, gray_color>;

/// Returns a plain color's fixed RGB equivalent without changing the argument.
template <Color T>
[[nodiscard]] constexpr auto to_rgb(T color) noexcept -> rgb {
  return rgb(color);
}

/// Preserves an unspecified color rather than converting it to RGB black.
[[nodiscard]] constexpr auto to_rgb(no_color_t) noexcept -> no_color_t {
  return no_color_t{};
}

/// A color variant with alternatives for each supported color kind.
using color_t = std::variant<no_color_t, rgb, term_color, cube_color, gray_color>;

namespace detail {

enum class color_kind : std::size_t { none, rgb, term, cube, gray };

constexpr auto is_unspecified(const color_t& color) noexcept -> bool {
  return color.valueless_by_exception() ||
         std::holds_alternative<no_color_t>(color);
}

struct to_rgb_visitor {
  constexpr auto operator()(const no_color_t&) const noexcept -> color_t {
    return color_t{no_color};
  }
  template <typename T>
  constexpr auto operator()(const T& color) const noexcept -> color_t {
    return color_t{tjg::to_rgb(color)};
  }
};
}  // namespace detail

/**
 * Returns a color variant holding the fixed RGB equivalent of a specified
 * color. An unspecified color remains unspecified. The argument is unchanged.
 */
[[nodiscard]] constexpr auto to_rgb(color_t color) noexcept -> color_t {
  if (color.valueless_by_exception()) return no_color;
  return std::visit(detail::to_rgb_visitor{}, color);
}

namespace detail {

template <typename T>
concept BlendColor = Color<T> ||
                     std::same_as<std::remove_cvref_t<T>, color_t> ||
                     std::same_as<std::remove_cvref_t<T>, no_color_t>;

struct blend_rgb {
  constexpr auto operator()(const no_color_t&) const noexcept -> rgb {
    return rgb{};
  }
  template <typename T>
  constexpr auto operator()(const T& color) const noexcept -> rgb {
    return rgb(color);
  }
};

struct blend_state {
  uint64_t r, g, b;
  size_t count;
  color_t first;

  constexpr blend_state(uint64_t red = 0, uint64_t green = 0, uint64_t blue = 0,
                        size_t n = 0, color_t color = no_color) noexcept
      : r{red}, g{green}, b{blue}, count{n}, first{color} {}

  constexpr auto add(rgb color, color_t original) const noexcept
      -> blend_state {
    return blend_state{r + color.r, g + color.g, b + color.b, count + 1,
                       count == 0 ? original : first};
  }

  constexpr auto add(const color_t& color) const noexcept -> blend_state {
    return is_unspecified(color) ? *this
                                 : add(std::visit(blend_rgb{}, color), color);
  }

  constexpr auto average() const noexcept -> rgb {
    return rgb{narrow_cast<rgb::chan_type>(r / count),
               narrow_cast<rgb::chan_type>(g / count),
               narrow_cast<rgb::chan_type>(b / count)};
  }

  constexpr auto result() const noexcept -> color_t {
    return count < 2 ? first : color_t{average()};
  }
};

}  // namespace detail

/**
 * Returns the component-wise arithmetic mean of two or more plain colors.
 * Accepts rgb, term_color, cube_color, and gray_color, converting each to RGB.
 * Each argument has equal weight. Fractional channels are rounded down after
 * summing all arguments. The arguments are unchanged.
 * Supports constant evaluation.
 */
template <Color First, Color Second, Color... Colors>
[[nodiscard]] constexpr auto blend(First first, Second second,
                                   Colors... rest) noexcept -> rgb {
  detail::blend_state state;
  const rgb colors[] = {rgb(first), rgb(second), rgb(rest)...};
  for (auto color : colors) state = state.add(color, color_t{color});
  return state.average();
}

/**
 * Blends two or more colors, returning the equal-weight RGB average of
 * specified colors and ignoring no_color.
 * Accepts color_t and plain colors. No specified colors produce no_color;
 * one specified color retains its kind and value; two or more produce RGB.
 * Fractional channels are rounded down after summing all specified colors.
 * The arguments are unchanged. Supports constant evaluation.
 */
template <detail::BlendColor Second, detail::BlendColor... Colors>
[[nodiscard]] constexpr auto blend(color_t first, Second second,
                                   Colors... rest) noexcept -> color_t {
  detail::blend_state state;
  const color_t colors[] = {first, color_t{second}, color_t{rest}...};
  for (auto color : colors) state = state.add(color);
  return state.result();
}

/**
 * A text style consisting of optional foreground and background colors and a
 * set of emphasis attributes. Each color may be RGB or a terminal palette
 * color.
 *
 * Default construction produces an empty style. Use `fg()`, `bg()`, and
 * emphasis values to construct styles, and `blend()` to combine them.
 * Blending combines emphasis bits by OR and averages the specified colors
 * independently for the foreground and background, ignoring unspecified colors.
 *
 * **Example (C++20 and later)**:
 *
 *     {
 *       using namespace tjg;
 *       using enum emphasis;
 *       using enum term_color;
 *
 *       auto style1 = text_style{color::red, black, bold | underline};
 *
 *       auto style2 = fg(color::red).bg(black).bold().underline();
 *
 *       auto style3 = text_style{bold | underline, color::red, black};
 *
 *       assert(style1 == style2 && style1 == style3);
 *
 *       auto style4 = style3.underline(false);
 *     }
 *
 * The final line disables underline on `style3` and copies the resulting style
 * into `style4`.
 */
class text_style {
  bool _absolute = false;
  emphasis _emph{};
  color_t _fg;
  color_t _bg;

 public:
  /**
   * Constructs a style from emphasis and optional foreground and background
   * colors. Both colors default to unspecified.
   *
   * Default construction produces an empty style. Conversion from `emphasis`
   * is implicit.
   */
  constexpr text_style(emphasis emp = emphasis::none, color_t fg = no_color,
                       color_t bg = no_color) noexcept
      : text_style{fg, bg, emp} {}

  /**
   * Constructs a style from foreground and background colors and emphasis.
   *
   * Colors may be named RGB constants, RGB, terminal, cube, or gray values,
   * or `no_color`.
   * The background and emphasis default to unspecified and empty, respectively.
   * The foreground is required; `no_color` specifies a background-only style.
   */
  constexpr text_style(color_t fg, color_t bg = no_color,
                       emphasis emp = emphasis::none) noexcept
      : _emph{emp}, _fg{fg}, _bg{bg} {}

  /// Returns true if no colors, emphasis, or absolute flag are set.
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return !_absolute && !has_emph() && !has_fg() && !has_bg();
  }

  /**
   * Clears both colors, all emphasis, and absolute, restoring an empty style.
   * Returns a reference to this style for chaining.
   */
  constexpr auto clear() noexcept -> text_style& {
    _absolute = false;
    _emph = emphasis::none;
    _fg = no_color;
    _bg = no_color;
    return *this;
  }

  /**
   * Enables absolute if `enable` is true, or disables it otherwise.
   * An absolute style resets the terminal before applying its attributes.
   * Unspecified colors select terminal defaults and unset emphasis is cleared.
   * Absolute does not prevent explicit modification of this style.
   */
  constexpr auto absolute(bool enable = true) noexcept -> text_style& {
    _absolute = enable;
    return *this;
  }

  /// Returns true if this style resets the terminal before applying attributes.
  [[nodiscard]] constexpr auto is_absolute() const noexcept -> bool {
    return _absolute;
  }

  /**
   * Applies `other` over this style.
   *
   * Specified foreground and background colors in `other` replace the matching
   * channels, preserving their color kinds. Unspecified channels leave this
   * style's colors unchanged. Emphasis bits are combined by OR. The result is
   * absolute if either input is absolute; explicit overlays remain allowed.
   */
  constexpr auto overlay(const text_style& other) noexcept -> text_style& {
    _absolute = _absolute || other._absolute;
    _emph |= other._emph;
    if (other.has_fg()) _fg = other._fg;
    if (other.has_bg()) _bg = other._bg;
    return *this;
  }  // overlay

  /**
   * Applies `other` under this style.
   *
   * Specified colors in `other` fill matching unspecified channels.
   * Existing colors in this style are preserved. Emphasis bits are combined by
   * OR. The result is absolute if either input is absolute; explicit underlays
   * remain allowed.
   */
  constexpr auto underlay(const text_style& other) noexcept -> text_style& {
    _absolute = _absolute || other._absolute;
    _emph |= other._emph;
    if (!has_fg()) _fg = other._fg;
    if (!has_bg()) _bg = other._bg;
    return *this;
  }  // underlay

  /// Returns true if a foreground color is specified.
  [[nodiscard]] constexpr auto has_fg() const noexcept -> bool {
    return !detail::is_unspecified(_fg);
  }

  /// Returns true if a background color is specified.
  [[nodiscard]] constexpr auto has_bg() const noexcept -> bool {
    return !detail::is_unspecified(_bg);
  }

  /// Returns true if at least one emphasis attribute is set.
  [[nodiscard]] constexpr auto has_emph() const noexcept -> bool {
    return _emph != emphasis::none;
  }

  /**
   * Returns the foreground color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  [[nodiscard]]
  constexpr auto fg() const noexcept -> color_t {
    return _fg;
  }

  /**
   * Returns the background color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  [[nodiscard]]
  constexpr auto bg() const noexcept -> color_t {
    return _bg;
  }

  /// Returns the combined emphasis bit mask.
  [[nodiscard]] constexpr auto emph() const noexcept -> emphasis {
    return _emph;
  }

  /**
   * Replaces the foreground color and returns a reference to this style.
   * Accepts named RGB constants, RGB, terminal, cube, or gray values, or
   * `no_color`. Passing `no_color` clears the foreground to unspecified; it
   * does not store a terminal reset. Preserves the background color and
   * emphasis attributes.
   */
  constexpr auto fg(color_t color) noexcept -> text_style& {
    _fg = color;
    return *this;
  }

  /**
   * Replaces the background color and returns a reference to this style.
   * Accepts named RGB constants, RGB, terminal, cube, or gray values, or
   * `no_color`. Passing `no_color` clears the background to unspecified; it
   * does not store a terminal reset. Preserves the foreground color and
   * emphasis attributes.
   */
  constexpr auto bg(color_t color) noexcept -> text_style& {
    _bg = color;
    return *this;
  }

  /**
   * Replaces all emphasis attributes and returns a reference to this style.
   * Only the bits in `em` remain enabled; `emphasis::none` clears them all.
   * Preserves both colors. Bold and faint remain independent bits.
   */
  constexpr auto emph(emphasis em) noexcept -> text_style& {
    _emph = em;
    return *this;
  }

 private:
  constexpr auto set(emphasis em, bool enable = true) noexcept -> text_style& {
    const auto bits = std::to_underlying(_emph);
    const auto mask = std::to_underlying(em);
    _emph = static_cast<emphasis>(enable ? (bits | mask) : (bits & ~mask));
    return *this;
  }

 public:
  /// Enables bold if `enable` is true, or disables it otherwise.
  constexpr auto bold(bool enable = true) noexcept -> text_style& {
    return set(emphasis::bold, enable);
  }

  /// Enables faint if `enable` is true, or disables it otherwise.
  constexpr auto faint(bool enable = true) noexcept -> text_style& {
    return set(emphasis::faint, enable);
  }

  /// Enables italic if `enable` is true, or disables it otherwise.
  constexpr auto italic(bool enable = true) noexcept -> text_style& {
    return set(emphasis::italic, enable);
  }

  /// Enables underline if `enable` is true, or disables it otherwise.
  constexpr auto underline(bool enable = true) noexcept -> text_style& {
    return set(emphasis::underline, enable);
  }

  /// Enables blink if `enable` is true, or disables it otherwise.
  constexpr auto blink(bool enable = true) noexcept -> text_style& {
    return set(emphasis::blink, enable);
  }

  /// Enables reverse if `enable` is true, or disables it otherwise.
  constexpr auto reverse(bool enable = true) noexcept -> text_style& {
    return set(emphasis::reverse, enable);
  }

  /// Enables conceal if `enable` is true, or disables it otherwise.
  constexpr auto conceal(bool enable = true) noexcept -> text_style& {
    return set(emphasis::conceal, enable);
  }

  /// Enables strikethrough if `enable` is true, or disables it otherwise.
  constexpr auto strike(bool enable = true) noexcept -> text_style& {
    return set(emphasis::strike, enable);
  }

 private:
  [[nodiscard]] constexpr auto is_enabled(emphasis em) const noexcept -> bool {
    return (std::to_underlying(_emph) & std::to_underlying(em)) != 0;
  }

 public:
  /// Returns true if bold is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_bold() const noexcept -> bool {
    return is_enabled(emphasis::bold);
  }

  /// Returns true if faint is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_faint() const noexcept -> bool {
    return is_enabled(emphasis::faint);
  }

  /// Returns true if italic is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_italic() const noexcept -> bool {
    return is_enabled(emphasis::italic);
  }

  /// Returns true if underline is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_underline() const noexcept -> bool {
    return is_enabled(emphasis::underline);
  }

  /// Returns true if blink is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_blink() const noexcept -> bool {
    return is_enabled(emphasis::blink);
  }

  /// Returns true if reverse is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_reverse() const noexcept -> bool {
    return is_enabled(emphasis::reverse);
  }

  /// Returns true if conceal is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_conceal() const noexcept -> bool {
    return is_enabled(emphasis::conceal);
  }

  /// Returns true if strikethrough is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_strike() const noexcept -> bool {
    return is_enabled(emphasis::strike);
  }

  [[nodiscard]] constexpr auto operator==(const text_style&) const noexcept
      -> bool = default;

  [[nodiscard]] constexpr auto operator<=>(const text_style&) const noexcept
      -> std::strong_ordering = default;
};  // text_style

/**
 * Returns the shared terminal style state. Formatting saves and restores this
 * value across nested calls. This state is single-threaded and is not protected
 * by a mutex. Raw terminal escapes and external terminal state are not tracked.
 */
inline auto get_terminal_state() noexcept -> text_style& {
  static auto state = text_style{};
  return state;
}

/// Returns a copy of `style` marked absolute, preserving its colors and
/// emphasis.
[[nodiscard]] constexpr inline auto absolute(text_style style) noexcept
    -> text_style {
  return style.absolute();
}

/**
 * Returns a style with only bold enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto bold(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::bold : emphasis{}};
}

/// Returns a copy of `style` with bold enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto bold(text_style style,
                                         bool enable = true) noexcept
    -> text_style {
  return style.bold(enable);
}

/**
 * Returns a style with only faint enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto faint(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::faint : emphasis{}};
}

/// Returns a copy of `style` with faint enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto faint(text_style style,
                                          bool enable = true) noexcept
    -> text_style {
  return style.faint(enable);
}

/**
 * Returns a style with only italic enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto italic(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::italic : emphasis{}};
}

/// Returns a copy of `style` with italic enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto italic(text_style style,
                                           bool enable = true) noexcept
    -> text_style {
  return style.italic(enable);
}

/**
 * Returns a style with only underline enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto underline(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::underline : emphasis{}};
}

/// Returns a copy of `style` with underline enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto underline(text_style style,
                                              bool enable = true) noexcept
    -> text_style {
  return style.underline(enable);
}

/**
 * Returns a style with only blink enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto blink(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::blink : emphasis{}};
}

/// Returns a copy of `style` with blink enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto blink(text_style style,
                                          bool enable = true) noexcept
    -> text_style {
  return style.blink(enable);
}

/**
 * Returns a style with only reverse enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto reverse(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::reverse : emphasis{}};
}

/// Returns a copy of `style` with reverse enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto reverse(text_style style,
                                            bool enable = true) noexcept
    -> text_style {
  return style.reverse(enable);
}

/**
 * Returns a style with only conceal enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto conceal(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::conceal : emphasis{}};
}

/// Returns a copy of `style` with conceal enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto conceal(text_style style,
                                            bool enable = true) noexcept
    -> text_style {
  return style.conceal(enable);
}

/**
 * Returns a style with only strike enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline auto strike(bool enable = true) noexcept
    -> text_style {
  return text_style{enable ? emphasis::strike : emphasis{}};
}

/// Returns a copy of `style` with strike enabled or disabled by `enable`.
[[nodiscard]] constexpr inline auto strike(text_style style,
                                           bool enable = true) noexcept
    -> text_style {
  return style.strike(enable);
}

/**
 * Returns a copy of `base` with `overrides` applied, leaving both inputs
 * unchanged. Specified colors in `overrides` replace matching channels and
 * preserve their kinds; unspecified channels retain the base colors.
 * Emphasis bits are combined by OR. The result is absolute if either input is.
 */
[[nodiscard]] constexpr inline auto overlay(text_style base,
                                            text_style overrides) noexcept
    -> text_style {
  return base.overlay(overrides);
}

/**
 * Returns a copy of `base` with `defaults` applied, leaving both inputs
 * unchanged. Specified colors in `defaults` replace matching unspecified
 * channels and preserve their kinds. Other channels retain the base colors.
 * Emphasis bits are combined by OR. The result is absolute if either input is.
 */
[[nodiscard]] constexpr inline auto underlay(text_style base,
                                             text_style defaults) noexcept
    -> text_style {
  return base.underlay(defaults);
}

namespace detail {

template <typename T>
concept TextStyle = std::same_as<std::remove_cvref_t<T>, text_style>;

constexpr auto checked_blend_style(const text_style& style) noexcept
    -> const text_style& {
  return !style.is_absolute() || (style.has_fg() && style.has_bg())
             ? style
             : (TJG_ASSERT(false, "cannot blend an absolute style with "
                                  "terminal-default colors"),
                style);
}

struct style_blend_state {
  blend_state foreground, background;
  emphasis attributes;

  constexpr style_blend_state(blend_state fg = blend_state{},
                              blend_state bg = blend_state{},
                              emphasis em = emphasis{}) noexcept
      : foreground{fg}, background{bg}, attributes{em} {}

  constexpr auto add_checked(const text_style& style) const noexcept
      -> style_blend_state {
    return style_blend_state{foreground.add(style.fg()),
                             background.add(style.bg()),
                             attributes | style.emph()};
  }

  constexpr auto add(const text_style& style) const noexcept
      -> style_blend_state {
    return add_checked(checked_blend_style(style));
  }

  constexpr auto result() const noexcept -> text_style {
    return text_style{foreground.result(), background.result(), attributes};
  }
};

}  // namespace detail

/**
 * Blends two or more text styles, averaging each color channel independently
 * and combining emphasis bits by OR. Unspecified colors do not contribute.
 * Each channel preserves its kind with one specified color, holds RGB with
 * two or more, and remains unspecified with none. The arguments are unchanged.
 * The result is relative, even when an input is absolute.
 *
 * Precondition: each absolute input specifies both foreground and background;
 * terminal-default colors have unknown RGB values. Violations assert. When
 * assertions are disabled, unspecified channels contribute nothing and the
 * absolute flag is still discarded.
 * Supports constant evaluation.
 */
template <detail::TextStyle First, detail::TextStyle Second,
          detail::TextStyle... Styles>
[[nodiscard]] constexpr auto blend(First first, Second second,
                                   Styles... rest) noexcept -> text_style {
  auto state = detail::style_blend_state{}.add(first).add(second);
  ((state = state.add(rest)), ...);
  return state.result();
}

/**
 * Creates a style with the specified foreground (text) color, no background
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
[[nodiscard]]
constexpr inline auto fg(color_t foreground) noexcept -> text_style {
  return text_style{foreground};
}

/**
 * Creates a style with the specified background (text) color, no foreground
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
[[nodiscard]]
constexpr inline auto bg(color_t background) noexcept -> text_style {
  return text_style{no_color, background};
}

/// Upper bound on style SGR output length.
constexpr auto sgr_max_length = std::size_t{60};

/// Length of the terminal-reset SGR sequence.
constexpr auto sgr_reset_length = std::size_t{4};

#if defined(__cpp_constexpr) && __cpp_constexpr >= 202211L

/**
 * Returns the ANSI SGR sequence that resets colors and emphasis to terminal
 * defaults. The returned view refers to static storage. `Char`
 * defaults to `char`.
 */
template <typename Char = char>
[[nodiscard]] constexpr auto sgr_reset() noexcept
    -> std::basic_string_view<Char> {
  static constexpr Char reset[] = {'\x1b', '[', '0', 'm'};
  return {reset, sgr_reset_length};
}

#else

namespace detail {

template <typename Char> struct sgr_reset_storage {
  static constexpr Char value[sgr_reset_length] = {'\x1b', '[', '0', 'm'};
};

}  // namespace detail

/**
 * Returns the ANSI SGR sequence that resets colors and emphasis to terminal
 * defaults. The returned view refers to static storage. `Char`
 * defaults to `char`.
 */
template <typename Char = char>
[[nodiscard]] constexpr auto sgr_reset() noexcept
    -> std::basic_string_view<Char> {
  return {detail::sgr_reset_storage<Char>::value, sgr_reset_length};
}

#endif

namespace detail {

constexpr const char* decimal_strings[256] = {
    "0",   "1",   "2",   "3",   "4",   "5",   "6",   "7",   "8",   "9",   "10",
    "11",  "12",  "13",  "14",  "15",  "16",  "17",  "18",  "19",  "20",  "21",
    "22",  "23",  "24",  "25",  "26",  "27",  "28",  "29",  "30",  "31",  "32",
    "33",  "34",  "35",  "36",  "37",  "38",  "39",  "40",  "41",  "42",  "43",
    "44",  "45",  "46",  "47",  "48",  "49",  "50",  "51",  "52",  "53",  "54",
    "55",  "56",  "57",  "58",  "59",  "60",  "61",  "62",  "63",  "64",  "65",
    "66",  "67",  "68",  "69",  "70",  "71",  "72",  "73",  "74",  "75",  "76",
    "77",  "78",  "79",  "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",
    "88",  "89",  "90",  "91",  "92",  "93",  "94",  "95",  "96",  "97",  "98",
    "99",  "100", "101", "102", "103", "104", "105", "106", "107", "108", "109",
    "110", "111", "112", "113", "114", "115", "116", "117", "118", "119", "120",
    "121", "122", "123", "124", "125", "126", "127", "128", "129", "130", "131",
    "132", "133", "134", "135", "136", "137", "138", "139", "140", "141", "142",
    "143", "144", "145", "146", "147", "148", "149", "150", "151", "152", "153",
    "154", "155", "156", "157", "158", "159", "160", "161", "162", "163", "164",
    "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175",
    "176", "177", "178", "179", "180", "181", "182", "183", "184", "185", "186",
    "187", "188", "189", "190", "191", "192", "193", "194", "195", "196", "197",
    "198", "199", "200", "201", "202", "203", "204", "205", "206", "207", "208",
    "209", "210", "211", "212", "213", "214", "215", "216", "217", "218", "219",
    "220", "221", "222", "223", "224", "225", "226", "227", "228", "229", "230",
    "231", "232", "233", "234", "235", "236", "237", "238", "239", "240", "241",
    "242", "243", "244", "245", "246", "247", "248", "249", "250", "251", "252",
    "253", "254", "255"};  // decimal_strings

template <typename Char> class ansi_color_escape {
  // Maximum color SGR is "\x1b[38;2;255;255;255m"
  // Masimum emph  SGR is "\x1b[1;2;3;4;5;7;8;9m
  Char buffer[20] = {};
  size_t size = 0;

  constexpr void append(char c) noexcept {
    buffer[size++] = narrow_cast<Char>(c);
  }

  constexpr void append(const char* s) noexcept {
    while (*s) append(*s++);
  }

  constexpr void append(const char* s, char c) noexcept {
    append(s);
    append(c);
  }

  constexpr void append(uint8_t n, char c) noexcept {
    append(decimal_strings[n]);
    append(c);
  }

  constexpr void append(no_color_t, bool) noexcept {}

  constexpr void append(term_color clr, bool is_bg) noexcept {
    constexpr auto BrightBlack =
        static_cast<int>(std::to_underlying(term_color::bright_black));
    auto value = static_cast<int>(std::to_underlying(clr));
    value += (value < BrightBlack) ? 30 : 90 - BrightBlack;

    // Background ASCII codes are the same as the foreground ones + 10.
    if (is_bg) value += 10;
    if (value >= 100) {
      append('1');
      value %= 100;
    }
    append(narrow_cast<char>('0' + value / 10));
    append(narrow_cast<char>('0' + value % 10));
    append(';');
  }

  constexpr void append(rgb clr, bool is_bg) noexcept {
    append("38;2;");
    if (is_bg) buffer[0] = Char{'4'};
    append(clr.r, ';');
    append(clr.g, ';');
    append(clr.b, ';');
  }

  constexpr void append(cube_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) buffer[0] = Char{'4'};
    auto index = narrow_cast<uint8_t>(clr.index() + 16);
    append(index, ';');
  }

  constexpr void append(gray_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) buffer[0] = Char{'4'};
    auto index = narrow_cast<uint8_t>(clr.index() + 232);
    append(index, ';');
  }

  struct color_visitor {
    ansi_color_escape& escape;
    bool is_bg;

    constexpr void operator()(const no_color_t&) const noexcept {}

    template <typename Color>
    constexpr void operator()(Color clr) const noexcept {
      escape.append(clr, is_bg);
    }
  };

 public:
  constexpr ansi_color_escape(color_t text_color, bool is_bg) noexcept {
    if (!text_color.valueless_by_exception())
      std::visit(color_visitor{*this, is_bg}, text_color);
  }

  constexpr ansi_color_escape(emphasis em) noexcept {
    unsigned bits = std::to_underlying(em);
    if (bits == 0) return;

    constexpr char sgr[] = {'1', '2', '3', '4', '5', '7', '8', '9'};

    for (int i = 0; i < 8; ++i, bits >>= 1) {
      if (!(bits & 1)) continue;
      append(sgr[i]);
      append(';');
    }
  }

  constexpr operator const Char*() const noexcept { return buffer; }

  [[nodiscard]] constexpr auto begin() const noexcept -> const Char* {
    return buffer;
  }

  [[nodiscard]] constexpr auto end() const noexcept -> const Char* {
    return buffer + size;
  }
};  // ansi_color_escape

template <typename Char>
constexpr auto make_foreground_color(color_t foreground) noexcept
    -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(foreground, false);
}

template <typename Char>
constexpr auto make_background_color(color_t background) noexcept
    -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(background, true);
}

template <typename Char>
constexpr auto make_emphasis(emphasis em) noexcept -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(em);
}

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr inline void reset_color(Buffer& buffer) {
  constexpr auto reset_color = sgr_reset<Char>();
  buffer.append(reset_color.data(), reset_color.data() + reset_color.size());
}

template <typename T> struct styled_arg {
  const T& value;
  text_style style;
  constexpr styled_arg(const T& v, text_style s) : value{v}, style{s} {}
};  // styled_arg

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr void append_sgr_raw(Buffer& buf, text_style ts) {
  if (ts.empty()) return;
  if (ts.is_absolute()) {
    buf.push_back(Char('0'));
    buf.push_back(Char(';'));
  }
  auto emphasis = make_emphasis<Char>(ts.emph());
  buf.append(emphasis.begin(), emphasis.end());
  auto foreground = make_foreground_color<Char>(ts.fg());
  buf.append(foreground.begin(), foreground.end());
  auto background = make_background_color<Char>(ts.bg());
  buf.append(background.begin(), background.end());
}  // append_sgr_raw

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr void append_sgr(Buffer& buf, text_style ts) {
  if (ts.empty()) return;
  buf.push_back(static_cast<Char>('\x1b'));
  buf.push_back(static_cast<Char>('['));
  append_sgr_raw(buf, ts);
  buf.data()[buf.size() - 1] = static_cast<Char>('m');
}  // append_sgr

// Nested formatters use the selected state. Independent string formatting
// selects a local state without changing the tracked terminal state.
inline auto active_style_state() noexcept -> text_style*& {
  static auto initial = text_style{};
  static auto active = &initial;
  return active;
}

inline auto get_style_state() noexcept -> text_style& {
  return *active_style_state();
}

class format_scope {
  text_style _state;
  text_style* _previous;

 public:
  format_scope() noexcept : format_scope{get_style_state()} {}

  explicit format_scope(text_style style) noexcept
      : _state{style.absolute(false)}, _previous{active_style_state()} {
    active_style_state() = &_state;
  }

  ~format_scope() { active_style_state() = _previous; }

  format_scope(const format_scope&) = delete;
  void operator=(const format_scope&) = delete;
};

class style_scope {
  text_style _saved;

 public:
  style_scope() noexcept : _saved{get_style_state()} {}

  // Restore shared state on exceptions; partial output may remain.
  ~style_scope() { get_style_state() = _saved; }

  style_scope(const style_scope&) = delete;
  void operator=(const style_scope&) = delete;
};  // style_scope

template <typename Char>
using format_context =
    std::conditional_t<std::same_as<Char, char>, std::format_context,
                       std::wformat_context>;
template <typename Char>
using format_args = std::basic_format_args<format_context<Char>>;

template <typename Buffer, typename Char = typename Buffer::value_type>
void vformat_to(Buffer& buf, text_style ts,
                std::basic_string_view<Char> pattern, format_args<Char> args,
                const std::locale* locale = nullptr) {
  format_scope scope{ts};
  append_sgr(buf, ts);
  if (locale)
    std::vformat_to(std::back_inserter(buf), *locale, pattern, args);
  else
    std::vformat_to(std::back_inserter(buf), pattern, args);
  if (ts != text_style()) reset_color(buf);
}  // vformat_to

}  // namespace detail

/**
 * Returns one ANSI SGR escape sequence for the specified foreground,
 * background, and emphasis, with unpadded parameters. Absolute styles start
 * with a reset parameter. An absolute-only style emits a reset.
 *
 * Returns an empty string for an empty style.
 * Unspecified color channels emit no color parameters. For absolute styles,
 * the leading reset selects terminal defaults for those channels.
 */
template <typename Char = char>
[[nodiscard]] inline auto to_sgr(const text_style& ts)
    TJG_NOEXCEPT_UNLESS_BAD_ALLOC
    -> std::basic_string<Char> {
  if (ts.empty()) return {};
  auto buf = basic_memory_buffer<Char, sgr_max_length>{};
  detail::append_sgr(buf, ts);
  return {buf.data(), buf.size()};
}

namespace detail {

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr void append_transition_code(Buffer& buf, uint8_t code) {
  for (auto cp = decimal_strings[code]; *cp; ++cp) buf.push_back(Char(*cp));
  buf.push_back(Char(';'));
}

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr void append_transition(Buffer& buf, text_style target,
                                 text_style initial) {
  auto before = static_cast<unsigned>(std::to_underlying(initial.emph()));
  auto after = static_cast<unsigned>(std::to_underlying(target.emph()));
  auto removed = before & ~after;
  // SGR 22 clears both intensity attributes; reapply any surviving bit.
  if ((removed & 3u) != 0) {
    append_transition_code(buf, 22);
    before &= ~3u;
  }
  constexpr uint8_t off[] = {23, 24, 25, 27, 28, 29};
  for (unsigned i = 0; i < 6; ++i) {
    if (removed & (4u << i)) append_transition_code(buf, off[i]);
  }
  after &= ~before;
  if (after != 0) {
    auto sgr = make_emphasis<Char>(enum_cast<emphasis>(after));
    buf.append(sgr.begin(), sgr.end());
  }
  auto fg = target.fg();
  if (!is_unspecified(fg)) {
    if (fg != initial.fg()) {
      auto sgr = make_foreground_color<Char>(fg);
      buf.append(sgr.begin(), sgr.end());
    }
  } else if (initial.has_fg()) {
    append_transition_code(buf, 39);
  }
  auto bg = target.bg();
  if (!is_unspecified(bg)) {
    if (bg != initial.bg()) {
      auto sgr = make_background_color<Char>(bg);
      buf.append(sgr.begin(), sgr.end());
    }
  } else if (initial.has_bg()) {
    append_transition_code(buf, 49);
  }
}  // append_transition

template <typename Buffer, typename Char = typename Buffer::value_type>
constexpr void append_sgr_transition(Buffer& buf, text_style target,
                                     text_style initial) {
  if (target.is_absolute()) return append_sgr(buf, target);
  initial.absolute(false);
  if (target == initial) return;
  auto relative = basic_memory_buffer<Char, sgr_max_length>{};
  append_transition(relative, target, initial);
  if (relative.size() == 0) return;
  auto absolute = basic_memory_buffer<Char, sgr_max_length>{};
  append_sgr_raw(absolute, text_style{target}.absolute());
  const auto& parameters =
      (absolute.size() < relative.size()) ? absolute : relative;
  buf.push_back(Char('\x1b'));
  buf.push_back(Char('['));
  buf.append(parameters.data(), parameters.data() + parameters.size());
  buf.data()[buf.size() - 1] = Char('m');
}

template <typename Char, typename OutputIt> class sgr_scope {
  OutputIt& _out;
  text_style& _state;
  text_style _saved;
  int _exceptions = std::uncaught_exceptions();
  basic_memory_buffer<Char, sgr_max_length> _sgr;

  void transition(text_style target) {
    _sgr.clear();
    append_sgr_transition(_sgr, target, _state);
    for (const auto ch : std::basic_string_view<Char>{_sgr.data(), _sgr.size()}) {
      *_out = ch;
      ++_out;
    }
    _state = target.absolute(false);
  }

 public:
  sgr_scope(OutputIt& out, text_style style)
      : _out{out}, _state{get_style_state()}, _saved{_state} {
    if (!style.is_absolute()) style.underlay(_saved);
    transition(style);
  }

  ~sgr_scope() noexcept {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    try {
#endif
      // A throwing nested formatter may have emitted SGR before restoring its
      // logical state. Rebuild the entry style when unwinding in that case.
      transition(text_style{_saved}.absolute(
          std::uncaught_exceptions() > _exceptions));
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
    } catch (...) {
      // Closing SGR is best effort. Record restoration only after success.
    }
#endif
  }

  sgr_scope(const sgr_scope&) = delete;
  void operator=(const sgr_scope&) = delete;
};

// Keep the caller's iterator current even if standard formatting throws after
// writing a prefix, so the guard appends its closing SGR at the correct position.
template <typename Char, typename OutputIt> class format_output_iterator {
  OutputIt* _out;

 public:
  using difference_type = std::ptrdiff_t;
  using iterator_category = std::output_iterator_tag;

  explicit format_output_iterator(OutputIt& out) noexcept : _out{&out} {}
  auto operator*() noexcept -> format_output_iterator& { return *this; }
  auto operator++() noexcept -> format_output_iterator& { return *this; }
  auto operator++(int) noexcept -> format_output_iterator { return *this; }
  auto operator=(Char value) -> format_output_iterator& {
    **_out = value;
    ++*_out;
    return *this;
  }
};

}  // namespace detail

/**
 * Returns the ANSI SGR transition from `initial` to `ts`, without a trailing
 * reset. Both arguments describe complete terminal states: empty color channels
 * mean terminal defaults, and absent emphasis means off. No colors are
 * inherited. The absolute flag on `initial` is ignored. An absolute target
 * always resets and delegates to the one-argument overload, regardless of the
 * initial state. Otherwise returns an empty string when no changes are needed.
 *
 * For relative targets, uses one unpadded sequence, choosing the shorter of
 * selective changes and reset followed by the target attributes.
 * Equal lengths prefer selective changes. `Char` defaults to `char`.
 */
template <typename Char = char>
[[nodiscard]] inline auto to_sgr(const text_style& target,
                                 const text_style& initial)
    TJG_NOEXCEPT_UNLESS_BAD_ALLOC -> std::basic_string<Char> {
  auto buf = basic_memory_buffer<Char, sgr_max_length>{};
  detail::append_sgr_transition(buf, target, initial);
  return {buf.data(), buf.size()};
}

/**
 * Returns a copy of `sv` wrapped in the SGR sequences for `ts` and a terminal
 * reset. Empty text returns an empty string; an empty style returns just the
 * text. The text is copied without formatting or interpreting embedded escapes.
 * The reset restores terminal defaults, not a previously active style.
 */
template <typename Char = char>
[[nodiscard]] inline auto wrap_sgr(const text_style& ts,
                                   std::basic_string_view<Char> sv)
    -> std::basic_string<Char> {
  if (sv.size() == 0) return {};
  if (ts.empty()) return {sv.data(), sv.size()};
  constexpr auto reset = sgr_reset<Char>();
  auto result = std::basic_string<Char>{to_sgr<Char>(ts)};
  result.reserve(sgr_max_length + sgr_reset_length + sv.size());
  result.append(sv.data(), sv.size());
  result.append(reset.data(), reset.size());
  return result;
}  // wrap_sgr

/// Narrow-character convenience overload of `wrap_sgr()`.
[[nodiscard]] inline auto wrap_sgr(const text_style& ts, std::string_view sv)
    -> std::string {
  return wrap_sgr<char>(ts, sv);
}

/**
 * The colored analog of
 * [std::vprint_nonunicode()](https://en.cppreference.com/w/cpp/io/vprint_nonunicode.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline void vprint_nonunicode(FILE* f, text_style ts, std::string_view fmt,
                              std::format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  const auto text = std::string_view{buf.data(), buf.size()};
  std::vprint_nonunicode(f, "{}", std::make_format_args(text));
}

/// Prints a styled message to stdout without Unicode-specific handling.
inline void vprint_nonunicode(text_style ts, std::string_view fmt,
                              std::format_args args) {
  tjg::vprint_nonunicode(stdout, ts, fmt, args);
}

/// Prints a styled message using the standard Unicode output path.
inline void vprint_unicode(FILE* f, text_style ts, std::string_view fmt,
                           std::format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  const auto text = std::string_view{buf.data(), buf.size()};
  std::vprint_unicode(f, "{}", std::make_format_args(text));
}

/// Prints a styled message to stdout using the standard Unicode output path.
inline void vprint_unicode(text_style ts, std::string_view fmt,
                           std::format_args args) {
  tjg::vprint_unicode(stdout, ts, fmt, args);
}

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     tjg::print(stderr, tjg::bold().fg(tjg::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void print(FILE* f, text_style ts, std::format_string<T...> fmt, T&&... args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt.get(), std::make_format_args(args...));
  std::print(f, "{}", std::string_view{buf.data(), buf.size()});
}

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     tjg::print(tjg::bold().fg(tjg::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void print(text_style ts, std::format_string<T...> fmt, T&&... args) {
  return print(stdout, ts, fmt, std::forward<T>(args)...);
}

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     tjg::println(stderr, tjg::bold().fg(tjg::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void println(FILE* f, text_style ts, std::format_string<T...> fmt,
             T&&... args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt.get(), std::make_format_args(args...));
  std::println(f, "{}", std::string_view{buf.data(), buf.size()});
}

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     tjg::println(tjg::bold().fg(tjg::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void println(text_style ts, std::format_string<T...> fmt, T&&... args) {
  return println(stdout, ts, fmt, std::forward<T>(args)...);
}

/**
 * Formats to an iterator using the enclosing formatting style. Outermost
 * formatting uses an independent default state. Output restores its entry style.
 * The enclosing active style and tracked terminal state are preserved.
 * Closing SGR is best effort if the output iterator throws.
 */
template <std::output_iterator<const char&> OutputIt>
auto vformat_to(OutputIt out, text_style style, std::string_view pattern,
                std::format_args args) -> OutputIt {
  detail::format_scope context{};
  {
    detail::sgr_scope<char, OutputIt> scope{out, style};
    std::vformat_to(detail::format_output_iterator<char, OutputIt>{out},
                    pattern, args);
  }
  return out;
}

/// Wide-character counterpart of the styled iterator formatter.
template <std::output_iterator<const wchar_t&> OutputIt>
auto vformat_to(OutputIt out, text_style style, std::wstring_view pattern,
                std::wformat_args args) -> OutputIt {
  detail::format_scope context{};
  {
    detail::sgr_scope<wchar_t, OutputIt> scope{out, style};
    std::vformat_to(detail::format_output_iterator<wchar_t, OutputIt>{out},
                    pattern, args);
  }
  return out;
}

/**
 * Formats directly to an iterator, including opening and closing SGR.
 * Inherits and restores the enclosing formatting style. Outermost formatting
 * uses an independent default state without changing the tracked terminal state.
 */
template <std::output_iterator<const char&> OutputIt, typename... Args>
auto format_to(OutputIt out, text_style style,
               std::format_string<Args...> fmt, Args&&... args) -> OutputIt {
  detail::format_scope context{};
  {
    detail::sgr_scope<char, OutputIt> scope{out, style};
    std::format_to(detail::format_output_iterator<char, OutputIt>{out},
                   fmt, std::forward<Args>(args)...);
  }
  return out;
}

/// Wide-character counterpart of the styled iterator formatter.
template <std::output_iterator<const wchar_t&> OutputIt, typename... Args>
auto format_to(OutputIt out, text_style style,
               std::wformat_string<Args...> fmt, Args&&... args) -> OutputIt {
  detail::format_scope context{};
  {
    detail::sgr_scope<wchar_t, OutputIt> scope{out, style};
    std::format_to(detail::format_output_iterator<wchar_t, OutputIt>{out},
                   fmt, std::forward<Args>(args)...);
  }
  return out;
}

/// Returns a styled string using the enclosing formatting style.
inline auto vformat(text_style style, std::string_view fmt,
                     std::format_args args) -> std::string {
  auto result = std::string{};
  tjg::vformat_to(std::back_inserter(result), style, fmt, args);
  return result;
}

/// Returns a styled wide string using the enclosing formatting style.
inline auto vformat(text_style style, std::wstring_view fmt,
                     std::wformat_args args) -> std::wstring {
  auto result = std::wstring{};
  tjg::vformat_to(std::back_inserter(result), style, fmt, args);
  return result;
}

/**
 * Formats directly into the returned string, with opening and closing SGR.
 * Inherits and restores the enclosing formatting style. Outermost formatting
 * uses an independent default state without changing the tracked terminal state.
 */
template <typename... Args>
auto format(text_style style, std::format_string<Args...> fmt, Args&&... args)
    -> std::string {
  auto result = std::string{};
  tjg::format_to(std::back_inserter(result), style, fmt,
                 std::forward<Args>(args)...);
  return result;
}

/// Wide-character counterpart of the styled string formatter.
template <typename... Args>
auto format(text_style style, std::wformat_string<Args...> fmt, Args&&... args)
    -> std::wstring {
  auto result = std::wstring{};
  tjg::format_to(std::back_inserter(result), style, fmt,
                 std::forward<Args>(args)...);
  return result;
}

namespace detail {

template <typename Char, typename OutputIt>
auto write_color_ascii(OutputIt out, const char* text) -> OutputIt {
  while (*text) *out++ = static_cast<Char>(*text++);
  return out;
}

// Preserve the stored color kind; RGB values never require a name lookup.
template <typename Char, typename OutputIt>
auto write_style_value(OutputIt out, color_t value) -> OutputIt {
  switch (static_cast<color_kind>(value.index())) {
  case color_kind::none:
  default: return write_color_ascii<Char>(out, "no_color");
  case color_kind::rgb:  {
    const auto packed = static_cast<uint32_t>(std::get<rgb>(value));
    *out++ = static_cast<Char>('#');
    for (int shift = 20; shift >= 0; shift -= 4)
      *out++ = static_cast<Char>("0123456789abcdef"[(packed >> shift) & 15]);
    return out;
  }
  case color_kind::term: {
    static const char* const names[] = {"black",        "red",
                                        "green",        "yellow",
                                        "blue",         "magenta",
                                        "cyan",         "white",
                                        "bright_black", "bright_red",
                                        "bright_green", "bright_yellow",
                                        "bright_blue",  "bright_magenta",
                                        "bright_cyan",  "bright_white"};
    const auto index = std::to_underlying(std::get<term_color>(value));
    if (index < 0 || index >= 16)
      detail::standard_format_error("invalid terminal color");
    return write_color_ascii<Char>(out, names[index]);
  }
  case color_kind::cube: {
    const auto color = std::get<cube_color>(value);
    out = write_color_ascii<Char>(out, "cube");
    *out++ = static_cast<Char>('0' + color.r());
    *out++ = static_cast<Char>('0' + color.g());
    *out++ = static_cast<Char>('0' + color.b());
    return out;
  }
  case color_kind::gray: {
    const auto index = std::get<gray_color>(value).index();
    out = write_color_ascii<Char>(out, "gray");
    if (index >= 10) *out++ = static_cast<Char>('0' + index / 10);
    *out++ = static_cast<Char>('0' + index % 10);
    return out;
  }
  }
}

template <typename Char, typename OutputIt>
auto write_style_value(OutputIt out, text_style value) -> OutputIt {
  if (value.empty()) return write_color_ascii<Char>(out, "{}");
  if (value.is_absolute()) *out++ = static_cast<Char>('=');
  const auto bits = std::to_underlying(value.emph());
  for (int bit = 0; bit < 8; ++bit) {
    if ((bits & (1u << bit)) != 0) *out++ = static_cast<Char>("bfi_!r?-"[bit]);
  }
  if (!value.has_fg() && !value.has_bg()) return out;
  if (value.is_absolute() || bits != 0) *out++ = static_cast<Char>(' ');
  if (value.has_fg()) out = write_style_value<Char>(out, value.fg());
  if (value.has_bg()) {
    *out++ = static_cast<Char>('/');
    out = write_style_value<Char>(out, value.bg());
  }
  return out;
}

template <typename Char, typename OutputIt>
auto write_emphasis(OutputIt out, emphasis value) -> OutputIt {
  static const char* const names[] = {"bold",      "faint", "italic",
                                      "underline", "blink", "reverse",
                                      "conceal",   "strike"};
  *out++ = static_cast<Char>('{');
  const auto bits = std::to_underlying(value);
  auto first = true;
  for (int bit = 0; bit < 8; ++bit) {
    if ((bits & (1u << bit)) == 0) continue;
    if (!first) *out++ = static_cast<Char>(' ');
    out = write_color_ascii<Char>(out, names[bit]);
    first = false;
  }
  *out++ = static_cast<Char>('}');
  return out;
}

template <typename Char, typename OutputIt>
auto write_style_value(OutputIt out, emphasis value) -> OutputIt {
  return write_emphasis<Char>(out, value);
}

// Parse the original context so argument IDs and its implementation-provided
// checking information are preserved. '?' is an unquoted alias for 's'.
template <typename Char> class standard_value_specs {
  tree_format_specs<Char> _specs;

 public:
  constexpr auto parse(std::basic_format_parse_context<Char>& ctx)
      -> decltype(ctx.begin()) {
    return _specs.parse(ctx, standard_format_error);
  }

 protected:
  template <typename FormatContext>
  auto write(std::basic_string_view<Char> text, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    const auto specs = _specs.resolve(ctx, standard_format_error);
    // Color/style notations contain only ASCII, so code units equal columns.
    const auto count = (std::min)(text.size(), specs.precision);
    const auto padding = specs.width > count ? specs.width - count : 0;
    const auto left = specs.alignment == '>'   ? padding
                      : specs.alignment == '^' ? padding / 2
                                               : 0;
    auto out = specs.pad(ctx.out(), left);
    out = std::copy_n(text.begin(), count, out);
    return specs.pad(out, padding - left);
  }
};

template <typename T, typename Char>
struct standard_style_value_formatter : standard_value_specs<Char> {
  template <typename FormatContext>
  auto format(const T& value, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto buffer = basic_memory_buffer<Char, 64>{};
    write_style_value<Char>(std::back_inserter(buffer), value);
    return this->write(
        std::basic_string_view<Char>{buffer.data(), buffer.size()}, ctx);
  }
};

}  // namespace detail

namespace detail {
/// Formats a styled argument using the underlying type's format specifications.
template <typename T, typename Char, typename Formatter>
struct basic_styled_arg_formatter : Formatter {
  template <typename FormatContext>
  auto format(const detail::styled_arg<T>& arg, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    const auto initial = get_style_state();
    const auto effective =
        arg.style.is_absolute() ? arg.style : tjg::overlay(initial, arg.style);
    detail::style_scope scope{};
    auto sgr = basic_memory_buffer<Char, sgr_max_length>{};
    detail::append_sgr_transition(sgr, effective, initial);
    auto out = std::copy(sgr.data(), sgr.data() + sgr.size(), ctx.out());
    ctx.advance_to(out);
    get_style_state() = text_style{effective}.absolute(false);
    out = Formatter::format(arg.value, ctx);
    sgr.clear();
    detail::append_sgr_transition(sgr, initial, get_style_state());
    return std::copy(sgr.data(), sgr.data() + sgr.size(), out);
  }
};  // basic_styled_arg_formatter

}  // namespace detail

/** Returns a formatting argument that applies text style `ts` to `value` using
 * ANSI escape sequences. The underlying type's format specifications are
 * preserved, including width, alignment, and precision. Padding produced by the
 * underlying formatter is inside the styled span.
 *
 * The result borrows `value` and stores a copy of `ts`. It does not copy or own
 * `value`, which must remain alive whenever the result is formatted. A
 * temporary value can be used in an immediate formatting call, but storing the
 * result does not extend that temporary's lifetime.
 *
 * Relative styles inherit the active formatting state's missing colors
 * and combine emphasis. Absolute styles use terminal defaults for unspecified
 * components. After the value, including padding, the enclosing style is
 * restored. Shared state initially assumes terminal defaults; styling already
 * active outside this formatting operation and raw escapes are not tracked.
 *
 * **Example**:
 *
 *     std::print("Elapsed time: {0:.2f} seconds",
 *                tjg::styled(1.23, tjg::fg(tjg::color::green)
 *                                      .bg(tjg::color::midnight_blue)));
 */
template <typename T>
constexpr auto styled(const T& value, text_style ts)
    -> detail::styled_arg<std::remove_cvref_t<T>> {
  return detail::styled_arg<std::remove_cvref_t<T>>{value, ts};
}

/**
 * Named RGB colors.
 *
 * This namespace contains constexpr `rgb` constants with fixed RGB values,
 * independently of the terminal palette. Pass a value to `fg()` or `bg()`.
 */
namespace color {
constexpr inline rgb alice_blue{240, 248, 255};
constexpr inline rgb antique_white{250, 235, 215};
constexpr inline rgb aqua{0, 255, 255};
constexpr inline rgb aquamarine{127, 255, 212};
constexpr inline rgb azure{240, 255, 255};
constexpr inline rgb beige{245, 245, 220};
constexpr inline rgb bisque{255, 228, 196};
constexpr inline rgb black{0, 0, 0};
constexpr inline rgb blanched_almond{255, 235, 205};
constexpr inline rgb blue{0, 0, 255};
constexpr inline rgb blue_violet{138, 43, 226};
constexpr inline rgb brown{165, 42, 42};
constexpr inline rgb burly_wood{222, 184, 135};
constexpr inline rgb cadet_blue{95, 158, 160};
constexpr inline rgb chartreuse{127, 255, 0};
constexpr inline rgb chocolate{210, 105, 30};
constexpr inline rgb coral{255, 127, 80};
constexpr inline rgb cornflower_blue{100, 149, 237};
constexpr inline rgb cornsilk{255, 248, 220};
constexpr inline rgb crimson{220, 20, 60};
constexpr inline rgb cyan{0, 255, 255};
constexpr inline rgb dark_blue{0, 0, 139};
constexpr inline rgb dark_cyan{0, 139, 139};
constexpr inline rgb dark_golden_rod{184, 134, 11};
constexpr inline rgb dark_gray{169, 169, 169};
constexpr inline rgb dark_green{0, 100, 0};
constexpr inline rgb dark_khaki{189, 183, 107};
constexpr inline rgb dark_magenta{139, 0, 139};
constexpr inline rgb dark_olive_green{85, 107, 47};
constexpr inline rgb dark_orange{255, 140, 0};
constexpr inline rgb dark_orchid{153, 50, 204};
constexpr inline rgb dark_red{139, 0, 0};
constexpr inline rgb dark_salmon{233, 150, 122};
constexpr inline rgb dark_sea_green{143, 188, 143};
constexpr inline rgb dark_slate_blue{72, 61, 139};
constexpr inline rgb dark_slate_gray{47, 79, 79};
constexpr inline rgb dark_turquoise{0, 206, 209};
constexpr inline rgb dark_violet{148, 0, 211};
constexpr inline rgb deep_pink{255, 20, 147};
constexpr inline rgb deep_sky_blue{0, 191, 255};
constexpr inline rgb dim_gray{105, 105, 105};
constexpr inline rgb dodger_blue{30, 144, 255};
constexpr inline rgb fire_brick{178, 34, 34};
constexpr inline rgb floral_white{255, 250, 240};
constexpr inline rgb forest_green{34, 139, 34};
constexpr inline rgb fuchsia{255, 0, 255};
constexpr inline rgb gainsboro{220, 220, 220};
constexpr inline rgb ghost_white{248, 248, 255};
constexpr inline rgb gold{255, 215, 0};
constexpr inline rgb golden_rod{218, 165, 32};
constexpr inline rgb gray{128, 128, 128};
constexpr inline rgb green{0, 128, 0};
constexpr inline rgb green_yellow{173, 255, 47};
constexpr inline rgb honey_dew{240, 255, 240};
constexpr inline rgb hot_pink{255, 105, 180};
constexpr inline rgb indian_red{205, 92, 92};
constexpr inline rgb indigo{75, 0, 130};
constexpr inline rgb ivory{255, 255, 240};
constexpr inline rgb khaki{240, 230, 140};
constexpr inline rgb lavender{230, 230, 250};
constexpr inline rgb lavender_blush{255, 240, 245};
constexpr inline rgb lawn_green{124, 252, 0};
constexpr inline rgb lemon_chiffon{255, 250, 205};
constexpr inline rgb light_blue{173, 216, 230};
constexpr inline rgb light_coral{240, 128, 128};
constexpr inline rgb light_cyan{224, 255, 255};
constexpr inline rgb light_golden_rod_yellow{250, 250, 210};
constexpr inline rgb light_gray{211, 211, 211};
constexpr inline rgb light_green{144, 238, 144};
constexpr inline rgb light_pink{255, 182, 193};
constexpr inline rgb light_salmon{255, 160, 122};
constexpr inline rgb light_sea_green{32, 178, 170};
constexpr inline rgb light_sky_blue{135, 206, 250};
constexpr inline rgb light_slate_gray{119, 136, 153};
constexpr inline rgb light_steel_blue{176, 196, 222};
constexpr inline rgb light_yellow{255, 255, 224};
constexpr inline rgb lime{0, 255, 0};
constexpr inline rgb lime_green{50, 205, 50};
constexpr inline rgb linen{250, 240, 230};
constexpr inline rgb magenta{255, 0, 255};
constexpr inline rgb maroon{128, 0, 0};
constexpr inline rgb medium_aquamarine{102, 205, 170};
constexpr inline rgb medium_blue{0, 0, 205};
constexpr inline rgb medium_orchid{186, 85, 211};
constexpr inline rgb medium_purple{147, 112, 219};
constexpr inline rgb medium_sea_green{60, 179, 113};
constexpr inline rgb medium_slate_blue{123, 104, 238};
constexpr inline rgb medium_spring_green{0, 250, 154};
constexpr inline rgb medium_turquoise{72, 209, 204};
constexpr inline rgb medium_violet_red{199, 21, 133};
constexpr inline rgb midnight_blue{25, 25, 112};
constexpr inline rgb mint_cream{245, 255, 250};
constexpr inline rgb misty_rose{255, 228, 225};
constexpr inline rgb moccasin{255, 228, 181};
constexpr inline rgb navajo_white{255, 222, 173};
constexpr inline rgb navy{0, 0, 128};
constexpr inline rgb old_lace{253, 245, 230};
constexpr inline rgb olive{128, 128, 0};
constexpr inline rgb olive_drab{107, 142, 35};
constexpr inline rgb orange{255, 165, 0};
constexpr inline rgb orange_red{255, 69, 0};
constexpr inline rgb orchid{218, 112, 214};
constexpr inline rgb pale_golden_rod{238, 232, 170};
constexpr inline rgb pale_green{152, 251, 152};
constexpr inline rgb pale_turquoise{175, 238, 238};
constexpr inline rgb pale_violet_red{219, 112, 147};
constexpr inline rgb papaya_whip{255, 239, 213};
constexpr inline rgb peach_puff{255, 218, 185};
constexpr inline rgb peru{205, 133, 63};
constexpr inline rgb pink{255, 192, 203};
constexpr inline rgb plum{221, 160, 221};
constexpr inline rgb powder_blue{176, 224, 230};
constexpr inline rgb purple{128, 0, 128};
constexpr inline rgb rebecca_purple{102, 51, 153};
constexpr inline rgb red{255, 0, 0};
constexpr inline rgb rosy_brown{188, 143, 143};
constexpr inline rgb royal_blue{65, 105, 225};
constexpr inline rgb saddle_brown{139, 69, 19};
constexpr inline rgb salmon{250, 128, 114};
constexpr inline rgb sandy_brown{244, 164, 96};
constexpr inline rgb sea_green{46, 139, 87};
constexpr inline rgb sea_shell{255, 245, 238};
constexpr inline rgb sienna{160, 82, 45};
constexpr inline rgb silver{192, 192, 192};
constexpr inline rgb sky_blue{135, 206, 235};
constexpr inline rgb slate_blue{106, 90, 205};
constexpr inline rgb slate_gray{112, 128, 144};
constexpr inline rgb snow{255, 250, 250};
constexpr inline rgb spring_green{0, 255, 127};
constexpr inline rgb steel_blue{70, 130, 180};
constexpr inline rgb tan{210, 180, 140};
constexpr inline rgb teal{0, 128, 128};
constexpr inline rgb thistle{216, 191, 216};
constexpr inline rgb tomato{255, 99, 71};
constexpr inline rgb turquoise{64, 224, 208};
constexpr inline rgb violet{238, 130, 238};
constexpr inline rgb wheat{245, 222, 179};
constexpr inline rgb white{255, 255, 255};
constexpr inline rgb white_smoke{245, 245, 245};
constexpr inline rgb yellow{255, 255, 0};
constexpr inline rgb yellow_green{154, 205, 50};
}  // namespace color

}  // namespace tjg

namespace std {

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::no_color_t, Char>
    : tjg::detail::standard_style_value_formatter<tjg::no_color_t, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::rgb, Char>
    : tjg::detail::standard_style_value_formatter<tjg::rgb, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::term_color, Char>
    : tjg::detail::standard_style_value_formatter<tjg::term_color, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::gray_color, Char>
    : tjg::detail::standard_style_value_formatter<tjg::gray_color, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::cube_color, Char>
    : tjg::detail::standard_style_value_formatter<tjg::cube_color, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::color_t, Char>
    : tjg::detail::standard_style_value_formatter<tjg::color_t, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::text_style, Char>
    : tjg::detail::standard_style_value_formatter<tjg::text_style, Char> {};

template <typename Char>
  requires(same_as<Char, char> || same_as<Char, wchar_t>)
struct formatter<tjg::emphasis, Char>
    : tjg::detail::standard_style_value_formatter<tjg::emphasis, Char> {};

template <typename T, typename Char>
  requires formattable<decay_t<const T>, Char>
struct formatter<tjg::detail::styled_arg<T>, Char>
    : tjg::detail::basic_styled_arg_formatter<
          T, Char, formatter<decay_t<const T>, Char>> {};

template <> struct hash<tjg::rgb> {
  auto operator()(tjg::rgb value) const noexcept -> size_t {
    return hash<uint32_t>{}(static_cast<uint32_t>(value));
  }
};

template <> struct hash<tjg::color_t> {
  auto operator()(tjg::color_t value) const noexcept -> size_t {
    auto result = hash<size_t>{}(value.index());
    if (value.valueless_by_exception()) return result;
    std::visit(
        [&](const auto& color) noexcept {
          using T = decay_t<decltype(color)>;
          if constexpr (is_same_v<T, tjg::gray_color> ||
                        is_same_v<T, tjg::cube_color>)
            tjg::hash_combine(result, color.index());
          else if constexpr (!is_same_v<T, tjg::no_color_t>)
            tjg::hash_combine(result, color);
        },
        value);
    return result;
  }
};

template <> struct hash<tjg::text_style> {
  auto operator()(tjg::text_style value) const noexcept -> size_t {
    auto result = hash<bool>{}(value.is_absolute());
    tjg::hash_combine(result, value.emph());
    tjg::hash_combine(result, value.fg());
    tjg::hash_combine(result, value.bg());
    return result;
  }
};

}  // namespace std

#endif  // TJG_COLOR_H_
