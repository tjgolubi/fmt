// Formatting library for C++ - color support
//
// Copyright (c) 2018 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

// Currently being refactored by Terry Golubiewski.

#ifndef FMT_COLOR_H_
#define FMT_COLOR_H_

#include "format.h"

#ifndef FMT_MODULE
#  include <functional>  // std::hash
#endif

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L && FMT_HAS_INCLUDE(<compare>)
#  include <compare>
#endif

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

/// A narrowing static_cast.
template<typename T, typename U>
constexpr inline auto narrow_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

/// An enum-related static_cast.
template<typename T, typename U>
constexpr inline auto enum_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

/// Provides the integer representation as the member type `type`.
template<typename T>
struct underlying_type { using type = typename T::value_type; };

/// Returns the integer representation of `value`.
template<typename T>
FMT_NODISCARD constexpr auto to_underlying(T value) noexcept
  -> typename underlying_type<T>::type
{ return static_cast<typename underlying_type<T>::type>(value); }

/**
 * Indicates an unspecified color, distinct from RGB or terminal black.
 *
 * Use `no_color` with `fg()` or `bg()` to construct an unspecified channel.
 */
struct no_color_t {};

/// The unspecified-color value.
FMT_INLINE_VARIABLE constexpr no_color_t no_color{};

/**
 * The eight normal and eight bright colors of the terminal's palette.
 *
 * Pass a value to `fg()` or `bg()` to select a foreground or background color.
 * The actual RGB values depend on the terminal's palette and theme.
 *
 * Values are palette indices: 0-7 for normal colors and 8-15 for bright colors,
 * in declaration order.  Prefer symbolic names when selecting colors.
 */
enum class term_color : uint8_t {
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
}; // term_color

/// Alias for `term_color`, retained for backward compatibility.
using terminal_color = term_color;

template<> struct underlying_type<term_color> { using type = uint8_t; };

/**
 * Independent text attributes that can be combined with `operator|`.
 * `emphasis()` specifies no attributes.
 *
 * The enum values are bit masks. Attribute support and the appearance of
 * combinations depend on the terminal.
 */
enum class emphasis : uint8_t {
  bold          = 1 << 0, ///< Increased intensity (SGR 1).
  faint         = 1 << 1, ///< Decreased intensity (SGR 2).
  italic        = 1 << 2, ///< Italic text         (SGR 3).
  underline     = 1 << 3, ///< Underlined text     (SGR 4).
  blink         = 1 << 4, ///< Blinking text       (SGR 5).
  reverse       = 1 << 5, ///< Reversed fg and bg  (SGR 7).
  conceal       = 1 << 6, ///< Concealed text      (SGR 8).
  strike        = 1 << 7, ///< Crossed-out text    (SGR 9).
  strikethrough = strike  ///< Alias for `strike`, for backward compatibility.
}; // emphasis

template<> struct underlying_type<emphasis> { using type = uint8_t; };

FMT_NODISCARD
FMT_CONSTEXPR inline auto operator|(emphasis lhs, emphasis rhs) noexcept
  -> emphasis
{
  auto val = to_underlying(lhs) | to_underlying(rhs);
  return enum_cast<emphasis>(val);
}

#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L

#define FMT_COLOR_COMPARISONS(C) \
  FMT_PRAGMA_CLANG(diagnostic push) \
  FMT_PRAGMA_CLANG(diagnostic ignored "-Wzero-as-null-pointer-constant") \
  FMT_NODISCARD constexpr auto operator<=>(const C&) const noexcept \
    -> std::strong_ordering = default; \
  FMT_PRAGMA_CLANG(diagnostic pop)

#else

#define FMT_COLOR_COMPARISONS(C) \
  FMT_NODISCARD constexpr auto operator==(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) == to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator!=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) != to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator< (const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) <  to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator<=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) <= to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator> (const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) >  to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator>=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) >= to_underlying(rhs); }

#endif

/// An RGB color with red, green, and blue components in the range 0-255.
struct rgb {
  using value_type = uint32_t;
  using chan_type  = uint8_t;

  chan_type r = 0;  ///< Red   component
  chan_type g = 0;  ///< Green component
  chan_type b = 0;  ///< Blue  component

  /// Constructs black, with all three components set to zero.
  constexpr rgb() noexcept = default;

  /// Constructs a color from red, green, and blue components, respectively.
  constexpr rgb(chan_type r_, chan_type g_, chan_type b_) noexcept
    : r{r_}, g{g_}, b{b_} {}

  /**
   * Constructs a color from the low 24 bits of `hex`, interpreted as
   * `0xRRGGBB`. The high byte is ignored.
   */
  constexpr explicit rgb(value_type hex) noexcept
    : r{narrow_cast<chan_type>(hex >> 16)}
    , g{narrow_cast<chan_type>(hex >>  8)}
    , b{narrow_cast<chan_type>(hex >>  0)}
    {}

  /// Returns a numeric value for the color as 0x00RRGGBB.
  FMT_NODISCARD
  constexpr explicit operator value_type() const noexcept {
    return (value_type{r} << 16)
         | (value_type{g} <<  8)
         | (value_type{b} <<  0);
  }

  FMT_COLOR_COMPARISONS(rgb)

  /// Combines another color.
  FMT_CONSTEXPR auto operator|=(rgb rhs) noexcept -> rgb& {
    r |= rhs.r;
    g |= rhs.g;
    b |= rhs.b;
    return *this;
  }

  /// Combines two colors.
  FMT_NODISCARD
  friend FMT_CONSTEXPR auto operator|(rgb lhs, rgb rhs) noexcept -> rgb
  { return lhs |= rhs; }

  FMT_CONSTEXPR auto blend(rgb c) noexcept -> rgb& {
    r = narrow_cast<chan_type>((unsigned{r} + unsigned{c.r}) / 2);
    g = narrow_cast<chan_type>((unsigned{g} + unsigned{c.g}) / 2);
    b = narrow_cast<chan_type>((unsigned{b} + unsigned{c.b}) / 2);
    return *this;
  }
}; // rgb

/**
 * Returns the component-wise arithmetic mean of one or more RGB colors.
 * Each argument has equal weight. Fractional channel values are rounded down
 * after summing all arguments. The arguments are unchanged.
 */
template<typename... Colors>
FMT_NODISCARD
FMT_CONSTEXPR auto blend(rgb first, Colors... rest) noexcept -> rgb {
  const rgb colors[] = {first, rest...};
  rgb::value_type r = 0, g = 0, b = 0;
  for (auto c : colors) {
    r += c.r;
    g += c.g;
    b += c.b;
  }
  constexpr auto count = sizeof...(Colors) + 1;
  return rgb{narrow_cast<rgb::chan_type>(r / count),
             narrow_cast<rgb::chan_type>(g / count),
             narrow_cast<rgb::chan_type>(b / count)};
} // blend

namespace detail {

// A bit-packed variant of an RGB color, a terminal color, or unset color.
class color_type {
public:
  using value_type = uint32_t;

private:
  enum class disc { none, rgb, rsvd, term };
  // These values must be in exactly this order for the overflow trick to work.

  // The information is packed as follows:
  // ┌──┐
  // │ 0│─┐
  // │..│ ├── color value
  // │23│─┘
  // ├──┤
  // │24│─┬── discriminator for the above value. 00 if unset, 01 if it's
  // │25│─┘   an RGB color, or 11 if it's a terminal color (10 is unused)
  // ├──┤
  // │26│──── overflow bit, always zero (see below)
  // ├──┤
  // │27│─┐
  // │..│ ├── unused
  // │31│─┘
  // └──┘

  static constexpr auto color_bits = 24;
  static constexpr auto disc_bits  =  2;
  static constexpr auto value_bits = color_bits + disc_bits;
  static constexpr auto color_mask = (value_type{1} << color_bits) - 1;
  static constexpr auto disc_mask  = value_type{0xff} << color_bits;
  static constexpr auto value_mask = (value_type{1} << value_bits) - 1;

  value_type value_ = 0;

  FMT_NODISCARD constexpr auto type() const noexcept -> disc
  { return enum_cast<disc>((value_ & disc_mask) >> color_bits); }

  FMT_CONSTEXPR auto type(disc t) noexcept -> color_type& {
    value_ = (value_ & ~disc_mask) | (enum_cast<value_type>(t) << color_bits);
    return *this;
  }

  FMT_CONSTEXPR void clear() noexcept { value_ = 0; }

  template<typename T>
  FMT_CONSTEXPR void set(disc t, T v) noexcept
  { value_ = (enum_cast<value_type>(t) << color_bits) | to_underlying(v); }

  FMT_CONSTEXPR void set(rgb val) noexcept { set(disc::rgb, val); }

  FMT_CONSTEXPR void set(term_color val) noexcept { set(disc::term, val); }

  template<typename T>
  friend constexpr auto holds_alternative(color_type color) noexcept -> bool;

public:
  constexpr color_type() noexcept = default;

  constexpr color_type(no_color_t) noexcept {}

  FMT_CONSTEXPR color_type(rgb clr) noexcept { set(clr); }

  FMT_CONSTEXPR color_type(term_color clr) noexcept { set(clr); }

  FMT_NODISCARD constexpr auto value() const noexcept -> value_type
  { return value_ & color_mask; }

  constexpr explicit color_type(value_type value) noexcept : value_{value} {}

  constexpr explicit operator value_type() const noexcept { return value_; }

  /// Combines RGB payloads, rejecting overlapping non-RGB colors.
  FMT_CONSTEXPR auto operator|=(color_type rhs) -> color_type& {
    constexpr auto overflow_mask = value_type{1} << value_bits;
    if (((value_ + rhs.value_) & overflow_mask) != 0)
      report_error("can't OR a terminal color");
    value_ |= rhs.value_;
    return *this;
  }

  FMT_NODISCARD
  friend FMT_CONSTEXPR auto operator|(color_type lhs, color_type rhs)
    -> color_type
  { return lhs |= rhs; }

  FMT_COLOR_COMPARISONS(color_type)

}; // color_type

template<typename T>
FMT_NODISCARD
constexpr auto holds_alternative(color_type color) noexcept -> bool {
  static_assert(std::is_same<T, no_color_t>::value ||
                std::is_same<T, term_color>::value ||
                std::is_same<T, rgb>::value, "unsupported color alternative");
  return color.type() == (std::is_same<T, no_color_t>::value
                          ? color_type::disc::none
                          : std::is_same<T, term_color>::value
                              ? color_type::disc::term
                              : color_type::disc::rgb);
} // holds_alternative

#if FMT_CLANG_VERSION && FMT_CLANG_VERSION <= 306
// Clang 3.6 and earlier treat specializations of a deleted template as deleted.
template<typename T>
auto get(color_type) -> T {
  static_assert(sizeof(T) == 0, "unsupported color alternative");
}
#else
template<typename T>
auto get(color_type color) -> T = delete;
#endif

template<>
FMT_NODISCARD FMT_CONSTEXPR inline auto get<no_color_t>(color_type color)
  -> no_color_t
{
  if (!holds_alternative<no_color_t>(color))
    report_error("color_type is not no_color_t");
  return no_color;
}

template<>
FMT_NODISCARD FMT_CONSTEXPR inline auto get<term_color>(color_type color)
  -> term_color
{
  if (!holds_alternative<term_color>(color))
    report_error("color_type is not term_color");
  return enum_cast<term_color>(color.value());
}

template<>
FMT_NODISCARD FMT_CONSTEXPR inline auto get<rgb>(color_type color) -> rgb {
  if (!holds_alternative<rgb>(color))
    report_error("color_type is not rgb");
  return rgb{color.value()};
}

} // detail

/**
 * A text style consisting of optional foreground and background colors and a
 * set of emphasis attributes. Each color may be RGB or a terminal palette
 * color.
 *
 * Default construction produces an empty style. Use `fg()`, `bg()`, and
 * emphasis values to construct styles, and `operator|` to combine them.
 * Composition combines emphasis bits and independently specified color
 * channels. Overlapping RGB colors are combined by bitwise OR; a same-channel
 * conflict involving a terminal color is an error.
 *
 * **Example**:
 *
 *     auto style = fmt::fg(fmt::color::red)
 *                | fmt::bg(fmt::term_color::black)
 *                | fmt::emphasis::bold;
 */
class text_style {
  // The information is packed as follows:
  // ┌──┐
  // │ 0│─┐
  // │..│ ├── foreground color value
  // │23│─┘
  // ├──┤
  // │24│─┬── discriminator for the above value. 00 if unset, 01 if it's
  // │25│─┘   an RGB color, or 11 if it's a terminal color (10 is unused)
  // ├──┤
  // │26│──── overflow bit, always zero (see below)
  // ├──┤
  // │27│─┐
  // │..│ │
  // │50│ │
  // ├──┤ │
  // │51│ ├── background color (same format as the foreground color)
  // │52│ │
  // ├──┤ │
  // │53│─┘
  // ├──┤
  // │54│─┐
  // │..│ ├── emphases
  // │61│─┘
  // ├──┤
  // │62│─┬── unused
  // │63│─┘
  // └──┘
  // The overflow bits are there to make operator|= efficient.
  // When ORing, we must throw if, for either the foreground or background,
  // one style specifies a terminal color and the other specifies any color
  // (terminal or RGB); in other words, if one discriminator is 11 and the
  // other is 11 or 01.
  //
  // We do that check by adding the styles. Consider what adding does to each
  // possible pair of discriminators:
  //    00 + 00 = 000
  //    01 + 00 = 001
  //    11 + 00 = 011
  //    01 + 01 = 010
  //    11 + 01 = 100 (!!)
  //    11 + 11 = 110 (!!)
  // In the last two cases, the ones we want to catch, the third bit——the
  // overflow bit——is set. Bingo.
  //
  // We must take into account the possible carry bit from the bits
  // before the discriminator. The only potentially problematic case is
  // 11 + 00 = 011 (a carry bit would make it 100, not good!), but a carry
  // bit is impossible in that case, because 00 (unset color) means the
  // 24 bits that precede the discriminator are all zero.
  //
  // This test can be applied to both colors simultaneously.

public:
  using value_type = uint64_t;

private:
  value_type style_ = 0;

  static constexpr auto color_bits = 24;
  static constexpr auto disc_bits  =  2;
  static constexpr auto emph_bits  =  8;
  static constexpr auto fg_shift   =  0;
  static constexpr auto bg_shift   = 27;
  static constexpr auto emph_shift = 54;

  static constexpr auto color_mask = (uint32_t{1} << color_bits) - 1;
  static constexpr auto disc_mask  = (1U  << disc_bits ) - 1;
  static constexpr auto value_bits = color_bits + disc_bits;
  static constexpr auto value_mask = (uint32_t{1} << value_bits) - 1;

  static constexpr auto fg_mask = value_type{value_mask} << fg_shift;
  static constexpr auto bg_mask = value_type{value_mask} << bg_shift;
  static constexpr auto fg_disc_mask =
                              value_type{disc_mask} << (fg_shift + color_bits);
  static constexpr auto bg_disc_mask =
                              value_type{disc_mask} << (bg_shift + color_bits);

  static constexpr auto emph_mask0 = (1U << emph_bits) - 1;
  static constexpr auto emph_mask  = value_type{emph_mask0} << emph_shift;

 public:
  /**
   * Constructs a style from emphasis and optional foreground and background
   * colors. Both colors default to unspecified.
   *
   * Default construction produces an empty style. Conversion from `emphasis`
   * is implicit.
   */
  constexpr text_style(emphasis emp=emphasis{},
                       detail::color_type fg = no_color,
                       detail::color_type bg = no_color) noexcept
    : text_style{fg, bg, emp} {}

  /**
   * Constructs a style from foreground and background colors and emphasis.
   *
   * Colors may be named colors, RGB values, terminal colors, or `no_color`.
   * The background and emphasis default to unspecified and empty, respectively.
   * The foreground is required; `no_color` specifies a background-only style.
   */
  constexpr text_style(detail::color_type fg, detail::color_type bg = no_color,
                       emphasis emp = emphasis{}) noexcept
    : style_{(value_type{to_underlying(fg)}  <<   fg_shift)
           | (value_type{to_underlying(bg)}  <<   bg_shift)
           | (value_type{to_underlying(emp)} << emph_shift)}
           {}

  /**
   * Combines this style with `rhs` and returns a reference to this style.
   *
   * Emphasis bits are combined by OR. An unspecified color channel takes the
   * other style's color. If both styles specify RGB colors in the same channel,
   * their corresponding component bits are ORed, without averaging or alpha
   * compositing. For example, RGB red OR RGB blue produces RGB magenta.
   *
   * Throws `format_error` if both styles specify a color in the same channel
   * and either color is a terminal color, even if the colors are identical.
   * On this error, this style is unchanged.
   */

  FMT_CONSTEXPR auto operator|=(text_style rhs) -> text_style& {
    constexpr auto overflow_mask = (value_type{1} << (fg_shift + value_bits))
                                 | (value_type{1} << (bg_shift + value_bits));
    if (((style_ + rhs.style_) & overflow_mask) != 0)
      report_error("can't OR a terminal color");
    style_ |= rhs.style_;
    return *this;
  }

  /**
   * Returns the composition of `lhs` and `rhs`.
   * Uses the same RGB-OR and terminal-color conflict rules as `operator|=`.
   * Throws `format_error` on a terminal-color conflict.
   */
  FMT_NODISCARD
  friend FMT_CONSTEXPR auto operator|(text_style lhs, text_style rhs)
    -> text_style
  { return lhs |= rhs; }

  /// Returns true if a foreground color is specified.
  FMT_NODISCARD constexpr auto has_fg() const noexcept -> bool
  { return ((style_ & fg_disc_mask) != 0); }

  /// Returns true if a background color is specified.
  FMT_NODISCARD constexpr auto has_bg() const noexcept -> bool
  { return ((style_ & bg_disc_mask) != 0); }

  /// Returns true if at least one emphasis attribute is set.
  FMT_NODISCARD constexpr auto has_emph() const noexcept -> bool
  { return ((style_ & emph_mask) != 0); }

  /// Returns true if a foreground color is specified.
  FMT_NODISCARD constexpr auto has_foreground() const noexcept -> bool
  { return has_fg(); }

  /// Returns true if a background color is specified.
  FMT_NODISCARD constexpr auto has_background() const noexcept -> bool
  { return has_bg(); }

  /// Returns true if at least one emphasis attribute is set.
  FMT_NODISCARD constexpr auto has_emphasis() const noexcept -> bool
  { return has_emph(); }

  /**
   * Returns the foreground color, preserving its RGB or terminal color kind.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD
  constexpr auto fg() const noexcept -> detail::color_type {
    using value_t = detail::color_type::value_type;
    return detail::color_type{
                      narrow_cast<value_t>((style_ >> fg_shift) & value_mask)};
  }

  /**
   * Returns the background color, preserving its RGB or terminal color kind.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD
  constexpr auto bg() const noexcept -> detail::color_type {
    using value_t = detail::color_type::value_type;
    return detail::color_type{
                      narrow_cast<value_t>((style_ >> bg_shift) & value_mask)};
  }

  /// Returns the combined emphasis bit mask.
  FMT_NODISCARD constexpr auto emph() const noexcept -> emphasis
  { return enum_cast<emphasis>((style_ & emph_mask) >> emph_shift); }

  /**
   * Returns the foreground color, preserving its RGB or terminal color kind.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD constexpr auto get_foreground() const noexcept
    -> detail::color_type
  { return fg(); }

  /**
   * Returns the background color, preserving its RGB or terminal color kind.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD constexpr auto get_background() const noexcept
    -> detail::color_type
  { return bg(); }

  /// Returns the combined emphasis bit mask, or `emphasis{}` if none is set.
  FMT_NODISCARD constexpr auto get_emphasis() const noexcept -> emphasis
  { return emph(); }

  /**
   * Replaces the foreground color and returns a reference to this style.
   * Accepts named RGB constants, RGB values, terminal colors, or `no_color`.
   * Passing `no_color` clears the foreground to unspecified; it does not store
   * a terminal reset. Preserves the background color and emphasis attributes.
   */
  FMT_CONSTEXPR auto fg(detail::color_type color) noexcept -> text_style& {
    style_ = (style_ & ~fg_mask)
           | (value_type{to_underlying(color)} << fg_shift);
    return *this;
  }

  /**
   * Replaces the background color and returns a reference to this style.
   * Accepts named RGB constants, RGB values, terminal colors, or `no_color`.
   * Passing `no_color` clears the background to unspecified; it does not store
   * a terminal reset. Preserves the foreground color and emphasis attributes.
   */
  FMT_CONSTEXPR auto bg(detail::color_type color) noexcept -> text_style& {
    style_ = (style_ & ~bg_mask)
           | (value_type{to_underlying(color)} << bg_shift);
    return *this;
  }

  /**
   * Replaces all emphasis attributes and returns a reference to this style.
   * Only the bits in `em` remain enabled; `emphasis{}` clears all attributes.
   * Preserves both colors. Bold and faint remain independent bits.
   */
  FMT_CONSTEXPR auto emph(emphasis em) noexcept -> text_style& {
    style_ = (style_ & ~emph_mask) | (enum_cast<value_type>(em) << emph_shift);
    return *this;
  }

private:

  FMT_CONSTEXPR auto set(emphasis em, bool enable=true) noexcept -> text_style&
  {
    auto mask = enum_cast<value_type>(em) << emph_shift;
    style_ = enable ? (style_ | mask) : (style_ & ~mask);
    return *this;
  }

public:

  /**
   * Enables bold if `enable` is true, or disables it otherwise.
   * Preserves faint, other attributes, and colors.
   */
  FMT_CONSTEXPR auto bold(bool enable = true) noexcept -> text_style&
  { return set(emphasis::bold, enable); }

  /**
   * Enables faint if `enable` is true, or disables it otherwise.
   * Preserves bold, other attributes, and colors.
   */
  FMT_CONSTEXPR auto faint(bool enable = true) noexcept -> text_style&
  { return set(emphasis::faint, enable); }

  /**
   * Enables italic if `enable` is true, or disables it otherwise.
   * Preserves all other attributes and colors.
   */
  FMT_CONSTEXPR auto italic(bool enable = true) noexcept -> text_style&
  { return set(emphasis::italic, enable); }

  /**
   * Enables underline if `enable` is true, or disables it otherwise.
   * Preserves other attributes and colors.
   */
  FMT_CONSTEXPR auto underline(bool enable = true) noexcept -> text_style&
  { return set(emphasis::underline, enable); }

  /**
   * Enables blink if `enable` is true, or disables it otherwise.
   * Preserves all other attributes and colors.
   */
  FMT_CONSTEXPR auto blink(bool enable = true) noexcept -> text_style&
  { return set(emphasis::blink, enable); }

  /**
   * Enables reverse if `enable` is true, or disables it otherwise.
   * Preserves all other attributes and colors.
   */
  FMT_CONSTEXPR auto reverse(bool enable = true) noexcept -> text_style&
  { return set(emphasis::reverse, enable); }

  /**
   * Enables conceal if `enable` is true, or disables it otherwise.
   * Preserves all other attributes and colors.
   */
  FMT_CONSTEXPR auto conceal(bool enable = true) noexcept -> text_style&
  { return set(emphasis::conceal, enable); }

  /**
   * Enables strikethrough if `enable` is true, or disables it otherwise.
   * Preserves other attributes and colors.
   */
  FMT_CONSTEXPR auto strike(bool enable = true) noexcept -> text_style&
  { return set(emphasis::strike, enable); }

private:

  FMT_NODISCARD constexpr auto is_enabled(emphasis em) const noexcept -> bool
  { return ((style_ & (enum_cast<value_type>(em) << emph_shift)) != 0); }

public:

  /// Returns true if bold is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_bold() const noexcept -> bool
  { return is_enabled(emphasis::bold); }

  /// Returns true if faint is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_faint() const noexcept -> bool
  { return is_enabled(emphasis::faint); }

  /// Returns true if italic is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_italic() const noexcept -> bool
  { return is_enabled(emphasis::italic); }

  /// Returns true if underline is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_underline() const noexcept -> bool
  { return is_enabled(emphasis::underline); }

  /// Returns true if blink is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_blink() const noexcept -> bool
  { return is_enabled(emphasis::blink); }

  /// Returns true if reverse is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_reverse() const noexcept -> bool
  { return is_enabled(emphasis::reverse); }

  /// Returns true if conceal is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_conceal() const noexcept -> bool
  { return is_enabled(emphasis::conceal); }

  /// Returns true if strikethrough is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_strike() const noexcept -> bool
  { return is_enabled(emphasis::strike); }

  /// Alias for `strike()`, retained for backward compatibility.
  FMT_CONSTEXPR auto strikethrough(bool enable = true) noexcept -> text_style&
  { return strike(enable); }

  /// Alias for `is_strike()`, retained for backward compatibility.
  FMT_NODISCARD constexpr auto is_strikethrough() const noexcept -> bool
  { return is_strike(); }

  FMT_COLOR_COMPARISONS(text_style)

 private:
  friend constexpr auto to_underlying(text_style value) noexcept -> value_type;

  constexpr explicit text_style(value_type style) noexcept : style_{style} {}

  friend FMT_CONSTEXPR auto fg(detail::color_type foreground) noexcept
    -> text_style;

  friend FMT_CONSTEXPR auto bg(detail::color_type background) noexcept
    -> text_style;
}; // text_style

FMT_NODISCARD constexpr inline auto to_underlying(text_style value) noexcept
  -> text_style::value_type
{ return value.style_; }

/**
 * Creates a style with the specified foreground (text) color, no background
 * color, and no emphasis. Accepts `rgb`, `term_color`, or `no_color_t`
 * values through implicit conversion to the parameter type.
 */
FMT_NODISCARD
FMT_CONSTEXPR inline auto fg(detail::color_type foreground) noexcept
  -> text_style
{
  return text_style{text_style::value_type{to_underlying(foreground)}
                    << text_style::fg_shift};
}

/**
 * Creates a style with the specified background (text) color, no foreground
 * color, and no emphasis. Accepts `rgb`, `term_color`, or `no_color_t`
 * values through implicit conversion to the parameter type.
 */
FMT_NODISCARD
FMT_CONSTEXPR inline auto bg(detail::color_type background) noexcept
  -> text_style
{
  return text_style{text_style::value_type{to_underlying(background)}
                    << text_style::bg_shift};
}

namespace detail {

template<typename Char>
class ansi_color_escape {
  // Maximum color SGR is "\x1b[38;2;255;255;255m"
  // Masimum emph  SGR is "\x1b[1;2;3;4;5;7;8;9m
  Char buffer[20] = {};
  size_t size = 0;

  FMT_CONSTEXPR void append(char c) noexcept
  { buffer[size++] = narrow_cast<Char>(c); }

  FMT_CONSTEXPR void append(const char* s) noexcept {
    while (*s)
      append(*s++);
  }

  FMT_CONSTEXPR void append(const char* s, char c) noexcept {
    append(s);
    append(c);
  }

  FMT_CONSTEXPR void append(uint8_t n, char c) noexcept {
    append(narrow_cast<char>('0' + n / 100));
    append(narrow_cast<char>('0' + n / 10 % 10));
    append(narrow_cast<char>('0' + n % 10));
    append(c);
  }

public:
  FMT_CONSTEXPR ansi_color_escape(color_type text_color, bool is_bg) noexcept {
    if (holds_alternative<no_color_t>(text_color))
      return;

    // If we have a terminal color, we need to output another escape code
    // sequence.
    if (holds_alternative<term_color>(text_color)) {
      constexpr unsigned BrightBlack =
                                    to_underlying(term_color::bright_black);
      unsigned value = to_underlying(get<term_color>(text_color));
      value += (value < BrightBlack) ? 30u : 90u - BrightBlack;

      // Background ASCII codes are the same as the foreground ones but with
      // 10 more.
      if (is_bg) value += 10u;
      append("\x1b[");
      if (value >= 100u) {
        append('1');
        value %= 100u;
      }
      append(narrow_cast<char>('0' + value / 10u));
      append(narrow_cast<char>('0' + value % 10u));
      append('m');
      return;
    }

    append("\x1b[38;2;");
    if (is_bg) buffer[2] = Char{'4'};

    auto color = get<rgb>(text_color);
    append(color.r, ';');
    append(color.g, ';');
    append(color.b, 'm');
  }

  FMT_CONSTEXPR ansi_color_escape(emphasis em) noexcept {
    unsigned bits = to_underlying(em);
    if (bits == 0)
      return;

    constexpr char sgr[] = { '1', '2', '3', '4', '5', '7', '8', '9' };

    append("\x1b[");

    for (int i = 0; i < 8; ++i, bits >>= 1) {
      if (!(bits & 1))
        continue;
      append(sgr[i]);
      append(';');
    }

    buffer[size - 1] = Char{'m'};
  }

  constexpr operator const Char*() const noexcept { return buffer; }

  FMT_NODISCARD constexpr auto begin() const noexcept -> const Char*
  { return buffer; }

  FMT_NODISCARD constexpr auto end() const noexcept -> const Char*
  { return buffer + size; }
}; // ansi_color_escape

template<typename Char>
FMT_CONSTEXPR auto make_foreground_color(color_type foreground) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(foreground, false); }

template<typename Char>
FMT_CONSTEXPR auto make_background_color(color_type background) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(background, true); }

template<typename Char>
FMT_CONSTEXPR auto make_emphasis(emphasis em) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(em); }

template<typename Char>
inline void reset_color(buffer<Char>& buffer) {
  auto reset_color = string_view{"\x1b[0m"};
  buffer.append(reset_color.begin(), reset_color.end());
}

template<typename T>
struct styled_arg : view {
  const T& value;
  text_style style;
  constexpr styled_arg(const T& v, text_style s) : value{v}, style{s} {}
}; // styled_arg

template<typename Char>
void vformat_to(buffer<Char>& buf, text_style ts, basic_string_view<Char> fmt,
                basic_format_args<buffered_context<Char>> args) {
  auto emphasis = make_emphasis<Char>(ts.get_emphasis());
  buf.append(emphasis.begin(), emphasis.end());
  auto foreground = make_foreground_color<Char>(ts.get_foreground());
  buf.append(foreground.begin(), foreground.end());
  auto background = make_background_color<Char>(ts.get_background());
  buf.append(background.begin(), background.end());
  vformat_to(buf, fmt, args);
  if (ts != text_style()) reset_color<Char>(buf);
} // vformat_to

}  // detail

/**
 * The colored analog of
 * [std::vprint_nonunicode()](https://en.cppreference.com/w/cpp/io/vprint_nonunicode.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline void vprint(FILE* f, text_style ts, string_view fmt, format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
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
 *     fmt::print(stderr, fmt::emphasis::bold | fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(FILE* f, text_style ts, format_string<T...> fmt, T&&... args)
{ vprint(f, ts, fmt.str, vargs<T...>{{args...}}); }

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
 *     fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(text_style ts, format_string<T...> fmt, T&&... args)
{ return print(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * Behaves like `vprint(f, ts, fmt, args)`, with an appended newline.
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 */
inline void vprintln(FILE* f, text_style ts, string_view fmt, format_args args)
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  buf.push_back('\n');
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
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
 *     fmt::println(stderr, fmt::emphasis::bold | fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(FILE* f, text_style ts, format_string<T...> fmt, T&&... args)
{ vprintln(f, ts, fmt.str, vargs<T...>{{args...}}); }

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
 *     fmt::println(fmt::emphasis::bold | fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(text_style ts, format_string<T...> fmt, T&&... args)
{ return println(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * The colored analog of
 * [std::vformat()](https://en.cppreference.com/w/cpp/utility/format/vformat.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline auto vformat(text_style ts, string_view fmt, format_args args)
  -> std::string
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  return fmt::to_string(buf);
}

/**
 * The colored analog of
 * [std::format()](https://en.cppreference.com/w/cpp/utility/format/format.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 * Reset does not restore a previously active style.
 *
 * **Example**:
 *
 * ```
 * #include <fmt/color.h>
 * std::string message = fmt::format(fmt::emphasis::bold | fg(fmt::color::red),
 *                                   "The answer is {}", 42);
 * ```
 */
template<typename... T>
inline auto format(text_style ts, format_string<T...> fmt, T&&... args)
  -> std::string
{ return fmt::vformat(ts, fmt.str, vargs<T...>{{args...}}); }

/**
 * The colored analog of
 * [std::vformat_to()](https://en.cppreference.com/w/cpp/utility/format/vformat_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
template<typename OutputIt,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
auto vformat_to(OutputIt out, text_style ts, string_view fmt, format_args args)
  -> OutputIt
{
  auto&& buf = detail::get_buffer<char>(out);
  detail::vformat_to(buf, ts, fmt, args);
  return detail::get_iterator(buf, out);
}

/**
 * The colored analog of
 * [std::format_to()](https://en.cppreference.com/w/cpp/utility/format/format_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     std::vector<char> out;
 *     fmt::format_to(std::back_inserter(out),
 *                    fmt::emphasis::bold | fg(fmt::color::red), "{}", 42);
 */
template<typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
inline auto format_to(OutputIt out, text_style ts, format_string<T...> fmt,
                      T&&... args) -> OutputIt
{ return vformat_to(out, ts, fmt.str, vargs<T...>{{args...}}); }

/// Formats a styled argument using the underlying type's format specifications.
template<typename T, typename Char>
struct formatter<detail::styled_arg<T>, Char> : formatter<T, Char> {
  template<typename FormatContext>
  FMT_CONSTEXPR auto format(const detail::styled_arg<T>& arg,
                            FormatContext& ctx) const -> decltype(ctx.out()) {
    const auto& ts = arg.style;
    auto out = ctx.out();

    bool has_style = false;
    if (ts.has_emphasis()) {
      has_style = true;
      auto emphasis = detail::make_emphasis<Char>(ts.get_emphasis());
      out = detail::copy<Char>(emphasis.begin(), emphasis.end(), out);
    }
    if (ts.has_foreground()) {
      has_style = true;
      auto foreground =
          detail::make_foreground_color<Char>(ts.get_foreground());
      out = detail::copy<Char>(foreground.begin(), foreground.end(), out);
    }
    if (ts.has_background()) {
      has_style = true;
      auto background =
          detail::make_background_color<Char>(ts.get_background());
      out = detail::copy<Char>(background.begin(), background.end(), out);
    }
    out = formatter<T, Char>::format(arg.value, ctx);
    if (has_style) {
      auto reset_color = string_view{"\x1b[0m"};
      out = detail::copy<Char>(reset_color.begin(), reset_color.end(), out);
    }
    return out;
  }
}; // formatter

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
 * If `ts` is not empty, the formatted value is preceded by SGR sequences for
 * its specified foreground, background, and emphasis, and followed by an SGR
 * reset to terminal defaults. Reset does not restore an enclosing style, so
 * subsequent output does not automatically resume a style set by an outer
 * formatting call.  This function does not save and restore nested terminal
 * styles.
 *
 * **Example**:
 *
 *     fmt::print("Elapsed time: {0:.2f} seconds",
 *                fmt::styled(1.23, fmt::fg(fmt::color::green)
 *                                | fmt::bg(fmt::color::blue)));
 */
template<typename T>
constexpr auto styled(const T& value, text_style ts)
    -> detail::styled_arg<remove_cvref_t<T>>
{ return detail::styled_arg<remove_cvref_t<T>>{value, ts}; }

/**
 * Named RGB colors.
 *
 * This namespace contains constexpr `rgb` constants with fixed RGB values,
 * independently of the terminal palette. Pass a value to `fg()` or `bg()`.
 */
namespace color {
constexpr FMT_INLINE_VARIABLE rgb alice_blue              {240, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb antique_white           {250, 235, 215};
constexpr FMT_INLINE_VARIABLE rgb aqua                    {  0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb aquamarine              {127, 255, 212};
constexpr FMT_INLINE_VARIABLE rgb azure                   {240, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb beige                   {245, 245, 220};
constexpr FMT_INLINE_VARIABLE rgb bisque                  {255, 228, 196};
constexpr FMT_INLINE_VARIABLE rgb black                   {  0,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb blanched_almond         {255, 235, 205};
constexpr FMT_INLINE_VARIABLE rgb blue                    {  0,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb blue_violet             {138,  43, 226};
constexpr FMT_INLINE_VARIABLE rgb brown                   {165,  42,  42};
constexpr FMT_INLINE_VARIABLE rgb burly_wood              {222, 184, 135};
constexpr FMT_INLINE_VARIABLE rgb cadet_blue              { 95, 158, 160};
constexpr FMT_INLINE_VARIABLE rgb chartreuse              {127, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb chocolate               {210, 105,  30};
constexpr FMT_INLINE_VARIABLE rgb coral                   {255, 127,  80};
constexpr FMT_INLINE_VARIABLE rgb cornflower_blue         {100, 149, 237};
constexpr FMT_INLINE_VARIABLE rgb cornsilk                {255, 248, 220};
constexpr FMT_INLINE_VARIABLE rgb crimson                 {220,  20,  60};
constexpr FMT_INLINE_VARIABLE rgb cyan                    {  0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb dark_blue               {  0,   0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_cyan               {  0, 139, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_golden_rod         {184, 134,  11};
constexpr FMT_INLINE_VARIABLE rgb dark_gray               {169, 169, 169};
constexpr FMT_INLINE_VARIABLE rgb dark_green              {  0, 100,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_khaki              {189, 183, 107};
constexpr FMT_INLINE_VARIABLE rgb dark_magenta            {139,   0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_olive_green        { 85, 107,  47};
constexpr FMT_INLINE_VARIABLE rgb dark_orange             {255, 140,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_orchid             {153,  50, 204};
constexpr FMT_INLINE_VARIABLE rgb dark_red                {139,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_salmon             {233, 150, 122};
constexpr FMT_INLINE_VARIABLE rgb dark_sea_green          {143, 188, 143};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_blue         { 72,  61, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_gray         { 47,  79,  79};
constexpr FMT_INLINE_VARIABLE rgb dark_turquoise          {  0, 206, 209};
constexpr FMT_INLINE_VARIABLE rgb dark_violet             {148,   0, 211};
constexpr FMT_INLINE_VARIABLE rgb deep_pink               {255,  20, 147};
constexpr FMT_INLINE_VARIABLE rgb deep_sky_blue           {  0, 191, 255};
constexpr FMT_INLINE_VARIABLE rgb dim_gray                {105, 105, 105};
constexpr FMT_INLINE_VARIABLE rgb dodger_blue             { 30, 144, 255};
constexpr FMT_INLINE_VARIABLE rgb fire_brick              {178,  34 , 34};
constexpr FMT_INLINE_VARIABLE rgb floral_white            {255, 250, 240};
constexpr FMT_INLINE_VARIABLE rgb forest_green            {34, 139,   34};
constexpr FMT_INLINE_VARIABLE rgb fuchsia                 {255,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb gainsboro               {220, 220, 220};
constexpr FMT_INLINE_VARIABLE rgb ghost_white             {248, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb gold                    {255, 215,   0};
constexpr FMT_INLINE_VARIABLE rgb golden_rod              {218, 165,  32};
constexpr FMT_INLINE_VARIABLE rgb gray                    {128, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb green                   {  0, 128  , 0};
constexpr FMT_INLINE_VARIABLE rgb green_yellow            {173, 255,  47};
constexpr FMT_INLINE_VARIABLE rgb honey_dew               {240, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb hot_pink                {255, 105, 180};
constexpr FMT_INLINE_VARIABLE rgb indian_red              {205,  92,  92};
constexpr FMT_INLINE_VARIABLE rgb indigo                  {75,    0, 130};
constexpr FMT_INLINE_VARIABLE rgb ivory                   {255, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb khaki                   {240, 230, 140};
constexpr FMT_INLINE_VARIABLE rgb lavender                {230, 230, 250};
constexpr FMT_INLINE_VARIABLE rgb lavender_blush          {255, 240, 245};
constexpr FMT_INLINE_VARIABLE rgb lawn_green              {124, 252,   0};
constexpr FMT_INLINE_VARIABLE rgb lemon_chiffon           {255, 250, 205};
constexpr FMT_INLINE_VARIABLE rgb light_blue              {173, 216, 230};
constexpr FMT_INLINE_VARIABLE rgb light_coral             {240, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb light_cyan              {224, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb light_golden_rod_yellow {250, 250, 210};
constexpr FMT_INLINE_VARIABLE rgb light_gray              {211, 211, 211};
constexpr FMT_INLINE_VARIABLE rgb light_green             {144, 238, 144};
constexpr FMT_INLINE_VARIABLE rgb light_pink              {255, 182, 193};
constexpr FMT_INLINE_VARIABLE rgb light_salmon            {255, 160, 122};
constexpr FMT_INLINE_VARIABLE rgb light_sea_green         { 32, 178, 170};
constexpr FMT_INLINE_VARIABLE rgb light_sky_blue          {135, 206, 250};
constexpr FMT_INLINE_VARIABLE rgb light_slate_gray        {119, 136, 153};
constexpr FMT_INLINE_VARIABLE rgb light_steel_blue        {176, 196, 222};
constexpr FMT_INLINE_VARIABLE rgb light_yellow            {255, 255, 224};
constexpr FMT_INLINE_VARIABLE rgb lime                    {  0, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb lime_green              { 50, 205,  50};
constexpr FMT_INLINE_VARIABLE rgb linen                   {250, 240, 230};
constexpr FMT_INLINE_VARIABLE rgb magenta                 {255,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb maroon                  {128,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb medium_aquamarine       {102, 205, 170};
constexpr FMT_INLINE_VARIABLE rgb medium_blue             {  0,   0, 205};
constexpr FMT_INLINE_VARIABLE rgb medium_orchid           {186,  85, 211};
constexpr FMT_INLINE_VARIABLE rgb medium_purple           {147, 112, 219};
constexpr FMT_INLINE_VARIABLE rgb medium_sea_green        { 60, 179, 113};
constexpr FMT_INLINE_VARIABLE rgb medium_slate_blue       {123, 104, 238};
constexpr FMT_INLINE_VARIABLE rgb medium_spring_green     {  0, 250, 154};
constexpr FMT_INLINE_VARIABLE rgb medium_turquoise        { 72, 209, 204};
constexpr FMT_INLINE_VARIABLE rgb medium_violet_red       {199,  21, 133};
constexpr FMT_INLINE_VARIABLE rgb midnight_blue           { 25,  25, 112};
constexpr FMT_INLINE_VARIABLE rgb mint_cream              {245, 255, 250};
constexpr FMT_INLINE_VARIABLE rgb misty_rose              {255, 228, 225};
constexpr FMT_INLINE_VARIABLE rgb moccasin                {255, 228, 181};
constexpr FMT_INLINE_VARIABLE rgb navajo_white            {255, 222, 173};
constexpr FMT_INLINE_VARIABLE rgb navy                    {  0,   0, 128};
constexpr FMT_INLINE_VARIABLE rgb old_lace                {253, 245, 230};
constexpr FMT_INLINE_VARIABLE rgb olive                   {128, 128  , 0};
constexpr FMT_INLINE_VARIABLE rgb olive_drab              {107, 142,  35};
constexpr FMT_INLINE_VARIABLE rgb orange                  {255, 165,   0};
constexpr FMT_INLINE_VARIABLE rgb orange_red              {255,  69,   0};
constexpr FMT_INLINE_VARIABLE rgb orchid                  {218, 112, 214};
constexpr FMT_INLINE_VARIABLE rgb pale_golden_rod         {238, 232, 170};
constexpr FMT_INLINE_VARIABLE rgb pale_green              {152, 251, 152};
constexpr FMT_INLINE_VARIABLE rgb pale_turquoise          {175, 238, 238};
constexpr FMT_INLINE_VARIABLE rgb pale_violet_red         {219, 112, 147};
constexpr FMT_INLINE_VARIABLE rgb papaya_whip             {255, 239, 213};
constexpr FMT_INLINE_VARIABLE rgb peach_puff              {255, 218, 185};
constexpr FMT_INLINE_VARIABLE rgb peru                    {205, 133,  63};
constexpr FMT_INLINE_VARIABLE rgb pink                    {255, 192, 203};
constexpr FMT_INLINE_VARIABLE rgb plum                    {221, 160, 221};
constexpr FMT_INLINE_VARIABLE rgb powder_blue             {176, 224, 230};
constexpr FMT_INLINE_VARIABLE rgb purple                  {128,   0, 128};
constexpr FMT_INLINE_VARIABLE rgb rebecca_purple          {102,  51, 153};
constexpr FMT_INLINE_VARIABLE rgb red                     {255,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb rosy_brown              {188, 143, 143};
constexpr FMT_INLINE_VARIABLE rgb royal_blue              { 65, 105, 225};
constexpr FMT_INLINE_VARIABLE rgb saddle_brown            {139,  69,  19};
constexpr FMT_INLINE_VARIABLE rgb salmon                  {250, 128, 114};
constexpr FMT_INLINE_VARIABLE rgb sandy_brown             {244, 164,  96};
constexpr FMT_INLINE_VARIABLE rgb sea_green               { 46, 139,  87};
constexpr FMT_INLINE_VARIABLE rgb sea_shell               {255, 245, 238};
constexpr FMT_INLINE_VARIABLE rgb sienna                  {160,  82,  45};
constexpr FMT_INLINE_VARIABLE rgb silver                  {192, 192, 192};
constexpr FMT_INLINE_VARIABLE rgb sky_blue                {135, 206, 235};
constexpr FMT_INLINE_VARIABLE rgb slate_blue              {106,  90, 205};
constexpr FMT_INLINE_VARIABLE rgb slate_gray              {112, 128, 144};
constexpr FMT_INLINE_VARIABLE rgb snow                    {255, 250, 250};
constexpr FMT_INLINE_VARIABLE rgb spring_green            {  0, 255, 127};
constexpr FMT_INLINE_VARIABLE rgb steel_blue              { 70, 130, 180};
constexpr FMT_INLINE_VARIABLE rgb tan                     {210, 180, 140};
constexpr FMT_INLINE_VARIABLE rgb teal                    {  0, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb thistle                 {216, 191, 216};
constexpr FMT_INLINE_VARIABLE rgb tomato                  {255,  99,  71};
constexpr FMT_INLINE_VARIABLE rgb turquoise               { 64, 224, 208};
constexpr FMT_INLINE_VARIABLE rgb violet                  {238, 130, 238};
constexpr FMT_INLINE_VARIABLE rgb wheat                   {245, 222, 179};
constexpr FMT_INLINE_VARIABLE rgb white                   {255, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb white_smoke             {245, 245, 245};
constexpr FMT_INLINE_VARIABLE rgb yellow                  {255, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb yellow_green            {154, 205,  50};
} // color

FMT_END_EXPORT
FMT_END_NAMESPACE

FMT_BEGIN_EXPORT
namespace std {

template<> struct hash<fmt::rgb> {
  auto operator()(fmt::rgb value) const noexcept -> size_t {
    return hash<fmt::rgb::value_type>{}(fmt::to_underlying(value));
  }
};

template<> struct hash<fmt::detail::color_type> {
  auto operator()(fmt::detail::color_type value) const noexcept -> size_t {
    return hash<fmt::detail::color_type::value_type>{}(fmt::to_underlying(value));
  }
};

template<> struct hash<fmt::text_style> {
  auto operator()(fmt::text_style value) const noexcept -> size_t {
    return hash<fmt::text_style::value_type>{}(fmt::to_underlying(value));
  }
};

} // std
FMT_END_EXPORT

#undef FMT_COLOR_COMPARISONS

#endif  // FMT_COLOR_H_
