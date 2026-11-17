// Formatting library for C++ - color support
// Copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/utility.hpp>
#include <tjg/color/bits/error_support.hpp>
#include <tjg/hash_support.hpp>

#include <algorithm>

#include <variant>
#include <array>
#include <string_view>
#include <iterator>

#include <compare>
#include <concepts>
#include <type_traits>
#include <utility>

#include <cstdint>

namespace tjg::color {

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
}; // no_color_t

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
enum class term_color : std::int8_t {
  black, red, green, yellow, blue, magenta, cyan, white,
  bright_black, bright_red    , bright_green, bright_yellow,
  bright_blue , bright_magenta, bright_cyan , bright_white
}; // term_color

[[nodiscard]] inline constexpr
auto name_of(term_color value) noexcept -> std::string_view {
  using namespace std::string_view_literals;
  static constexpr auto names = std::array{
    "black"sv,        "red"sv,            "green"sv,       "yellow"sv,
    "blue"sv,         "magenta"sv,        "cyan"sv,        "white"sv,
    "bright_black"sv, "bright_red"sv,     "bright_green"sv, "bright_yellow"sv,
    "bright_blue"sv,  "bright_magenta"sv, "bright_cyan"sv,  "bright_white"sv
  };
  const auto index = std::to_underlying(value);
  TJG_EXPECTS(index >= 0 && index < std::ssize(names),
              "tjg::color::name_of: invalid terminal color");
  return names[index];
} // name_of term_color

/// An RGB color with red, green, and blue components in the range 0-255.
struct rgb {
  using value_type = std::uint32_t;
  using chan_type  = std::uint8_t;

  chan_type r = 0;  ///< Red   component
  chan_type g = 0;  ///< Green component
  chan_type b = 0;  ///< Blue  component

 private:
  [[nodiscard]] constexpr auto value() const noexcept -> value_type {
    return (value_type{r} << 16) | (value_type{g} << 8) | (value_type{b} << 0);
  }

  [[nodiscard]] static constexpr auto check_arg(int v) noexcept -> chan_type {
   TJG_ASSERT(v >= 0 && v <= 255, "rgb::rgb: channel out of range");
   return narrow_cast<chan_type>(std::clamp(v, 0, 255));
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
    : r{narrow_cast<chan_type>(hex >> 16)}
    , g{narrow_cast<chan_type>(hex >>  8)}
    , b{narrow_cast<chan_type>(hex >>  0)}
    {}

  /// Converts a terminal color using the fixed Xterm-default RGB palette.
  constexpr rgb(term_color clr) noexcept;

  /// Returns a numeric value for the color as 0x00RRGGBB.
  [[nodiscard]]
  constexpr explicit operator value_type() const noexcept { return value(); }

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

  /// Fixed Xterm-default RGB equivalents of terminal colors.
  static const rgb term_black;
  static const rgb term_red;
  static const rgb term_green;
  static const rgb term_yellow;
  static const rgb term_blue;
  static const rgb term_magenta;
  static const rgb term_cyan;
  static const rgb term_white;
  static const rgb term_bright_black;
  static const rgb term_bright_red;
  static const rgb term_bright_green;
  static const rgb term_bright_yellow;
  static const rgb term_bright_blue;
  static const rgb term_bright_magenta;
  static const rgb term_bright_cyan;
  static const rgb term_bright_white;

  /// Named RGB constants, independent of the terminal palette.
  static const rgb alice_blue;
  static const rgb antique_white;
  static const rgb aqua;
  static const rgb aquamarine;
  static const rgb azure;
  static const rgb beige;
  static const rgb bisque;
  static const rgb black;
  static const rgb blanched_almond;
  static const rgb blue;
  static const rgb blue_violet;
  static const rgb brown;
  static const rgb burly_wood;
  static const rgb cadet_blue;
  static const rgb chartreuse;
  static const rgb chocolate;
  static const rgb coral;
  static const rgb cornflower_blue;
  static const rgb cornsilk;
  static const rgb crimson;
  static const rgb cyan;
  static const rgb dark_blue;
  static const rgb dark_cyan;
  static const rgb dark_golden_rod;
  static const rgb dark_gray;
  static const rgb dark_green;
  static const rgb dark_khaki;
  static const rgb dark_magenta;
  static const rgb dark_olive_green;
  static const rgb dark_orange;
  static const rgb dark_orchid;
  static const rgb dark_red;
  static const rgb dark_salmon;
  static const rgb dark_sea_green;
  static const rgb dark_slate_blue;
  static const rgb dark_slate_gray;
  static const rgb dark_turquoise;
  static const rgb dark_violet;
  static const rgb deep_pink;
  static const rgb deep_sky_blue;
  static const rgb dim_gray;
  static const rgb dodger_blue;
  static const rgb fire_brick;
  static const rgb floral_white;
  static const rgb forest_green;
  static const rgb fuchsia;
  static const rgb gainsboro;
  static const rgb ghost_white;
  static const rgb gold;
  static const rgb golden_rod;
  static const rgb gray;
  static const rgb green;
  static const rgb green_yellow;
  static const rgb honey_dew;
  static const rgb hot_pink;
  static const rgb indian_red;
  static const rgb indigo;
  static const rgb ivory;
  static const rgb khaki;
  static const rgb lavender;
  static const rgb lavender_blush;
  static const rgb lawn_green;
  static const rgb lemon_chiffon;
  static const rgb light_blue;
  static const rgb light_coral;
  static const rgb light_cyan;
  static const rgb light_golden_rod_yellow;
  static const rgb light_gray;
  static const rgb light_green;
  static const rgb light_pink;
  static const rgb light_salmon;
  static const rgb light_sea_green;
  static const rgb light_sky_blue;
  static const rgb light_slate_gray;
  static const rgb light_steel_blue;
  static const rgb light_yellow;
  static const rgb lime;
  static const rgb lime_green;
  static const rgb linen;
  static const rgb magenta;
  static const rgb maroon;
  static const rgb medium_aquamarine;
  static const rgb medium_blue;
  static const rgb medium_orchid;
  static const rgb medium_purple;
  static const rgb medium_sea_green;
  static const rgb medium_slate_blue;
  static const rgb medium_spring_green;
  static const rgb medium_turquoise;
  static const rgb medium_violet_red;
  static const rgb midnight_blue;
  static const rgb mint_cream;
  static const rgb misty_rose;
  static const rgb moccasin;
  static const rgb navajo_white;
  static const rgb navy;
  static const rgb old_lace;
  static const rgb olive;
  static const rgb olive_drab;
  static const rgb orange;
  static const rgb orange_red;
  static const rgb orchid;
  static const rgb pale_golden_rod;
  static const rgb pale_green;
  static const rgb pale_turquoise;
  static const rgb pale_violet_red;
  static const rgb papaya_whip;
  static const rgb peach_puff;
  static const rgb peru;
  static const rgb pink;
  static const rgb plum;
  static const rgb powder_blue;
  static const rgb purple;
  static const rgb rebecca_purple;
  static const rgb red;
  static const rgb rosy_brown;
  static const rgb royal_blue;
  static const rgb saddle_brown;
  static const rgb salmon;
  static const rgb sandy_brown;
  static const rgb sea_green;
  static const rgb sea_shell;
  static const rgb sienna;
  static const rgb silver;
  static const rgb sky_blue;
  static const rgb slate_blue;
  static const rgb slate_gray;
  static const rgb snow;
  static const rgb spring_green;
  static const rgb steel_blue;
  static const rgb tan;
  static const rgb teal;
  static const rgb thistle;
  static const rgb tomato;
  static const rgb turquoise;
  static const rgb violet;
  static const rgb wheat;
  static const rgb white;
  static const rgb white_smoke;
  static const rgb yellow;
  static const rgb yellow_green;

}; // rgb

// Xterm RGB mapping for terminal palette indices 0-15.
inline constexpr rgb rgb::term_black         {  0,   0,   0};
inline constexpr rgb rgb::term_red           {205,   0,   0};
inline constexpr rgb rgb::term_green         {  0, 205,   0};
inline constexpr rgb rgb::term_yellow        {205, 205,   0};
inline constexpr rgb rgb::term_blue          {  0,   0, 238};
inline constexpr rgb rgb::term_magenta       {205  , 0, 205};
inline constexpr rgb rgb::term_cyan          {  0, 205, 205};
inline constexpr rgb rgb::term_white         {229, 229, 229};
inline constexpr rgb rgb::term_bright_black  {127, 127, 127};
inline constexpr rgb rgb::term_bright_red    {255,   0,   0};
inline constexpr rgb rgb::term_bright_green  {  0, 255,   0};
inline constexpr rgb rgb::term_bright_yellow {255, 255,   0};
inline constexpr rgb rgb::term_bright_blue   { 92,  92, 255};
inline constexpr rgb rgb::term_bright_magenta{255,   0, 255};
inline constexpr rgb rgb::term_bright_cyan   {  0, 255, 255};
inline constexpr rgb rgb::term_bright_white  {255, 255, 255};

namespace detail {

inline constexpr auto term_color_palette = std::array{
  // 0–7 normal
  rgb::term_black, rgb::term_red,     rgb::term_green, rgb::term_yellow,
  rgb::term_blue,  rgb::term_magenta, rgb::term_cyan , rgb::term_white,

  // 8–15 bright
  rgb::term_bright_black, rgb::term_bright_red ,
  rgb::term_bright_green, rgb::term_bright_yellow,
  rgb::term_bright_blue , rgb::term_bright_magenta,
  rgb::term_bright_cyan , rgb::term_bright_white
}; // term_color_palette

constexpr auto checked_term_index(term_color clr) noexcept
  -> std::underlying_type_t<term_color>
{
  auto c = std::to_underlying(clr);
  constexpr auto min_c = std::to_underlying(term_color::black);
  constexpr auto max_c = std::to_underlying(term_color::bright_white);
  TJG_ASSERT((c>=min_c && c<=max_c), "rgb::rgb: terminal color out of range");
  return std::clamp(c, min_c, max_c);
}

} // detail

constexpr inline rgb::rgb(term_color clr) noexcept
{ *this = detail::term_color_palette[detail::checked_term_index(clr)]; }

/// A zero-based gray palette index from 0 to 23.
class gray_color {
  using chan_type = std::int8_t;

  chan_type _value = 0;

  static constexpr auto make_rgb(int level) noexcept -> rgb
  { return rgb{level, level, level}; }

  [[nodiscard]] static constexpr chan_type check_arg(int v) noexcept {
    TJG_ASSERT((v>=0 && v<=23), "gray_color::gray_color: index out of range");
    return narrow_cast<chan_type>(std::clamp(v, 0, 23));
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
  [[nodiscard]] constexpr operator rgb() const noexcept
  { return make_rgb(narrow_cast<rgb::chan_type>(10 * _value + 8)); }

}; // gray_color

/// A 6x6x6 palette color with channel indices 0-5 and a combined index 0-215.
class cube_color {
  using chan_type = std::int8_t;

  chan_type _r = 0;
  chan_type _g = 0;
  chan_type _b = 0;

  [[nodiscard]]
  static constexpr auto check_arg(int v) noexcept -> chan_type {
    TJG_ASSERT((v>=0 && v<=5), "cube_color::cube_color: channel out of range");
    return narrow_cast<chan_type>(std::clamp(v, 0, 5));
  }

  [[nodiscard]] static constexpr auto check_index(int v) noexcept -> int {
    TJG_ASSERT((v>=0 && v<=215), "cube_color::cube_color: index out of range");
    return std::clamp(v, 0, 215);
  }

 public:
  /// Returns the zero-based cube palette index, from 0 to 215.
  [[nodiscard]] constexpr auto index() const noexcept -> int
  { return 36 * _r + 6 * _g + _b; }

  /// Returns the red channel index, from 0 to 5.
  [[nodiscard]] constexpr auto r() const noexcept -> int { return _r; };

  /// Returns the green channel index, from 0 to 5.
  [[nodiscard]] constexpr auto g() const noexcept -> int { return _g; };

  /// Returns the blue channel index, from 0 to 5.
  [[nodiscard]] constexpr auto b() const noexcept -> int { return _b; };

  /// Sets the red index.
  constexpr auto r(int v) noexcept -> cube_color& {
    _r = check_arg(v);
    return *this;
  }

  /// Sets the green index.
  constexpr auto g(int v) noexcept -> cube_color& {
    _g = check_arg(v);
    return *this;
  }

  /// Sets the blue index.
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
  constexpr cube_color(int r_, int g_, int b_) noexcept
    : _r{check_arg(r_)}, _g{check_arg(g_)}, _b{check_arg(b_)} {}

  /// Constructs a zero-based index 0-215.
  constexpr explicit cube_color(int value) noexcept {
    auto v = check_index(value);
    _r = narrow_cast<chan_type>(v / 36);
    _g = narrow_cast<chan_type>((v / 6) % 6);
    _b = narrow_cast<chan_type>(v % 6);
  }

private:

  static constexpr auto cube_levels = std::array<rgb::chan_type, 6>{
                                                    0, 95, 135, 175, 215, 255};

public:

  /// Converts channel indices to RGB levels {0, 95, 135, 175, 215, 255}.
  [[nodiscard]] constexpr operator rgb() const noexcept
  { return rgb{cube_levels[_r], cube_levels[_g], cube_levels[_b]}; }

}; // cube_color

/// Identifies types that can be used directly as color values.
template<typename T>
concept Color = std::same_as<std::remove_cvref_t<T>, rgb>
             || std::same_as<std::remove_cvref_t<T>, term_color>
             || std::same_as<std::remove_cvref_t<T>, cube_color>
             || std::same_as<std::remove_cvref_t<T>, gray_color>;

/// Returns a plain color's fixed RGB equivalent without changing the argument.
template<Color T>
[[nodiscard]] constexpr auto to_rgb(const T& color) noexcept -> rgb
{ return rgb(color); }

/// Preserves an unspecified color rather than converting it to RGB black.
[[nodiscard]] constexpr auto to_rgb(no_color_t) noexcept -> no_color_t
{ return no_color_t{}; }

/// A color variant with alternatives for each supported color kind.
using color_t =
              std::variant<no_color_t, rgb, term_color, cube_color, gray_color>;

namespace detail {

enum class color_kind { none, rgb, term, cube, gray };

constexpr auto is_unspecified(const color_t& color) noexcept -> bool {
  return color.valueless_by_exception()
          || std::holds_alternative<no_color_t>(color);
}

struct to_rgb_visitor {
  constexpr auto operator()(const no_color_t&) const noexcept -> color_t
  { return color_t{no_color}; }

  template<typename T>
  constexpr auto operator()(const T& color) const noexcept -> color_t
  { return color_t{to_rgb(color)}; }
};

} // detail

/**
 * Returns a color variant holding the fixed RGB equivalent of a specified
 * color. An unspecified color remains unspecified.
 */
[[nodiscard]] constexpr auto to_rgb(const color_t& color) noexcept -> color_t {
  if (color.valueless_by_exception()) return no_color;
  return std::visit(detail::to_rgb_visitor{}, color);
}

namespace detail {

template<typename T>
concept BlendColor = Color<T>
                  || std::same_as<std::remove_cvref_t<T>, color_t>
                  || std::same_as<std::remove_cvref_t<T>, no_color_t>;

struct blend_rgb {
  constexpr auto operator()(const no_color_t&) const noexcept -> rgb
  { return rgb{}; }

  template<typename T>
  constexpr auto operator()(const T& color) const noexcept -> rgb
  { return rgb{color}; }
}; // blend_rgb

struct blend_state {
  std::int32_t r{0}, g{0}, b{0};
  int count{0};

  constexpr blend_state() = default;

  constexpr void add(rgb color) noexcept {
    if (count == 0) _first = color;
    accumulate(color);
  }

  constexpr void add(const color_t& color) noexcept {
    if (is_unspecified(color)) return;
    if (count == 0) _first = color;
    accumulate(std::visit(blend_rgb{}, color));
  }

  constexpr auto average() const noexcept -> rgb {
    return rgb{narrow_cast<rgb::chan_type>(r / count),
               narrow_cast<rgb::chan_type>(g / count),
               narrow_cast<rgb::chan_type>(b / count)};
  }

  constexpr auto result() const noexcept -> color_t {
    if (count == 0) return no_color;
    if (count == 1) return _first;
    return color_t{average()};
  }

 private:
  color_t _first{no_color};

  constexpr void accumulate(rgb color) noexcept {
    r += color.r;
    g += color.g;
    b += color.b;
    ++count;
  }
}; // blend_state

} // detail

/**
 * Returns the component-wise arithmetic mean of two or more plain colors.
 * Accepts rgb, term_color, cube_color, and gray_color, converting each to RGB.
 * Each argument has equal weight. Fractional channels are rounded down after
 * summing all arguments. The arguments are unchanged.
 * Supports constant evaluation.
 */
template<Color First, Color Second, Color... Colors>
[[nodiscard]] constexpr
auto blend(First first, Second second, Colors... rest) noexcept -> rgb {
  auto state = detail::blend_state{};
  const rgb colors[] = {rgb{first}, rgb{second}, rgb{rest}...};
  for (auto color : colors)
    state.add(color);
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
template<detail::BlendColor Second, detail::BlendColor... Colors>
[[nodiscard]] constexpr
auto blend(color_t first, Second second, Colors... rest) noexcept -> color_t {
  auto state = detail::blend_state{};
  const color_t colors[] = {first, color_t{second}, color_t{rest}...};
  for (auto color : colors)
    state.add(color);
  return state.result();
}

/**
 * Named RGB colors.
 *
 * These static members are constexpr `rgb` constants with fixed RGB values,
 * independently of the terminal palette. Pass a value to `fg()` or `bg()`.
 */
inline constexpr rgb rgb::alice_blue              {240, 248, 255};
inline constexpr rgb rgb::antique_white           {250, 235, 215};
inline constexpr rgb rgb::aqua                    {  0, 255, 255};
inline constexpr rgb rgb::aquamarine              {127, 255, 212};
inline constexpr rgb rgb::azure                   {240, 255, 255};
inline constexpr rgb rgb::beige                   {245, 245, 220};
inline constexpr rgb rgb::bisque                  {255, 228, 196};
inline constexpr rgb rgb::black                   {  0,   0,   0};
inline constexpr rgb rgb::blanched_almond         {255, 235, 205};
inline constexpr rgb rgb::blue                    {  0,   0, 255};
inline constexpr rgb rgb::blue_violet             {138,  43, 226};
inline constexpr rgb rgb::brown                   {165,  42,  42};
inline constexpr rgb rgb::burly_wood              {222, 184, 135};
inline constexpr rgb rgb::cadet_blue              { 95, 158, 160};
inline constexpr rgb rgb::chartreuse              {127, 255,   0};
inline constexpr rgb rgb::chocolate               {210, 105,  30};
inline constexpr rgb rgb::coral                   {255, 127,  80};
inline constexpr rgb rgb::cornflower_blue         {100, 149, 237};
inline constexpr rgb rgb::cornsilk                {255, 248, 220};
inline constexpr rgb rgb::crimson                 {220,  20,  60};
inline constexpr rgb rgb::cyan                    {  0, 255, 255};
inline constexpr rgb rgb::dark_blue               {  0,   0, 139};
inline constexpr rgb rgb::dark_cyan               {  0, 139, 139};
inline constexpr rgb rgb::dark_golden_rod         {184, 134,  11};
inline constexpr rgb rgb::dark_gray               {169, 169, 169};
inline constexpr rgb rgb::dark_green              {  0, 100,   0};
inline constexpr rgb rgb::dark_khaki              {189, 183, 107};
inline constexpr rgb rgb::dark_magenta            {139,   0, 139};
inline constexpr rgb rgb::dark_olive_green        { 85, 107,  47};
inline constexpr rgb rgb::dark_orange             {255, 140,   0};
inline constexpr rgb rgb::dark_orchid             {153,  50, 204};
inline constexpr rgb rgb::dark_red                {139,   0,   0};
inline constexpr rgb rgb::dark_salmon             {233, 150, 122};
inline constexpr rgb rgb::dark_sea_green          {143, 188, 143};
inline constexpr rgb rgb::dark_slate_blue         { 72,  61, 139};
inline constexpr rgb rgb::dark_slate_gray         { 47,  79,  79};
inline constexpr rgb rgb::dark_turquoise          {  0, 206, 209};
inline constexpr rgb rgb::dark_violet             {148,   0, 211};
inline constexpr rgb rgb::deep_pink               {255,  20, 147};
inline constexpr rgb rgb::deep_sky_blue           {  0, 191, 255};
inline constexpr rgb rgb::dim_gray                {105, 105, 105};
inline constexpr rgb rgb::dodger_blue             { 30, 144, 255};
inline constexpr rgb rgb::fire_brick              {178,  34,  34};
inline constexpr rgb rgb::floral_white            {255, 250, 240};
inline constexpr rgb rgb::forest_green            { 34, 139,  34};
inline constexpr rgb rgb::fuchsia                 {255,   0, 255};
inline constexpr rgb rgb::gainsboro               {220, 220, 220};
inline constexpr rgb rgb::ghost_white             {248, 248, 255};
inline constexpr rgb rgb::gold                    {255, 215,   0};
inline constexpr rgb rgb::golden_rod              {218, 165,  32};
inline constexpr rgb rgb::gray                    {128, 128, 128};
inline constexpr rgb rgb::green                   {  0, 128,   0};
inline constexpr rgb rgb::green_yellow            {173, 255,  47};
inline constexpr rgb rgb::honey_dew               {240, 255, 240};
inline constexpr rgb rgb::hot_pink                {255, 105, 180};
inline constexpr rgb rgb::indian_red              {205,  92,  92};
inline constexpr rgb rgb::indigo                  { 75,   0, 130};
inline constexpr rgb rgb::ivory                   {255, 255, 240};
inline constexpr rgb rgb::khaki                   {240, 230, 140};
inline constexpr rgb rgb::lavender                {230, 230, 250};
inline constexpr rgb rgb::lavender_blush          {255, 240, 245};
inline constexpr rgb rgb::lawn_green              {124, 252,   0};
inline constexpr rgb rgb::lemon_chiffon           {255, 250, 205};
inline constexpr rgb rgb::light_blue              {173, 216, 230};
inline constexpr rgb rgb::light_coral             {240, 128, 128};
inline constexpr rgb rgb::light_cyan              {224, 255, 255};
inline constexpr rgb rgb::light_golden_rod_yellow {250, 250, 210};
inline constexpr rgb rgb::light_gray              {211, 211, 211};
inline constexpr rgb rgb::light_green             {144, 238, 144};
inline constexpr rgb rgb::light_pink              {255, 182, 193};
inline constexpr rgb rgb::light_salmon            {255, 160, 122};
inline constexpr rgb rgb::light_sea_green         { 32, 178, 170};
inline constexpr rgb rgb::light_sky_blue          {135, 206, 250};
inline constexpr rgb rgb::light_slate_gray        {119, 136, 153};
inline constexpr rgb rgb::light_steel_blue        {176, 196, 222};
inline constexpr rgb rgb::light_yellow            {255, 255, 224};
inline constexpr rgb rgb::lime                    {  0, 255,   0};
inline constexpr rgb rgb::lime_green              { 50, 205,  50};
inline constexpr rgb rgb::linen                   {250, 240, 230};
inline constexpr rgb rgb::magenta                 {255,   0, 255};
inline constexpr rgb rgb::maroon                  {128,   0,   0};
inline constexpr rgb rgb::medium_aquamarine       {102, 205, 170};
inline constexpr rgb rgb::medium_blue             {  0,   0, 205};
inline constexpr rgb rgb::medium_orchid           {186,  85, 211};
inline constexpr rgb rgb::medium_purple           {147, 112, 219};
inline constexpr rgb rgb::medium_sea_green        { 60, 179, 113};
inline constexpr rgb rgb::medium_slate_blue       {123, 104, 238};
inline constexpr rgb rgb::medium_spring_green     {  0, 250, 154};
inline constexpr rgb rgb::medium_turquoise        { 72, 209, 204};
inline constexpr rgb rgb::medium_violet_red       {199,  21, 133};
inline constexpr rgb rgb::midnight_blue           { 25,  25, 112};
inline constexpr rgb rgb::mint_cream              {245, 255, 250};
inline constexpr rgb rgb::misty_rose              {255, 228, 225};
inline constexpr rgb rgb::moccasin                {255, 228, 181};
inline constexpr rgb rgb::navajo_white            {255, 222, 173};
inline constexpr rgb rgb::navy                    {  0,   0, 128};
inline constexpr rgb rgb::old_lace                {253, 245, 230};
inline constexpr rgb rgb::olive                   {128, 128,   0};
inline constexpr rgb rgb::olive_drab              {107, 142,  35};
inline constexpr rgb rgb::orange                  {255, 165,   0};
inline constexpr rgb rgb::orange_red              {255,  69,   0};
inline constexpr rgb rgb::orchid                  {218, 112, 214};
inline constexpr rgb rgb::pale_golden_rod         {238, 232, 170};
inline constexpr rgb rgb::pale_green              {152, 251, 152};
inline constexpr rgb rgb::pale_turquoise          {175, 238, 238};
inline constexpr rgb rgb::pale_violet_red         {219, 112, 147};
inline constexpr rgb rgb::papaya_whip             {255, 239, 213};
inline constexpr rgb rgb::peach_puff              {255, 218, 185};
inline constexpr rgb rgb::peru                    {205, 133,  63};
inline constexpr rgb rgb::pink                    {255, 192, 203};
inline constexpr rgb rgb::plum                    {221, 160, 221};
inline constexpr rgb rgb::powder_blue             {176, 224, 230};
inline constexpr rgb rgb::purple                  {128,   0, 128};
inline constexpr rgb rgb::rebecca_purple          {102,  51, 153};
inline constexpr rgb rgb::red                     {255,   0,   0};
inline constexpr rgb rgb::rosy_brown              {188, 143, 143};
inline constexpr rgb rgb::royal_blue              { 65, 105, 225};
inline constexpr rgb rgb::saddle_brown            {139,  69,  19};
inline constexpr rgb rgb::salmon                  {250, 128, 114};
inline constexpr rgb rgb::sandy_brown             {244, 164,  96};
inline constexpr rgb rgb::sea_green               { 46, 139,  87};
inline constexpr rgb rgb::sea_shell               {255, 245, 238};
inline constexpr rgb rgb::sienna                  {160,  82,  45};
inline constexpr rgb rgb::silver                  {192, 192, 192};
inline constexpr rgb rgb::sky_blue                {135, 206, 235};
inline constexpr rgb rgb::slate_blue              {106,  90, 205};
inline constexpr rgb rgb::slate_gray              {112, 128, 144};
inline constexpr rgb rgb::snow                    {255, 250, 250};
inline constexpr rgb rgb::spring_green            {  0, 255, 127};
inline constexpr rgb rgb::steel_blue              { 70, 130, 180};
inline constexpr rgb rgb::tan                     {210, 180, 140};
inline constexpr rgb rgb::teal                    {  0, 128, 128};
inline constexpr rgb rgb::thistle                 {216, 191, 216};
inline constexpr rgb rgb::tomato                  {255,  99,  71};
inline constexpr rgb rgb::turquoise               { 64, 224, 208};
inline constexpr rgb rgb::violet                  {238, 130, 238};
inline constexpr rgb rgb::wheat                   {245, 222, 179};
inline constexpr rgb rgb::white                   {255, 255, 255};
inline constexpr rgb rgb::white_smoke             {245, 245, 245};
inline constexpr rgb rgb::yellow                  {255, 255,   0};
inline constexpr rgb rgb::yellow_green            {154, 205,  50};

} // tjg::color

namespace std {

template<> struct hash<tjg::color::term_color> {
  auto operator()(tjg::color::term_color value) const noexcept -> std::size_t
  { return tjg::hash_value(std::to_underlying(value)); }
};

template<> struct hash<tjg::color::rgb> {
  auto operator()(tjg::color::rgb value) const noexcept -> std::size_t
  { return tjg::hash_value(static_cast<std::uint32_t>(value)); }
};

template<> struct hash<tjg::color::cube_color> {
  auto operator()(tjg::color::cube_color value) const noexcept -> std::size_t
  { return tjg::hash_value(value.index()); }
};

template<> struct hash<tjg::color::gray_color> {
  auto operator()(tjg::color::gray_color value) const noexcept -> std::size_t
  { return tjg::hash_value(value.index()); }
};

template<> struct hash<tjg::color::color_t> {
  auto operator()(tjg::color::color_t value) const noexcept -> std::size_t {
    auto result = tjg::hash_value(value.index());
    if (value.valueless_by_exception()) return result;
    std::visit([&](const auto& color) noexcept {
      if constexpr (!std::same_as<std::remove_cvref_t<decltype(color)>,
                                  tjg::color::no_color_t>)
        tjg::hash_combine(result, color);
    }, value);
    return result;
  }
}; // hash tjg::color::color_t

} // std
