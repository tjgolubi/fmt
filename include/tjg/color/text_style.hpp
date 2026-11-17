/// @file
/// Defines `text_style`.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/color/colors.hpp>
#include <tjg/color/bits/emphasis.hpp>
#include <tjg/color/bits/error_support.hpp>
#include <tjg/hash_support.hpp>
#include <tjg/utility.hpp>

#include <functional>

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

#include <cstddef>

namespace tjg::color {

/**
 * A text style consisting of optional foreground and background colors and a
 * set of emphasis attributes.
 *
 * Default construction produces an empty style. Use `fg()`, `bg()`, and
 * emphasis values to construct styles, and `blend()` to combine them.
 * Blending combines emphasis bits by OR and averages the specified colors
 * independently for the foreground and background, ignoring unspecified colors.
 *
 * **Example:
 *
 *     {
 *       using namespace tjg::color;
 *       using enum emphasis;
 *       using enum term_color;
 *
 *       constexpr auto style1 = text_style{rgb::red, black, bold | underline};
 *
 *       constexpr auto style2 = fg(rgb::red).bg(black).bold().underline();
 *
 *       constexpr auto style3 = text_style{bold | underline, rgb::red, black};
 *
 *       static_assert(style1 == style2 && style1 == style3);
 *
 *       constexpr auto style4 = text_style{style3}.underline(false);
 *     }
 *
 * The final line makes a copy of style3, but with underline disabled.
 */
class text_style {
  bool _absolute = false;
  emphasis _emph{};
  color_t  _fg{};
  color_t  _bg{};

 public:
  /**
   * Constructs a style from emphasis and optional foreground and background
   * colors. Both colors default to unspecified.
   *
   * Default construction produces an empty style.
   */
  constexpr text_style(emphasis emp = emphasis::none, color_t fg = {},
                       color_t bg = {}) noexcept
    : text_style{fg, bg, emp} {}

  /**
   * Constructs a style from foreground and background colors and emphasis.
   *
   * Colors may be named RGB constants, RGB, terminal, cube, or gray values,
   * or `no_color`.
   * The background and emphasis default to unspecified and empty, respectively.
   * The foreground is required; `no_color` specifies a background-only style.
   */
  constexpr text_style(color_t fg, color_t bg = {},
                       emphasis emp = emphasis::none) noexcept
    : _emph{emp}, _fg{fg}, _bg{bg} {}

  /// Returns true if no colors, emphasis, or absolute flag are set.
  [[nodiscard]] constexpr auto empty() const noexcept -> bool
  { return !_absolute && !has_emph() && !has_fg() && !has_bg(); }

  /**
   * Clears both colors, all emphasis, and absolute, restoring an empty style.
   * Returns a reference to this style for chaining.
   */
  constexpr auto clear() noexcept -> text_style&
  { return *this = text_style{}; }

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
  [[nodiscard]] constexpr auto is_absolute() const noexcept -> bool
  { return _absolute; }

  /**
   * Applies `other` over this style.
   *
   * Specified foreground and background colors in `other` replace the matching
   * channels, preserving their color kinds. Unspecified channels leave this
   * style's colors unchanged. Emphasis bits are combined by OR. The result is
   * absolute if either input is absolute.
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
   * OR. The result is absolute if either input is absolute.
   */
  constexpr auto underlay(const text_style& other) noexcept -> text_style& {
    _absolute = _absolute || other._absolute;
    _emph |= other._emph;
    if (!has_fg()) _fg = other._fg;
    if (!has_bg()) _bg = other._bg;
    return *this;
  }  // underlay

  /// Returns true if a foreground color is specified.
  [[nodiscard]] constexpr auto has_fg() const noexcept -> bool
  { return !detail::is_unspecified(_fg); }

  /// Returns true if a background color is specified.
  [[nodiscard]] constexpr auto has_bg() const noexcept -> bool
  { return !detail::is_unspecified(_bg); }

  /// Returns true if at least one emphasis attribute is set.
  [[nodiscard]] constexpr auto has_emph() const noexcept -> bool
  { return _emph != emphasis::none; }

  /**
   * Returns the foreground color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  [[nodiscard]] constexpr auto fg() const noexcept -> color_t { return _fg; }

  /**
   * Returns the background color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  [[nodiscard]] constexpr auto bg() const noexcept -> color_t { return _bg; }

  /// Returns the combined emphasis bit mask.
  [[nodiscard]]
  constexpr auto emph() const noexcept -> emphasis { return _emph; }

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
    _emph = enum_cast<emphasis>(enable ? (bits | mask) : (bits & ~mask));
    return *this;
  }

 public:
  /// Enables bold if `enable` is true, or disables it otherwise.
  constexpr auto bold(bool enable = true) noexcept -> text_style&
  { return set(emphasis::bold, enable); }

  /// Enables faint if `enable` is true, or disables it otherwise.
  constexpr auto faint(bool enable = true) noexcept -> text_style&
  { return set(emphasis::faint, enable); }

  /// Enables italic if `enable` is true, or disables it otherwise.
  constexpr auto italic(bool enable = true) noexcept -> text_style&
  { return set(emphasis::italic, enable); }

  /// Enables underline if `enable` is true, or disables it otherwise.
  constexpr auto underline(bool enable = true) noexcept -> text_style&
  { return set(emphasis::underline, enable); }

  /// Enables blink if `enable` is true, or disables it otherwise.
  constexpr auto blink(bool enable = true) noexcept -> text_style&
  { return set(emphasis::blink, enable); }

  /// Enables reverse if `enable` is true, or disables it otherwise.
  constexpr auto reverse(bool enable = true) noexcept -> text_style&
  { return set(emphasis::reverse, enable); }

  /// Enables conceal if `enable` is true, or disables it otherwise.
  constexpr auto conceal(bool enable = true) noexcept -> text_style&
  { return set(emphasis::conceal, enable); }

  /// Enables strikethrough if `enable` is true, or disables it otherwise.
  constexpr auto strike(bool enable = true) noexcept -> text_style&
  { return set(emphasis::strike, enable); }

 private:
  [[nodiscard]] constexpr auto is_enabled(emphasis em) const noexcept -> bool
  { return ((std::to_underlying(_emph) & std::to_underlying(em)) != 0); }

 public:
  /// Returns true if bold is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_bold() const noexcept -> bool
  { return is_enabled(emphasis::bold); }

  /// Returns true if faint is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_faint() const noexcept -> bool
  { return is_enabled(emphasis::faint); }

  /// Returns true if italic is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_italic() const noexcept -> bool
  { return is_enabled(emphasis::italic); }

  /// Returns true if underline is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_underline() const noexcept -> bool
  { return is_enabled(emphasis::underline); }

  /// Returns true if blink is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_blink() const noexcept -> bool
  { return is_enabled(emphasis::blink); }

  /// Returns true if reverse is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_reverse() const noexcept -> bool
  { return is_enabled(emphasis::reverse); }

  /// Returns true if conceal is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_conceal() const noexcept -> bool
  { return is_enabled(emphasis::conceal); }

  /// Returns true if strikethrough is enabled, or false if it is unset.
  [[nodiscard]] constexpr auto is_strike() const noexcept -> bool
  { return is_enabled(emphasis::strike); }

  [[nodiscard]] constexpr auto operator==(const text_style&) const noexcept
    -> bool = default;

  [[nodiscard]] constexpr auto operator<=>(const text_style&) const noexcept
    -> std::strong_ordering = default;
}; // text_style

/**
 * Creates a style with the specified foreground (text) color, no background
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
[[nodiscard]]
constexpr inline auto fg(color_t foreground) noexcept -> text_style
{ return text_style{foreground}; }

/**
 * Creates a style with the specified background (text) color, no foreground
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
[[nodiscard]]
constexpr inline auto bg(color_t background) noexcept -> text_style
{ return text_style{no_color, background}; }

/// Returns a copy of `style` marked absolute.
[[nodiscard]] constexpr inline
auto absolute(text_style style) noexcept -> text_style
{ return style.absolute(); }

/**
 * Returns a style with only bold enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto bold(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::bold : emphasis{}}; }

/**
 * Returns a style with only faint enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto faint(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::faint : emphasis{}}; }

/**
 * Returns a style with only italic enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto italic(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::italic : emphasis{}}; }

/**
 * Returns a style with only underline enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto underline(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::underline : emphasis{}}; }

/**
 * Returns a style with only blink enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto blink(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::blink : emphasis{}}; }

/**
 * Returns a style with only reverse enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto reverse(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::reverse : emphasis{}}; }

/**
 * Returns a style with only conceal enabled when `enable` is true (the
 * default), or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto conceal(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::conceal : emphasis{}}; }

/**
 * Returns a style with only strike enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
[[nodiscard]] constexpr inline
auto strike(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::strike : emphasis{}}; }

/// Returns a copy of `style` with bold enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto bold(text_style style, bool enable = true) noexcept -> text_style
{ return style.bold(enable); }

/// Returns a copy of `style` with faint enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto faint(text_style style, bool enable = true) noexcept -> text_style
{ return style.faint(enable); }

/// Returns a copy of `style` with italic enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto italic(text_style style, bool enable = true) noexcept -> text_style
{ return style.italic(enable); }

/// Returns a copy of `style` with underline enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto underline(text_style style, bool enable = true) noexcept -> text_style
{ return style.underline(enable); }

/// Returns a copy of `style` with blink enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto blink(text_style style, bool enable = true) noexcept -> text_style
{ return style.blink(enable); }

/// Returns a copy of `style` with reverse enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto reverse(text_style style, bool enable = true) noexcept -> text_style
{ return style.reverse(enable); }

/// Returns a copy of `style` with conceal enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto conceal(text_style style, bool enable = true) noexcept -> text_style
{ return style.conceal(enable); }

/// Returns a copy of `style` with strike enabled or disabled by `enable`.
[[nodiscard]] constexpr inline
auto strike(text_style style, bool enable = true) noexcept -> text_style
{ return style.strike(enable); }

/**
 * Returns a copy of `base` with `overrides` applied, leaving both inputs
 * unchanged. Specified colors in `overrides` replace matching channels and
 * preserve their kinds; unspecified channels retain the base colors.
 * Emphasis bits are combined by OR. The result is absolute if either input is.
 */
[[nodiscard]] constexpr inline
auto overlay(text_style base, text_style overrides) noexcept -> text_style
{ return base.overlay(overrides); }

/**
 * Returns a copy of `base` with `defaults` applied, leaving both inputs
 * unchanged. Specified colors in `defaults` replace matching unspecified
 * channels and preserve their kinds. Other channels retain the base colors.
 * Emphasis bits are combined by OR. The result is absolute if either input is.
 */
[[nodiscard]] constexpr inline
auto underlay(text_style base, text_style defaults) noexcept -> text_style
{ return base.underlay(defaults); }

namespace detail {

template<typename T>
concept TextStyle = std::same_as<std::remove_cvref_t<T>, text_style>;

constexpr auto checked_blend_style(const text_style& style) noexcept
  -> const text_style&
{
  TJG_ASSERT(!style.is_absolute() || (style.has_fg() && style.has_bg()),
             "cannot blend an absolute style with terminal-default colors");
  return style;
}

struct style_blend_state {
  blend_state foreground{};
  blend_state background{};
  emphasis attributes{};

  constexpr style_blend_state() = default;

  constexpr auto add_checked(const text_style& style) noexcept
    -> style_blend_state&
  {
    foreground.add(style.fg());
    background.add(style.bg());
    attributes |= style.emph();
    return *this;
  }

  constexpr auto add(const text_style& style) noexcept -> style_blend_state&
  { return add_checked(checked_blend_style(style)); }

  constexpr auto result() const noexcept -> text_style
  { return text_style{foreground.result(), background.result(), attributes}; }
}; // style_blend_state

} // detail

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
template<detail::TextStyle First, detail::TextStyle Second,
         detail::TextStyle... Styles>
[[nodiscard]] constexpr
auto blend(First first, Second second, Styles... rest) noexcept -> text_style {
  auto state = detail::style_blend_state{}.add(first).add(second);
  ((state.add(rest)), ...);
  return state.result();
}

} // tjg::color

namespace std {

template<> struct hash<tjg::color::text_style> {
  auto operator()(tjg::color::text_style value) const noexcept -> std::size_t {
    return tjg::hash_values(
                    value.is_absolute(), value.emph(), value.fg(), value.bg());
  }
};

} // std
