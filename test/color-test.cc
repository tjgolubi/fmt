// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "tjg/color.hpp"

#include "standard-format-test.h"
#include "test-support.h"  // EXPECT_WRITE

#include <map>            // std::map
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <tuple>
#include <iterator>       // std::back_inserter

#include <type_traits>  // std::is_convertible, std::is_same
#include <utility>      // std::declval, std::move
#include <limits>       // std::numeric_limits

TEST(color_test, absolute_style_flag) {
  static_assert(!tjg::color::text_style{}.is_absolute(), "styles default to relative");
  static_assert(noexcept(std::declval<tjg::color::text_style&>().absolute()) &&
                    noexcept(tjg::color::text_style{}.is_absolute()),
                "absolute accessors do not throw");
  auto style = tjg::color::fg(tjg::color::term_color::red)
                   .bg(tjg::color::gray_color{12}).underline();
  const auto original = style;
  EXPECT_EQ(&style.absolute(), &style);
  EXPECT_TRUE(style.is_absolute());
  EXPECT_NE(style, original);
  EXPECT_EQ(style.fg(), original.fg());
  EXPECT_EQ(style.bg(), original.bg());
  EXPECT_EQ(style.emph(), original.emph());
  EXPECT_EQ(&style.absolute(false), &style);
  EXPECT_EQ(style, original);
  style.absolute().fg(tjg::color::no_color).bg(tjg::color::no_color).emph(tjg::color::emphasis{});
  EXPECT_FALSE(style.empty());
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
  EXPECT_EQ(&style.clear(), &style);
  EXPECT_EQ(style, tjg::color::text_style{});
  EXPECT_FALSE(style.is_absolute());
}

TEST(color_test, absolute_style_helper) {
  static_assert(noexcept(tjg::color::absolute(tjg::color::text_style{})),
                "absolute style helper does not throw");
  const auto source = tjg::color::fg(tjg::color::term_color::red)
                          .bg(tjg::color::gray_color{12}).underline();
  auto expected = source;
  expected.absolute();
  EXPECT_EQ(tjg::color::absolute(source), expected);
  EXPECT_FALSE(source.is_absolute());
  EXPECT_EQ(tjg::color::absolute(expected), expected);
  EXPECT_EQ(tjg::color::absolute(tjg::color::text_style{}), tjg::color::text_style{}.absolute());
  constexpr auto value = tjg::color::absolute(tjg::color::text_style{});
  static_assert(value.is_absolute(), "absolute helper supports constant evaluation");
}

TEST(color_test, absolute_style_composition) {
  const tjg::color::color_t colors[] = {tjg::color::no_color, tjg::color::rgb{1, 2, 3},
      tjg::color::term_color::red, tjg::color::cube_color{1, 2, 3}, tjg::color::gray_color{12}};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto base = tjg::color::text_style{foreground, background,
                                       tjg::color::emphasis::underline};
      const auto other = tjg::color::text_style{background, foreground,
                                        tjg::color::emphasis::bold};
      for (unsigned flags = 0; flags != 4; ++flags) {
        auto lhs = base;
        auto rhs = other;
        lhs.absolute((flags & 1u) != 0);
        rhs.absolute((flags & 2u) != 0);
        auto expected_overlay = tjg::color::overlay(base, other);
        expected_overlay.absolute(flags != 0);
        EXPECT_EQ(tjg::color::overlay(lhs, rhs), expected_overlay);
      }
    }
  }
  const auto absolute_only = tjg::color::text_style{}.absolute();
  EXPECT_EQ(tjg::color::overlay(tjg::color::text_style{}, absolute_only), absolute_only);
  EXPECT_EQ(tjg::color::overlay(absolute_only, tjg::color::text_style{}), absolute_only);
  auto style = tjg::color::underline().absolute();
  style.overlay(tjg::color::bold());
  EXPECT_TRUE(style.is_absolute());
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_underline());
  style.italic();
  EXPECT_TRUE(style.is_italic());
  EXPECT_TRUE(style.is_absolute());
}

TEST(color_test, absolute_style_blend) {
  const auto full = tjg::color::text_style{tjg::color::rgb{10, 20, 30}, tjg::color::gray_color{12},
                                   tjg::color::emphasis::bold}.absolute();
  auto relative = full;
  relative.absolute(false);
  const auto other = tjg::color::text_style{tjg::color::term_color::blue, tjg::color::no_color,
                                    tjg::color::emphasis::underline};
  const auto expected = tjg::color::blend(relative, other);
  EXPECT_EQ(tjg::color::blend(full, other), expected);
  EXPECT_EQ(tjg::color::blend(other, full), expected);
  EXPECT_EQ(tjg::color::blend(other, relative, full),
            tjg::color::blend(other, relative, relative));
  EXPECT_FALSE(tjg::color::blend(full, other).is_absolute());
  EXPECT_TRUE(full.is_absolute());
  static_assert(noexcept(tjg::color::blend(full, other)), "blend remains noexcept");
  const tjg::color::text_style invalid[] = {
      tjg::color::text_style{}.absolute(),
      tjg::color::fg(tjg::color::rgb::red).absolute(),
      tjg::color::bg(tjg::color::rgb::blue).absolute()};
  for (auto input : invalid) {
#ifdef NDEBUG
    auto cleared = input;
    cleared.absolute(false);
    EXPECT_EQ(tjg::color::blend(input, other), tjg::color::blend(cleared, other));
    EXPECT_EQ(tjg::color::blend(other, input), tjg::color::blend(other, cleared));
    EXPECT_EQ(tjg::color::blend(other, full, input),
              tjg::color::blend(other, relative, cleared));
    EXPECT_FALSE(tjg::color::blend(input, other).is_absolute());
#elif !defined(FMTCOLOR_SKIP)
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::color::blend(input, other),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::color::blend(other, input),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::color::blend(other, full, input),
                              "terminal-default colors");
#else
    (void)input;
#endif
  }
}

TEST(color_test, absolute_style_constexpr) {
  constexpr auto reset = tjg::color::text_style{}.absolute();
  static_assert(reset.is_absolute() && !reset.empty(),
                "a reset-only style is absolute and nonempty");
  constexpr auto full = tjg::color::text_style{tjg::color::rgb{10, 20, 30},
                                       tjg::color::gray_color{12}}.absolute();
  constexpr auto modified = tjg::color::text_style{full}.overlay(tjg::color::bold());
  static_assert(modified.is_absolute() && modified.is_bold(),
                "explicit overlays preserve absolute during evaluation");
  constexpr auto combined = tjg::color::fg(tjg::color::term_color::red).overlay(full);
  static_assert(combined.is_absolute(), "overlay preserves absolute");
  constexpr auto result = tjg::color::blend(full, tjg::color::text_style{});
  static_assert(!result.is_absolute() && result.fg() == full.fg() &&
                    result.bg() == full.bg(),
                "valid absolute blends remain constant-evaluable and relative");
  static_assert([full] {
                  auto style = full;
                  return style.clear().empty() && !style.is_absolute();
                }(),
                "clear removes absolute during evaluation");
}

template <typename T>
auto comparison_key(const T& value) {
  if constexpr (std::is_enum_v<T>)
    return static_cast<uint64_t>(std::to_underlying(value));
  else if constexpr (std::is_same_v<T, tjg::color::rgb>)
    return static_cast<uint64_t>(static_cast<uint32_t>(value));
  else if constexpr (std::is_same_v<T, tjg::color::no_color_t>)
    return uint64_t{0};
  else
    return static_cast<uint64_t>(value.index());
}

auto comparison_key(const tjg::color::color_t& value) {
  return std::pair{value.index(), std::visit([](const auto& color) {
    return comparison_key(color);
  }, value)};
}

auto comparison_key(const tjg::color::text_style& value) {
  return std::tuple{value.is_absolute(), value.emph(), value.fg(), value.bg()};
}

template<typename T>
void check_color_comparisons(const T* values, size_t count) {
  if constexpr (!std::is_same_v<T, tjg::color::color_t>) {
    static_assert(noexcept(T{} == T{}), "equality must be noexcept");
    static_assert(noexcept(T{} != T{}), "inequality must be noexcept");
    static_assert(noexcept(T{} < T{}), "ordering must be noexcept");
    static_assert(noexcept(T{} <= T{}), "ordering must be noexcept");
    static_assert(noexcept(T{} > T{}), "ordering must be noexcept");
    static_assert(noexcept(T{} >= T{}), "ordering must be noexcept");
  }
  static_assert(T{} == T{}, "default values must compare equal");
  static_assert(!(T{} < T{}), "ordering must be irreflexive");
  if constexpr (!std::is_same_v<T, tjg::color::color_t>)
    static_assert(noexcept(T{} <=> T{}), "three-way comparison must be noexcept");
  static_assert(std::is_same<decltype(T{} <=> T{}),
                             std::strong_ordering>::value,
                "integral representations must have a strong ordering");
  static_assert((T{} <=> T{}) == std::strong_ordering::equal,
                "default values must compare equal");

  auto keys = std::map<T, size_t>();
  auto integers = std::map<decltype(comparison_key(T{})), size_t>();
  for (size_t i = 0; i < count; ++i) {
    keys[values[i]] = i;
    integers[comparison_key(values[i])] = i;
    for (size_t j = 0; j < count; ++j) {
      auto lhs = comparison_key(values[i]);
      auto rhs = comparison_key(values[j]);
      EXPECT_EQ(values[i] == values[j], lhs == rhs);
      EXPECT_EQ(values[i] != values[j], lhs != rhs);
      EXPECT_EQ(values[i] < values[j], lhs < rhs);
      EXPECT_EQ(values[i] <= values[j], lhs <= rhs);
      EXPECT_EQ(values[i] > values[j], lhs > rhs);
      EXPECT_EQ(values[i] >= values[j], lhs >= rhs);
      EXPECT_EQ(values[i] <=> values[j], lhs <=> rhs);
    }
  }
  EXPECT_EQ(keys.size(), integers.size());
  auto expected = integers.begin();
  for (const auto& entry : keys) {
    ASSERT_NE(expected, integers.end());
    EXPECT_EQ(comparison_key(entry.first), expected->first);
    EXPECT_EQ(entry.second, expected->second);
    ++expected;
  }
}

TEST(color_test, rgb_comparisons) {
  constexpr auto low = tjg::color::rgb{0, 255, 255};
  constexpr auto high = tjg::color::rgb{1, 0, 0};
  static_assert(low < high, "red precedes green and blue in packed ordering");
  static_assert(tjg::color::rgb{0xAB123456} == tjg::color::rgb{0x123456},
                "the discarded high byte must not affect equality");
  const tjg::color::rgb values[] = {
      {}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 0, 0},
      {0, 255, 255}, {255, 255, 255},
      tjg::color::rgb{0xAB123456}, tjg::color::rgb{0x123456}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, gray_color) {
  using tjg::color::gray_color;
  static_assert(noexcept(std::declval<const gray_color&>().index()));

  static_assert(std::is_same<decltype(gray_color{}.index()),
                             int>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<gray_color, tjg::color::rgb>::value &&
                    !std::is_convertible<int, gray_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(noexcept(static_cast<tjg::color::rgb>(gray_color{})),
                "RGB conversion must be noexcept");
  constexpr gray_color first{0}, last{23};
  static_assert(first.index() == 0 &&
                    last.index() == 23,
                "boundary indices must support C++11 constant evaluation");
  constexpr tjg::color::rgb dark = first, light = last;
  static_assert(dark == tjg::color::rgb{8, 8, 8} && light == tjg::color::rgb{238, 238, 238},
                "grayscale conversion supports C++11 constant evaluation");
  constexpr tjg::color::rgb middle = gray_color{12};
  static_assert(middle == tjg::color::rgb{128, 128, 128},
                "a temporary gray color supports C++11 RGB conversion");
  EXPECT_EQ(gray_color{}.index(), 0);
  const int indices[] = {0, 1, 12, 22, 23};
  const unsigned levels[] = {8, 18, 128, 228, 238};
  for (size_t i = 0; i != sizeof(indices) / sizeof(indices[0]); ++i) {
    SCOPED_TRACE(indices[i]);
    gray_color color{indices[i]};
    tjg::color::rgb converted = color;
    EXPECT_EQ(color.index(), indices[i]);
    EXPECT_EQ(converted.r, levels[i]);
    EXPECT_EQ(converted.g, levels[i]);
    EXPECT_EQ(converted.b, levels[i]);
    auto copy = color;
    gray_color assigned;
    assigned = color;
    EXPECT_EQ(copy, color);
    EXPECT_EQ(assigned, color);
  }
  static_assert(noexcept(gray_color{0}), "gray construction must be noexcept");
#ifdef NDEBUG
  const int invalid[] = {-1, 24, 255, 256,
                         (std::numeric_limits<int>::min)(),
                         (std::numeric_limits<int>::max)()};
  for (auto index : invalid) {
    SCOPED_TRACE(index);
    EXPECT_EQ(gray_color{index}, gray_color{index < 0 ? 0 : 23});
  }
#endif
  const gray_color ordered[] = {gray_color{0}, gray_color{1}, gray_color{23}};
  check_color_comparisons(ordered, sizeof(ordered) / sizeof(ordered[0]));
}

TEST(color_test, cube_color) {
  using tjg::color::cube_color;
  static_assert(noexcept(std::declval<const cube_color&>().index()));

  static_assert(std::is_same<decltype(cube_color{}.index()),
                             int>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<cube_color, tjg::color::rgb>::value &&
                    !std::is_convertible<unsigned, cube_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(std::is_same<decltype(cube_color{}.r()), int>::value &&
                    std::is_same<decltype(cube_color{}.g()), int>::value &&
                    std::is_same<decltype(cube_color{}.b()), int>::value,
                "cube channel getters return int");
  static_assert(noexcept(static_cast<tjg::color::rgb>(cube_color{})),
                "RGB conversion must be noexcept");
  constexpr cube_color first{0}, last{215}, middle{1, 2, 3};
  constexpr tjg::color::rgb converted = middle;
  static_assert(first.index() == 0 &&
                    last.index() == 215 && last.r() == 5 &&
                    last.g() == 5 && last.b() == 5,
                "cube index decoding supports C++11 constant evaluation");
  static_assert(converted == tjg::color::rgb{95, 135, 175} &&
                    middle.index() == 51,
                "cube encoding and RGB conversion support C++11");
  EXPECT_EQ(cube_color{}.index(), 0);
  static_assert(noexcept(cube_color{0}) && noexcept(cube_color{0, 0, 0}),
                "cube construction must be noexcept");
  static_assert(noexcept(std::declval<cube_color&>().r(0)) &&
                    noexcept(std::declval<cube_color&>().g(0)) &&
                    noexcept(std::declval<cube_color&>().b(0)),
                "cube setters must be noexcept");
#ifdef NDEBUG
  const int invalid_indices[] = {(std::numeric_limits<int>::min)(), -1,
                                 216, 217, 251, 255, 256,
                                 (std::numeric_limits<int>::max)()};
  for (auto index : invalid_indices) {
    SCOPED_TRACE(index);
    EXPECT_EQ(cube_color{index}, cube_color{index < 0 ? 0 : 215});
  }
  const int invalid_channels[] = {-1, 6, 255, 256,
                                 (std::numeric_limits<int>::min)(),
                                 (std::numeric_limits<int>::max)()};
  for (auto channel : invalid_channels) {
    SCOPED_TRACE(channel);
    const int clamped = channel < 0 ? 0 : 5;
    EXPECT_EQ((cube_color{channel, 2, 3}), (cube_color{clamped, 2, 3}));
    EXPECT_EQ((cube_color{1, channel, 3}), (cube_color{1, clamped, 3}));
    EXPECT_EQ((cube_color{1, 2, channel}), (cube_color{1, 2, clamped}));
    auto color = middle;
    EXPECT_EQ(&color.r(channel), &color);
    EXPECT_EQ(color, (cube_color{clamped, 2, 3}));
    color = middle;
    EXPECT_EQ(&color.g(channel), &color);
    EXPECT_EQ(color, (cube_color{1, clamped, 3}));
    color = middle;
    EXPECT_EQ(&color.b(channel), &color);
    EXPECT_EQ(color, (cube_color{1, 2, clamped}));
  }
#endif
  auto color = middle;
  EXPECT_EQ(&color.r(5), &color);
  EXPECT_EQ(color, (cube_color{5, 2, 3}));
  EXPECT_EQ(&color.g(0), &color);
  EXPECT_EQ(color, (cube_color{5, 0, 3}));
  EXPECT_EQ(&color.b(4), &color);
  EXPECT_EQ(color, (cube_color{5, 0, 4}));
  cube_color assigned;
  assigned = color;
  EXPECT_EQ(assigned, color);
  const cube_color ordered[] = {cube_color{0}, cube_color{1}, cube_color{5},
                                cube_color{6}, cube_color{35}, cube_color{36},
                                cube_color{215}};
  check_color_comparisons(ordered, sizeof(ordered) / sizeof(ordered[0]));
}

TEST(color_test, cube_color_xterm_rgb) {
  // Independent truth source: Xterm FAQ, "Can I set a color by its number?"
  // https://invisible-island.net/xterm/xterm.faq.html#color_by_number
  // Use its arithmetic formula, never the implementation's cube_levels table.
  // Our zero-based index omits the SGR palette's offset of 16.
  for (int red = 0; red != 6; ++red) {
    for (int green = 0; green != 6; ++green) {
      for (int blue = 0; blue != 6; ++blue) {
        auto index = 36 * red + 6 * green + blue;
        SCOPED_TRACE(index);
        tjg::color::cube_color color{index};
        EXPECT_EQ(color.r(), red);
        EXPECT_EQ(color.g(), green);
        EXPECT_EQ(color.b(), blue);
        EXPECT_EQ(color.index(), index);
        EXPECT_EQ(color, (tjg::color::cube_color{red, green, blue}));
        tjg::color::rgb converted = color;
        EXPECT_EQ(converted.r, red   == 0 ? 0 : 55 + 40 * red);
        EXPECT_EQ(converted.g, green == 0 ? 0 : 55 + 40 * green);
        EXPECT_EQ(converted.b, blue  == 0 ? 0 : 55 + 40 * blue);
      }
    }
  }
}

TEST(color_test, terminal_color_xterm_rgb) {
  // Agreed Xterm-default palette, independent of the implementation table.
  const tjg::color::rgb expected[] = {
      {0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
      {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
      {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
      {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
  const tjg::color::rgb named[] = {
      tjg::color::rgb::term_black, tjg::color::rgb::term_red, tjg::color::rgb::term_green,
      tjg::color::rgb::term_yellow, tjg::color::rgb::term_blue, tjg::color::rgb::term_magenta,
      tjg::color::rgb::term_cyan, tjg::color::rgb::term_white,
      tjg::color::rgb::term_bright_black, tjg::color::rgb::term_bright_red,
      tjg::color::rgb::term_bright_green, tjg::color::rgb::term_bright_yellow,
      tjg::color::rgb::term_bright_blue, tjg::color::rgb::term_bright_magenta,
      tjg::color::rgb::term_bright_cyan, tjg::color::rgb::term_bright_white};
  static_assert(std::is_convertible<tjg::color::term_color, tjg::color::rgb>::value,
                "terminal-to-RGB conversion must be implicit");
  static_assert(std::is_same<std::underlying_type<tjg::color::term_color>::type,
                             int8_t>::value,
                "terminal enum storage must be signed");
  static_assert(std::to_underlying(static_cast<tjg::color::term_color>(-1)) == -1,
                "underlying conversion must preserve signed enum values");
  constexpr tjg::color::rgb black = tjg::color::term_color::black;
  constexpr tjg::color::rgb bright = tjg::color::term_color::bright_white;
  static_assert(black == tjg::color::rgb{} && bright == tjg::color::rgb{255, 255, 255},
                "terminal conversion supports C++11 constant evaluation");
  for (unsigned index = 0; index != 16; ++index) {
    SCOPED_TRACE(index);
    tjg::color::rgb converted = static_cast<tjg::color::term_color>(index);
    EXPECT_EQ(converted, expected[index]);
    EXPECT_EQ(named[index], expected[index]);
  }
}

TEST(color_test, color_type_comparisons) {
  using tjg::color::color_t;
  constexpr auto low = color_t{tjg::color::rgb{}};
  constexpr auto high = color_t{tjg::color::term_color::black};
  static_assert(color_t{} < low, "unspecified differs from RGB black");
  static_assert(low < high, "RGB black differs from terminal black");
  const color_t values[] = {
      {}, tjg::color::no_color, tjg::color::rgb{}, tjg::color::term_color::black,
      tjg::color::rgb{0xFFFFFF}, tjg::color::term_color::bright_white,
      tjg::color::rgb{0x123456}, tjg::color::rgb::coral, tjg::color::rgb{0xFF7F50},
      tjg::color::cube_color{0}, tjg::color::cube_color{215}, tjg::color::gray_color{0},
      tjg::color::gray_color{23}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_comparisons) {
  constexpr auto low = tjg::color::text_style{};
  constexpr auto high = tjg::color::text_style{tjg::color::emphasis::bold};
  static_assert(low < high, "an empty style sorts before bold");
  const tjg::color::text_style values[] = {
      {}, tjg::color::fg(tjg::color::no_color), tjg::color::fg(tjg::color::rgb{}),
      tjg::color::fg(tjg::color::term_color::black), tjg::color::bg(tjg::color::rgb{}),
      tjg::color::bg(tjg::color::term_color::black), tjg::color::fg(tjg::color::rgb{0xFFFFFF}),
      tjg::color::emphasis::bold, tjg::color::emphasis::strike,
      tjg::color::fg(tjg::color::rgb::coral).bg(tjg::color::term_color::bright_white)
          .bold().strike(),
      tjg::color::fg(tjg::color::cube_color{0}), tjg::color::bg(tjg::color::cube_color{215}),
      tjg::color::fg(tjg::color::gray_color{0}), tjg::color::bg(tjg::color::gray_color{23})};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

template<typename T>
void check_color_hashes(const T* values, size_t count) {
  static_assert(noexcept(std::hash<T>{}(T{})), "hashing must be noexcept");
  static_assert(std::is_same<decltype(std::hash<T>{}(T{})), size_t>::value,
                "hashing must return size_t");
  auto hasher = std::hash<T>();
  auto keys = std::unordered_set<T>();
  auto entries = std::unordered_map<T, size_t>();
  auto integers = std::map<decltype(comparison_key(T{})), size_t>();
  for (size_t i = 0; i < count; ++i) {
    keys.insert(values[i]);
    entries[values[i]] = i;
    integers[comparison_key(values[i])] = i;
    for (size_t j = 0; j < count; ++j) {
      if (values[i] == values[j]) {
        EXPECT_EQ(hasher(values[i]), hasher(values[j]));
      }
    }
  }
  EXPECT_EQ(keys.size(), integers.size());
  EXPECT_EQ(entries.size(), integers.size());
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(keys.count(values[i]), size_t{1});
    EXPECT_EQ(entries.at(values[i]), integers.at(comparison_key(values[i])));
  }
}

TEST(color_test, rgb_hash) {
  const tjg::color::rgb values[] = {
      {}, {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 255, 255},
      tjg::color::rgb{0xAB123456}, tjg::color::rgb{0x123456},
      tjg::color::rgb::coral, tjg::color::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, color_type_hash) {
  using tjg::color::color_t;
  const color_t values[] = {
      {}, tjg::color::no_color, tjg::color::rgb{}, tjg::color::term_color::black,
      tjg::color::rgb{0xFFFFFF}, tjg::color::term_color::bright_white,
      tjg::color::rgb::coral, tjg::color::rgb{0xFF7F50}, tjg::color::cube_color{0},
      tjg::color::cube_color{215}, tjg::color::gray_color{0}, tjg::color::gray_color{23}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
  const auto hash = std::hash<color_t>{};
  // Same numeric payload, different alternatives must influence the hash.
  EXPECT_NE(hash(color_t{tjg::color::rgb{}}), hash(color_t{tjg::color::term_color::black}));
  EXPECT_NE(hash(color_t{tjg::color::cube_color{0}}), hash(color_t{tjg::color::gray_color{0}}));
}

TEST(color_test, text_style_hash) {
  const tjg::color::text_style values[] = {
      {}, tjg::color::absolute(tjg::color::text_style{}),
      tjg::color::absolute(tjg::color::fg(tjg::color::rgb::coral)), tjg::color::fg(tjg::color::no_color),
      tjg::color::fg(tjg::color::rgb{}),
      tjg::color::fg(tjg::color::term_color::black), tjg::color::bg(tjg::color::rgb{}),
      tjg::color::bg(tjg::color::term_color::black), tjg::color::emphasis::bold,
      tjg::color::emphasis::strike, tjg::color::fg(tjg::color::rgb::coral),
      tjg::color::fg(tjg::color::rgb{0xFF7F50}),
      tjg::color::fg(tjg::color::rgb::coral).bg(tjg::color::term_color::bright_white)
          .bold().strike(),
      tjg::color::fg(tjg::color::cube_color{0}), tjg::color::bg(tjg::color::cube_color{215}),
      tjg::color::fg(tjg::color::gray_color{0}), tjg::color::bg(tjg::color::gray_color{23})};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, emphasis_or_assign) {
  using tjg::color::emphasis;
  static_assert(std::is_same<decltype(std::declval<emphasis&>() |=
                                     emphasis::bold), emphasis&>::value,
                "compound assignment must return an emphasis reference");
  static_assert(noexcept(std::declval<emphasis&>() |= emphasis::bold),
                "compound assignment must be noexcept");
  for (unsigned lhs = 0; lhs < 256; ++lhs) {
    for (unsigned rhs = 0; rhs < 256; ++rhs) {
      auto value = static_cast<emphasis>(lhs);
      const auto other = static_cast<emphasis>(rhs);
      auto& result = (value |= other);
      EXPECT_EQ(std::to_underlying(value), lhs | rhs);
      EXPECT_EQ(&result, &value);
      EXPECT_EQ(std::to_underlying(other), rhs);
    }
  }
  auto value = emphasis::bold;
  (value |= emphasis::underline) |= emphasis::strike;
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
  tjg::color::operator|=(value, value);
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
}

constexpr auto check_emphasis_or_assign() -> bool {
  auto value = tjg::color::emphasis::bold;
  auto& result = (value |= tjg::color::emphasis::underline);
  (result |= tjg::color::emphasis::strike) |= tjg::color::emphasis::none;
  tjg::color::operator|=(value, value);
  return &result == &value &&
         value == (tjg::color::emphasis::bold | tjg::color::emphasis::underline |
                   tjg::color::emphasis::strike);
}

TEST(color_test, emphasis_or_assign_constexpr) {
  static_assert(check_emphasis_or_assign(),
                "compound assignment must support constant evaluation");
}

TEST(color_test, numeric_conversions) {
  static_assert(std::is_same<std::underlying_type<tjg::color::term_color>::type,
                             int8_t>::value, "terminal color underlying type");
  static_assert(std::is_same<std::underlying_type<tjg::color::emphasis>::type,
                             uint8_t>::value, "emphasis underlying type");

  static_assert(std::is_same<decltype(std::to_underlying(
                                 tjg::color::term_color::black)),
                             int8_t>::value, "terminal color return type");
  static_assert(std::is_same<decltype(std::to_underlying(tjg::color::emphasis::bold)),
                             uint8_t>::value, "emphasis return type");
  static_assert(std::is_same<decltype(static_cast<uint32_t>(tjg::color::rgb{})),
                             uint32_t>::value, "RGB return type");

  static_assert(noexcept(std::to_underlying(tjg::color::term_color::black)),
                "terminal color conversion must be noexcept");
  static_assert(noexcept(std::to_underlying(tjg::color::emphasis::bold)),
                "emphasis conversion must be noexcept");
  static_assert(noexcept(static_cast<uint32_t>(tjg::color::rgb{})),
                "RGB conversion must be noexcept");

  static_assert(std::to_underlying(tjg::color::term_color::black) ==
                    static_cast<uint8_t>(tjg::color::term_color::black),
                "terminal color conversion must preserve the enum value");
  static_assert(std::to_underlying(tjg::color::term_color::bright_white) ==
                    static_cast<uint8_t>(tjg::color::term_color::bright_white),
                "bright terminal color conversion must preserve the enum value");
  static_assert(std::to_underlying(tjg::color::emphasis::none) == 0);
  static_assert(tjg::color::emphasis{} == tjg::color::emphasis::none);
  static_assert(tjg::color::text_style{tjg::color::emphasis::none}.empty());
  static_assert(std::to_underlying(tjg::color::emphasis::bold) == 1,
                "bold must retain its bit");
  static_assert(std::to_underlying(tjg::color::emphasis::strike) == 128,
                "strike must retain its bit");
  static_assert(static_cast<uint32_t>(tjg::color::rgb{0xAB123456}) == 0x123456,
                "packed RGB must have a zero high byte");
  static_assert(static_cast<uint32_t>(tjg::color::rgb{}) == 0,
                "default RGB must be black");
  const auto style = tjg::color::fg(tjg::color::rgb{0x123456})
                         .bg(tjg::color::term_color::black)
                         .emph(tjg::color::emphasis::bold | tjg::color::emphasis::strike);
  EXPECT_EQ(style.fg(), tjg::color::color_t{tjg::color::rgb{0x123456}});
  EXPECT_EQ(style.bg(), tjg::color::color_t{tjg::color::term_color::black});
  EXPECT_EQ(style.emph(), tjg::color::emphasis::bold | tjg::color::emphasis::strike);
}

namespace {
struct blend_sample {
  tjg::color::color_t color;
  tjg::color::rgb promoted;
};

const blend_sample blend_samples[] = {
    {tjg::color::no_color, {0, 0, 0}},
    {tjg::color::rgb{1, 2, 3}, {1, 2, 3}},
    {tjg::color::term_color::red, {205, 0, 0}},
    {tjg::color::cube_color{1, 2, 3}, {95, 135, 175}},
    {tjg::color::gray_color{12}, {128, 128, 128}}};

// Expected RGB values above are independent of the conversion implementation.
}  // namespace

TEST(color_test, color_sentinels) {
  using tjg::color::no_color_t;
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER <= 1951
  static_assert(sizeof(no_color_t) == sizeof(char),
                "the MSVC sentinel workaround adds only one byte");
#else
  static_assert(std::is_empty<no_color_t>::value,
                "the unspecified sentinel has no payload");
#endif
  static_assert(no_color_t{} == no_color_t{} &&
                    (no_color_t{} <=> no_color_t{}) == 0,
                "default sentinel values must compare equal");
  static_assert(std::is_default_constructible<no_color_t>::value,
                "the unspecified sentinel must be constructible");
  static_assert(std::is_same_v<
                tjg::color::color_t, std::variant<no_color_t, tjg::color::rgb, tjg::color::term_color,
                                           tjg::color::cube_color, tjg::color::gray_color>>);
}

TEST(color_test, text_style_empty_and_clear) {
  constexpr auto empty = tjg::color::text_style{};
  static_assert(empty.empty(), "default style must be empty");
  static_assert(!tjg::color::text_style{tjg::color::rgb{}}.empty(),
                "specified RGB black must not be empty");
  static_assert(!tjg::color::text_style{tjg::color::term_color::black}.empty(),
                "specified terminal black must not be empty");
  static_assert(!tjg::color::text_style{tjg::color::emphasis::strike}.empty(),
                "emphasis alone must not be empty");
  static_assert(noexcept(empty.empty()) &&
                    noexcept(std::declval<tjg::color::text_style&>().clear()),
                "empty and clear must be noexcept");
  static_assert(std::is_same<decltype(empty.empty()), bool>::value &&
                    std::is_same<decltype(std::declval<tjg::color::text_style&>().clear()),
                                 tjg::color::text_style&>::value,
                "empty returns bool and clear returns a style reference");

  const tjg::color::color_t colors[] = {tjg::color::no_color, tjg::color::rgb{},
      tjg::color::term_color::black, tjg::color::cube_color{0}, tjg::color::gray_color{0}};
  const tjg::color::emphasis attributes[] = {
      tjg::color::emphasis{}, tjg::color::emphasis::bold, tjg::color::emphasis::strike,
      static_cast<tjg::color::emphasis>(255)};
  for (size_t fg = 0; fg < 5; ++fg) {
    for (size_t bg = 0; bg < 5; ++bg) {
      for (auto attribute : attributes) {
        auto style = tjg::color::text_style{colors[fg], colors[bg], attribute};
        EXPECT_EQ(style.empty(),
                  fg == 0 && bg == 0 && attribute == tjg::color::emphasis{});
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        EXPECT_EQ(style, tjg::color::text_style{});
        EXPECT_FALSE(style.has_fg());
        EXPECT_FALSE(style.has_bg());
        EXPECT_FALSE(style.has_emph());
        EXPECT_EQ(tjg::color::format(style, "plain"), "plain");
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        style.clear().fg(tjg::color::gray_color{12}).underline();
        EXPECT_EQ(style, (tjg::color::text_style{tjg::color::gray_color{12}, tjg::color::no_color,
                                         tjg::color::emphasis::underline}));
      }
    }
  }
}

TEST(color_test, text_style_overlay) {
  using tjg::color::emphasis;
  using tjg::color::text_style;
  static_assert(noexcept(std::declval<text_style&>().overlay(
                    std::declval<const text_style&>())),
                "overlay must be noexcept");
  static_assert(std::is_same<decltype(std::declval<text_style&>().overlay(
                                std::declval<const text_style&>())),
                             text_style&>::value,
                "overlay must return a style reference");
  static_assert(noexcept(tjg::color::overlay(text_style{}, text_style{})),
                "free overlay must be noexcept");
  static_assert(std::is_same<decltype(tjg::color::overlay(text_style{}, text_style{})),
                             text_style>::value,
                "free overlay must return a style value");

  const tjg::color::color_t base_colors[] = {
      tjg::color::no_color, tjg::color::rgb{200, 10, 30}, tjg::color::term_color::red,
      tjg::color::cube_color{5, 4, 3}, tjg::color::gray_color{23}};
  const tjg::color::color_t overlay_colors[] = {
      tjg::color::no_color, tjg::color::rgb{}, tjg::color::term_color::black,
      tjg::color::cube_color{0}, tjg::color::gray_color{0}};
  for (auto base_fg : base_colors) {
    for (auto base_bg : base_colors) {
      const auto base = text_style{base_fg, base_bg, emphasis::bold};
      for (size_t fg = 0; fg < 5; ++fg) {
        for (size_t bg = 0; bg < 5; ++bg) {
          const auto other = text_style{overlay_colors[fg], overlay_colors[bg],
                                       emphasis::faint};
          const auto saved = other;
          auto input = base;
          const auto combined = tjg::color::overlay(input, other);
          EXPECT_EQ(combined,
                    (text_style{fg == 0 ? base_fg : overlay_colors[fg],
                                bg == 0 ? base_bg : overlay_colors[bg],
                                emphasis::bold | emphasis::faint}));
          EXPECT_EQ(input, base);
          EXPECT_EQ(other, saved);
          auto style = base;
          EXPECT_EQ(&style.overlay(other), &style);
          EXPECT_EQ(style, (text_style{fg == 0 ? base_fg : overlay_colors[fg],
                                      bg == 0 ? base_bg : overlay_colors[bg],
                                      emphasis::bold | emphasis::faint}));
          EXPECT_EQ(other, saved);
        }
      }
      auto style = base;
      EXPECT_EQ(tjg::color::overlay(base, text_style{}), base);
      EXPECT_EQ(tjg::color::overlay(text_style{}, base), base);
      EXPECT_EQ(tjg::color::overlay(base, base), base);
      EXPECT_EQ(&style.overlay(text_style{}), &style);
      EXPECT_EQ(style, base);
      EXPECT_EQ(&style.overlay(style), &style);
      EXPECT_EQ(style, base);
      EXPECT_EQ(&style.overlay(base), &style);
      EXPECT_EQ(style, base);
      auto empty = text_style{};
      EXPECT_EQ(&empty.overlay(base), &empty);
      EXPECT_EQ(empty, base);
    }
  }
  auto empty = text_style{};
  EXPECT_EQ(tjg::color::overlay(empty, empty), empty);
  EXPECT_EQ(&empty.overlay(empty), &empty);
  EXPECT_TRUE(empty.empty());
  auto style = text_style{tjg::color::term_color::red, tjg::color::gray_color{12},
                          emphasis::bold};
  style.overlay(text_style{tjg::color::rgb{}}).underline();
  EXPECT_EQ(style, (text_style{tjg::color::rgb{}, tjg::color::gray_color{12},
                              emphasis::bold | emphasis::underline}));
  const auto first = text_style{tjg::color::term_color::red};
  const auto second = text_style{tjg::color::term_color::blue};
  EXPECT_EQ(tjg::color::overlay(first, second), second);
  EXPECT_EQ(tjg::color::overlay(second, first), first);
}

TEST(color_test, text_style_overlay_constexpr) {
  constexpr auto base = tjg::color::text_style{tjg::color::term_color::red,
                                       tjg::color::gray_color{12}, tjg::color::emphasis::bold};
  constexpr auto overlaid = tjg::color::text_style{base}.overlay(
      tjg::color::text_style{tjg::color::rgb{}, tjg::color::no_color, tjg::color::emphasis::faint});
  static_assert(overlaid == tjg::color::text_style{
                                tjg::color::rgb{}, tjg::color::gray_color{12},
                                tjg::color::emphasis::bold | tjg::color::emphasis::faint},
                "overlay must support constant evaluation");
  constexpr auto unchanged = tjg::color::text_style{base}.overlay(tjg::color::text_style{});
  static_assert(unchanged == base, "an empty overlay must preserve the style");
  constexpr auto copied = tjg::color::text_style{}.overlay(base);
  static_assert(copied == base, "overlay must replace an empty base");
  constexpr auto equal = tjg::color::text_style{base}.overlay(base);
  static_assert(equal == base, "an equal overlay must preserve the style");
  constexpr auto overrides = tjg::color::text_style{tjg::color::rgb{}, tjg::color::no_color,
                                            tjg::color::emphasis::faint};
  constexpr auto combined = tjg::color::overlay(base, overrides);
  static_assert(combined == overlaid,
                "free overlay must support constant evaluation");
  static_assert(tjg::color::overlay(base, tjg::color::text_style{}) == base &&
                    tjg::color::overlay(tjg::color::text_style{}, base) == base &&
                    tjg::color::overlay(base, base) == base,
                "free overlay must preserve empty and equal cases");
  static_assert(std::get<tjg::color::term_color>(base.fg()) == tjg::color::term_color::red &&
                    std::get<tjg::color::rgb>(overrides.fg()) == tjg::color::rgb{},
                "free overlay must preserve its inputs");
}

template <typename Left, typename Right>
concept has_bitwise_or = requires(Left lhs, Right rhs) { lhs | rhs; };

template <typename Left, typename Right>
concept has_bitwise_or_assign = requires(Left lhs, Right rhs) { lhs |= rhs; };

TEST(color_test, bitwise_composition_constraints) {
  using tjg::color::text_style;
  using tjg::color::emphasis;
  static_assert(!has_bitwise_or<text_style, text_style>);
  static_assert(!has_bitwise_or_assign<text_style, text_style>);
  static_assert(!has_bitwise_or<text_style, emphasis>);
  static_assert(!has_bitwise_or<emphasis, text_style>);
  static_assert(!has_bitwise_or_assign<text_style, emphasis>);
  static_assert(has_bitwise_or<emphasis, emphasis>);
  static_assert(has_bitwise_or_assign<emphasis, emphasis>);
  static_assert(!has_bitwise_or<tjg::color::color_t, tjg::color::color_t>);
  static_assert(!has_bitwise_or_assign<tjg::color::color_t, tjg::color::color_t>);
}

TEST(color_test, palette_style_replacement) {
  const tjg::color::color_t replacements[] = {
      tjg::color::gray_color{23}, tjg::color::cube_color{215}, tjg::color::term_color::black,
      tjg::color::rgb{}, tjg::color::no_color};
  auto style = tjg::color::text_style{tjg::color::gray_color{0}, tjg::color::gray_color{23},
                              static_cast<tjg::color::emphasis>(0xFF)};
  for (auto replacement : replacements) {
    const auto background = style.bg();
    EXPECT_EQ(&style.fg(replacement), &style);
    EXPECT_EQ(style.fg(), replacement);
    EXPECT_EQ(style.bg(), background);
    EXPECT_EQ(style.has_fg(), replacement.index() != 0);
    EXPECT_EQ(std::to_underlying(style.emph()), 0xFF);
  }
  style.fg(tjg::color::gray_color{23});
  for (auto replacement : replacements) {
    EXPECT_EQ(&style.bg(replacement), &style);
    EXPECT_EQ(style.bg(), replacement);
    EXPECT_EQ(style.fg(), tjg::color::color_t{tjg::color::gray_color{23}});
    EXPECT_EQ(style.has_bg(), replacement.index() != 0);
    EXPECT_EQ(std::to_underlying(style.emph()), 0xFF);
  }
  style.fg(tjg::color::no_color).emph(tjg::color::emphasis{});
  EXPECT_EQ(style, tjg::color::text_style{});
}

namespace {
void check_to_sgr(const tjg::color::text_style& style, const std::string& expected) {
  static_assert(std::is_same<decltype(tjg::color::to_sgr(style)), std::string>::value,
                "the default character type must be char");
  const auto original = style;
  EXPECT_EQ(tjg::color::to_sgr(style), expected);
  auto wide = tjg::color::basic_sgr_buffer<wchar_t>{};
  tjg::color::detail::append_sgr(wide, style);
  EXPECT_EQ(wide.view(), (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original);
}

template<typename Char>
std::basic_string<Char> color_escape(tjg::color::color_t color, bool background) {
  const auto escape = tjg::color::detail::ansi_color_escape{color, background};
  return std::basic_string<Char>{escape.begin(), escape.end()};
}
}  // namespace

namespace {
void check_sgr_transition(const tjg::color::text_style& target,
                          const tjg::color::text_style& initial,
                          const std::string& expected) {
  const auto target_copy = target;
  const auto initial_copy = initial;
  EXPECT_EQ(tjg::color::to_sgr(target, initial), expected);
  auto wide = tjg::color::basic_sgr_buffer<wchar_t>{};
  tjg::color::detail::append_sgr_transition(wide, target, initial);
  EXPECT_EQ(wide.view(), (std::wstring{expected.begin(), expected.end()}));
  // The initial value describes a complete state regardless of its flag.
  auto absolute_initial = initial;
  absolute_initial.absolute();
  EXPECT_EQ(tjg::color::to_sgr(target, absolute_initial), expected);
  wide.clear();
  tjg::color::detail::append_sgr_transition(wide, target, absolute_initial);
  EXPECT_EQ(wide.view(), (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(target, target_copy);
  EXPECT_EQ(initial, initial_copy);
}
} // namespace

TEST(color_test, sgr_transition) {
  check_sgr_transition({}, {}, "");
  const auto red = tjg::color::fg(tjg::color::term_color::red);
  check_sgr_transition(red, red, "");
  check_sgr_transition({}, red, "\x1b[0m");
  check_sgr_transition(tjg::color::bold(red), red,
      "\x1b[1m");
  check_sgr_transition(tjg::color::fg(tjg::color::term_color::blue), red,
      "\x1b[34m");
  check_sgr_transition(tjg::color::fg(tjg::color::rgb{1, 2, 3}), red,
      "\x1b[38;2;1;2;3m");
  check_sgr_transition(tjg::color::bg(tjg::color::cube_color{0}), red,
      "\x1b[0;48;5;16m");
  check_sgr_transition(tjg::color::fg(tjg::color::gray_color{0}), {},
      "\x1b[38;5;232m");
  check_sgr_transition(tjg::color::bold(), {}, "\x1b[1m");
  check_sgr_transition(red, {}, "\x1b[31m");
  check_sgr_transition(tjg::color::bold(red), {},
      "\x1b[1;31m");
  check_sgr_transition(tjg::color::fg(tjg::color::rgb{1, 2, 3}), {},
      "\x1b[38;2;1;2;3m");
  // Identical RGB appearance does not erase a palette-kind transition.
  check_sgr_transition(tjg::color::fg(tjg::color::rgb::black),
      tjg::color::fg(tjg::color::term_color::black),
      "\x1b[38;2;0;0;0m");
  check_sgr_transition(tjg::color::bold(), tjg::color::faint(),
      "\x1b[0;1m");
  check_sgr_transition({}, tjg::color::bold(), "\x1b[0m");

  const auto all = static_cast<tjg::color::emphasis>(0xff);
  // Missing target colors select terminal defaults after a reset.
  const auto initial = tjg::color::text_style{all, tjg::color::term_color::red,
                                      tjg::color::term_color::blue};
  check_sgr_transition({}, initial, "\x1b[0m");
  // Clearing intensity reapplies the surviving bit.
  const auto target = tjg::color::text_style{static_cast<tjg::color::emphasis>(0xfe),
                                     initial.fg(), initial.bg()};
  check_sgr_transition(target, initial,
      "\x1b[22;2m");
  check_sgr_transition(tjg::color::text_style{static_cast<tjg::color::emphasis>(0xfd),
                                      initial.fg(), initial.bg()}, initial,
      "\x1b[22;1m");
}

TEST(color_test, sgr_transition_terminal_defaults) {
  const auto fg = tjg::color::rgb{12, 34, 56};
  const auto bg = tjg::color::rgb{78, 90, 123};
  const auto initial = tjg::color::text_style{fg, bg, tjg::color::emphasis::bold};
  // Clearing a channel preserves the other channel and emphasis.
  check_sgr_transition(tjg::color::text_style{tjg::color::no_color, bg, tjg::color::emphasis::bold},
      initial, "\x1b[39m");
  check_sgr_transition(tjg::color::text_style{fg, tjg::color::no_color, tjg::color::emphasis::bold},
      initial, "\x1b[49m");
  // Ties prefer selective changes.
  const auto palette = tjg::color::text_style{tjg::color::term_color::red,
                                      tjg::color::term_color::blue};
  check_sgr_transition(tjg::color::fg(tjg::color::term_color::red), palette,
      "\x1b[49m");
  check_sgr_transition(tjg::color::bg(tjg::color::term_color::blue), palette,
      "\x1b[39m");
  // When both channels and emphasis are removed, a full reset is shortest.
  check_sgr_transition({}, initial, "\x1b[0m");
  check_sgr_transition(tjg::color::bold(), initial,
      "\x1b[0;1m");
}

TEST(color_test, sgr_transition_absolute_target) {
  const tjg::color::text_style states[] = {
      {}, tjg::color::underline(), tjg::color::fg(tjg::color::term_color::red),
      tjg::color::text_style{tjg::color::rgb{1, 2, 3}, tjg::color::gray_color{12},
                      tjg::color::emphasis::bold}};
  for (auto target : states) {
    target.absolute();
    // Absolute targets always reset, including from defaults.
    for (const auto& initial : states)
      check_sgr_transition(target, initial, tjg::color::to_sgr(target));
    // Equal complete states still require the explicit absolute reset.
    check_sgr_transition(target, target, tjg::color::to_sgr(target));
  }
}

TEST(color_test, sgr_transition_all_emphasis_pairs) {
  // Interpret the emitted ANSI independently of the transition algorithm.
  const unsigned on[] = {1, 2, 3, 4, 5, 7, 8, 9};
  const unsigned off[] = {22, 22, 23, 24, 25, 27, 28, 29};
  for (unsigned before = 0; before < 256; ++before) {
    for (unsigned after = 0; after < 256; ++after) {
      const auto initial = tjg::color::text_style{static_cast<tjg::color::emphasis>(before)};
      const auto target = tjg::color::text_style{static_cast<tjg::color::emphasis>(after)};
      const auto sgr = tjg::color::to_sgr(target, initial);
      if (before == after) {
        EXPECT_TRUE(sgr.empty());
        continue;
      }
      ASSERT_GE(sgr.size(), size_t{4});
      unsigned state = before;
      size_t i = 0;
      // Interpret the sequence from the initial terminal state.
      while (i < sgr.size()) {
        ASSERT_LT(i + 1, sgr.size());
        ASSERT_EQ(sgr[i++], '\x1b');
        ASSERT_EQ(sgr[i++], '[');
        unsigned code = 0;
        for (;;) {
          ASSERT_LT(i, sgr.size());
          const auto c = sgr[i++];
          if (c >= '0' && c <= '9') {
            code = code * 10 + static_cast<unsigned>(c - '0');
            continue;
          }
          ASSERT_TRUE(c == ';' || c == 'm');
          bool recognized = code == 0;
          if (code == 0) state = 0;
          for (unsigned bit = 0; bit < 8; ++bit) {
            if (code == on[bit]) { state |= 1u << bit; recognized = true; }
            if (code == off[bit]) { state &= ~(1u << bit); recognized = true; }
          }
          ASSERT_TRUE(recognized);
          code = 0;
          if (c == 'm') break;
        }
      }
      EXPECT_EQ(state, after) << "initial=" << before << " target=" << after;
      // Compact output stays combined and no longer than reset-and-rebuild.
      EXPECT_EQ(sgr.find('\x1b', 1), std::string::npos);
      size_t absolute_size = 4; // ESC [ 0 m
      for (unsigned bit = 0; bit < 8; ++bit)
        if (after & (1u << bit)) absolute_size += 2;
      EXPECT_LE(sgr.size(), absolute_size);
    }
  }
}

TEST(color_test, palette_ansi_escape) {
  const struct {
    tjg::color::color_t color;
    const char* foreground;
    const char* background;
  } samples[] = {
      {tjg::color::no_color, "", ""},
      {tjg::color::rgb{1, 2, 3}, "38;2;1;2;3;", "48;2;1;2;3;"},
      {tjg::color::term_color::red, "31;", "41;"},
      {tjg::color::term_color::bright_white, "97;", "107;"},
      {tjg::color::cube_color{0}, "38;5;16;", "48;5;16;"},
      {tjg::color::cube_color{215}, "38;5;231;", "48;5;231;"},
      {tjg::color::gray_color{0}, "38;5;232;", "48;5;232;"},
      {tjg::color::gray_color{23}, "38;5;255;", "48;5;255;"}};
  for (const auto& sample : samples) {
    for (bool background : {false, true}) {
      const auto expected = std::string{background ? sample.background
                                                  : sample.foreground};
      EXPECT_EQ(color_escape<char>(sample.color, background), expected);
      EXPECT_EQ(color_escape<wchar_t>(sample.color, background),
                (std::wstring{expected.begin(), expected.end()}));
      const auto style =
          background ? tjg::color::bg(sample.color) : tjg::color::fg(sample.color);
      const auto sgr = expected.empty()
                           ? std::string{}
                           : "\x1b[" + expected.substr(0, expected.size() - 1) + "m";
      check_to_sgr(style, sgr);
      EXPECT_EQ(tjg::color::format(style, "value"),
                sgr + "value" + (sgr.empty() ? "" : "\x1b[0m"));
    }
  }
}

namespace {
template<typename T, size_t Index>
void check_color_access(T value) {
  using tjg::color::color_t;
  static_assert(std::is_convertible<T, color_t>::value,
                "color construction must be implicit");
  static_assert(noexcept(color_t{value}) &&
                    noexcept(std::declval<color_t&>() = value),
                "construction and alternative assignment must be noexcept");
#define CHECK_COLOR_GET(Color, Result)                                      \
  static_assert(std::is_same<decltype(std::get<T>(std::declval<Color>())),    \
                             Result>::value &&                             \
                    std::is_same<decltype(std::get<Index>(                  \
                                     std::declval<Color>())),              \
                                 Result>::value,                           \
                "type and index access must preserve cv/ref qualifiers")
  CHECK_COLOR_GET(color_t&, T&);
  CHECK_COLOR_GET(const color_t&, const T&);
  CHECK_COLOR_GET(color_t&&, T&&);
  CHECK_COLOR_GET(const color_t&&, const T&&);
#undef CHECK_COLOR_GET
  static_assert(std::is_same<decltype(std::get_if<T>(
                                 std::declval<color_t*>())), T*>::value &&
                    std::is_same<decltype(std::get_if<Index>(
                                     std::declval<const color_t*>())),
                                 const T*>::value,
                "pointer access must preserve constness");

  color_t color = value;
  const color_t& constant = color;
  EXPECT_EQ(color.index(), Index);
  EXPECT_TRUE(std::holds_alternative<T>(constant));
  EXPECT_EQ(&std::get<T>(color), &std::get<Index>(color));
  EXPECT_EQ(&std::get<T>(constant), &std::get<Index>(constant));
  EXPECT_EQ(std::get_if<T>(&color), &std::get<T>(color));
  EXPECT_EQ(std::get_if<Index>(&color), &std::get<T>(color));
  EXPECT_EQ(std::get_if<T>(&constant), &std::get<T>(constant));
  EXPECT_EQ(std::get_if<Index>(&constant), &std::get<T>(constant));
  EXPECT_EQ(std::get_if<T>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(std::get_if<T>(static_cast<const color_t*>(nullptr)), nullptr);
  EXPECT_EQ(std::get_if<Index>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(std::get_if<Index>(static_cast<const color_t*>(nullptr)), nullptr);
  static_assert(noexcept(std::holds_alternative<T>(constant)) &&
                    noexcept(std::get_if<T>(&color)) &&
                    noexcept(std::get_if<T>(&constant)) &&
                    noexcept(std::get_if<Index>(&color)) &&
                    noexcept(std::get_if<Index>(&constant)),
                "classification and pointer access must be noexcept");
  EXPECT_EQ(std::get<T>(std::move(color)), value);
  EXPECT_EQ(std::get<Index>(std::move(color)), value);
  EXPECT_EQ(std::get<T>(std::move(constant)), value);
  EXPECT_EQ(std::get<Index>(std::move(constant)), value);

  const color_t alternatives[] = {tjg::color::no_color, tjg::color::rgb{},
                                 tjg::color::term_color::black, tjg::color::cube_color{0},
                                 tjg::color::gray_color{0}};
  for (auto other : alternatives) {
    if (other.index() == Index) continue;
    const auto& const_other = other;
    EXPECT_FALSE(std::holds_alternative<T>(other));
    EXPECT_EQ(std::get_if<T>(&other), nullptr);
    EXPECT_EQ(std::get_if<T>(&const_other), nullptr);
    EXPECT_EQ(std::get_if<Index>(&other), nullptr);
    EXPECT_EQ(std::get_if<Index>(&const_other), nullptr);
    EXPECT_THROW((void)std::get<T>(other), std::bad_variant_access);
    EXPECT_THROW((void)std::get<T>(const_other), std::bad_variant_access);
    EXPECT_THROW((void)std::get<T>(std::move(other)), std::bad_variant_access);
    EXPECT_THROW((void)std::get<T>(std::move(const_other)), std::bad_variant_access);
    EXPECT_THROW((void)std::get<Index>(other), std::bad_variant_access);
    EXPECT_THROW((void)std::get<Index>(const_other), std::bad_variant_access);
    EXPECT_THROW((void)std::get<Index>(std::move(other)), std::bad_variant_access);
    EXPECT_THROW((void)std::get<Index>(std::move(const_other)), std::bad_variant_access);
  }
}

template<typename T>
void check_color_assignment(T replacement) {
  const tjg::color::color_t original[] = {tjg::color::no_color, tjg::color::rgb{1, 2, 3},
                                  tjg::color::term_color::bright_white,
                                  tjg::color::cube_color{215}, tjg::color::gray_color{23}};
  for (auto color : original) {
    const tjg::color::color_t expected = replacement;
    EXPECT_EQ(&(color = replacement), &color);
    EXPECT_EQ(color, expected);
    // Assignment from the active member must read it before replacing storage.
    EXPECT_EQ(&(color = std::get<T>(color)), &color);
    EXPECT_EQ(color, expected);
  }
}

struct color_kind_visitor {
  constexpr size_t operator()(const tjg::color::no_color_t&) const { return 0; }
  constexpr size_t operator()(tjg::color::rgb) const { return 1; }
  constexpr size_t operator()(tjg::color::term_color) const { return 2; }
  constexpr size_t operator()(tjg::color::cube_color) const { return 3; }
  constexpr size_t operator()(tjg::color::gray_color) const { return 4; }
};

template<typename T, size_t Index>
constexpr bool check_color_constexpr_access(tjg::color::color_t&& color) {
  return
         // GCC 4.9 rejects constexpr pointer comparisons involving temporaries.
         std::get_if<T>(&color) == &std::get<Index>(color) &&
         std::get_if<Index>(&color) == &std::get<T>(color) &&
         color_kind_visitor{}(std::get<Index>(std::move(color))) == Index &&
         std::visit(color_kind_visitor{}, color) == Index &&
         std::visit(color_kind_visitor{}, std::move(color)) == Index &&
         tjg::color::fg(color).fg() == color && tjg::color::bg(color).bg() == color;
}

struct color_category_visitor {
  color_category_visitor() = default;
  color_category_visitor(const color_category_visitor&) = delete;
  color_category_visitor(color_category_visitor&&) = delete;
  template<typename T> unsigned operator()(T&&) & {
    return (std::is_const<typename std::remove_reference<T>::type>::value
                ? 1u : 0u) + (std::is_lvalue_reference<T>::value ? 0u : 2u);
  }
  template<typename T> unsigned operator()(T&& value) && {
    return (*this)(std::forward<T>(value)) + 4u;
  }
};

struct color_mutating_visitor {
  int& calls;
  void operator()(tjg::color::no_color_t&) const { ++calls; }
  void operator()(tjg::color::rgb& value) const {
    ++calls;
    value.r = 42;
  }
  void operator()(tjg::color::cube_color& value) const {
    ++calls;
    value.r(5);
  }
  void operator()(tjg::color::gray_color& value) const {
    ++calls;
    value = tjg::color::gray_color{23};
  }
  void operator()(tjg::color::term_color& value) const {
    ++calls;
    value = tjg::color::term_color::bright_white;
  }
};

struct color_reference_visitor {
  int& value;
  template<typename T> int& operator()(T&&) const { return value; }
};

struct color_throwing_visitor {
  template<typename T> void operator()(T&&) const { throw 42; }
};

constexpr bool check_color_constexpr_assignment() {
  tjg::color::color_t color;
  color = tjg::color::rgb{1, 2, 3};
  std::get<tjg::color::rgb>(color).r = 4;
  std::get_if<1>(&color)->b = 5;
  bool rgb_ok = std::get<1>(color) == tjg::color::rgb{4, 2, 5};
  color = tjg::color::term_color::black;
  bool terminal_ok = std::holds_alternative<tjg::color::term_color>(color) &&
                     std::get<2>(color) == tjg::color::term_color::black;
  color = tjg::color::cube_color{215};
  bool cube_ok = std::get<3>(color) == tjg::color::cube_color{215};
  color = tjg::color::gray_color{23};
  bool gray_ok = std::get<4>(color) == tjg::color::gray_color{23};
  color = tjg::color::no_color;
  return rgb_ok && terminal_ok && cube_ok && gray_ok &&
         std::holds_alternative<tjg::color::no_color_t>(color);
}
}  // namespace

TEST(color_test, color_variant_access) {
  check_color_access<tjg::color::no_color_t, 0>(tjg::color::no_color);
  check_color_access<tjg::color::rgb, 1>(tjg::color::rgb{});
  check_color_access<tjg::color::rgb, 1>(tjg::color::rgb::coral);
  check_color_access<tjg::color::term_color, 2>(tjg::color::term_color::black);
  check_color_access<tjg::color::term_color, 2>(tjg::color::term_color::bright_white);
  check_color_access<tjg::color::cube_color, 3>(tjg::color::cube_color{0});
  check_color_access<tjg::color::cube_color, 3>(tjg::color::cube_color{215});
  check_color_access<tjg::color::gray_color, 4>(tjg::color::gray_color{0});
  check_color_access<tjg::color::gray_color, 4>(tjg::color::gray_color{23});
  auto color = tjg::color::color_t{tjg::color::rgb{1, 2, 3}};
  std::get<tjg::color::rgb>(color).r = 4;
  std::get<1>(color).g = 5;
  std::get_if<tjg::color::rgb>(&color)->b = 6;
  std::get_if<1>(&color)->r = 7;
  EXPECT_EQ(std::get<tjg::color::rgb>(color), (tjg::color::rgb{7, 5, 6}));
  EXPECT_EQ(std::get_if<2>(&color), nullptr);
  EXPECT_EQ(std::get_if<tjg::color::gray_color>(&color), nullptr);
  EXPECT_THROW((void)std::get<2>(color), std::bad_variant_access);
}

TEST(color_test, color_variant_assignment) {
  check_color_assignment(tjg::color::no_color);
  check_color_assignment(tjg::color::rgb{});
  check_color_assignment(tjg::color::rgb::coral);
  check_color_assignment(tjg::color::term_color::black);
  check_color_assignment(tjg::color::term_color::bright_white);
  check_color_assignment(tjg::color::cube_color{215});
  check_color_assignment(tjg::color::gray_color{23});
  const tjg::color::color_t colors[] = {tjg::color::no_color, tjg::color::rgb{1, 2, 3},
                                tjg::color::term_color::black, tjg::color::cube_color{1, 2, 3},
                                tjg::color::gray_color{12}};
  for (const auto& source : colors) {
    tjg::color::color_t copy(source);
    tjg::color::color_t moved(std::move(copy));
    EXPECT_EQ(moved, source);
    for (auto target : colors) {
      auto destination = target;
      EXPECT_EQ(&(target = source), &target);
      EXPECT_EQ(target, source);
      copy = source;
      EXPECT_EQ(&(destination = std::move(copy)), &destination);
      EXPECT_EQ(destination, source);
    }
  }
}

TEST(color_test, color_variant_visit) {
  const tjg::color::color_t colors[] = {tjg::color::no_color, tjg::color::rgb{1, 2, 3},
                                tjg::color::term_color::black, tjg::color::cube_color{1, 2, 3},
                                tjg::color::gray_color{12}};
  const tjg::color::color_t changed[] = {tjg::color::no_color, tjg::color::rgb{42, 2, 3},
                                 tjg::color::term_color::bright_white,
                                 tjg::color::cube_color{5, 2, 3}, tjg::color::gray_color{23}};
  for (size_t i = 0; i < 5; ++i) {
    auto color = colors[i];
    const auto& constant = color;
    EXPECT_EQ(std::visit(color_kind_visitor{}, constant), color.index());
    color_category_visitor visitor;
    EXPECT_EQ(std::visit(visitor, color), 0u);
    EXPECT_EQ(std::visit(visitor, constant), 1u);
    EXPECT_EQ(std::visit(visitor, std::move(color)), 2u);
    EXPECT_EQ(std::visit(visitor, std::move(constant)), 3u);
    EXPECT_EQ(std::visit(std::move(visitor), color), 4u);
    EXPECT_EQ(std::visit(std::move(visitor), constant), 5u);
    EXPECT_EQ(std::visit(std::move(visitor), std::move(color)), 6u);
    EXPECT_EQ(std::visit(std::move(visitor), std::move(constant)), 7u);
    int calls = 0;
    std::visit(color_mutating_visitor{calls}, color);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(color, changed[i]);
    int result = 0;
    EXPECT_EQ(&std::visit(color_reference_visitor{result}, color), &result);
    std::visit(color_reference_visitor{result}, color) = 9;
    EXPECT_EQ(result, 9);
    EXPECT_THROW(std::visit(color_throwing_visitor{}, color), int);
    EXPECT_EQ(color, changed[i]);
  }
}

TEST(color_test, color_variant_constexpr) {
  static_assert(
      check_color_constexpr_access<tjg::color::no_color_t, 0>(tjg::color::no_color) &&
          check_color_constexpr_access<tjg::color::rgb, 1>(tjg::color::rgb{1, 2, 3}) &&
          check_color_constexpr_access<tjg::color::term_color, 2>(tjg::color::term_color::red) &&
          check_color_constexpr_access<tjg::color::cube_color, 3>(tjg::color::cube_color{215}) &&
          check_color_constexpr_access<tjg::color::gray_color, 4>(tjg::color::gray_color{23}),
      "mutable access, visitation, and style factories must be constexpr in C++11");
  constexpr tjg::color::color_t empty;
  constexpr tjg::color::color_t rgb = tjg::color::rgb{1, 2, 3};
  constexpr tjg::color::color_t terminal = tjg::color::term_color::black;
  constexpr tjg::color::color_t cube = tjg::color::cube_color{215};
  constexpr tjg::color::color_t gray = tjg::color::gray_color{23};
  static_assert(std::get<3>(cube) == tjg::color::cube_color{215} &&
                    std::get<4>(gray) == tjg::color::gray_color{23},
                "palette access must be constexpr in C++11");
  constexpr auto palette_style = tjg::color::text_style{cube, gray, tjg::color::emphasis::bold};
  static_assert(palette_style.fg() == cube && palette_style.bg() == gray &&
                    palette_style.emph() == tjg::color::emphasis::bold,
                "palette style getters must be constexpr in C++11");
  static_assert(empty.index() == 0 && std::holds_alternative<tjg::color::no_color_t>(empty),
                "default construction must hold the unspecified alternative");
  static_assert(rgb.index() == 1 && std::get<tjg::color::rgb>(rgb).g == 2 &&
                    std::get<1>(rgb).b == 3,
                "RGB type and index access must be constexpr in C++11");
  static_assert(terminal.index() == 2 &&
                    std::get<tjg::color::term_color>(terminal) == tjg::color::term_color::black,
                "terminal black must remain specified in constant evaluation");
  static_assert(std::get_if<1>(&rgb) == &std::get<tjg::color::rgb>(rgb) &&
                    std::get_if<tjg::color::term_color>(&rgb) == nullptr &&
                    std::get_if<0>(static_cast<const tjg::color::color_t*>(nullptr)) ==
                        nullptr,
                "pointer access must be constexpr in C++11");
  static_assert(std::visit(color_kind_visitor{}, empty) == 0 &&
                    std::visit(color_kind_visitor{}, rgb) == 1 &&
                    std::visit(color_kind_visitor{}, terminal) == 2 &&
                    std::visit(color_kind_visitor{}, cube) == 3 &&
                    std::visit(color_kind_visitor{}, gray) == 4,
                "visitation must be constexpr in C++11");
  static_assert(check_color_constexpr_assignment(),
                "replacement and mutable access must support constant evaluation");
}

TEST(color_test, color_variant_style_values) {
  static_assert(std::is_same<decltype(tjg::color::text_style{}.fg()),
                             tjg::color::color_t>::value &&
                    std::is_same<decltype(tjg::color::text_style{}.bg()),
                                 tjg::color::color_t>::value,
                "style getters must return color values");
  auto foreground = tjg::color::color_t{tjg::color::rgb{1, 2, 3}};
  auto background = tjg::color::color_t{tjg::color::term_color::black};
  const auto style =
      tjg::color::text_style{foreground, background, tjg::color::emphasis::strike};
  std::get<1>(foreground).r = 255;
  std::get<2>(background) = tjg::color::term_color::white;
  EXPECT_EQ(std::get<tjg::color::rgb>(style.fg()), (tjg::color::rgb{1, 2, 3}));
  EXPECT_EQ(std::get<tjg::color::term_color>(style.bg()), tjg::color::term_color::black);
  auto extracted = style.fg();
  std::get<1>(extracted).b = 255;
  EXPECT_EQ(std::get<1>(style.fg()).b, 3);
  // Copying an alternative from a temporary color is safe.
  auto copy = std::get<tjg::color::rgb>(style.fg());
  copy.g = 255;
  EXPECT_EQ(copy, (tjg::color::rgb{1, 255, 3}));
  EXPECT_EQ(std::get<1>(style.fg()).g, 2);
  EXPECT_EQ(tjg::color::format(style, "value"),
            "\x1b[9;38;2;1;2;3;40mvalue\x1b[0m");
}

TEST(color_test, rgb_checked_channels) {
  constexpr uint8_t byte_channel = 128;
  static_assert(tjg::color::rgb{byte_channel, 0, 255} == tjg::color::rgb{128, 0, 255} &&
                    tjg::color::rgb{0, byte_channel, 255} == tjg::color::rgb{0, 128, 255} &&
                    tjg::color::rgb{0, 255, byte_channel} == tjg::color::rgb{0, 255, 128},
                "mixed byte and int inputs must resolve to the checked constructor");
  static_assert(tjg::color::rgb{0, 255, 128} == tjg::color::rgb{0x00FF80},
                "signed channel inputs must support C++11 constant evaluation");
  static_assert(noexcept(tjg::color::rgb{0, 0, 0}),
                "RGB channel construction must be noexcept");
  for (int channel = 0; channel <= 255; ++channel) {
    auto color = tjg::color::rgb{channel, channel, channel};
    EXPECT_EQ(color.r, channel);
    EXPECT_EQ(color.g, channel);
    EXPECT_EQ(color.b, channel);
  }
#ifdef NDEBUG
  const int invalid[] = {-1, 256, (std::numeric_limits<int>::min)(),
                        (std::numeric_limits<int>::max)()};
  for (auto channel : invalid) {
    SCOPED_TRACE(channel);
    const int clamped = channel < 0 ? 0 : 255;
    EXPECT_EQ((tjg::color::rgb{channel, 128, 64}), (tjg::color::rgb{clamped, 128, 64}));
    EXPECT_EQ((tjg::color::rgb{128, channel, 64}), (tjg::color::rgb{128, clamped, 64}));
    EXPECT_EQ((tjg::color::rgb{128, 64, channel}), (tjg::color::rgb{128, 64, clamped}));
  }
#endif
}

TEST(color_test, rgb) {
  static_assert(!std::is_convertible<uint32_t, tjg::color::rgb>::value,
                "integer to rgb conversion must be explicit");
  static_assert(!std::is_convertible<tjg::color::rgb, uint32_t>::value,
                "rgb to integer conversion must be explicit");
  static_assert(std::is_same<decltype(tjg::color::rgb::red), const tjg::color::rgb>::value,
                "named colors must be rgb constants");
  constexpr auto c = tjg::color::rgb{0xAB123456};
  static_assert(c.r == 0x12 && c.g == 0x34 && c.b == 0x56,
                "the high byte must be ignored");
  static_assert(static_cast<uint32_t>(c) == 0x123456,
                "packed rgb must have a zero high byte");
  static_assert(static_cast<uint32_t>(tjg::color::rgb::alice_blue) == 0xF0F8FF,
                "named colors must preserve their values");
  auto components = tjg::color::rgb{0x12, 0x34, 0x56};
  EXPECT_EQ(static_cast<uint32_t>(components), 0x123456u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::color::rgb{}), 0u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::color::rgb::black), 0u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::color::rgb::white), 0xFFFFFFu);
}

TEST(color_test, rgb_or) {
  constexpr auto lhs = tjg::color::rgb{0xC0F000};
  constexpr auto rhs = tjg::color::rgb{0x000FEE};
  static_assert((lhs | rhs) == tjg::color::rgb{0xC0FFEE},
                "RGB OR supports C++11 constant evaluation");
  static_assert(noexcept(lhs | rhs), "RGB OR must be noexcept");
  EXPECT_EQ(lhs | rhs, tjg::color::rgb{0xC0FFEE});
  EXPECT_EQ(lhs, tjg::color::rgb{0xC0F000});
  EXPECT_EQ(rhs, tjg::color::rgb{0x000FEE});
  auto assigned = lhs;
  EXPECT_EQ(&(assigned |= rhs), &assigned);
  EXPECT_EQ(assigned, lhs | rhs);
}

TEST(color_test, rgb_blend) {
  static_assert(std::is_same<decltype(tjg::color::blend(tjg::color::rgb{}, tjg::color::rgb{})),
                             tjg::color::rgb>::value,
                "blend must return an RGB value");
  static_assert(noexcept(tjg::color::blend(tjg::color::rgb{}, tjg::color::rgb{})),
                "blend must be noexcept");
  auto black = tjg::color::rgb{};
  auto white = tjg::color::rgb{255, 255, 255};
  EXPECT_EQ(tjg::color::blend(black, black), black);
  EXPECT_EQ(tjg::color::blend(white, white), white);
  EXPECT_EQ(tjg::color::blend(black, black, black), black);
  EXPECT_EQ(tjg::color::blend(white, white, white, white), white);
  EXPECT_EQ(tjg::color::blend(black, white), (tjg::color::rgb{127, 127, 127}));
  EXPECT_EQ(tjg::color::blend(black, white, white), (tjg::color::rgb{170, 170, 170}));
  EXPECT_EQ(tjg::color::blend(white, black, white), (tjg::color::rgb{170, 170, 170}));
  EXPECT_EQ(tjg::color::blend(white, white, black), (tjg::color::rgb{170, 170, 170}));
  EXPECT_EQ(black, tjg::color::rgb{});
  EXPECT_EQ(white, (tjg::color::rgb{255, 255, 255}));

  auto updated = black;
  updated = tjg::color::blend(updated, white);
  EXPECT_EQ(updated, (tjg::color::rgb{127, 127, 127}));
  EXPECT_EQ(black, tjg::color::rgb{});
  EXPECT_EQ(white, (tjg::color::rgb{255, 255, 255}));

  auto first  = tjg::color::rgb{0, 1, 254};
  auto second = tjg::color::rgb{0, 2, 255};
  auto third  = tjg::color::rgb{2, 4, 255};
  EXPECT_EQ(tjg::color::blend(first, second, third), (tjg::color::rgb{0, 2, 254}));
  EXPECT_EQ(tjg::color::blend(third, second, first), (tjg::color::rgb{0, 2, 254}));
  EXPECT_EQ(tjg::color::blend(tjg::color::rgb::red, tjg::color::rgb::green, tjg::color::rgb::blue),
            (tjg::color::rgb{85, 42, 85}));
  constexpr auto same = tjg::color::blend(tjg::color::rgb{1, 2, 3}, tjg::color::rgb{1, 2, 3});
  constexpr auto average = tjg::color::blend(tjg::color::rgb{255, 0, 0}, tjg::color::rgb{0, 255, 0},
                                     tjg::color::rgb{0, 0, 255});
  static_assert(same.r == 1 && same.g == 2 && same.b == 3,
                "equal RGB colors preserve all channels");
  static_assert(average.r == 85 && average.g == 85 && average.b == 85,
                "blend must support constant evaluation");
}

namespace {
template <typename... Colors>
concept CanBlend = requires { tjg::color::blend(std::declval<Colors>()...); };

template <typename T>
concept CanConvertToRgb = requires { tjg::color::to_rgb(std::declval<T>()); };
} // namespace

TEST(color_test, to_rgb) {
  static_assert(std::is_same<decltype(tjg::color::to_rgb(tjg::color::rgb{})), tjg::color::rgb>::value &&
                    std::is_same<decltype(tjg::color::to_rgb(tjg::color::term_color::red)),
                                 tjg::color::rgb>::value &&
                    std::is_same<decltype(tjg::color::to_rgb(std::declval<tjg::color::gray_color>())),
                                 tjg::color::rgb>::value &&
                    std::is_same<decltype(tjg::color::to_rgb(std::declval<tjg::color::cube_color>())),
                                 tjg::color::rgb>::value,
                "plain colors convert to RGB values");
  static_assert(std::is_same<decltype(tjg::color::to_rgb(tjg::color::no_color)),
                             tjg::color::no_color_t>::value &&
                    std::is_same<decltype(tjg::color::to_rgb(tjg::color::color_t{})),
                                 tjg::color::color_t>::value,
                "unspecified and variant conversions preserve their result types");
  static_assert(!CanConvertToRgb<int> &&
                    !CanConvertToRgb<const char*> &&
                    !CanConvertToRgb<tjg::color::emphasis> &&
                    !CanConvertToRgb<tjg::color::text_style>,
                "to_rgb rejects non-color arguments");
  static_assert(noexcept(tjg::color::to_rgb(tjg::color::rgb{})) &&
                    noexcept(tjg::color::to_rgb(tjg::color::term_color::red)) &&
                    noexcept(tjg::color::to_rgb(std::declval<tjg::color::gray_color>())) &&
                    noexcept(tjg::color::to_rgb(std::declval<tjg::color::cube_color>())) &&
                    noexcept(tjg::color::to_rgb(tjg::color::no_color)) &&
                    noexcept(tjg::color::to_rgb(tjg::color::color_t{})),
                "all to_rgb overloads must be noexcept");
  static_assert(tjg::color::to_rgb(tjg::color::rgb{0x123456}) == tjg::color::rgb{0x123456} &&
                    tjg::color::to_rgb(tjg::color::term_color::red) == tjg::color::rgb{205, 0, 0} &&
                    tjg::color::to_rgb(tjg::color::term_color::bright_blue) == tjg::color::rgb{92, 92, 255} &&
                    tjg::color::to_rgb(tjg::color::gray_color{0}) == tjg::color::rgb{8, 8, 8} &&
                    tjg::color::to_rgb(tjg::color::gray_color{23}) == tjg::color::rgb{238, 238, 238} &&
                    tjg::color::to_rgb(tjg::color::cube_color{0}) == tjg::color::rgb{} &&
                    tjg::color::to_rgb(tjg::color::cube_color{215}) == tjg::color::rgb{255, 255, 255},
                "plain conversion supports C++11 constant evaluation");
  constexpr auto unspecified = tjg::color::to_rgb(tjg::color::no_color);
  static_assert(tjg::color::color_t{unspecified} == tjg::color::color_t{},
                "the unspecified sentinel remains unspecified");
  static_assert(tjg::color::to_rgb(tjg::color::color_t{}) == tjg::color::color_t{} &&
                    tjg::color::to_rgb(tjg::color::color_t{tjg::color::rgb{1, 2, 3}}) ==
                        tjg::color::color_t{tjg::color::rgb{1, 2, 3}} &&
                    tjg::color::to_rgb(tjg::color::color_t{tjg::color::term_color::black}) ==
                        tjg::color::color_t{tjg::color::rgb{}} &&
                    tjg::color::to_rgb(tjg::color::color_t{tjg::color::gray_color{12}}) ==
                        tjg::color::color_t{tjg::color::rgb{128, 128, 128}} &&
                    tjg::color::to_rgb(tjg::color::color_t{tjg::color::cube_color{1, 2, 3}}) ==
                        tjg::color::color_t{tjg::color::rgb{95, 135, 175}},
                "all variant alternatives support C++11 constant conversion");
  for (const auto& sample : blend_samples) {
    const auto source = sample.color;
    const auto converted = tjg::color::to_rgb(source);
    EXPECT_EQ(converted, source.index() == 0 ? tjg::color::color_t{}
                                             : tjg::color::color_t{sample.promoted});
    EXPECT_EQ(source, sample.color);
    EXPECT_EQ(tjg::color::to_rgb(converted), converted);
  }
  EXPECT_EQ(tjg::color::to_rgb(tjg::color::gray_color{12}), (tjg::color::rgb{128, 128, 128}));
  EXPECT_EQ(tjg::color::to_rgb(tjg::color::cube_color{1, 2, 3}), (tjg::color::rgb{95, 135, 175}));
}

TEST(color_test, blend_overloads) {
  static_assert(std::is_same<decltype(tjg::color::blend(tjg::color::term_color::red,
                                                tjg::color::term_color::red)),
                             tjg::color::rgb>::value &&
                    std::is_same<decltype(tjg::color::blend(tjg::color::cube_color{0},
                                                     tjg::color::gray_color{0})),
                                 tjg::color::rgb>::value,
                "plain palette arguments return RGB");
  static_assert(std::is_same<decltype(tjg::color::blend(tjg::color::rgb{}, tjg::color::color_t{})),
                             tjg::color::color_t>::value &&
                    std::is_same<decltype(tjg::color::blend(tjg::color::color_t{}, tjg::color::rgb{})),
                                 tjg::color::color_t>::value &&
                    std::is_same<decltype(tjg::color::blend(tjg::color::rgb{}, tjg::color::no_color)),
                                 tjg::color::color_t>::value,
                "a variant or unspecified argument selects variant blending");
  static_assert(!CanBlend<> &&
                    !CanBlend<int> &&
                    !CanBlend<tjg::color::rgb, int> &&
                    !CanBlend<int, tjg::color::rgb> &&
                    !CanBlend<tjg::color::color_t, const char*>,
                "blending requires valid color arguments");
  static_assert(noexcept(tjg::color::blend(tjg::color::term_color::red,
                                    std::declval<tjg::color::gray_color>())) &&
                    noexcept(tjg::color::blend(tjg::color::color_t{}, tjg::color::rgb{})),
                "both blend overloads must be noexcept");
  static_assert(tjg::color::Color<const tjg::color::rgb&> && tjg::color::Color<tjg::color::term_color> &&
                    tjg::color::Color<tjg::color::cube_color> && tjg::color::Color<tjg::color::gray_color> &&
                    !tjg::color::Color<tjg::color::color_t> && !tjg::color::Color<tjg::color::no_color_t> &&
                    !tjg::color::Color<int>,
                "Color identifies only plain color types");
  constexpr auto mixed = tjg::color::blend(tjg::color::term_color::red,
                                    tjg::color::cube_color{1, 2, 3},
                                    tjg::color::gray_color{12});
  static_assert(mixed == tjg::color::rgb{142, 87, 101},
                "plain palette promotion and averaging must be constexpr");
  constexpr auto terminal = tjg::color::blend(tjg::color::term_color::red,
                                       tjg::color::term_color::red);
  static_assert(terminal == tjg::color::rgb{205, 0, 0},
                "equal plain terminal colors return their RGB equivalent");
}

TEST(color_test, blend_minimum_arity) {
  static_assert(!CanBlend<> &&
                    !CanBlend<tjg::color::rgb> &&
                    !CanBlend<tjg::color::term_color> &&
                    !CanBlend<tjg::color::cube_color> &&
                    !CanBlend<tjg::color::gray_color> &&
                    !CanBlend<tjg::color::color_t> &&
                    !CanBlend<tjg::color::no_color_t> &&
                    !CanBlend<tjg::color::text_style>,
                "public blend requires at least two arguments");
  static_assert(CanBlend<tjg::color::rgb, tjg::color::rgb> &&
                    CanBlend<tjg::color::rgb, tjg::color::color_t> &&
                    CanBlend<tjg::color::color_t, tjg::color::rgb> &&
                    CanBlend<tjg::color::no_color_t, tjg::color::no_color_t> &&
                    CanBlend<tjg::color::text_style, tjg::color::text_style>,
                "two-argument color and style blends remain available");
}

TEST(color_test, color_variant_blend) {
  constexpr auto empty = tjg::color::blend(tjg::color::no_color, tjg::color::color_t{}, tjg::color::no_color);
  static_assert(empty == tjg::color::color_t{}, "all unspecified colors remain empty");
  constexpr auto single = tjg::color::blend(tjg::color::no_color, tjg::color::gray_color{12},
                                     tjg::color::color_t{});
  static_assert(single == tjg::color::color_t{tjg::color::gray_color{12}},
                "one specified color retains its palette kind");
  constexpr auto same = tjg::color::blend(tjg::color::color_t{tjg::color::gray_color{12}},
                                   tjg::color::gray_color{12});
  static_assert(same == tjg::color::color_t{tjg::color::rgb{128, 128, 128}},
                "two specified palette colors produce RGB");
  constexpr auto average = tjg::color::blend(
      tjg::color::rgb{0, 1, 254}, tjg::color::no_color, tjg::color::rgb{0, 2, 255},
      tjg::color::color_t{tjg::color::rgb{2, 4, 255}}, tjg::color::no_color);
  static_assert(average == tjg::color::color_t{tjg::color::rgb{0, 2, 254}},
                "ignore unspecified colors and divide once after summation");
  constexpr auto palette = tjg::color::blend(tjg::color::cube_color{1, 2, 3},
                                      tjg::color::color_t{tjg::color::gray_color{12}});
  static_assert(palette == tjg::color::color_t{tjg::color::rgb{111, 131, 151}},
                "a trailing variant selects RGB promotion of plain arguments");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("blend kinds {} and {}", i, j));
      const auto lhs = blend_samples[i].color;
      const auto rhs = blend_samples[j].color;
      const auto a = blend_samples[i].promoted;
      const auto b = blend_samples[j].promoted;
      const auto expected = i == 0 ? rhs : j == 0 ? lhs : tjg::color::color_t{tjg::color::rgb{
          static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
          static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
          static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      EXPECT_EQ(tjg::color::blend(lhs, rhs), expected);
      EXPECT_EQ(tjg::color::blend(tjg::color::no_color, lhs, tjg::color::no_color, rhs,
                           tjg::color::no_color), expected);
      EXPECT_EQ(tjg::color::blend(lhs, tjg::color::no_color), lhs);
      EXPECT_EQ(lhs, blend_samples[i].color);
      EXPECT_EQ(rhs, blend_samples[j].color);
    }
  }
}

TEST(color_test, text_style_blend) {
  static_assert(std::is_same<decltype(tjg::color::blend(tjg::color::text_style{},
                                                tjg::color::text_style{})),
                             tjg::color::text_style>::value &&
                    CanBlend<const tjg::color::text_style&, tjg::color::text_style>,
                "style blending accepts style values and returns a style");
  static_assert(!CanBlend<tjg::color::text_style, tjg::color::rgb> &&
                    !CanBlend<tjg::color::rgb, tjg::color::text_style> &&
                    !CanBlend<tjg::color::text_style, int> &&
                    !CanBlend<tjg::color::emphasis>,
                "style blending requires style arguments without color overlap");
  static_assert(noexcept(tjg::color::blend(tjg::color::text_style{}, tjg::color::text_style{})),
                "style blending must be noexcept");
  constexpr auto empty = tjg::color::blend(tjg::color::text_style{}, tjg::color::text_style{});
  static_assert(empty == tjg::color::text_style{}, "empty styles blend to an empty style");
  constexpr auto single = tjg::color::text_style{tjg::color::cube_color{1, 2, 3},
                                         tjg::color::gray_color{12},
                                         tjg::color::emphasis::bold};
  static_assert(tjg::color::blend(tjg::color::text_style{}, single) == single &&
                    tjg::color::blend(tjg::color::text_style{}, single, tjg::color::text_style{}) == single,
                "one specified color per channel retains its kind");
  constexpr auto first = tjg::color::text_style{tjg::color::term_color::red, tjg::color::no_color,
                                        tjg::color::emphasis::bold};
  constexpr auto second = tjg::color::text_style{tjg::color::gray_color{12},
                                         tjg::color::cube_color{1, 2, 3},
                                         tjg::color::emphasis::underline};
  constexpr auto third = tjg::color::text_style{tjg::color::rgb{}, tjg::color::no_color,
                                        tjg::color::emphasis::strike};
  constexpr auto averaged = tjg::color::blend(first, second, third);
  static_assert(averaged.fg() == tjg::color::color_t{tjg::color::rgb{111, 42, 42}} &&
                    averaged.bg() == tjg::color::color_t{tjg::color::cube_color{1, 2, 3}} &&
                    averaged.emph() == (tjg::color::emphasis::bold |
                                        tjg::color::emphasis::underline |
                                        tjg::color::emphasis::strike),
                "channels use independent counts and emphasis bits are ORed");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("style blend kinds {} and {}", i, j));
      const auto a = blend_samples[i].promoted;
      const auto b = blend_samples[j].promoted;
      const auto foreground = i == 0 ? blend_samples[j].color
          : j == 0 ? blend_samples[i].color : tjg::color::color_t{tjg::color::rgb{
              static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
              static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
              static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      const auto lhs = tjg::color::text_style{blend_samples[i].color,
                                       tjg::color::gray_color{12}, tjg::color::emphasis::bold};
      const auto rhs = tjg::color::text_style{blend_samples[j].color, tjg::color::no_color,
                                       tjg::color::emphasis::underline};
      EXPECT_EQ(tjg::color::blend(lhs, rhs),
                (tjg::color::text_style{foreground, tjg::color::gray_color{12},
                                 tjg::color::emphasis::bold | tjg::color::emphasis::underline}));
      EXPECT_EQ(tjg::color::blend(lhs, rhs, tjg::color::bg(tjg::color::cube_color{1, 2, 3})),
                (tjg::color::text_style{foreground, tjg::color::rgb{111, 131, 151},
                                 tjg::color::emphasis::bold | tjg::color::emphasis::underline}));
      EXPECT_EQ(lhs.fg(), blend_samples[i].color);
      EXPECT_EQ(lhs.bg(), tjg::color::color_t{tjg::color::gray_color{12}});
      EXPECT_EQ(rhs.fg(), blend_samples[j].color);
      EXPECT_FALSE(rhs.has_bg());
    }
  }
  for (unsigned mask = 0; mask != 256; ++mask) {
    auto lhs = tjg::color::text_style{static_cast<tjg::color::emphasis>(mask)};
    auto rhs = tjg::color::text_style{static_cast<tjg::color::emphasis>(mask ^ 0xFFu)};
    EXPECT_EQ(tjg::color::blend(lhs, rhs),
              tjg::color::text_style{static_cast<tjg::color::emphasis>(0xFF)});
  }
}

TEST(color_test, text_style_color_constructor) {
  using tjg::color::color_t;
  static_assert(std::is_default_constructible<tjg::color::text_style>::value,
                "default construction must remain unambiguous");
  static_assert(std::is_convertible<tjg::color::emphasis, tjg::color::text_style>::value,
                "emphasis conversion must remain implicit");
  static_assert(std::is_convertible<color_t, tjg::color::text_style>::value,
                "the color constructor must not be explicit");
  static_assert(noexcept(tjg::color::text_style{tjg::color::rgb::red,
                                         tjg::color::term_color::black,
                                         tjg::color::emphasis::bold}),
                "color construction must be noexcept");
  static_assert(noexcept(tjg::color::text_style{tjg::color::emphasis::bold,
                                         tjg::color::rgb::red,
                                         tjg::color::term_color::black}),
                "emphasis-first construction must be noexcept");
  constexpr auto emphasis_only = tjg::color::text_style{tjg::color::emphasis::bold};
  static_assert(emphasis_only.emph() == tjg::color::emphasis::bold &&
                    !emphasis_only.has_fg() && !emphasis_only.has_bg(),
                "emphasis-only construction must remain constexpr");
  constexpr auto empty = tjg::color::text_style{tjg::color::no_color};
  static_assert(empty.empty(),
                "an unspecified foreground must produce an empty style");

  const color_t colors[] = {
      tjg::color::no_color, tjg::color::rgb{}, tjg::color::rgb{0xFFFFFF},
      tjg::color::term_color::black, tjg::color::term_color::bright_white,
      tjg::color::rgb::coral, tjg::color::cube_color{0}, tjg::color::cube_color{215},
      tjg::color::gray_color{0}, tjg::color::gray_color{23}};
  const tjg::color::emphasis attributes[] = {
      tjg::color::emphasis{}, tjg::color::emphasis::bold | tjg::color::emphasis::faint,
      tjg::color::emphasis::strike, static_cast<tjg::color::emphasis>(0xFF)};
  for (auto foreground : colors) {
    tjg::color::text_style implicit = foreground;
    EXPECT_EQ(implicit, tjg::color::fg(foreground));
    for (auto emphasis : attributes) {
      tjg::color::text_style implicit_emphasis = emphasis;
      EXPECT_EQ(implicit_emphasis, tjg::color::text_style{emphasis});
      EXPECT_EQ((tjg::color::text_style{emphasis, foreground}),
                tjg::color::fg(foreground).emph(emphasis));
    }
    for (auto background : colors) {
      EXPECT_EQ((tjg::color::text_style{foreground, background}),
                tjg::color::fg(foreground).bg(background));
      for (auto emphasis : attributes) {
        auto style = tjg::color::text_style{foreground, background, emphasis};
        EXPECT_EQ((tjg::color::text_style{emphasis, foreground, background}), style);
        EXPECT_EQ(style, tjg::color::fg(foreground).bg(background).emph(emphasis));
        EXPECT_EQ(style.fg(), foreground);
        EXPECT_EQ(style.bg(), background);
        EXPECT_EQ(style.emph(), emphasis);
      }
    }
  }

  EXPECT_EQ(tjg::color::text_style{}, tjg::color::text_style{tjg::color::emphasis{}});
  EXPECT_EQ(tjg::color::text_style{tjg::color::rgb::red}, tjg::color::fg(tjg::color::rgb::red));
  EXPECT_TRUE(tjg::color::text_style{tjg::color::rgb{}}.has_fg());
  EXPECT_TRUE(tjg::color::text_style{tjg::color::term_color::black}.has_fg());
  tjg::color::text_style listed = {tjg::color::rgb::red, tjg::color::rgb::blue,
                           tjg::color::emphasis::bold};
  EXPECT_EQ(listed, tjg::color::fg(tjg::color::rgb::red).bg(tjg::color::rgb::blue).bold());
  tjg::color::text_style attributes_first = {tjg::color::emphasis::bold, tjg::color::rgb::red,
                                     tjg::color::rgb::blue};
  EXPECT_EQ(attributes_first, listed);
  EXPECT_EQ((tjg::color::text_style{tjg::color::emphasis::bold, tjg::color::no_color,
                             tjg::color::term_color::black}),
            tjg::color::bg(tjg::color::term_color::black).bold());
  constexpr auto style = tjg::color::text_style{tjg::color::rgb::coral,
                                         tjg::color::term_color::black,
                                         tjg::color::emphasis::bold};
  static_assert(style.fg() == tjg::color::color_t{tjg::color::rgb::coral},
                "constant evaluation must preserve the RGB kind and payload");
  static_assert(style.bg() == tjg::color::color_t{tjg::color::term_color::black},
                "constant evaluation must preserve specified terminal black");
  static_assert(style.emph() == tjg::color::emphasis::bold,
                "constant evaluation must preserve emphasis");
  constexpr auto constexpr_attributes_first = tjg::color::text_style{
      tjg::color::emphasis::bold, tjg::color::rgb::coral, tjg::color::term_color::black};
  static_assert(constexpr_attributes_first == style,
                "both constructor orders must agree in constant evaluation");
}

void check_fluent_emphasis(tjg::color::emphasis attribute,
                          tjg::color::text_style& (tjg::color::text_style::*set)(bool),
                          bool (tjg::color::text_style::*get)() const) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = tjg::color::text_style{static_cast<tjg::color::emphasis>(bits),
                                    tjg::color::rgb::coral,
                                    tjg::color::term_color::black};
    auto mask = static_cast<unsigned>(attribute);
    EXPECT_EQ((original.*get)(), (bits & mask) != 0);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      auto style = original;
      auto expected = tjg::color::text_style{
          static_cast<tjg::color::emphasis>(enabled ? bits | mask : bits & ~mask),
          tjg::color::rgb::coral, tjg::color::term_color::black};
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);
      EXPECT_EQ((style.*get)(), enabled);
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);  // Repeating a setter is idempotent.
    }
  }
}

void check_free_emphasis(tjg::color::emphasis attribute,
                        tjg::color::text_style (*set)(tjg::color::text_style, bool)) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = tjg::color::text_style{static_cast<tjg::color::emphasis>(bits),
                                    tjg::color::cube_color{1, 2, 3},
                                    tjg::color::gray_color{12}};
    const auto saved = original;
    const auto mask = static_cast<unsigned>(attribute);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      const auto result = set(original, enabled);
      EXPECT_EQ(result, (tjg::color::text_style{
                            static_cast<tjg::color::emphasis>(
                                enabled ? bits | mask : bits & ~mask),
                            tjg::color::cube_color{1, 2, 3}, tjg::color::gray_color{12}}));
      EXPECT_EQ(original, saved);
      EXPECT_EQ(set(result, enabled), result);
    }
  }
}

TEST(color_test, emphasis_factories) {
#define CHECK_EMPHASIS_FACTORY(name)                                      \
  static_assert(std::is_same<decltype(tjg::color::name()),                        \
                             tjg::color::text_style>::value,                    \
                "attribute factories must return text_style");           \
  static_assert(noexcept(tjg::color::name()) && noexcept(tjg::color::name(true)) &&      \
                    noexcept(tjg::color::name(false)),                          \
                "attribute factories must be noexcept");                 \
  static_assert(tjg::color::name() == tjg::color::text_style{tjg::color::emphasis::name} &&      \
                    tjg::color::name(true) == tjg::color::name(),                       \
                "default and true must set exactly one attribute");       \
  static_assert(tjg::color::name(false).empty(),                                 \
                "false must produce an empty style");                    \
  for (bool enabled : {false, true}) {                                    \
    const auto style = tjg::color::name(enabled);                               \
    EXPECT_EQ(style.emph(), enabled ? tjg::color::emphasis::name                  \
                                   : tjg::color::emphasis{});                   \
    EXPECT_FALSE(style.has_fg());                                        \
    EXPECT_FALSE(style.has_bg());                                        \
  }
  CHECK_EMPHASIS_FACTORY(bold);
  CHECK_EMPHASIS_FACTORY(faint);
  CHECK_EMPHASIS_FACTORY(italic);
  CHECK_EMPHASIS_FACTORY(underline);
  CHECK_EMPHASIS_FACTORY(blink);
  CHECK_EMPHASIS_FACTORY(reverse);
  CHECK_EMPHASIS_FACTORY(conceal);
  CHECK_EMPHASIS_FACTORY(strike);
#undef CHECK_EMPHASIS_FACTORY
  EXPECT_EQ(std::format("{}", tjg::color::styled(42, tjg::color::bold())),
            "\x1b[1m42\x1b[0m");
  EXPECT_EQ(std::format("{}", tjg::color::styled(42, tjg::color::bold(false))), "42");
}

TEST(color_test, free_emphasis) {
#define CHECK_FREE_EMPHASIS(name)                                            \
  static_assert(noexcept(tjg::color::name(tjg::color::text_style{})) &&                    \
                    noexcept(tjg::color::name(tjg::color::text_style{}, false)),          \
                "free attribute functions must be noexcept");               \
  static_assert(std::is_same<decltype(tjg::color::name(tjg::color::text_style{})),          \
                             tjg::color::text_style>::value,                      \
                "free attribute functions must return a style value");      \
  EXPECT_EQ(tjg::color::name(tjg::color::text_style{}),                                    \
            (tjg::color::text_style{tjg::color::emphasis::name}));                         \
  EXPECT_EQ(tjg::color::name(tjg::color::text_style{}),                                    \
            tjg::color::name(tjg::color::text_style{}, true));                            \
  check_free_emphasis(tjg::color::emphasis::name, &tjg::color::name)
  CHECK_FREE_EMPHASIS(bold);
  CHECK_FREE_EMPHASIS(faint);
  CHECK_FREE_EMPHASIS(italic);
  CHECK_FREE_EMPHASIS(underline);
  CHECK_FREE_EMPHASIS(blink);
  CHECK_FREE_EMPHASIS(reverse);
  CHECK_FREE_EMPHASIS(conceal);
  CHECK_FREE_EMPHASIS(strike);
#undef CHECK_FREE_EMPHASIS
}

TEST(color_test, free_emphasis_constexpr) {
  constexpr auto base = tjg::color::text_style{tjg::color::cube_color{1, 2, 3},
                                       tjg::color::gray_color{12}};
#define CHECK_FREE_CONSTEXPR(name)                                          \
  constexpr auto name##_style = tjg::color::name(base);                            \
  static_assert(name##_style.is_##name(), "default must enable attribute"); \
  static_assert(tjg::color::name(name##_style, false) == base,                     \
                "disabling must restore the original style");              \
  static_assert(name##_style.fg() == base.fg() &&                           \
                    name##_style.bg() == base.bg(),                        \
                "free attribute functions must preserve colors")
  CHECK_FREE_CONSTEXPR(bold);
  CHECK_FREE_CONSTEXPR(faint);
  CHECK_FREE_CONSTEXPR(italic);
  CHECK_FREE_CONSTEXPR(underline);
  CHECK_FREE_CONSTEXPR(blink);
  CHECK_FREE_CONSTEXPR(reverse);
  CHECK_FREE_CONSTEXPR(conceal);
  CHECK_FREE_CONSTEXPR(strike);
#undef CHECK_FREE_CONSTEXPR
  static_assert(!base.has_emph(), "the original style must remain unchanged");
}

TEST(color_test, fluent_emphasis) {
  using tjg::color::text_style;
#define CHECK_EMPHASIS(name)                                                 \
  static_assert(noexcept(text_style{}.name()), "setter must be noexcept");   \
  static_assert(noexcept(text_style{}.is_##name()),                          \
                "getter must be noexcept");                                \
  static_assert(!text_style{}.is_##name(), "empty getter must be constexpr"); \
  check_fluent_emphasis(tjg::color::emphasis::name, &text_style::name,                \
                        &text_style::is_##name)
  CHECK_EMPHASIS(bold);
  CHECK_EMPHASIS(faint);
  CHECK_EMPHASIS(italic);
  CHECK_EMPHASIS(underline);
  CHECK_EMPHASIS(blink);
  CHECK_EMPHASIS(reverse);
  CHECK_EMPHASIS(conceal);
  CHECK_EMPHASIS(strike);
#undef CHECK_EMPHASIS

  auto style = text_style{};
  EXPECT_EQ(&style.bold().faint().italic().underline().blink().reverse()
                 .conceal().strike(), &style);
  EXPECT_EQ(style.emph(), static_cast<tjg::color::emphasis>(0xFF));
  EXPECT_EQ(&style.bold(false).faint(false).italic(false).underline(false)
                 .blink(false).reverse(false).conceal(false)
                 .strike(false), &style);
  EXPECT_EQ(style, text_style{});

  style.bold().faint();
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  EXPECT_EQ(tjg::color::format(style, "both"), "\x1b[1;2mboth\x1b[0m");
  style.bold(false);
  EXPECT_FALSE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  style.bold().faint(false);
  EXPECT_TRUE(style.is_bold());
  EXPECT_FALSE(style.is_faint());
}

TEST(color_test, fluent_components) {
  using tjg::color::color_t;
  using tjg::color::emphasis;
  using tjg::color::text_style;
  constexpr auto empty = text_style{};
  static_assert(empty.fg() == color_t{tjg::color::no_color},
                "empty foreground getter must be constexpr");
  static_assert(empty.bg() == color_t{tjg::color::no_color},
                "empty background getter must be constexpr");
  static_assert(empty.emph() == emphasis{},
                "empty emphasis getter must be constexpr");
  constexpr auto filled = text_style{emphasis::strike};
  static_assert(filled.emph() == emphasis::strike,
                "the emphasis getter must support constant evaluation");
  static_assert(noexcept(empty.fg()) && noexcept(empty.bg()) &&
                    noexcept(empty.emph()), "getters must be noexcept");
  static_assert(noexcept(text_style{}.fg(tjg::color::no_color)) &&
                    noexcept(text_style{}.bg(tjg::color::no_color)) &&
                    noexcept(text_style{}.emph(emphasis{})),
                "setters must be noexcept");

  const color_t colors[] = {
      tjg::color::no_color, tjg::color::rgb{}, tjg::color::rgb::coral,
      tjg::color::term_color::black, tjg::color::term_color::bright_white};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto original = text_style{
          static_cast<emphasis>(0xFF), foreground, background};
      for (auto replacement : colors) {
        auto style = original;
        EXPECT_EQ(&style.fg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     replacement, background}));
        EXPECT_EQ(style.fg(), replacement);
        EXPECT_EQ(style.has_fg(), replacement != color_t{});
        style = original;
        EXPECT_EQ(&style.bg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     foreground, replacement}));
        EXPECT_EQ(style.bg(), replacement);
        EXPECT_EQ(style.has_bg(), replacement != color_t{});
      }
      auto style = original;
      auto selected = emphasis::bold | emphasis::blink | emphasis::strike;
      EXPECT_EQ(&style.emph(selected), &style);
      EXPECT_EQ(style, (text_style{selected, foreground, background}));
      EXPECT_EQ(style.emph(), selected);
      EXPECT_EQ(&style.emph(emphasis{}), &style);
      EXPECT_EQ(style, (text_style{foreground, background}));
      EXPECT_FALSE(style.has_emph());
    }
  }

  auto style = text_style{};
  EXPECT_EQ(&style.fg(tjg::color::rgb::coral).bg(tjg::color::term_color::black)
                 .emph(emphasis::italic).underline(), &style);
  EXPECT_EQ(style, (text_style{emphasis::italic | emphasis::underline,
                               tjg::color::rgb::coral, tjg::color::term_color::black}));
  EXPECT_EQ(&style.fg(tjg::color::no_color).bg(tjg::color::no_color).emph(emphasis{}), &style);
  EXPECT_EQ(style, empty);
  EXPECT_EQ(tjg::color::format(style, "plain"), "plain");
}

TEST(color_test, fluent_presence) {
  using tjg::color::emphasis;
  using tjg::color::text_style;
  constexpr auto empty = text_style{};
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "empty presence queries must be constexpr in C++11");
  static_assert(noexcept(empty.has_fg()) && noexcept(empty.has_bg()) &&
                    noexcept(empty.has_emph()), "presence queries must be noexcept");
  static_assert(std::is_same<decltype(empty.has_fg()), bool>::value &&
                    std::is_same<decltype(empty.has_bg()), bool>::value &&
                    std::is_same<decltype(empty.has_emph()), bool>::value,
                "presence queries must return bool");
  constexpr auto high_bit = text_style{emphasis::strike};
  static_assert(high_bit.has_emph() && !high_bit.has_fg() && !high_bit.has_bg(),
                "emphasis presence must be constexpr and independent of colors");

  const struct {
    tjg::color::color_t value;
    bool specified;
  } colors[] = {{tjg::color::no_color, false}, {tjg::color::rgb{}, true},
                {tjg::color::term_color::black, true}, {tjg::color::rgb::coral, true},
                {tjg::color::term_color::bright_white, true}};
  const emphasis attributes[] = {
      emphasis{}, emphasis::bold, emphasis::faint, emphasis::italic,
      emphasis::underline, emphasis::blink, emphasis::reverse,
      emphasis::conceal, emphasis::strike, static_cast<emphasis>(0xFF)};
  for (const auto& foreground : colors) {
    for (const auto& background : colors) {
      for (auto attribute : attributes) {
        const auto style = text_style{attribute, foreground.value,
                                       background.value};
        EXPECT_EQ(style.has_fg(), foreground.specified);
        EXPECT_EQ(style.has_bg(), background.specified);
        EXPECT_EQ(style.has_emph(), attribute != emphasis{});
      }
    }
  }

  auto style = text_style{emphasis::bold, tjg::color::rgb{}, tjg::color::term_color::black};
  style.fg(tjg::color::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bg(tjg::color::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bold(false);
  EXPECT_FALSE(style.has_emph());
  style.emph(emphasis::faint).fg(tjg::color::term_color::black).bg(tjg::color::rgb{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.emph(emphasis{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
}

TEST(color_test, fluent_constexpr) {
  constexpr auto all = tjg::color::text_style{}.bold().faint().italic().underline()
                          .blink().reverse().conceal().strike();
  static_assert(all.is_bold() && all.is_faint() && all.is_italic() &&
                    all.is_underline() && all.is_blink() && all.is_reverse() &&
                    all.is_conceal() && all.is_strike(),
                "all emphasis setters must support constant evaluation");
  constexpr auto cleared = tjg::color::text_style{all}.bold(false).faint(false)
                              .italic(false).underline(false).blink(false)
                              .reverse(false).conceal(false).strike(false);
  static_assert(!cleared.has_emph(),
                "disabling emphasis must support constant evaluation");
  constexpr auto replaced = tjg::color::text_style{all}.fg(tjg::color::rgb::coral)
                               .bg(tjg::color::term_color::black)
                               .emph(tjg::color::emphasis::underline);
  static_assert(replaced == tjg::color::text_style{tjg::color::emphasis::underline,
                                             tjg::color::rgb::coral,
                                             tjg::color::term_color::black},
                "component replacement must support constant evaluation");
  static_assert(replaced.has_fg() && replaced.has_bg() && replaced.has_emph(),
                "presence queries must detect constexpr component replacement");
  constexpr auto empty = tjg::color::text_style{replaced}.fg(tjg::color::no_color)
                            .bg(tjg::color::no_color).emph(tjg::color::emphasis{});
  static_assert(empty == tjg::color::text_style{},
                "clearing components must support constant evaluation");
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "presence queries must detect constexpr component clearing");
  static_assert([replaced] {
                  auto style = replaced;
                  style.clear();
                  return style.empty() && style == tjg::color::text_style{};
                }(),
                "clear must restore an empty style during constant evaluation");
  static_assert([replaced] {
                  auto style = replaced;
                  return style.clear().strike() ==
                         tjg::color::text_style{tjg::color::emphasis::strike};
                }(),
                "clear must support fluent reuse during constant evaluation");
}

TEST(color_test, text_style) {
  EXPECT_FALSE(tjg::color::text_style().has_fg());
  EXPECT_FALSE(tjg::color::text_style().has_bg());
  EXPECT_FALSE(tjg::color::text_style().has_emph());

  EXPECT_TRUE(fg(tjg::color::rgb(0)).has_fg());
  EXPECT_FALSE(fg(tjg::color::rgb(0)).has_bg());
  EXPECT_FALSE(fg(tjg::color::rgb(0)).has_emph());
  EXPECT_TRUE(bg(tjg::color::rgb(0)).has_bg());
  EXPECT_FALSE(bg(tjg::color::rgb(0)).has_fg());
  EXPECT_FALSE(bg(tjg::color::rgb(0)).has_emph());

  const auto both = fg(tjg::color::rgb(0xFFFFFF)).bg(tjg::color::rgb(0xFFFFFF));
  EXPECT_TRUE(both.has_fg());
  EXPECT_TRUE(both.has_bg());
  EXPECT_FALSE(both.has_emph());
}

TEST(color_test, strike_style) {
  using tjg::color::emphasis;
  using tjg::color::term_color;
  using tjg::color::text_style;

  auto style = text_style{emphasis::bold, term_color::red, term_color::black};
  EXPECT_EQ(&style.strike(), &style);
  EXPECT_TRUE(style.is_strike());
  EXPECT_EQ(tjg::color::format(style, "mixed"), "\x1b[1;9;31;40mmixed\x1b[0m");
  EXPECT_EQ(&style.strike(false), &style);
  EXPECT_FALSE(style.is_strike());
  EXPECT_EQ(style, (text_style{emphasis::bold, term_color::red,
                              term_color::black}));
  constexpr auto cleared = text_style{}.strike().strike(false);
  static_assert(!cleared.is_strike(),
                "strike setters must support constant evaluation");
}

TEST(color_test, term_color_boundaries) {
  using tjg::color::term_color;
  EXPECT_EQ(static_cast<int>(term_color::black), 0);
  EXPECT_EQ(static_cast<int>(term_color::white), 7);
  EXPECT_EQ(static_cast<int>(term_color::bright_black), 8);
  EXPECT_EQ(static_cast<int>(term_color::bright_white), 15);

  EXPECT_TRUE(fg(term_color::black).has_fg());
  EXPECT_TRUE(bg(term_color::black).has_bg());

  EXPECT_EQ(tjg::color::format(fg(term_color::black), "black"),
            "\x1b[30mblack\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(term_color::black), "black"),
            "\x1b[40mblack\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(term_color::white), "white"),
            "\x1b[37mwhite\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(term_color::white), "white"),
            "\x1b[47mwhite\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(term_color::bright_black), "bright black"),
            "\x1b[90mbright black\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(term_color::bright_black), "bright black"),
            "\x1b[100mbright black\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(term_color::bright_white), "bright white"),
            "\x1b[97mbright white\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(term_color::bright_white), "bright white"),
            "\x1b[107mbright white\x1b[0m");
}

TEST(color_test, format) {
  EXPECT_EQ(tjg::color::format(tjg::color::text_style(), "no style"), "no style");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::rgb(255, 20, 30)), "rgb(255,20,30)"),
            "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::rgb(255, 0, 0) | tjg::color::rgb(0, 20, 30)),
                        "rgb(255,20,30)"),
            "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(
      tjg::color::format(fg(tjg::color::rgb(0, 0, 0) | tjg::color::rgb(0, 0, 0)), "rgb(0,0,0)"),
      "\x1b[38;2;0;0;0mrgb(0,0,0)\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::rgb::blue), "blue"),
            "\x1b[38;2;0;0;255mblue\x1b[0m");
  EXPECT_EQ(
      tjg::color::format(fg(tjg::color::rgb::blue).bg(tjg::color::rgb::red), "two color"),
      "\x1b[38;2;0;0;255;48;2;255;0;0mtwo color\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::bold, "bold"), "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::faint, "faint"), "\x1b[2mfaint\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::blink, "blink"), "\x1b[5mblink\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::reverse, "reverse"),
            "\x1b[7mreverse\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::conceal, "conceal"),
            "\x1b[8mconceal\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::strike, "strikethrough"),
            "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      tjg::color::format(fg(tjg::color::rgb::blue).bold(), "blue/bold"),
      "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::rgb::blue), "blue log"),
            "\x1b[38;2;0;0;255mblue log\x1b[0m");
  EXPECT_EQ(tjg::color::format(tjg::color::text_style(), "hi"), "hi");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::term_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(tjg::color::term_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::term_color::bright_green), "tbgreen"),
            "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(tjg::color::format(bg(tjg::color::term_color::bright_magenta), "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(tjg::color::format(fg(tjg::color::term_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(std::format("{}{}", tjg::color::styled("red", fg(tjg::color::rgb::red)),
                        tjg::color::styled("bold", tjg::color::emphasis::bold)),
            "\x1b[38;2;255;0;0mred\x1b[0m\x1b[1mbold\x1b[0m");
  EXPECT_EQ(std::format("{}", tjg::color::styled("bar", fg(tjg::color::rgb::blue).underline())),
            "\x1b[4;38;2;0;0;255mbar\x1b[0m");
  EXPECT_EQ(
      std::format(
          "{}", tjg::color::styled(
                    "all", tjg::color::emphasis::bold | tjg::color::emphasis::faint |
                               tjg::color::emphasis::italic |
                               tjg::color::emphasis::underline | tjg::color::emphasis::blink |
                               tjg::color::emphasis::reverse | tjg::color::emphasis::conceal |
                               tjg::color::emphasis::strike)),
      "\x1b[1;2;3;4;5;7;8;9mall\x1b[0m");
}

TEST(color_test, format_to) {
  auto out = std::string();
  tjg::color::format_to(std::back_inserter(out), fg(tjg::color::rgb(255, 20, 30)),
                 "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(std::string(out.data(), out.size()),
            "\x1b[38;2;255;20;30mrgb(255,20,30)123\x1b[0m");
}

TEST(color_test, print) {
  EXPECT_WRITE(stdout, tjg::color::print(fg(tjg::color::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m");
}

TEST(color_test, runtime_print) {
  auto value = 42;
  auto args = std::make_format_args(value);
  const auto style = tjg::color::bold();
  EXPECT_WRITE(stdout, tjg::color::vprint_unicode(stdout, style, "{:04}", args),
               "\x1b[1m0042\x1b[0m");
  EXPECT_WRITE(stdout, tjg::color::vprint_unicode(style, "{}", args),
               "\x1b[1m42\x1b[0m");
  EXPECT_WRITE(stdout, tjg::color::vprint_nonunicode(stdout, style, "{:04}", args),
               "\x1b[1m0042\x1b[0m");
  EXPECT_WRITE(stdout, tjg::color::vprint_nonunicode(style, "{}", args),
               "\x1b[1m42\x1b[0m");
}

TEST(color_test, println) {
  EXPECT_WRITE(stdout,
               tjg::color::println(fg(tjg::color::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m\n");
  EXPECT_WRITE(stdout, tjg::color::println(tjg::color::emphasis::bold, "bold"),
               "\x1b[1mbold\x1b[0m\n");
  EXPECT_WRITE(
      stdout,
      tjg::color::println(fg(tjg::color::rgb::blue).bold(), "blue/bold"),
      "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m\n");
}

TEST(color_test, compact_sgr_combined_style) {
  const auto style = tjg::color::text_style{static_cast<tjg::color::emphasis>(0xff),
                                    tjg::color::rgb{0, 9, 255}, tjg::color::cube_color{0}};
  const auto sgr = std::string{
      "\x1b[1;2;3;4;5;7;8;9;38;2;0;9;255;48;5;16m"};
  check_to_sgr(style, sgr);
  EXPECT_EQ(tjg::color::format(style, "value"), sgr + "value\x1b[0m");
  EXPECT_EQ(std::format("{:>5}", tjg::color::styled(42, style)),
            sgr + "   42\x1b[0m");
  EXPECT_EQ(std::format("{}plain", tjg::color::styled("value", style)),
            sgr + "value\x1b[0mplain");

  // Appending a style must preserve existing buffer contents.
  auto buf = tjg::color::sgr_buffer{};
  const auto prefix = std::string{"prefix"};
  buf.append(prefix.data(), prefix.data() + prefix.size());
  tjg::color::detail::append_sgr(buf, style);
  EXPECT_EQ(std::string(buf.data(), buf.size()), prefix + sgr);
}

TEST(color_test, compact_sgr_empty_and_single_components) {
  check_to_sgr(tjg::color::text_style{}, "");
  EXPECT_EQ(tjg::color::format(tjg::color::text_style{}, "plain"), "plain");
  EXPECT_EQ(std::format("{:>5}", tjg::color::styled(42, tjg::color::text_style{})), "   42");
  check_to_sgr(tjg::color::text_style{tjg::color::emphasis::bold}, "\x1b[1m");
  check_to_sgr(tjg::color::fg(tjg::color::term_color::black), "\x1b[30m");
  check_to_sgr(tjg::color::bg(tjg::color::term_color::bright_white), "\x1b[107m");
  check_to_sgr(tjg::color::fg(tjg::color::gray_color{0}), "\x1b[38;5;232m");
  check_to_sgr(tjg::color::bg(tjg::color::gray_color{23}), "\x1b[48;5;255m");
}

TEST(color_test, compact_sgr_decimal_channels) {
  for (unsigned n = 0; n < 256; ++n) {
    const auto channel = static_cast<uint8_t>(n);
    const auto rgb = tjg::color::rgb{channel, channel, channel};
    const auto value = std::to_string(n);
    const auto parameters = value + ";" + value + ";" + value;
    check_to_sgr(tjg::color::fg(rgb), "\x1b[38;2;" + parameters + "m");
    check_to_sgr(tjg::color::bg(rgb), "\x1b[48;2;" + parameters + "m");
  }
}

namespace {
void check_wrap_sgr(const tjg::color::text_style& style, const std::string& text,
                   const std::string& expected) {
  const auto original_style = style;
  const auto original_text = text;
  EXPECT_EQ(tjg::color::wrap_sgr(style, text), expected);
  EXPECT_EQ(tjg::color::wrap_sgr<char>(style, std::string_view{text.data(), text.size()}),
            expected);
  const auto wide_text = std::wstring{text.begin(), text.end()};
  EXPECT_EQ(tjg::color::wrap_sgr<wchar_t>(
                style, std::basic_string_view<wchar_t>{wide_text.data(),
                                                     wide_text.size()}),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original_style);
  EXPECT_EQ(text, original_text);
}
}  // namespace

TEST(color_test, wrap_sgr) {
  static_assert(std::is_same<decltype(tjg::color::wrap_sgr(tjg::color::text_style{}, "text")),
                             std::string>::value,
                "the convenience overload must return std::string");
  static_assert(std::is_same<decltype(tjg::color::wrap_sgr<wchar_t>(
                                tjg::color::text_style{},
                                std::basic_string_view<wchar_t>{L"text"})),
                             std::wstring>::value,
                "wide wrapping must return std::wstring");
  EXPECT_EQ(tjg::color::wrap_sgr(tjg::color::text_style{tjg::color::emphasis::bold}, "literal"),
            "\x1b[1mliteral\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{}, "value", "value");
  check_wrap_sgr(tjg::color::text_style{tjg::color::emphasis::bold}, "value",
                 "\x1b[1mvalue\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{tjg::color::rgb{1, 2, 3}}, "value",
                 "\x1b[38;2;1;2;3mvalue\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{tjg::color::term_color::black,
                                 tjg::color::term_color::bright_white},
                 "value", "\x1b[30;107mvalue\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{tjg::color::cube_color{0}, tjg::color::gray_color{23}},
                 "value", "\x1b[38;5;16;48;5;255mvalue\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{static_cast<tjg::color::emphasis>(0xff),
                                 tjg::color::rgb{255, 255, 255},
                                 tjg::color::rgb{255, 255, 255}},
                 "value", "\x1b[1;2;3;4;5;7;8;9;38;2;255;255;255;"
                              "48;2;255;255;255mvalue\x1b[0m");
}

TEST(color_test, wrap_sgr_empty) {
  const tjg::color::text_style styles[] = {tjg::color::text_style{},
                                    tjg::color::text_style{tjg::color::emphasis::bold}};
  for (const auto& style : styles) {
    EXPECT_EQ(tjg::color::wrap_sgr(style, std::string_view{}), "");
    EXPECT_EQ(tjg::color::wrap_sgr(style, ""), "");
    EXPECT_EQ(tjg::color::wrap_sgr<wchar_t>(style, std::basic_string_view<wchar_t>{}),
              L"");
    EXPECT_EQ(tjg::color::wrap_sgr<wchar_t>(style, L""), L"");
  }
}

TEST(color_test, wrap_sgr_literal_payload) {
  const auto style = tjg::color::text_style{tjg::color::emphasis::bold};
  const char bytes[] = "before\0{}\x1b[31mafter";
  const auto text = std::string{bytes, sizeof(bytes) - 1};
  check_wrap_sgr(style, text, "\x1b[1m" + text + "\x1b[0m");
  check_wrap_sgr(tjg::color::text_style{}, text, text);
  const auto long_text = std::string(4096, 'x');
  check_wrap_sgr(style, long_text, "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(tjg::color::wrap_sgr<wchar_t>(style, L"wide \u03a9"),
            L"\x1b[1mwide \u03a9\x1b[0m");

  auto source = std::string{"owned"};
  const auto wrapped = tjg::color::wrap_sgr(style, source);
  source.assign("changed");
  EXPECT_EQ(wrapped, "\x1b[1mowned\x1b[0m");
}

TEST(color_test, absolute_style_sgr) {
  const auto reset = tjg::color::text_style{}.absolute();
  check_to_sgr(reset, "\x1b[0m");
  const auto style = tjg::color::underline().absolute();
  const auto prefix = std::string{"\x1b[0;4m"};
  check_to_sgr(style, prefix);
  EXPECT_EQ(tjg::color::format(style, "text"), prefix + "text\x1b[0m");
  // Styled arguments honor the explicit absolute reset.
  EXPECT_EQ(std::format("{}", tjg::color::styled("text", style)),
            prefix + "text\x1b[0m");
  EXPECT_EQ(tjg::color::wrap_sgr(reset, "text"), "\x1b[0mtext\x1b[0m");
  EXPECT_EQ(tjg::color::wrap_sgr<wchar_t>(reset, L"text"),
            L"\x1b[0mtext\x1b[0m");
  const auto maximal = tjg::color::text_style{tjg::color::rgb{255, 255, 255},
      tjg::color::rgb{255, 255, 255}, static_cast<tjg::color::emphasis>(0xFF)}.absolute();
  const auto expected = std::string{
      "\x1b[0;1;2;3;4;5;7;8;9;38;2;255;255;255;48;2;255;255;255m"};
  check_to_sgr(maximal, expected);
  EXPECT_EQ(expected.size(), size_t{54});
  EXPECT_LE(expected.size(), tjg::color::sgr_max_length);
}


TEST(color_test, text_style_underlay) {
  using tc = tjg::color::term_color;
  const tjg::color::color_t colors[] = {tjg::color::no_color, tc::black, tc::red,
      tjg::color::rgb{1, 2, 3}, tjg::color::cube_color{3}, tjg::color::gray_color{12}};
  for (auto foreground : colors) {
    for (auto background : colors) {
      for (auto supplied : colors) {
        const auto base = tjg::color::text_style{foreground, background}.bold();
        const auto defaults = tjg::color::text_style{supplied, supplied}.underline().absolute();
        const auto result = tjg::color::underlay(base, defaults);
        EXPECT_EQ(result.fg(), base.has_fg() ? foreground : supplied);
        EXPECT_EQ(result.bg(), base.has_bg() ? background : supplied);
        EXPECT_TRUE(result.is_bold() && result.is_underline() && result.is_absolute());
        auto member = base;
        EXPECT_EQ(&member.underlay(defaults), &member);
        EXPECT_EQ(member, result);
        EXPECT_EQ(tjg::color::underlay(base, tjg::color::text_style{}), base);
        EXPECT_EQ(tjg::color::underlay(base, base), base);
      }
    }
  }
}

TEST(color_test, text_style_underlay_constexpr) {
  constexpr auto base = tjg::color::fg(tjg::color::term_color::yellow);
  constexpr auto defaults = tjg::color::fg(tjg::color::term_color::white).bg(tjg::color::term_color::blue);
  constexpr auto result = tjg::color::underlay(base, defaults);
  static_assert(std::get<tjg::color::term_color>(result.fg()) == tjg::color::term_color::yellow,
                "preserve foreground");
  static_assert(std::get<tjg::color::term_color>(result.bg()) == tjg::color::term_color::blue,
                "supply background");
  constexpr auto member = tjg::color::text_style{base}.underlay(defaults);
  static_assert(member == result, "member and free underlay agree");
}

TEST(color_test, standard_sgr_reset_view) {
  static_assert(std::is_same_v<decltype(tjg::color::sgr_reset()), std::string_view>);
  static_assert(std::is_same_v<decltype(tjg::color::sgr_reset<wchar_t>()),
                               std::wstring_view>);
  static_assert(tjg::color::sgr_reset() == std::string_view{"\x1b[0m"});
  static_assert(tjg::color::sgr_reset<wchar_t>() == std::wstring_view{L"\x1b[0m"});
}
