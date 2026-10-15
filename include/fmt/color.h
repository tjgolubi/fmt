// Formatting library for C++ - color support
//
// Copyright (c) 2018 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COLOR_H_
#define FMT_COLOR_H_

#include "format.h"

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

// rgb is a struct for red, green and blue colors.
// Using the name "rgb" makes some editors show the color in a tooltip.
struct rgb {
  constexpr rgb() : r(0), g(0), b(0) {}
  constexpr rgb(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
  explicit constexpr rgb(uint32_t hex)
      : r((hex >> 16) & 0xFF), g((hex >> 8) & 0xFF), b(hex & 0xFF) {}
  explicit constexpr operator uint32_t() const {
    return (static_cast<uint32_t>(r) << 16) |
           (static_cast<uint32_t>(g) << 8) | b;
  }
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

namespace color {
constexpr FMT_INLINE_VARIABLE rgb alice_blue{240, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb antique_white{250, 235, 215};
constexpr FMT_INLINE_VARIABLE rgb aqua{0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb aquamarine{127, 255, 212};
constexpr FMT_INLINE_VARIABLE rgb azure{240, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb beige{245, 245, 220};
constexpr FMT_INLINE_VARIABLE rgb bisque{255, 228, 196};
constexpr FMT_INLINE_VARIABLE rgb black{0, 0, 0};
constexpr FMT_INLINE_VARIABLE rgb blanched_almond{255, 235, 205};
constexpr FMT_INLINE_VARIABLE rgb blue{0, 0, 255};
constexpr FMT_INLINE_VARIABLE rgb blue_violet{138, 43, 226};
constexpr FMT_INLINE_VARIABLE rgb brown{165, 42, 42};
constexpr FMT_INLINE_VARIABLE rgb burly_wood{222, 184, 135};
constexpr FMT_INLINE_VARIABLE rgb cadet_blue{95, 158, 160};
constexpr FMT_INLINE_VARIABLE rgb chartreuse{127, 255, 0};
constexpr FMT_INLINE_VARIABLE rgb chocolate{210, 105, 30};
constexpr FMT_INLINE_VARIABLE rgb coral{255, 127, 80};
constexpr FMT_INLINE_VARIABLE rgb cornflower_blue{100, 149, 237};
constexpr FMT_INLINE_VARIABLE rgb cornsilk{255, 248, 220};
constexpr FMT_INLINE_VARIABLE rgb crimson{220, 20, 60};
constexpr FMT_INLINE_VARIABLE rgb cyan{0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb dark_blue{0, 0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_cyan{0, 139, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_golden_rod{184, 134, 11};
constexpr FMT_INLINE_VARIABLE rgb dark_gray{169, 169, 169};
constexpr FMT_INLINE_VARIABLE rgb dark_green{0, 100, 0};
constexpr FMT_INLINE_VARIABLE rgb dark_khaki{189, 183, 107};
constexpr FMT_INLINE_VARIABLE rgb dark_magenta{139, 0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_olive_green{85, 107, 47};
constexpr FMT_INLINE_VARIABLE rgb dark_orange{255, 140, 0};
constexpr FMT_INLINE_VARIABLE rgb dark_orchid{153, 50, 204};
constexpr FMT_INLINE_VARIABLE rgb dark_red{139, 0, 0};
constexpr FMT_INLINE_VARIABLE rgb dark_salmon{233, 150, 122};
constexpr FMT_INLINE_VARIABLE rgb dark_sea_green{143, 188, 143};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_blue{72, 61, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_gray{47, 79, 79};
constexpr FMT_INLINE_VARIABLE rgb dark_turquoise{0, 206, 209};
constexpr FMT_INLINE_VARIABLE rgb dark_violet{148, 0, 211};
constexpr FMT_INLINE_VARIABLE rgb deep_pink{255, 20, 147};
constexpr FMT_INLINE_VARIABLE rgb deep_sky_blue{0, 191, 255};
constexpr FMT_INLINE_VARIABLE rgb dim_gray{105, 105, 105};
constexpr FMT_INLINE_VARIABLE rgb dodger_blue{30, 144, 255};
constexpr FMT_INLINE_VARIABLE rgb fire_brick{178, 34, 34};
constexpr FMT_INLINE_VARIABLE rgb floral_white{255, 250, 240};
constexpr FMT_INLINE_VARIABLE rgb forest_green{34, 139, 34};
constexpr FMT_INLINE_VARIABLE rgb fuchsia{255, 0, 255};
constexpr FMT_INLINE_VARIABLE rgb gainsboro{220, 220, 220};
constexpr FMT_INLINE_VARIABLE rgb ghost_white{248, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb gold{255, 215, 0};
constexpr FMT_INLINE_VARIABLE rgb golden_rod{218, 165, 32};
constexpr FMT_INLINE_VARIABLE rgb gray{128, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb green{0, 128, 0};
constexpr FMT_INLINE_VARIABLE rgb green_yellow{173, 255, 47};
constexpr FMT_INLINE_VARIABLE rgb honey_dew{240, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb hot_pink{255, 105, 180};
constexpr FMT_INLINE_VARIABLE rgb indian_red{205, 92, 92};
constexpr FMT_INLINE_VARIABLE rgb indigo{75, 0, 130};
constexpr FMT_INLINE_VARIABLE rgb ivory{255, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb khaki{240, 230, 140};
constexpr FMT_INLINE_VARIABLE rgb lavender{230, 230, 250};
constexpr FMT_INLINE_VARIABLE rgb lavender_blush{255, 240, 245};
constexpr FMT_INLINE_VARIABLE rgb lawn_green{124, 252, 0};
constexpr FMT_INLINE_VARIABLE rgb lemon_chiffon{255, 250, 205};
constexpr FMT_INLINE_VARIABLE rgb light_blue{173, 216, 230};
constexpr FMT_INLINE_VARIABLE rgb light_coral{240, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb light_cyan{224, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb light_golden_rod_yellow{250, 250, 210};
constexpr FMT_INLINE_VARIABLE rgb light_gray{211, 211, 211};
constexpr FMT_INLINE_VARIABLE rgb light_green{144, 238, 144};
constexpr FMT_INLINE_VARIABLE rgb light_pink{255, 182, 193};
constexpr FMT_INLINE_VARIABLE rgb light_salmon{255, 160, 122};
constexpr FMT_INLINE_VARIABLE rgb light_sea_green{32, 178, 170};
constexpr FMT_INLINE_VARIABLE rgb light_sky_blue{135, 206, 250};
constexpr FMT_INLINE_VARIABLE rgb light_slate_gray{119, 136, 153};
constexpr FMT_INLINE_VARIABLE rgb light_steel_blue{176, 196, 222};
constexpr FMT_INLINE_VARIABLE rgb light_yellow{255, 255, 224};
constexpr FMT_INLINE_VARIABLE rgb lime{0, 255, 0};
constexpr FMT_INLINE_VARIABLE rgb lime_green{50, 205, 50};
constexpr FMT_INLINE_VARIABLE rgb linen{250, 240, 230};
constexpr FMT_INLINE_VARIABLE rgb magenta{255, 0, 255};
constexpr FMT_INLINE_VARIABLE rgb maroon{128, 0, 0};
constexpr FMT_INLINE_VARIABLE rgb medium_aquamarine{102, 205, 170};
constexpr FMT_INLINE_VARIABLE rgb medium_blue{0, 0, 205};
constexpr FMT_INLINE_VARIABLE rgb medium_orchid{186, 85, 211};
constexpr FMT_INLINE_VARIABLE rgb medium_purple{147, 112, 219};
constexpr FMT_INLINE_VARIABLE rgb medium_sea_green{60, 179, 113};
constexpr FMT_INLINE_VARIABLE rgb medium_slate_blue{123, 104, 238};
constexpr FMT_INLINE_VARIABLE rgb medium_spring_green{0, 250, 154};
constexpr FMT_INLINE_VARIABLE rgb medium_turquoise{72, 209, 204};
constexpr FMT_INLINE_VARIABLE rgb medium_violet_red{199, 21, 133};
constexpr FMT_INLINE_VARIABLE rgb midnight_blue{25, 25, 112};
constexpr FMT_INLINE_VARIABLE rgb mint_cream{245, 255, 250};
constexpr FMT_INLINE_VARIABLE rgb misty_rose{255, 228, 225};
constexpr FMT_INLINE_VARIABLE rgb moccasin{255, 228, 181};
constexpr FMT_INLINE_VARIABLE rgb navajo_white{255, 222, 173};
constexpr FMT_INLINE_VARIABLE rgb navy{0, 0, 128};
constexpr FMT_INLINE_VARIABLE rgb old_lace{253, 245, 230};
constexpr FMT_INLINE_VARIABLE rgb olive{128, 128, 0};
constexpr FMT_INLINE_VARIABLE rgb olive_drab{107, 142, 35};
constexpr FMT_INLINE_VARIABLE rgb orange{255, 165, 0};
constexpr FMT_INLINE_VARIABLE rgb orange_red{255, 69, 0};
constexpr FMT_INLINE_VARIABLE rgb orchid{218, 112, 214};
constexpr FMT_INLINE_VARIABLE rgb pale_golden_rod{238, 232, 170};
constexpr FMT_INLINE_VARIABLE rgb pale_green{152, 251, 152};
constexpr FMT_INLINE_VARIABLE rgb pale_turquoise{175, 238, 238};
constexpr FMT_INLINE_VARIABLE rgb pale_violet_red{219, 112, 147};
constexpr FMT_INLINE_VARIABLE rgb papaya_whip{255, 239, 213};
constexpr FMT_INLINE_VARIABLE rgb peach_puff{255, 218, 185};
constexpr FMT_INLINE_VARIABLE rgb peru{205, 133, 63};
constexpr FMT_INLINE_VARIABLE rgb pink{255, 192, 203};
constexpr FMT_INLINE_VARIABLE rgb plum{221, 160, 221};
constexpr FMT_INLINE_VARIABLE rgb powder_blue{176, 224, 230};
constexpr FMT_INLINE_VARIABLE rgb purple{128, 0, 128};
constexpr FMT_INLINE_VARIABLE rgb rebecca_purple{102, 51, 153};
constexpr FMT_INLINE_VARIABLE rgb red{255, 0, 0};
constexpr FMT_INLINE_VARIABLE rgb rosy_brown{188, 143, 143};
constexpr FMT_INLINE_VARIABLE rgb royal_blue{65, 105, 225};
constexpr FMT_INLINE_VARIABLE rgb saddle_brown{139, 69, 19};
constexpr FMT_INLINE_VARIABLE rgb salmon{250, 128, 114};
constexpr FMT_INLINE_VARIABLE rgb sandy_brown{244, 164, 96};
constexpr FMT_INLINE_VARIABLE rgb sea_green{46, 139, 87};
constexpr FMT_INLINE_VARIABLE rgb sea_shell{255, 245, 238};
constexpr FMT_INLINE_VARIABLE rgb sienna{160, 82, 45};
constexpr FMT_INLINE_VARIABLE rgb silver{192, 192, 192};
constexpr FMT_INLINE_VARIABLE rgb sky_blue{135, 206, 235};
constexpr FMT_INLINE_VARIABLE rgb slate_blue{106, 90, 205};
constexpr FMT_INLINE_VARIABLE rgb slate_gray{112, 128, 144};
constexpr FMT_INLINE_VARIABLE rgb snow{255, 250, 250};
constexpr FMT_INLINE_VARIABLE rgb spring_green{0, 255, 127};
constexpr FMT_INLINE_VARIABLE rgb steel_blue{70, 130, 180};
constexpr FMT_INLINE_VARIABLE rgb tan{210, 180, 140};
constexpr FMT_INLINE_VARIABLE rgb teal{0, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb thistle{216, 191, 216};
constexpr FMT_INLINE_VARIABLE rgb tomato{255, 99, 71};
constexpr FMT_INLINE_VARIABLE rgb turquoise{64, 224, 208};
constexpr FMT_INLINE_VARIABLE rgb violet{238, 130, 238};
constexpr FMT_INLINE_VARIABLE rgb wheat{245, 222, 179};
constexpr FMT_INLINE_VARIABLE rgb white{255, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb white_smoke{245, 245, 245};
constexpr FMT_INLINE_VARIABLE rgb yellow{255, 255, 0};
constexpr FMT_INLINE_VARIABLE rgb yellow_green{154, 205, 50};
}  // namespace color

enum class terminal_color : uint8_t {
  black = 30,
  red,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  white,
  bright_black = 90,
  bright_red,
  bright_green,
  bright_yellow,
  bright_blue,
  bright_magenta,
  bright_cyan,
  bright_white
};

enum class emphasis : uint8_t {
  bold = 1,
  faint = 1 << 1,
  italic = 1 << 2,
  underline = 1 << 3,
  blink = 1 << 4,
  reverse = 1 << 5,
  conceal = 1 << 6,
  strikethrough = 1 << 7,
};

namespace detail {

// A bit-packed variant of an RGB color, a terminal color, or unset color.
// see text_style for the bit-packing scheme.
struct color_type {
  constexpr color_type() noexcept = default;
  constexpr color_type(rgb rgb_color) noexcept
      : value_(static_cast<uint32_t>(rgb_color) | (1 << 24)) {}
  constexpr color_type(terminal_color term_color) noexcept
      : value_(static_cast<uint32_t>(term_color) | (3 << 24)) {}

  constexpr auto is_terminal_color() const noexcept -> bool {
    return (value_ & (1 << 25)) != 0;
  }

  constexpr auto value() const noexcept -> uint32_t {
    return value_ & 0xFFFFFF;
  }

  constexpr color_type(uint32_t value) noexcept : value_(value) {}

  uint32_t value_ = 0;
};
}  // namespace detail

/// A text style consisting of foreground and background colors and emphasis.
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
  FMT_CONSTEXPR text_style(emphasis em = emphasis()) noexcept
      : style_(static_cast<uint64_t>(em) << 54) {}

  FMT_CONSTEXPR auto operator|=(text_style rhs) -> text_style& {
    if (((style_ + rhs.style_) & ((1ULL << 26) | (1ULL << 53))) != 0)
      report_error("can't OR a terminal color");
    style_ |= rhs.style_;
    return *this;
  }

  friend FMT_CONSTEXPR auto operator|(text_style lhs, text_style rhs)
      -> text_style {
    return lhs |= rhs;
  }

  FMT_CONSTEXPR auto operator==(text_style rhs) const noexcept -> bool {
    return style_ == rhs.style_;
  }

  FMT_CONSTEXPR auto operator!=(text_style rhs) const noexcept -> bool {
    return !(*this == rhs);
  }

  FMT_CONSTEXPR auto has_foreground() const noexcept -> bool {
    return (style_ & (1 << 24)) != 0;
  }
  FMT_CONSTEXPR auto has_background() const noexcept -> bool {
    return (style_ & (1ULL << 51)) != 0;
  }
  FMT_CONSTEXPR auto has_emphasis() const noexcept -> bool {
    return (style_ >> 54) != 0;
  }
  FMT_CONSTEXPR auto get_foreground() const noexcept -> detail::color_type {
    FMT_ASSERT(has_foreground(), "no foreground specified for this style");
    return style_ & 0x3FFFFFF;
  }
  FMT_CONSTEXPR auto get_background() const noexcept -> detail::color_type {
    FMT_ASSERT(has_background(), "no background specified for this style");
    return (style_ >> 27) & 0x3FFFFFF;
  }
  FMT_CONSTEXPR auto get_emphasis() const noexcept -> emphasis {
    FMT_ASSERT(has_emphasis(), "no emphasis specified for this style");
    return static_cast<emphasis>(style_ >> 54);
  }

 private:
  FMT_CONSTEXPR text_style(uint64_t style) noexcept : style_(style) {}

  friend FMT_CONSTEXPR auto fg(detail::color_type foreground) noexcept
      -> text_style;

  friend FMT_CONSTEXPR auto bg(detail::color_type background) noexcept
      -> text_style;

  uint64_t style_ = 0;
};

/// Creates a text style from the foreground (text) color.
FMT_CONSTEXPR inline auto fg(detail::color_type foreground) noexcept
    -> text_style {
  return foreground.value_;
}

/// Creates a text style from the background color.
FMT_CONSTEXPR inline auto bg(detail::color_type background) noexcept
    -> text_style {
  return static_cast<uint64_t>(background.value_) << 27;
}

FMT_CONSTEXPR inline auto operator|(emphasis lhs, emphasis rhs) noexcept
    -> text_style {
  return text_style(lhs) | rhs;
}

namespace detail {

template <typename Char> struct ansi_color_escape {
  FMT_CONSTEXPR ansi_color_escape(color_type text_color,
                                  const char* esc) noexcept {
    // If we have a terminal color, we need to output another escape code
    // sequence.
    if (text_color.is_terminal_color()) {
      bool is_background = esc == string_view("\x1b[48;2;");
      uint32_t value = text_color.value();
      // Background ASCII codes are the same as the foreground ones but with
      // 10 more.
      if (is_background) value += 10u;

      buffer[size++] = static_cast<Char>('\x1b');
      buffer[size++] = static_cast<Char>('[');

      if (value >= 100u) {
        buffer[size++] = static_cast<Char>('1');
        value %= 100u;
      }
      buffer[size++] = static_cast<Char>('0' + value / 10u);
      buffer[size++] = static_cast<Char>('0' + value % 10u);

      buffer[size++] = static_cast<Char>('m');
      return;
    }

    for (int i = 0; i < 7; i++) {
      buffer[i] = static_cast<Char>(esc[i]);
    }
    rgb color(text_color.value());
    to_esc(color.r, buffer + 7, ';');
    to_esc(color.g, buffer + 11, ';');
    to_esc(color.b, buffer + 15, 'm');
    size = 19;
  }
  FMT_CONSTEXPR ansi_color_escape(emphasis em) noexcept {
    uint8_t em_codes[num_emphases] = {};
    if (has_emphasis(em, emphasis::bold)) em_codes[0] = 1;
    if (has_emphasis(em, emphasis::faint)) em_codes[1] = 2;
    if (has_emphasis(em, emphasis::italic)) em_codes[2] = 3;
    if (has_emphasis(em, emphasis::underline)) em_codes[3] = 4;
    if (has_emphasis(em, emphasis::blink)) em_codes[4] = 5;
    if (has_emphasis(em, emphasis::reverse)) em_codes[5] = 7;
    if (has_emphasis(em, emphasis::conceal)) em_codes[6] = 8;
    if (has_emphasis(em, emphasis::strikethrough)) em_codes[7] = 9;

    buffer[size++] = static_cast<Char>('\x1b');
    buffer[size++] = static_cast<Char>('[');

    for (size_t i = 0; i < num_emphases; ++i) {
      if (!em_codes[i]) continue;
      buffer[size++] = static_cast<Char>('0' + em_codes[i]);
      buffer[size++] = static_cast<Char>(';');
    }

    buffer[size - 1] = static_cast<Char>('m');
  }
  FMT_CONSTEXPR operator const Char*() const noexcept { return buffer; }

  FMT_CONSTEXPR auto begin() const noexcept -> const Char* { return buffer; }
  FMT_CONSTEXPR auto end() const noexcept -> const Char* {
    return buffer + size;
  }

 private:
  static constexpr size_t num_emphases = 8;
  Char buffer[7u + 4u * num_emphases] = {};
  size_t size = 0;

  static FMT_CONSTEXPR void to_esc(uint8_t c, Char* out,
                                   char delimiter) noexcept {
    out[0] = static_cast<Char>('0' + c / 100);
    out[1] = static_cast<Char>('0' + c / 10 % 10);
    out[2] = static_cast<Char>('0' + c % 10);
    out[3] = static_cast<Char>(delimiter);
  }
  static FMT_CONSTEXPR auto has_emphasis(emphasis em, emphasis mask) noexcept
      -> bool {
    return static_cast<uint8_t>(em) & static_cast<uint8_t>(mask);
  }
};

template <typename Char>
FMT_CONSTEXPR auto make_foreground_color(color_type foreground) noexcept
    -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(foreground, "\x1b[38;2;");
}

template <typename Char>
FMT_CONSTEXPR auto make_background_color(color_type background) noexcept
    -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(background, "\x1b[48;2;");
}

template <typename Char>
FMT_CONSTEXPR auto make_emphasis(emphasis em) noexcept
    -> ansi_color_escape<Char> {
  return ansi_color_escape<Char>(em);
}

template <typename Char> inline void reset_color(buffer<Char>& buffer) {
  auto reset_color = string_view("\x1b[0m");
  buffer.append(reset_color.begin(), reset_color.end());
}

template <typename T> struct styled_arg : view {
  const T& value;
  text_style style;
  FMT_CONSTEXPR styled_arg(const T& v, text_style s) : value(v), style(s) {}
};

template <typename Char>
void vformat_to(buffer<Char>& buf, text_style ts, basic_string_view<Char> fmt,
                basic_format_args<buffered_context<Char>> args) {
  if (ts.has_emphasis()) {
    auto emphasis = make_emphasis<Char>(ts.get_emphasis());
    buf.append(emphasis.begin(), emphasis.end());
  }
  if (ts.has_foreground()) {
    auto foreground = make_foreground_color<Char>(ts.get_foreground());
    buf.append(foreground.begin(), foreground.end());
  }
  if (ts.has_background()) {
    auto background = make_background_color<Char>(ts.get_background());
    buf.append(background.begin(), background.end());
  }
  vformat_to(buf, fmt, args);
  if (ts != text_style()) reset_color<Char>(buf);
}
}  // namespace detail

inline void vprint(FILE* f, text_style ts, string_view fmt, format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
}

/**
 * Formats a string and prints it to the specified file stream using ANSI
 * escape sequences to specify text formatting.
 *
 * **Example**:
 *
 *     fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void print(FILE* f, text_style ts, format_string<T...> fmt, T&&... args) {
  vprint(f, ts, fmt.str, vargs<T...>{{args...}});
}

/**
 * Formats a string and prints it to stdout using ANSI escape sequences to
 * specify text formatting.
 *
 * **Example**:
 *
 *     fmt::print(fmt::emphasis::bold | fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void print(text_style ts, format_string<T...> fmt, T&&... args) {
  return print(stdout, ts, fmt, std::forward<T>(args)...);
}

inline void vprintln(FILE* f, text_style ts, string_view fmt,
                     format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  buf.push_back('\n');
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
}

/**
 * Formats a string and prints it to the specified file stream followed by a
 * newline, using ANSI escape sequences to specify text formatting.
 *
 * **Example**:
 *
 *     fmt::println(fmt::emphasis::bold | fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void println(FILE* f, text_style ts, format_string<T...> fmt, T&&... args) {
  vprintln(f, ts, fmt.str, vargs<T...>{{args...}});
}

/**
 * Formats a string and prints it to stdout followed by a newline, using ANSI
 * escape sequences to specify text formatting.
 *
 * **Example**:
 *
 *     fmt::println(fmt::emphasis::bold | fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template <typename... T>
void println(text_style ts, format_string<T...> fmt, T&&... args) {
  return println(stdout, ts, fmt, std::forward<T>(args)...);
}

inline auto vformat(text_style ts, string_view fmt, format_args args)
    -> std::string {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  return fmt::to_string(buf);
}

/**
 * Formats arguments and returns the result as a string using ANSI escape
 * sequences to specify text formatting.
 *
 * **Example**:
 *
 * ```
 * #include <fmt/color.h>
 * std::string message = fmt::format(fmt::emphasis::bold | fg(fmt::color::red),
 *                                   "The answer is {}", 42);
 * ```
 */
template <typename... T>
inline auto format(text_style ts, format_string<T...> fmt, T&&... args)
    -> std::string {
  return fmt::vformat(ts, fmt.str, vargs<T...>{{args...}});
}

/// Formats a string with the given text_style and writes the output to `out`.
template <typename OutputIt,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
auto vformat_to(OutputIt out, text_style ts, string_view fmt, format_args args)
    -> OutputIt {
  auto&& buf = detail::get_buffer<char>(out);
  detail::vformat_to(buf, ts, fmt, args);
  return detail::get_iterator(buf, out);
}

/**
 * Formats arguments with the given text style, writes the result to the output
 * iterator `out` and returns the iterator past the end of the output range.
 *
 * **Example**:
 *
 *     std::vector<char> out;
 *     fmt::format_to(std::back_inserter(out),
 *                    fmt::emphasis::bold | fg(fmt::color::red), "{}", 42);
 */
template <typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
inline auto format_to(OutputIt out, text_style ts, format_string<T...> fmt,
                      T&&... args) -> OutputIt {
  return vformat_to(out, ts, fmt.str, vargs<T...>{{args...}});
}

template <typename T, typename Char>
struct formatter<detail::styled_arg<T>, Char> : formatter<T, Char> {
  template <typename FormatContext>
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
      auto reset_color = string_view("\x1b[0m");
      out = detail::copy<Char>(reset_color.begin(), reset_color.end(), out);
    }
    return out;
  }
};

/**
 * Returns an argument that will be formatted using ANSI escape sequences,
 * to be used in a formatting function.
 *
 * **Example**:
 *
 *     fmt::print("Elapsed time: {0:.2f} seconds",
 *                fmt::styled(1.23, fmt::fg(fmt::color::green) |
 *                                  fmt::bg(fmt::color::blue)));
 */
template <typename T>
FMT_CONSTEXPR auto styled(const T& value, text_style ts)
    -> detail::styled_arg<remove_cvref_t<T>> {
  return detail::styled_arg<remove_cvref_t<T>>{value, ts};
}

FMT_END_EXPORT
FMT_END_NAMESPACE

#endif  // FMT_COLOR_H_
