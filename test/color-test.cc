#include "standard-format-test.h"
// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "tjg/color.h"

#include <iterator>  // std::back_inserter
#include <limits>    // std::numeric_limits
#include <map>       // std::map

#include <type_traits>  // std::is_convertible, std::is_same
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <utility>  // std::declval, std::move

#include "gtest-extra.h"  // EXPECT_WRITE, EXPECT_THROW_MSG

#if TJG_COLOR_COMPACT_SGR == 1
#  define FMT_SGR_EXPECTED(legacy, compact) compact
#else
#  define FMT_SGR_EXPECTED(legacy, compact) legacy
#endif

TEST(color_test, absolute_style_flag) {
  static_assert(sizeof(tjg::text_style) == sizeof(uint64_t),
                "absolute does not increase style size");
  static_assert(!tjg::text_style{}.is_absolute(), "styles default to relative");
  static_assert(noexcept(std::declval<tjg::text_style&>().absolute()) &&
                    noexcept(tjg::text_style{}.is_absolute()),
                "absolute accessors do not throw");
  auto style = tjg::fg(tjg::term_color::red) | tjg::bg(tjg::gray_color{12}) |
               tjg::underline();
  const auto original = style;
  EXPECT_EQ(&style.absolute(), &style);
  EXPECT_TRUE(style.is_absolute());
  EXPECT_EQ(tjg::to_underlying(style),
            tjg::to_underlying(original) | (uint64_t{1} << 62));
  EXPECT_NE(style, original);
  EXPECT_EQ(style.fg(), original.fg());
  EXPECT_EQ(style.bg(), original.bg());
  EXPECT_EQ(style.emph(), original.emph());
  EXPECT_EQ(&style.absolute(false), &style);
  EXPECT_EQ(style, original);
  style.absolute().fg(tjg::no_color).bg(tjg::no_color).emph(tjg::emphasis{});
  EXPECT_FALSE(style.empty());
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
  EXPECT_EQ(&style.clear(), &style);
  EXPECT_EQ(style, tjg::text_style{});
  EXPECT_FALSE(style.is_absolute());
}

TEST(color_test, absolute_style_helper) {
  static_assert(noexcept(tjg::absolute(tjg::text_style{})),
                "absolute style helper does not throw");
  const auto source = tjg::fg(tjg::term_color::red) |
                      tjg::bg(tjg::gray_color{12}) | tjg::underline();
  auto expected = source;
  expected.absolute();
  EXPECT_EQ(tjg::absolute(source), expected);
  EXPECT_FALSE(source.is_absolute());
  EXPECT_EQ(tjg::absolute(expected), expected);
  EXPECT_EQ(tjg::absolute(tjg::text_style{}), tjg::text_style{}.absolute());
#if 1
  constexpr auto value = tjg::absolute(tjg::text_style{});
  static_assert(value.is_absolute(), "absolute helper supports constant evaluation");
#endif
}

TEST(color_test, absolute_style_composition) {
  const tjg::color_t colors[] = {tjg::no_color, tjg::rgb{1, 2, 3},
      tjg::term_color::red, tjg::cube_color{1, 2, 3}, tjg::gray_color{12}};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto base = tjg::text_style{foreground, background,
                                       tjg::emphasis::underline};
      const auto other = tjg::text_style{background, foreground,
                                        tjg::emphasis::bold};
      for (unsigned flags = 0; flags != 4; ++flags) {
        auto lhs = base;
        auto rhs = other;
        lhs.absolute((flags & 1u) != 0);
        rhs.absolute((flags & 2u) != 0);
        auto expected_overlay = tjg::overlay(base, other);
        auto expected_or = base | other;
        expected_overlay.absolute(flags != 0);
        expected_or.absolute(flags != 0);
        EXPECT_EQ(tjg::overlay(lhs, rhs), expected_overlay);
        EXPECT_EQ(lhs | rhs, expected_or);
        EXPECT_EQ(rhs | lhs, expected_or);
        auto assigned = lhs;
        assigned |= rhs;
        EXPECT_EQ(assigned, expected_or);
      }
    }
  }
  const auto absolute_only = tjg::text_style{}.absolute();
  EXPECT_EQ(tjg::overlay(tjg::text_style{}, absolute_only), absolute_only);
  EXPECT_EQ(tjg::overlay(absolute_only, tjg::text_style{}), absolute_only);
  auto style = tjg::underline().absolute();
  style.overlay(tjg::bold());
  EXPECT_TRUE(style.is_absolute());
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_underline());
  style |= tjg::italic();
  EXPECT_TRUE(style.is_italic());
  EXPECT_TRUE(style.is_absolute());
}

TEST(color_test, absolute_style_blend) {
  const auto full = tjg::text_style{tjg::rgb{10, 20, 30}, tjg::gray_color{12},
                                   tjg::emphasis::bold}.absolute();
  auto relative = full;
  relative.absolute(false);
  const auto other = tjg::text_style{tjg::term_color::blue, tjg::no_color,
                                    tjg::emphasis::underline};
  const auto expected = tjg::blend(relative, other);
  EXPECT_EQ(tjg::blend(full, other), expected);
  EXPECT_EQ(tjg::blend(other, full), expected);
  EXPECT_EQ(tjg::blend(other, relative, full),
            tjg::blend(other, relative, relative));
  EXPECT_FALSE(tjg::blend(full, other).is_absolute());
  EXPECT_TRUE(full.is_absolute());
  static_assert(noexcept(tjg::blend(full, other)), "blend remains noexcept");
  const tjg::text_style invalid[] = {
      tjg::text_style{}.absolute(),
      tjg::fg(tjg::color::red).absolute(),
      tjg::bg(tjg::color::blue).absolute()};
  for (auto input : invalid) {
#ifdef NDEBUG
    auto cleared = input;
    cleared.absolute(false);
    EXPECT_EQ(tjg::blend(input, other), tjg::blend(cleared, other));
    EXPECT_EQ(tjg::blend(other, input), tjg::blend(other, cleared));
    EXPECT_EQ(tjg::blend(other, full, input),
              tjg::blend(other, relative, cleared));
    EXPECT_FALSE(tjg::blend(input, other).is_absolute());
#elif !defined(FMTCOLOR_SKIP)
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::blend(input, other),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::blend(other, input),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)tjg::blend(other, full, input),
                              "terminal-default colors");
#else
    (void)input;
#endif
  }
}

#if 1
TEST(color_test, absolute_style_constexpr) {
  constexpr auto reset = tjg::text_style{}.absolute();
  static_assert(reset.is_absolute() && !reset.empty(),
                "a reset-only style is absolute and nonempty");
  constexpr auto full = tjg::text_style{tjg::rgb{10, 20, 30},
                                       tjg::gray_color{12}}.absolute();
  constexpr auto modified = tjg::text_style{full}.overlay(tjg::bold());
  static_assert(modified.is_absolute() && modified.is_bold(),
                "explicit overlays preserve absolute during evaluation");
  constexpr auto combined = tjg::fg(tjg::term_color::red) | full;
  static_assert(combined.is_absolute(), "promoted OR preserves absolute");
  constexpr auto result = tjg::blend(full, tjg::text_style{});
  static_assert(!result.is_absolute() && result.fg() == full.fg() &&
                    result.bg() == full.bg(),
                "valid absolute blends remain constant-evaluable and relative");
  constexpr auto cleared = tjg::text_style{full}.clear();
  static_assert(cleared.empty() && !cleared.is_absolute(),
                "clear removes absolute during evaluation");
}
#endif

template<typename T>
void check_color_comparisons(const T* values, size_t count) {
  static_assert(noexcept(T{} == T{}), "equality must be noexcept");
  static_assert(noexcept(T{} != T{}), "inequality must be noexcept");
  static_assert(noexcept(T{} < T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} <= T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} > T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} >= T{}), "ordering must be noexcept");
  static_assert(T{} == T{}, "default values must compare equal");
  static_assert(!(T{} < T{}), "ordering must be irreflexive");
#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L
  static_assert(noexcept(T{} <=> T{}), "three-way comparison must be noexcept");
  static_assert(std::is_same<decltype(T{} <=> T{}),
                             std::strong_ordering>::value,
                "integral representations must have a strong ordering");
  static_assert((T{} <=> T{}) == std::strong_ordering::equal,
                "default values must compare equal");
#endif

  auto keys = std::map<T, size_t>();
  auto integers = std::map<uint64_t, size_t>();
  for (size_t i = 0; i < count; ++i) {
    keys[values[i]] = i;
    integers[static_cast<uint64_t>(tjg::to_underlying(values[i]))] = i;
    for (size_t j = 0; j < count; ++j) {
      auto lhs = tjg::to_underlying(values[i]);
      auto rhs = tjg::to_underlying(values[j]);
      EXPECT_EQ(values[i] == values[j], lhs == rhs);
      EXPECT_EQ(values[i] != values[j], lhs != rhs);
      EXPECT_EQ(values[i] < values[j], lhs < rhs);
      EXPECT_EQ(values[i] <= values[j], lhs <= rhs);
      EXPECT_EQ(values[i] > values[j], lhs > rhs);
      EXPECT_EQ(values[i] >= values[j], lhs >= rhs);
#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L
      EXPECT_EQ(values[i] <=> values[j], lhs <=> rhs);
#endif
    }
  }
  EXPECT_EQ(keys.size(), integers.size());
  auto expected = integers.begin();
  for (const auto& entry : keys) {
    ASSERT_NE(expected, integers.end());
    EXPECT_EQ(tjg::to_underlying(entry.first), expected->first);
    EXPECT_EQ(entry.second, expected->second);
    ++expected;
  }
}

TEST(color_test, rgb_comparisons) {
  constexpr auto low = tjg::rgb{0, 255, 255};
  constexpr auto high = tjg::rgb{1, 0, 0};
  static_assert(low < high, "red precedes green and blue in packed ordering");
  static_assert(tjg::rgb{0xAB123456} == tjg::rgb{0x123456},
                "the discarded high byte must not affect equality");
  const tjg::rgb values[] = {
      {}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 0, 0},
      {0, 255, 255}, {255, 255, 255},
      tjg::rgb{0xAB123456}, tjg::rgb{0x123456}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, gray_color) {
  using tjg::gray_color;
  static_assert(std::is_same<gray_color::value_type, uint8_t>::value &&
                    std::is_same<tjg::underlying_type<gray_color>::type,
                                 uint8_t>::value,
                "grayscale indices use uint8_t");
  static_assert(std::is_same<decltype(tjg::to_underlying(gray_color{})),
                             uint8_t>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<gray_color, tjg::rgb>::value &&
                    !std::is_convertible<int, gray_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(noexcept(static_cast<tjg::rgb>(gray_color{})),
                "RGB conversion must be noexcept");
  constexpr gray_color first{0}, last{23};
  static_assert(tjg::to_underlying(first) == 0 &&
                    tjg::to_underlying(last) == 23,
                "boundary indices must support C++11 constant evaluation");
  constexpr tjg::rgb dark = first, light = last;
  static_assert(dark == tjg::rgb{8, 8, 8} && light == tjg::rgb{238, 238, 238},
                "grayscale conversion supports C++11 constant evaluation");
  constexpr tjg::rgb middle = gray_color{12};
  static_assert(middle == tjg::rgb{128, 128, 128},
                "a temporary gray color supports C++11 RGB conversion");
  EXPECT_EQ(tjg::to_underlying(gray_color{}), 0);
  const int indices[] = {0, 1, 12, 22, 23};
  const unsigned levels[] = {8, 18, 128, 228, 238};
  for (size_t i = 0; i != sizeof(indices) / sizeof(indices[0]); ++i) {
    SCOPED_TRACE(indices[i]);
    gray_color color{indices[i]};
    tjg::rgb converted = color;
    EXPECT_EQ(tjg::to_underlying(color), indices[i]);
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
  using tjg::cube_color;
  static_assert(std::is_same<cube_color::value_type, uint8_t>::value &&
                    std::is_same<tjg::underlying_type<cube_color>::type,
                                 uint8_t>::value,
                "cube indices use uint8_t");
  static_assert(std::is_same<decltype(tjg::to_underlying(cube_color{})),
                             uint8_t>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<cube_color, tjg::rgb>::value &&
                    !std::is_convertible<unsigned, cube_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(std::is_same<decltype(cube_color{}.r()), int>::value &&
                    std::is_same<decltype(cube_color{}.g()), int>::value &&
                    std::is_same<decltype(cube_color{}.b()), int>::value,
                "cube channel getters return int");
  static_assert(noexcept(static_cast<tjg::rgb>(cube_color{})),
                "RGB conversion must be noexcept");
  constexpr cube_color first{0}, last{215}, middle{1, 2, 3};
  constexpr tjg::rgb converted = middle;
  static_assert(tjg::to_underlying(first) == 0 &&
                    tjg::to_underlying(last) == 215 && last.r() == 5 &&
                    last.g() == 5 && last.b() == 5,
                "cube index decoding supports C++11 constant evaluation");
  static_assert(converted == tjg::rgb{95, 135, 175} &&
                    tjg::to_underlying(middle) == 51,
                "cube encoding and RGB conversion support C++11");
  EXPECT_EQ(tjg::to_underlying(cube_color{}), 0);
  static_assert(noexcept(cube_color{0}) && noexcept(cube_color{0, 0, 0}),
                "cube construction must be noexcept");
  static_assert(noexcept(std::declval<cube_color&>().r(0)) &&
                    noexcept(std::declval<cube_color&>().g(0)) &&
                    noexcept(std::declval<cube_color&>().b(0)),
                "cube setters must be noexcept");
#ifdef NDEBUG
  const unsigned invalid_indices[] = {216, 217, 251, 255, 256,
                                     (std::numeric_limits<unsigned>::max)()};
  for (auto index : invalid_indices) {
    SCOPED_TRACE(index);
    EXPECT_EQ(cube_color{index}, cube_color{215});
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
        unsigned index = static_cast<unsigned>(36 * red + 6 * green + blue);
        SCOPED_TRACE(index);
        tjg::cube_color color{index};
        EXPECT_EQ(color.r(), red);
        EXPECT_EQ(color.g(), green);
        EXPECT_EQ(color.b(), blue);
        EXPECT_EQ(tjg::to_underlying(color), index);
        EXPECT_EQ(color, (tjg::cube_color{red, green, blue}));
        tjg::rgb converted = color;
        EXPECT_EQ(converted.r, red == 0 ? 0 : 55 + 40 * red);
        EXPECT_EQ(converted.g, green == 0 ? 0 : 55 + 40 * green);
        EXPECT_EQ(converted.b, blue == 0 ? 0 : 55 + 40 * blue);
      }
    }
  }
}

TEST(color_test, terminal_color_xterm_rgb) {
  // Agreed Xterm-default palette, independent of the implementation table.
  const tjg::rgb expected[] = {
      {0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
      {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
      {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
      {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
  const tjg::rgb named[] = {
      tjg::color::term_black, tjg::color::term_red, tjg::color::term_green,
      tjg::color::term_yellow, tjg::color::term_blue, tjg::color::term_magenta,
      tjg::color::term_cyan, tjg::color::term_white,
      tjg::color::term_bright_black, tjg::color::term_bright_red,
      tjg::color::term_bright_green, tjg::color::term_bright_yellow,
      tjg::color::term_bright_blue, tjg::color::term_bright_magenta,
      tjg::color::term_bright_cyan, tjg::color::term_bright_white};
  static_assert(std::is_convertible<tjg::term_color, tjg::rgb>::value,
                "terminal-to-RGB conversion must be implicit");
  static_assert(std::is_same<std::underlying_type<tjg::term_color>::type,
                             int8_t>::value,
                "terminal enum storage must be signed");
  static_assert(tjg::to_underlying(static_cast<tjg::term_color>(-1)) == -1,
                "underlying conversion must preserve signed enum values");
  constexpr tjg::rgb black = tjg::term_color::black;
  constexpr tjg::rgb bright = tjg::term_color::bright_white;
  static_assert(black == tjg::rgb{} && bright == tjg::rgb{255, 255, 255},
                "terminal conversion supports C++11 constant evaluation");
  for (unsigned index = 0; index != 16; ++index) {
    SCOPED_TRACE(index);
    tjg::rgb converted = static_cast<tjg::term_color>(index);
    EXPECT_EQ(converted, expected[index]);
    EXPECT_EQ(named[index], expected[index]);
  }
}

TEST(color_test, color_type_comparisons) {
  using tjg::color_t;
  constexpr auto low = tjg::detail::unpack_color(uint32_t{1} << 24);
  constexpr auto high = tjg::detail::unpack_color(uint32_t{2} << 24);
  static_assert(color_t{} < low, "unspecified differs from RGB black");
  static_assert(low < high, "RGB black differs from terminal black");
  const color_t values[] = {
      {}, tjg::no_color, tjg::rgb{}, tjg::term_color::black,
      tjg::rgb{0xFFFFFF}, tjg::term_color::bright_white,
      tjg::rgb{0x123456}, tjg::color::coral, tjg::rgb{0xFF7F50},
      tjg::cube_color{0}, tjg::cube_color{215}, tjg::gray_color{0},
      tjg::gray_color{23}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_comparisons) {
  constexpr auto low = tjg::text_style{};
  constexpr auto high = tjg::text_style{tjg::emphasis::bold};
  static_assert(low < high, "an empty style sorts before bold");
  const tjg::text_style values[] = {
      {}, tjg::fg(tjg::no_color), tjg::fg(tjg::rgb{}),
      tjg::fg(tjg::term_color::black), tjg::bg(tjg::rgb{}),
      tjg::bg(tjg::term_color::black), tjg::fg(tjg::rgb{0xFFFFFF}),
      tjg::emphasis::bold, tjg::emphasis::strike,
      tjg::fg(tjg::color::coral) | tjg::bg(tjg::term_color::bright_white) |
          tjg::emphasis::bold | tjg::emphasis::strike,
      tjg::fg(tjg::cube_color{0}), tjg::bg(tjg::cube_color{215}),
      tjg::fg(tjg::gray_color{0}), tjg::bg(tjg::gray_color{23})};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

template<typename T>
void check_color_hashes(const T* values, size_t count) {
  using value_type = typename tjg::underlying_type<T>::type;
  static_assert(noexcept(std::hash<T>{}(T{})), "hashing must be noexcept");
  static_assert(std::is_same<decltype(std::hash<T>{}(T{})), size_t>::value,
                "hashing must return size_t");
  auto hasher = std::hash<T>();
  auto integer_hasher = std::hash<value_type>();
  auto keys = std::unordered_set<T>();
  auto entries = std::unordered_map<T, size_t>();
  auto integers = std::map<value_type, size_t>();
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(hasher(values[i]), integer_hasher(tjg::to_underlying(values[i])));
    keys.insert(values[i]);
    entries[values[i]] = i;
    integers[tjg::to_underlying(values[i])] = i;
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
    EXPECT_EQ(entries.at(values[i]), integers.at(tjg::to_underlying(values[i])));
  }
}

TEST(color_test, rgb_hash) {
  const tjg::rgb values[] = {
      {}, {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 255, 255},
      tjg::rgb{0xAB123456}, tjg::rgb{0x123456},
      tjg::color::coral, tjg::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, color_type_hash) {
  using tjg::color_t;
  const color_t values[] = {
      {}, tjg::no_color, tjg::rgb{}, tjg::term_color::black,
      tjg::rgb{0xFFFFFF}, tjg::term_color::bright_white,
      tjg::color::coral, tjg::rgb{0xFF7F50}, tjg::cube_color{0},
      tjg::cube_color{215}, tjg::gray_color{0}, tjg::gray_color{23}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_hash) {
  const tjg::text_style values[] = {
      {}, tjg::fg(tjg::no_color), tjg::fg(tjg::rgb{}),
      tjg::fg(tjg::term_color::black), tjg::bg(tjg::rgb{}),
      tjg::bg(tjg::term_color::black), tjg::emphasis::bold,
      tjg::emphasis::strike, tjg::fg(tjg::color::coral),
      tjg::fg(tjg::rgb{0xFF7F50}),
      tjg::fg(tjg::color::coral) | tjg::bg(tjg::term_color::bright_white) |
          tjg::emphasis::bold | tjg::emphasis::strike,
      tjg::fg(tjg::cube_color{0}), tjg::bg(tjg::cube_color{215}),
      tjg::fg(tjg::gray_color{0}), tjg::bg(tjg::gray_color{23})};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, emphasis_or_assign) {
  using tjg::emphasis;
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
      EXPECT_EQ(tjg::to_underlying(value), lhs | rhs);
      EXPECT_EQ(&result, &value);
      EXPECT_EQ(tjg::to_underlying(other), rhs);
    }
  }
  auto value = emphasis::bold;
  (value |= emphasis::underline) |= emphasis::strike;
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
  tjg::operator|=(value, value);
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
}

#if 1
constexpr auto check_emphasis_or_assign() -> bool {
  auto value = tjg::emphasis::bold;
  auto& result = (value |= tjg::emphasis::underline);
  (result |= tjg::emphasis::strike) |= tjg::emphasis{};
  tjg::operator|=(value, value);
  return &result == &value &&
         value == (tjg::emphasis::bold | tjg::emphasis::underline |
                   tjg::emphasis::strike);
}

TEST(color_test, emphasis_or_assign_constexpr) {
  static_assert(check_emphasis_or_assign(),
                "compound assignment must support constant evaluation");
}
#endif

TEST(color_test, underlying) {
  static_assert(std::is_same<tjg::underlying_type<tjg::term_color>::type,
                             int8_t>::value, "terminal color underlying type");
  static_assert(std::is_same<tjg::underlying_type<tjg::emphasis>::type,
                             uint8_t>::value, "emphasis underlying type");
  static_assert(std::is_same<tjg::underlying_type<tjg::rgb>::type,
                             uint32_t>::value, "RGB underlying type");
  static_assert(std::is_same<tjg::underlying_type<tjg::text_style>::type,
                             uint64_t>::value, "text style underlying type");

  static_assert(std::is_same<decltype(tjg::to_underlying(
                                 tjg::term_color::black)),
                             int8_t>::value, "terminal color return type");
  static_assert(std::is_same<decltype(tjg::to_underlying(tjg::emphasis::bold)),
                             uint8_t>::value, "emphasis return type");
  static_assert(std::is_same<decltype(tjg::to_underlying(tjg::rgb{})),
                             uint32_t>::value, "RGB return type");
  static_assert(std::is_same<decltype(tjg::to_underlying(tjg::text_style{})),
                             uint64_t>::value, "text style return type");

  static_assert(noexcept(tjg::to_underlying(tjg::term_color::black)),
                "terminal color conversion must be noexcept");
  static_assert(noexcept(tjg::to_underlying(tjg::emphasis::bold)),
                "emphasis conversion must be noexcept");
  static_assert(noexcept(tjg::to_underlying(tjg::rgb{})),
                "RGB conversion must be noexcept");
  static_assert(noexcept(tjg::to_underlying(tjg::text_style{})),
                "text style conversion must be noexcept");

  static_assert(tjg::to_underlying(tjg::term_color::black) ==
                    static_cast<uint8_t>(tjg::term_color::black),
                "terminal color conversion must preserve the enum value");
  static_assert(tjg::to_underlying(tjg::term_color::bright_white) ==
                    static_cast<uint8_t>(tjg::term_color::bright_white),
                "bright terminal color conversion must preserve the enum value");
  static_assert(tjg::to_underlying(tjg::emphasis::bold) == 1,
                "bold must retain its bit");
  static_assert(tjg::to_underlying(tjg::emphasis::strike) == 128,
                "strike must retain its bit");
  static_assert(tjg::to_underlying(tjg::rgb{0xAB123456}) == 0x123456,
                "packed RGB must have a zero high byte");
  static_assert(tjg::to_underlying(tjg::rgb{}) == 0,
                "default RGB must be black");
  static_assert(tjg::to_underlying(tjg::text_style{}) == 0,
                "default text style must be empty");
  static_assert(tjg::to_underlying(
                    tjg::text_style{tjg::emphasis::strike}) ==
                    (uint64_t{128} << 54),
                "text style conversion must preserve high emphasis bits");

  auto foreground = tjg::fg(tjg::rgb{0x123456});
  auto background = tjg::bg(tjg::term_color::black);
  auto attributes = tjg::text_style{tjg::emphasis::bold |
                                    tjg::emphasis::strike};
  auto style = foreground | background | attributes;
  EXPECT_EQ(tjg::to_underlying(style),
            tjg::to_underlying(foreground) | tjg::to_underlying(background) |
                tjg::to_underlying(attributes));
  EXPECT_NE(tjg::to_underlying(style),
            tjg::to_underlying(background | attributes));
  EXPECT_NE(tjg::to_underlying(style),
            tjg::to_underlying(foreground | attributes));
  EXPECT_NE(tjg::to_underlying(style),
            tjg::to_underlying(foreground | background));
}

TEST(color_test, color_type_underlying) {
  using tjg::color_t;
  static_assert(std::is_same<tjg::underlying_type<color_t>::type,
                             uint32_t>::value, "packed color underlying type");
  static_assert(std::is_same<decltype(tjg::to_underlying(color_t{})),
                             uint32_t>::value, "packed color return type");
  static_assert(noexcept(tjg::to_underlying(color_t{})),
                "packed color conversion must be noexcept");
  static_assert(tjg::to_underlying(color_t{}) == 0,
                "default packed color must be empty");

  // Check the internal packed representation against independent expected values.
  const color_t colors[] = {
      color_t{}, color_t{tjg::rgb{0}},
      color_t{tjg::rgb{0x123456}},
      color_t{tjg::term_color::black}, color_t{tjg::cube_color{215}},
      color_t{tjg::gray_color{23}}};
  const uint32_t expected[] = {
      0u, (uint32_t{1} << 24),
      (uint32_t{1} << 24) | 0x123456u, (uint32_t{2} << 24),
      (uint32_t{3} << 24) | 215u, (uint32_t{4} << 24) | 23u};
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
    EXPECT_EQ(tjg::to_underlying(colors[i]), expected[i]);
}

namespace {
struct composition_sample {
  tjg::color_t color;
  tjg::rgb promoted;
};

const composition_sample composition_samples[] = {
    {tjg::no_color, {0, 0, 0}},
    {tjg::rgb{1, 2, 3}, {1, 2, 3}},
    {tjg::term_color::red, {205, 0, 0}},
    {tjg::cube_color{1, 2, 3}, {95, 135, 175}},
    {tjg::gray_color{12}, {128, 128, 128}}};

// Expected RGB values above are independent of the conversion implementation.
tjg::color_t expected_composition(size_t left, size_t right) {
  if (left == 0) return composition_samples[right].color;
  if (right == 0) return composition_samples[left].color;
  auto lhs = composition_samples[left].promoted;
  auto rhs = composition_samples[right].promoted;
  return tjg::rgb{static_cast<uint8_t>(lhs.r | rhs.r),
                 static_cast<uint8_t>(lhs.g | rhs.g),
                 static_cast<uint8_t>(lhs.b | rhs.b)};
}
}  // namespace

TEST(color_test, color_type_composition) {
  static_assert(noexcept(tjg::color_t{} | tjg::color_t{}) &&
                    noexcept(std::declval<tjg::color_t&>() |= tjg::color_t{}),
                "color composition must be noexcept");
  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("color kinds {} and {}", i, j));
      auto lhs = composition_samples[i].color;
      auto rhs = composition_samples[j].color;
      const auto expected = expected_composition(i, j);
      EXPECT_EQ(lhs | rhs, expected);
      EXPECT_EQ(lhs, composition_samples[i].color);
      EXPECT_EQ(&(lhs |= rhs), &lhs);
      EXPECT_EQ(lhs, expected);
      EXPECT_EQ(rhs, composition_samples[j].color);
    }
    auto self = composition_samples[i].color;
    EXPECT_EQ(&(self |= self), &self);
    EXPECT_EQ(self, expected_composition(i, i));
  }
  auto lhs = tjg::color_t{tjg::rgb{0xC0F000}};
  auto rhs = tjg::color_t{tjg::rgb{0x000FEE}};
  EXPECT_EQ(lhs | rhs, tjg::color_t{tjg::rgb{0xC0FFEE}});
  EXPECT_EQ(lhs, tjg::color_t{tjg::rgb{0xC0F000}});
  EXPECT_EQ(rhs, tjg::color_t{tjg::rgb{0x000FEE}});
}

TEST(color_test, color_sentinels) {
  using tjg::no_color_t;
  static_assert(std::is_empty<no_color_t>::value,
                "the unspecified sentinel has no payload");
  static_assert(std::is_default_constructible<no_color_t>::value,
                "the unspecified sentinel must be constructible");
  // GCC 4.9's standard library does not provide this trait.
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
  static_assert(std::is_trivially_copyable<tjg::color_t>::value,
                "alternative replacement relies on trivial union copying");
#endif
  static_assert(std::is_same<tjg::underlying_type<no_color_t>::type,
                             unsigned>::value,
                "the sentinel has an unsigned numeric representation");
  static_assert(tjg::to_underlying(tjg::no_color) == 0,
                "the unspecified sentinel has zero payload");
  static_assert(noexcept(tjg::to_underlying(tjg::no_color)),
                "sentinel conversion must be noexcept");
}

TEST(color_test, text_style_empty_and_clear) {
  constexpr auto empty = tjg::text_style{};
  static_assert(empty.empty(), "default style must be empty");
  static_assert(!tjg::text_style{tjg::rgb{}}.empty(),
                "specified RGB black must not be empty");
  static_assert(!tjg::text_style{tjg::term_color::black}.empty(),
                "specified terminal black must not be empty");
  static_assert(!tjg::text_style{tjg::emphasis::strike}.empty(),
                "emphasis alone must not be empty");
  static_assert(noexcept(empty.empty()) &&
                    noexcept(std::declval<tjg::text_style&>().clear()),
                "empty and clear must be noexcept");
  static_assert(std::is_same<decltype(empty.empty()), bool>::value &&
                    std::is_same<decltype(std::declval<tjg::text_style&>().clear()),
                                 tjg::text_style&>::value,
                "empty returns bool and clear returns a style reference");

  const tjg::color_t colors[] = {tjg::no_color, tjg::rgb{},
      tjg::term_color::black, tjg::cube_color{0}, tjg::gray_color{0}};
  const tjg::emphasis attributes[] = {
      tjg::emphasis{}, tjg::emphasis::bold, tjg::emphasis::strike,
      static_cast<tjg::emphasis>(255)};
  for (size_t fg = 0; fg < 5; ++fg) {
    for (size_t bg = 0; bg < 5; ++bg) {
      for (auto attribute : attributes) {
        auto style = tjg::text_style{colors[fg], colors[bg], attribute};
        EXPECT_EQ(style.empty(),
                  fg == 0 && bg == 0 && attribute == tjg::emphasis{});
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        EXPECT_EQ(style, tjg::text_style{});
        EXPECT_FALSE(style.has_fg());
        EXPECT_FALSE(style.has_bg());
        EXPECT_FALSE(style.has_emph());
        EXPECT_EQ(tjg::format(style, "plain"), "plain");
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        style.clear().fg(tjg::gray_color{12}).underline();
        EXPECT_EQ(style, (tjg::text_style{tjg::gray_color{12}, tjg::no_color,
                                         tjg::emphasis::underline}));
      }
    }
  }
}

TEST(color_test, text_style_overlay) {
  using tjg::emphasis;
  using tjg::text_style;
  static_assert(noexcept(std::declval<text_style&>().overlay(
                    std::declval<const text_style&>())),
                "overlay must be noexcept");
  static_assert(std::is_same<decltype(std::declval<text_style&>().overlay(
                                std::declval<const text_style&>())),
                             text_style&>::value,
                "overlay must return a style reference");
  static_assert(noexcept(tjg::overlay(text_style{}, text_style{})),
                "free overlay must be noexcept");
  static_assert(std::is_same<decltype(tjg::overlay(text_style{}, text_style{})),
                             text_style>::value,
                "free overlay must return a style value");

  const tjg::color_t base_colors[] = {
      tjg::no_color, tjg::rgb{200, 10, 30}, tjg::term_color::red,
      tjg::cube_color{5, 4, 3}, tjg::gray_color{23}};
  const tjg::color_t overlay_colors[] = {
      tjg::no_color, tjg::rgb{}, tjg::term_color::black,
      tjg::cube_color{0}, tjg::gray_color{0}};
  for (auto base_fg : base_colors) {
    for (auto base_bg : base_colors) {
      const auto base = text_style{base_fg, base_bg, emphasis::bold};
      for (size_t fg = 0; fg < 5; ++fg) {
        for (size_t bg = 0; bg < 5; ++bg) {
          const auto other = text_style{overlay_colors[fg], overlay_colors[bg],
                                       emphasis::faint};
          const auto saved = other;
          auto input = base;
          const auto combined = tjg::overlay(input, other);
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
      EXPECT_EQ(tjg::overlay(base, text_style{}), base);
      EXPECT_EQ(tjg::overlay(text_style{}, base), base);
      EXPECT_EQ(tjg::overlay(base, base), base);
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
  EXPECT_EQ(tjg::overlay(empty, empty), empty);
  EXPECT_EQ(&empty.overlay(empty), &empty);
  EXPECT_TRUE(empty.empty());
  auto style = text_style{tjg::term_color::red, tjg::gray_color{12},
                          emphasis::bold};
  style.overlay(text_style{tjg::rgb{}}).underline();
  EXPECT_EQ(style, (text_style{tjg::rgb{}, tjg::gray_color{12},
                              emphasis::bold | emphasis::underline}));
  const auto first = text_style{tjg::term_color::red};
  const auto second = text_style{tjg::term_color::blue};
  EXPECT_EQ(tjg::overlay(first, second), second);
  EXPECT_EQ(tjg::overlay(second, first), first);
}

#if 1
TEST(color_test, text_style_overlay_constexpr) {
  constexpr auto base = tjg::text_style{tjg::term_color::red,
                                       tjg::gray_color{12}, tjg::emphasis::bold};
  constexpr auto overlaid = tjg::text_style{base}.overlay(
      tjg::text_style{tjg::rgb{}, tjg::no_color, tjg::emphasis::faint});
  static_assert(overlaid == tjg::text_style{
                                tjg::rgb{}, tjg::gray_color{12},
                                tjg::emphasis::bold | tjg::emphasis::faint},
                "overlay must support constant evaluation");
  constexpr auto unchanged = tjg::text_style{base}.overlay(tjg::text_style{});
  static_assert(unchanged == base, "an empty overlay must preserve the style");
  constexpr auto copied = tjg::text_style{}.overlay(base);
  static_assert(copied == base, "overlay must replace an empty base");
  constexpr auto equal = tjg::text_style{base}.overlay(base);
  static_assert(equal == base, "an equal overlay must preserve the style");
  constexpr auto overrides = tjg::text_style{tjg::rgb{}, tjg::no_color,
                                            tjg::emphasis::faint};
  constexpr auto combined = tjg::overlay(base, overrides);
  static_assert(combined == overlaid,
                "free overlay must support constant evaluation");
  static_assert(tjg::overlay(base, tjg::text_style{}) == base &&
                    tjg::overlay(tjg::text_style{}, base) == base &&
                    tjg::overlay(base, base) == base,
                "free overlay must preserve empty and equal cases");
  static_assert(base.fg() == tjg::term_color::red &&
                    overrides.fg() == tjg::rgb{},
                "free overlay must preserve its inputs");
}
#endif

TEST(color_test, text_style_color_composition) {
  static_assert(noexcept(tjg::text_style{} | tjg::text_style{}) &&
                    noexcept(std::declval<tjg::text_style&>() |=
                             tjg::text_style{}),
                "style composition must be noexcept");
  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("style color kinds {} and {}", i, j));
      const auto left = tjg::text_style{composition_samples[i].color,
                                       composition_samples[j].color,
                                       tjg::emphasis::bold};
      const auto right = tjg::text_style{composition_samples[j].color,
                                        composition_samples[i].color,
                                        tjg::emphasis::strike};
      const auto expected = tjg::text_style{
          expected_composition(i, j), expected_composition(j, i),
          tjg::emphasis::bold | tjg::emphasis::strike};
      EXPECT_EQ(left | right, expected);
      auto assigned = left;
      EXPECT_EQ(&(assigned |= right), &assigned);
      EXPECT_EQ(assigned, expected);
      EXPECT_EQ(left.fg(), composition_samples[i].color);
      EXPECT_EQ(right.bg(), composition_samples[i].color);
      auto self = left;
      self.operator|=(self);
      EXPECT_EQ(self, (tjg::text_style{expected_composition(i, i),
                                      expected_composition(j, j),
                                      tjg::emphasis::bold}));
    }
  }
}

TEST(color_test, palette_style_replacement) {
  const tjg::color_t replacements[] = {
      tjg::gray_color{23}, tjg::cube_color{215}, tjg::term_color::black,
      tjg::rgb{}, tjg::no_color};
  auto style = tjg::text_style{tjg::gray_color{0}, tjg::gray_color{23},
                              static_cast<tjg::emphasis>(0xFF)};
  for (auto replacement : replacements) {
    const auto background = style.bg();
    EXPECT_EQ(&style.fg(replacement), &style);
    EXPECT_EQ(style.fg(), replacement);
    EXPECT_EQ(style.bg(), background);
    EXPECT_EQ(style.has_fg(), replacement.index() != 0);
    EXPECT_EQ(tjg::to_underlying(style.emph()), 0xFF);
  }
  style.fg(tjg::gray_color{23});
  for (auto replacement : replacements) {
    EXPECT_EQ(&style.bg(replacement), &style);
    EXPECT_EQ(style.bg(), replacement);
    EXPECT_EQ(style.fg(), tjg::color_t{tjg::gray_color{23}});
    EXPECT_EQ(style.has_bg(), replacement.index() != 0);
    EXPECT_EQ(tjg::to_underlying(style.emph()), 0xFF);
  }
  style.fg(tjg::no_color).emph(tjg::emphasis{});
  EXPECT_EQ(style, tjg::text_style{});
}

namespace {
void check_to_sgr(const tjg::text_style& style, const std::string& expected) {
  static_assert(std::is_same<decltype(tjg::to_sgr(style)), std::string>::value,
                "the default character type must be char");
  static_assert(std::is_same<decltype(tjg::to_sgr<char>(style)),
                             std::string>::value,
                "explicit char rendering must return std::string");
  static_assert(std::is_same<decltype(tjg::to_sgr<wchar_t>(style)),
                             std::wstring>::value,
                "wide rendering must return std::wstring");
  const auto original = style;
  EXPECT_EQ(tjg::to_sgr(style), expected);
  EXPECT_EQ(tjg::to_sgr<char>(style), expected);
  EXPECT_EQ(tjg::to_sgr<wchar_t>(style),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original);
}

template<typename Char>
std::basic_string<Char> color_escape(tjg::color_t color, bool background) {
  const auto escape = tjg::detail::ansi_color_escape<Char>{color, background};
  return std::basic_string<Char>{escape.begin(), escape.end()};
}
}  // namespace

namespace {
void check_sgr_transition(const tjg::text_style& target,
                          const tjg::text_style& initial,
                          const std::string& expected) {
  const auto target_copy = target;
  const auto initial_copy = initial;
  EXPECT_EQ(tjg::to_sgr(target, initial), expected);
  EXPECT_EQ(tjg::to_sgr<char>(target, initial), expected);
  EXPECT_EQ(tjg::to_sgr<wchar_t>(target, initial),
            (std::wstring{expected.begin(), expected.end()}));
  // The initial value describes a complete state regardless of its flag.
  auto absolute_initial = initial;
  absolute_initial.absolute();
  EXPECT_EQ(tjg::to_sgr(target, absolute_initial), expected);
  EXPECT_EQ(tjg::to_sgr<wchar_t>(target, absolute_initial),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(target, target_copy);
  EXPECT_EQ(initial, initial_copy);
}
} // namespace

TEST(color_test, sgr_transition) {
  check_sgr_transition({}, {}, "");
  const auto red = tjg::fg(tjg::term_color::red);
  check_sgr_transition(red, red, "");
  check_sgr_transition({}, red, "\x1b[0m");
  check_sgr_transition(tjg::bold(red), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[31m", "\x1b[1m"));
  check_sgr_transition(tjg::fg(tjg::term_color::blue), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[34m", "\x1b[34m"));
  check_sgr_transition(tjg::fg(tjg::rgb{1, 2, 3}), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[38;2;001;002;003m", "\x1b[38;2;1;2;3m"));
  check_sgr_transition(tjg::bg(tjg::cube_color{0}), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[48;5;016m", "\x1b[0;48;5;16m"));
  check_sgr_transition(tjg::fg(tjg::gray_color{0}), {},
      FMT_SGR_EXPECTED("\x1b[38;5;232m", "\x1b[38;5;232m"));
  check_sgr_transition(tjg::bold(), {}, "\x1b[1m");
  check_sgr_transition(red, {}, "\x1b[31m");
  check_sgr_transition(tjg::bold(red), {},
      FMT_SGR_EXPECTED("\x1b[1m\x1b[31m", "\x1b[1;31m"));
  check_sgr_transition(tjg::fg(tjg::rgb{1, 2, 3}), {},
      FMT_SGR_EXPECTED("\x1b[38;2;001;002;003m", "\x1b[38;2;1;2;3m"));
  // Identical RGB appearance does not erase a palette-kind transition.
  check_sgr_transition(tjg::fg(tjg::color::black),
      tjg::fg(tjg::term_color::black),
      FMT_SGR_EXPECTED("\x1b[0m\x1b[38;2;000;000;000m", "\x1b[38;2;0;0;0m"));
  check_sgr_transition(tjg::bold(), tjg::faint(),
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m", "\x1b[0;1m"));
  check_sgr_transition({}, tjg::bold(), "\x1b[0m");

  const auto all = static_cast<tjg::emphasis>(0xff);
  // Missing target colors select terminal defaults after a reset.
  const auto initial = tjg::text_style{all, tjg::term_color::red,
                                      tjg::term_color::blue};
  check_sgr_transition({}, initial, "\x1b[0m");
  // Compact mode selectively clears intensity and reapplies the surviving bit.
  // Noncompact mode resets and restores all requested colors and attributes.
  const auto target = tjg::text_style{static_cast<tjg::emphasis>(0xfe),
                                     initial.fg(), initial.bg()};
  check_sgr_transition(target, initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[2;3;4;5;7;8;9m\x1b[31m\x1b[44m",
                       "\x1b[22;2m"));
  check_sgr_transition(tjg::text_style{static_cast<tjg::emphasis>(0xfd),
                                      initial.fg(), initial.bg()}, initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1;3;4;5;7;8;9m\x1b[31m\x1b[44m",
                       "\x1b[22;1m"));
}

TEST(color_test, sgr_transition_terminal_defaults) {
  const auto fg = tjg::rgb{12, 34, 56};
  const auto bg = tjg::rgb{78, 90, 123};
  const auto initial = tjg::text_style{fg, bg, tjg::emphasis::bold};
  // Compact mode selectively clears a channel. Noncompact mode rebuilds the
  // remaining channel and emphasis after its reset.
  check_sgr_transition(tjg::text_style{tjg::no_color, bg, tjg::emphasis::bold},
      initial, FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[48;2;078;090;123m",
                                "\x1b[39m"));
  check_sgr_transition(tjg::text_style{fg, tjg::no_color, tjg::emphasis::bold},
      initial, FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[38;2;012;034;056m",
                                "\x1b[49m"));
  // Compact-mode ties prefer selective changes.
  const auto palette = tjg::text_style{tjg::term_color::red,
                                      tjg::term_color::blue};
  check_sgr_transition(tjg::fg(tjg::term_color::red), palette,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[31m", "\x1b[49m"));
  check_sgr_transition(tjg::bg(tjg::term_color::blue), palette,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[44m", "\x1b[39m"));
  // When both channels and emphasis are removed, a full reset is shortest.
  check_sgr_transition({}, initial, "\x1b[0m");
  check_sgr_transition(tjg::bold(), initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m", "\x1b[0;1m"));
}

TEST(color_test, sgr_transition_absolute_target) {
  const tjg::text_style states[] = {
      {}, tjg::underline(), tjg::fg(tjg::term_color::red),
      tjg::text_style{tjg::rgb{1, 2, 3}, tjg::gray_color{12},
                      tjg::emphasis::bold}};
  for (auto target : states) {
    target.absolute();
    // Absolute targets always reset, including from defaults, in both modes.
    for (const auto& initial : states)
      check_sgr_transition(target, initial, tjg::to_sgr(target));
    // Equal complete states still require the explicit absolute reset.
    check_sgr_transition(target, target, tjg::to_sgr(target));
  }
}

TEST(color_test, sgr_transition_all_emphasis_pairs) {
  // Interpret the emitted ANSI independently of the transition algorithm.
  const unsigned on[] = {1, 2, 3, 4, 5, 7, 8, 9};
  const unsigned off[] = {22, 22, 23, 24, 25, 27, 28, 29};
  for (unsigned before = 0; before < 256; ++before) {
    for (unsigned after = 0; after < 256; ++after) {
      const auto initial = tjg::text_style{static_cast<tjg::emphasis>(before)};
      const auto target = tjg::text_style{static_cast<tjg::emphasis>(after)};
      const auto sgr = tjg::to_sgr(target, initial);
      if (before == after) {
        EXPECT_TRUE(sgr.empty());
        continue;
      }
      ASSERT_GE(sgr.size(), size_t{4});
      unsigned state = before;
      size_t i = 0;
      // Interpret every complete sequence, carrying terminal state across them.
      // Compact output uses one sequence; noncompact resets a non-default
      // initial state before applying the target attributes.
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
#if TJG_COLOR_COMPACT_SGR
      // Compact output stays combined and no longer than reset-and-rebuild.
      EXPECT_EQ(sgr.find('\x1b', 1), std::string::npos);
      size_t absolute_size = 4; // ESC [ 0 m
      for (unsigned bit = 0; bit < 8; ++bit)
        if (after & (1u << bit)) absolute_size += 2;
      EXPECT_LE(sgr.size(), absolute_size);
#else
      // Reset only when the initial state is not already terminal defaults.
      if (before != 0)
        EXPECT_EQ(sgr.substr(0, 4), "\x1b[0m");
      else
        EXPECT_NE(sgr.substr(0, 4), "\x1b[0m");
#endif
    }
  }
}

TEST(color_test, palette_ansi_escape) {
  const struct {
    tjg::color_t color;
    const char* foreground;
    const char* background;
  } samples[] = {
      {tjg::no_color, "", ""},
      {tjg::rgb{1, 2, 3},
       FMT_SGR_EXPECTED("38;2;001;002;003;", "38;2;1;2;3;"),
       FMT_SGR_EXPECTED("48;2;001;002;003;", "48;2;1;2;3;")},
      {tjg::term_color::red, "31;", "41;"},
      {tjg::term_color::bright_white, "97;", "107;"},
      {tjg::cube_color{0},
       FMT_SGR_EXPECTED("38;5;016;", "38;5;16;"),
       FMT_SGR_EXPECTED("48;5;016;", "48;5;16;")},
      {tjg::cube_color{215}, "38;5;231;", "48;5;231;"},
      {tjg::gray_color{0}, "38;5;232;", "48;5;232;"},
      {tjg::gray_color{23}, "38;5;255;", "48;5;255;"}};
  for (const auto& sample : samples) {
    for (bool background : {false, true}) {
      const auto expected = std::string{background ? sample.background
                                                  : sample.foreground};
      EXPECT_EQ(color_escape<char>(sample.color, background), expected);
      EXPECT_EQ(color_escape<wchar_t>(sample.color, background),
                (std::wstring{expected.begin(), expected.end()}));
      const auto style =
          background ? tjg::bg(sample.color) : tjg::fg(sample.color);
      const auto sgr = expected.empty()
                           ? std::string{}
                           : "\x1b[" + expected.substr(0, expected.size() - 1) + "m";
      check_to_sgr(style, sgr);
      EXPECT_EQ(tjg::format(style, "value"),
                sgr + "value" + (sgr.empty() ? "" : "\x1b[0m"));
    }
  }
}

namespace {
template<typename T, size_t Index>
void check_color_access(T value) {
  using tjg::color_t;
  static_assert(std::is_convertible<T, color_t>::value,
                "color construction must be implicit");
  static_assert(noexcept(color_t{value}) &&
                    noexcept(std::declval<color_t&>() = value),
                "construction and alternative assignment must be noexcept");
#define CHECK_COLOR_GET(Color, Result)                                      \
  static_assert(std::is_same<decltype(tjg::get<T>(std::declval<Color>())),    \
                             Result>::value &&                             \
                    std::is_same<decltype(tjg::get<Index>(                  \
                                     std::declval<Color>())),              \
                                 Result>::value,                           \
                "type and index access must preserve cv/ref qualifiers")
  CHECK_COLOR_GET(color_t&, T&);
  CHECK_COLOR_GET(const color_t&, const T&);
  CHECK_COLOR_GET(color_t&&, T&&);
  CHECK_COLOR_GET(const color_t&&, const T&&);
#undef CHECK_COLOR_GET
  static_assert(std::is_same<decltype(tjg::get_if<T>(
                                 std::declval<color_t*>())), T*>::value &&
                    std::is_same<decltype(tjg::get_if<Index>(
                                     std::declval<const color_t*>())),
                                 const T*>::value,
                "pointer access must preserve constness");

  color_t color = value;
  const color_t& constant = color;
  EXPECT_EQ(color.index(), Index);
  EXPECT_TRUE(tjg::holds_alternative<T>(constant));
  EXPECT_EQ(&tjg::get<T>(color), &tjg::get<Index>(color));
  EXPECT_EQ(&tjg::get<T>(constant), &tjg::get<Index>(constant));
  EXPECT_EQ(tjg::get_if<T>(&color), &tjg::get<T>(color));
  EXPECT_EQ(tjg::get_if<Index>(&color), &tjg::get<T>(color));
  EXPECT_EQ(tjg::get_if<T>(&constant), &tjg::get<T>(constant));
  EXPECT_EQ(tjg::get_if<Index>(&constant), &tjg::get<T>(constant));
  EXPECT_EQ(tjg::get_if<T>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(tjg::get_if<T>(static_cast<const color_t*>(nullptr)), nullptr);
  EXPECT_EQ(tjg::get_if<Index>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(tjg::get_if<Index>(static_cast<const color_t*>(nullptr)), nullptr);
  static_assert(noexcept(tjg::holds_alternative<T>(constant)) &&
                    noexcept(tjg::get_if<T>(&color)) &&
                    noexcept(tjg::get_if<T>(&constant)) &&
                    noexcept(tjg::get_if<Index>(&color)) &&
                    noexcept(tjg::get_if<Index>(&constant)),
                "classification and pointer access must be noexcept");
  EXPECT_EQ(tjg::to_underlying(tjg::get<T>(std::move(color))),
            tjg::to_underlying(value));
  EXPECT_EQ(tjg::to_underlying(tjg::get<Index>(std::move(color))),
            tjg::to_underlying(value));
  EXPECT_EQ(tjg::to_underlying(tjg::get<T>(std::move(constant))),
            tjg::to_underlying(value));
  EXPECT_EQ(tjg::to_underlying(tjg::get<Index>(std::move(constant))),
            tjg::to_underlying(value));

  const color_t alternatives[] = {tjg::no_color, tjg::rgb{},
                                 tjg::term_color::black, tjg::cube_color{0},
                                 tjg::gray_color{0}};
  for (auto other : alternatives) {
    if (other.index() == Index) continue;
    const auto& const_other = other;
    EXPECT_FALSE(tjg::holds_alternative<T>(other));
    EXPECT_EQ(tjg::get_if<T>(&other), nullptr);
    EXPECT_EQ(tjg::get_if<T>(&const_other), nullptr);
    EXPECT_EQ(tjg::get_if<Index>(&other), nullptr);
    EXPECT_EQ(tjg::get_if<Index>(&const_other), nullptr);
    EXPECT_THROW_MSG((void)tjg::get<T>(other), std::format_error,
                     "wrong color_t alternative");
    EXPECT_THROW((void)tjg::get<T>(const_other), std::format_error);
    EXPECT_THROW((void)tjg::get<T>(std::move(other)), std::format_error);
    EXPECT_THROW((void)tjg::get<T>(std::move(const_other)), std::format_error);
    EXPECT_THROW((void)tjg::get<Index>(other), std::format_error);
    EXPECT_THROW((void)tjg::get<Index>(const_other), std::format_error);
    EXPECT_THROW((void)tjg::get<Index>(std::move(other)), std::format_error);
    EXPECT_THROW((void)tjg::get<Index>(std::move(const_other)), std::format_error);
  }
}

template<typename T>
void check_color_assignment(T replacement) {
  const tjg::color_t original[] = {tjg::no_color, tjg::rgb{1, 2, 3},
                                  tjg::term_color::bright_white,
                                  tjg::cube_color{215}, tjg::gray_color{23}};
  for (auto color : original) {
    const tjg::color_t expected = replacement;
    EXPECT_EQ(&(color = replacement), &color);
    EXPECT_EQ(color, expected);
    // Assignment from the active member must read it before replacing storage.
    EXPECT_EQ(&(color = tjg::get<T>(color)), &color);
    EXPECT_EQ(color, expected);
  }
}

struct color_kind_visitor {
#if FMT_MSC_VERSION >= 1929 && FMT_MSC_VERSION <= 1944 && !FMT_CLANG_VERSION
  // MSVC 19.29 and 19.44 reject copying the empty union member in constexpr.
  constexpr size_t operator()(const tjg::no_color_t&) const { return 0; }
#else
  constexpr size_t operator()(tjg::no_color_t) const { return 0; }
#endif
  constexpr size_t operator()(tjg::rgb) const { return 1; }
  constexpr size_t operator()(tjg::term_color) const { return 2; }
  constexpr size_t operator()(tjg::cube_color) const { return 3; }
  constexpr size_t operator()(tjg::gray_color) const { return 4; }
};

template<typename T, size_t Index>
constexpr bool check_color_constexpr_access(tjg::color_t&& color) {
  return
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
         // GCC 4.9 rejects constexpr pointer comparisons involving temporaries.
         tjg::get_if<T>(&color) == &tjg::get<Index>(color) &&
         tjg::get_if<Index>(&color) == &tjg::get<T>(color) &&
#endif
         color_kind_visitor{}(tjg::get<Index>(std::move(color))) == Index &&
         tjg::visit(color_kind_visitor{}, color) == Index &&
         tjg::visit(color_kind_visitor{}, std::move(color)) == Index &&
         tjg::fg(color).fg() == color && tjg::bg(color).bg() == color;
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
  void operator()(tjg::no_color_t&) const { ++calls; }
  void operator()(tjg::rgb& value) const {
    ++calls;
    value.r = 42;
  }
  void operator()(tjg::cube_color& value) const {
    ++calls;
    value.r(5);
  }
  void operator()(tjg::gray_color& value) const {
    ++calls;
    value = tjg::gray_color{23};
  }
  void operator()(tjg::term_color& value) const {
    ++calls;
    value = tjg::term_color::bright_white;
  }
};

struct color_reference_visitor {
  int& value;
  template<typename T> int& operator()(T&&) const { return value; }
};

struct color_throwing_visitor {
  template<typename T> void operator()(T&&) const { throw 42; }
};

#if 1
constexpr bool check_color_constexpr_assignment() {
  tjg::color_t color;
  color = tjg::rgb{1, 2, 3};
  tjg::get<tjg::rgb>(color).r = 4;
  tjg::get_if<1>(&color)->b = 5;
  bool rgb_ok = tjg::get<1>(color) == tjg::rgb{4, 2, 5};
  color = tjg::term_color::black;
  bool terminal_ok = tjg::holds_alternative<tjg::term_color>(color) &&
                     tjg::get<2>(color) == tjg::term_color::black;
  color = tjg::cube_color{215};
  bool cube_ok = tjg::get<3>(color) == tjg::cube_color{215};
  color = tjg::gray_color{23};
  bool gray_ok = tjg::get<4>(color) == tjg::gray_color{23};
  color = tjg::no_color;
  return rgb_ok && terminal_ok && cube_ok && gray_ok &&
         tjg::holds_alternative<tjg::no_color_t>(color);
}
#endif
}  // namespace

TEST(color_test, color_variant_access) {
  check_color_access<tjg::no_color_t, 0>(tjg::no_color);
  check_color_access<tjg::rgb, 1>(tjg::rgb{});
  check_color_access<tjg::rgb, 1>(tjg::color::coral);
  check_color_access<tjg::term_color, 2>(tjg::term_color::black);
  check_color_access<tjg::term_color, 2>(tjg::term_color::bright_white);
  check_color_access<tjg::cube_color, 3>(tjg::cube_color{0});
  check_color_access<tjg::cube_color, 3>(tjg::cube_color{215});
  check_color_access<tjg::gray_color, 4>(tjg::gray_color{0});
  check_color_access<tjg::gray_color, 4>(tjg::gray_color{23});
  auto color = tjg::color_t{tjg::rgb{1, 2, 3}};
  tjg::get<tjg::rgb>(color).r = 4;
  tjg::get<1>(color).g = 5;
  tjg::get_if<tjg::rgb>(&color)->b = 6;
  tjg::get_if<1>(&color)->r = 7;
  EXPECT_EQ(tjg::get<tjg::rgb>(color), (tjg::rgb{7, 5, 6}));
  EXPECT_EQ(tjg::get_if<2>(&color), nullptr);
  EXPECT_EQ(tjg::get_if<tjg::gray_color>(&color), nullptr);
  EXPECT_THROW((void)tjg::get<2>(color), std::format_error);
}

TEST(color_test, color_variant_assignment) {
  check_color_assignment(tjg::no_color);
  check_color_assignment(tjg::rgb{});
  check_color_assignment(tjg::color::coral);
  check_color_assignment(tjg::term_color::black);
  check_color_assignment(tjg::term_color::bright_white);
  check_color_assignment(tjg::cube_color{215});
  check_color_assignment(tjg::gray_color{23});
  const tjg::color_t colors[] = {tjg::no_color, tjg::rgb{1, 2, 3},
                                tjg::term_color::black, tjg::cube_color{1, 2, 3},
                                tjg::gray_color{12}};
  for (const auto& source : colors) {
    tjg::color_t copy(source);
    tjg::color_t moved(std::move(copy));
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
  const tjg::color_t colors[] = {tjg::no_color, tjg::rgb{1, 2, 3},
                                tjg::term_color::black, tjg::cube_color{1, 2, 3},
                                tjg::gray_color{12}};
  const tjg::color_t changed[] = {tjg::no_color, tjg::rgb{42, 2, 3},
                                 tjg::term_color::bright_white,
                                 tjg::cube_color{5, 2, 3}, tjg::gray_color{23}};
  for (size_t i = 0; i < 5; ++i) {
    auto color = colors[i];
    const auto& constant = color;
    EXPECT_EQ(tjg::visit(color_kind_visitor{}, constant), color.index());
    color_category_visitor visitor;
    EXPECT_EQ(tjg::visit(visitor, color), 0u);
    EXPECT_EQ(tjg::visit(visitor, constant), 1u);
    EXPECT_EQ(tjg::visit(visitor, std::move(color)), 2u);
    EXPECT_EQ(tjg::visit(visitor, std::move(constant)), 3u);
    EXPECT_EQ(tjg::visit(std::move(visitor), color), 4u);
    EXPECT_EQ(tjg::visit(std::move(visitor), constant), 5u);
    EXPECT_EQ(tjg::visit(std::move(visitor), std::move(color)), 6u);
    EXPECT_EQ(tjg::visit(std::move(visitor), std::move(constant)), 7u);
    int calls = 0;
    tjg::visit(color_mutating_visitor{calls}, color);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(color, changed[i]);
    int result = 0;
    EXPECT_EQ(&tjg::visit(color_reference_visitor{result}, color), &result);
    tjg::visit(color_reference_visitor{result}, color) = 9;
    EXPECT_EQ(result, 9);
    EXPECT_THROW(tjg::visit(color_throwing_visitor{}, color), int);
    EXPECT_EQ(color, changed[i]);
  }
}

TEST(color_test, color_variant_constexpr) {
  static_assert(
      check_color_constexpr_access<tjg::no_color_t, 0>(tjg::no_color) &&
          check_color_constexpr_access<tjg::rgb, 1>(tjg::rgb{1, 2, 3}) &&
          check_color_constexpr_access<tjg::term_color, 2>(tjg::term_color::red) &&
          check_color_constexpr_access<tjg::cube_color, 3>(tjg::cube_color{215}) &&
          check_color_constexpr_access<tjg::gray_color, 4>(tjg::gray_color{23}),
      "mutable access, visitation, and style factories must be constexpr in C++11");
#if FMT_GCC_VERSION && FMT_GCC_VERSION < 500
  // GCC 4.9 requires static storage for the constexpr pointer checks below.
  static constexpr tjg::color_t empty{};
  static constexpr tjg::color_t rgb = tjg::rgb{1, 2, 3};
  static constexpr tjg::color_t terminal = tjg::term_color::black;
#else
  constexpr tjg::color_t empty;
  constexpr tjg::color_t rgb = tjg::rgb{1, 2, 3};
  constexpr tjg::color_t terminal = tjg::term_color::black;
#endif
  constexpr tjg::color_t cube = tjg::cube_color{215};
  constexpr tjg::color_t gray = tjg::gray_color{23};
  static_assert(tjg::get<3>(cube) == tjg::cube_color{215} &&
                    tjg::get<4>(gray) == tjg::gray_color{23},
                "palette access must be constexpr in C++11");
  static_assert(tjg::detail::unpack_color(tjg::to_underlying(cube)) == cube &&
                    tjg::detail::unpack_color(tjg::to_underlying(gray)) == gray,
                "palette packing must round-trip in constant evaluation");
  constexpr auto palette_style = tjg::text_style{cube, gray, tjg::emphasis::bold};
  static_assert(palette_style.fg() == cube && palette_style.bg() == gray &&
                    palette_style.emph() == tjg::emphasis::bold,
                "palette style getters must be constexpr in C++11");
  static_assert(empty.index() == 0 && tjg::holds_alternative<tjg::no_color_t>(empty),
                "default construction must hold the unspecified alternative");
  static_assert(rgb.index() == 1 && tjg::get<tjg::rgb>(rgb).g == 2 &&
                    tjg::get<1>(rgb).b == 3,
                "RGB type and index access must be constexpr in C++11");
  static_assert(terminal.index() == 2 &&
                    tjg::get<tjg::term_color>(terminal) == tjg::term_color::black,
                "terminal black must remain specified in constant evaluation");
  static_assert(tjg::get_if<1>(&rgb) == &tjg::get<tjg::rgb>(rgb) &&
                    tjg::get_if<tjg::term_color>(&rgb) == nullptr &&
                    tjg::get_if<0>(static_cast<const tjg::color_t*>(nullptr)) ==
                        nullptr,
                "pointer access must be constexpr in C++11");
  static_assert(tjg::visit(color_kind_visitor{}, empty) == 0 &&
                    tjg::visit(color_kind_visitor{}, rgb) == 1 &&
                    tjg::visit(color_kind_visitor{}, terminal) == 2 &&
                    tjg::visit(color_kind_visitor{}, cube) == 3 &&
                    tjg::visit(color_kind_visitor{}, gray) == 4,
                "visitation must be constexpr in C++11");
#if 1
  static_assert(check_color_constexpr_assignment(),
                "replacement and mutable access must support constant evaluation");
#endif
}

TEST(color_test, color_variant_style_values) {
  static_assert(std::is_same<decltype(tjg::text_style{}.fg()),
                             tjg::color_t>::value &&
                    std::is_same<decltype(tjg::text_style{}.bg()),
                                 tjg::color_t>::value &&
                    std::is_same<decltype(tjg::text_style{}.get_foreground()),
                                 tjg::color_t>::value &&
                    std::is_same<decltype(tjg::text_style{}.get_background()),
                                 tjg::color_t>::value,
                "short and compatibility getters must return color values");
  auto foreground = tjg::color_t{tjg::rgb{1, 2, 3}};
  auto background = tjg::color_t{tjg::term_color::black};
  const auto style =
      tjg::text_style{foreground, background, tjg::emphasis::strike};
  tjg::get<1>(foreground).r = 255;
  tjg::get<2>(background) = tjg::term_color::white;
  EXPECT_EQ(tjg::get<tjg::rgb>(style.fg()), (tjg::rgb{1, 2, 3}));
  EXPECT_EQ(tjg::get<tjg::term_color>(style.bg()), tjg::term_color::black);
  auto extracted = style.fg();
  tjg::get<1>(extracted).b = 255;
  EXPECT_EQ(tjg::get<1>(style.fg()).b, 3);
  // Copying an alternative from a temporary color is safe.
  auto copy = tjg::get<tjg::rgb>(style.fg());
  copy.g = 255;
  EXPECT_EQ(copy, (tjg::rgb{1, 255, 3}));
  EXPECT_EQ(tjg::get<1>(style.fg()).g, 2);
  EXPECT_EQ(tjg::format(style, "value"),
            FMT_SGR_EXPECTED(
                "\x1b[9m\x1b[38;2;001;002;003m\x1b[40mvalue\x1b[0m",
                "\x1b[9;38;2;1;2;3;40mvalue\x1b[0m"));
}

TEST(color_test, rgb_checked_channels) {
  constexpr uint8_t byte_channel = 128;
  static_assert(tjg::rgb{byte_channel, 0, 255} == tjg::rgb{128, 0, 255} &&
                    tjg::rgb{0, byte_channel, 255} == tjg::rgb{0, 128, 255} &&
                    tjg::rgb{0, 255, byte_channel} == tjg::rgb{0, 255, 128},
                "mixed byte and int inputs must resolve to the checked constructor");
  static_assert(tjg::rgb{0, 255, 128} == tjg::rgb{0x00FF80},
                "signed channel inputs must support C++11 constant evaluation");
  static_assert(noexcept(tjg::rgb{0, 0, 0}),
                "RGB channel construction must be noexcept");
  for (int channel = 0; channel <= 255; ++channel) {
    auto color = tjg::rgb{channel, channel, channel};
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
    EXPECT_EQ((tjg::rgb{channel, 128, 64}), (tjg::rgb{clamped, 128, 64}));
    EXPECT_EQ((tjg::rgb{128, channel, 64}), (tjg::rgb{128, clamped, 64}));
    EXPECT_EQ((tjg::rgb{128, 64, channel}), (tjg::rgb{128, 64, clamped}));
  }
#endif
}

TEST(color_test, rgb) {
  static_assert(!std::is_convertible<uint32_t, tjg::rgb>::value,
                "integer to rgb conversion must be explicit");
  static_assert(!std::is_convertible<tjg::rgb, uint32_t>::value,
                "rgb to integer conversion must be explicit");
  static_assert(std::is_same<decltype(tjg::color::red), const tjg::rgb>::value,
                "named colors must be rgb constants");
  constexpr auto c = tjg::rgb{0xAB123456};
  static_assert(c.r == 0x12 && c.g == 0x34 && c.b == 0x56,
                "the high byte must be ignored");
  static_assert(static_cast<uint32_t>(c) == 0x123456,
                "packed rgb must have a zero high byte");
  static_assert(static_cast<uint32_t>(tjg::color::alice_blue) == 0xF0F8FF,
                "named colors must preserve their values");
  auto components = tjg::rgb{0x12, 0x34, 0x56};
  EXPECT_EQ(static_cast<uint32_t>(components), 0x123456u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::rgb{}), 0u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::color::black), 0u);
  EXPECT_EQ(static_cast<uint32_t>(tjg::color::white), 0xFFFFFFu);
}

TEST(color_test, rgb_or) {
  constexpr auto lhs = tjg::rgb{0xC0F000};
  constexpr auto rhs = tjg::rgb{0x000FEE};
  static_assert((lhs | rhs) == tjg::rgb{0xC0FFEE},
                "RGB OR supports C++11 constant evaluation");
  static_assert(noexcept(lhs | rhs), "RGB OR must be noexcept");
  EXPECT_EQ(lhs | rhs, tjg::rgb{0xC0FFEE});
  EXPECT_EQ(lhs, tjg::rgb{0xC0F000});
  EXPECT_EQ(rhs, tjg::rgb{0x000FEE});
  auto assigned = lhs;
  EXPECT_EQ(&(assigned |= rhs), &assigned);
  EXPECT_EQ(assigned, lhs | rhs);
}

TEST(color_test, rgb_blend) {
  static_assert(std::is_same<decltype(tjg::blend(tjg::rgb{}, tjg::rgb{})),
                             tjg::rgb>::value,
                "blend must return an RGB value");
  static_assert(noexcept(tjg::blend(tjg::rgb{}, tjg::rgb{})),
                "blend must be noexcept");
  auto black = tjg::rgb{};
  auto white = tjg::rgb{255, 255, 255};
  EXPECT_EQ(tjg::blend(black, black), black);
  EXPECT_EQ(tjg::blend(white, white), white);
  EXPECT_EQ(tjg::blend(black, black, black), black);
  EXPECT_EQ(tjg::blend(white, white, white, white), white);
  EXPECT_EQ(tjg::blend(black, white), (tjg::rgb{127, 127, 127}));
  EXPECT_EQ(tjg::blend(black, white, white), (tjg::rgb{170, 170, 170}));
  EXPECT_EQ(tjg::blend(white, black, white), (tjg::rgb{170, 170, 170}));
  EXPECT_EQ(tjg::blend(white, white, black), (tjg::rgb{170, 170, 170}));
  EXPECT_EQ(black, tjg::rgb{});
  EXPECT_EQ(white, (tjg::rgb{255, 255, 255}));

  auto updated = black;
  updated = tjg::blend(updated, white);
  EXPECT_EQ(updated, (tjg::rgb{127, 127, 127}));
  EXPECT_EQ(black, tjg::rgb{});
  EXPECT_EQ(white, (tjg::rgb{255, 255, 255}));

  auto first  = tjg::rgb{0, 1, 254};
  auto second = tjg::rgb{0, 2, 255};
  auto third  = tjg::rgb{2, 4, 255};
  EXPECT_EQ(tjg::blend(first, second, third), (tjg::rgb{0, 2, 254}));
  EXPECT_EQ(tjg::blend(third, second, first), (tjg::rgb{0, 2, 254}));
  EXPECT_EQ(tjg::blend(tjg::color::red, tjg::color::green, tjg::color::blue),
            (tjg::rgb{85, 42, 85}));
  constexpr auto same = tjg::blend(tjg::rgb{1, 2, 3}, tjg::rgb{1, 2, 3});
  constexpr auto average = tjg::blend(tjg::rgb{255, 0, 0}, tjg::rgb{0, 255, 0},
                                     tjg::rgb{0, 0, 255});
  static_assert(same.r == 1 && same.g == 2 && same.b == 3,
                "equal RGB colors preserve all channels");
  static_assert(average.r == 85 && average.g == 85 && average.b == 85,
                "blend must support constant evaluation");
}

namespace {
template<typename... Colors>
auto can_blend(int) -> decltype(tjg::blend(std::declval<Colors>()...),
                                std::true_type{});
template<typename... Colors> auto can_blend(...) -> std::false_type;
template<typename T>
auto can_to_rgb(int) -> decltype(tjg::to_rgb(std::declval<T>()), std::true_type{});
template<typename T> auto can_to_rgb(...) -> std::false_type;
} // namespace

TEST(color_test, to_rgb) {
  static_assert(std::is_same<decltype(tjg::to_rgb(tjg::rgb{})), tjg::rgb>::value &&
                    std::is_same<decltype(tjg::to_rgb(tjg::term_color::red)),
                                 tjg::rgb>::value &&
                    std::is_same<decltype(tjg::to_rgb(std::declval<tjg::gray_color>())),
                                 tjg::rgb>::value &&
                    std::is_same<decltype(tjg::to_rgb(std::declval<tjg::cube_color>())),
                                 tjg::rgb>::value,
                "plain colors convert to RGB values");
  static_assert(std::is_same<decltype(tjg::to_rgb(tjg::no_color)),
                             tjg::no_color_t>::value &&
                    std::is_same<decltype(tjg::to_rgb(tjg::color_t{})),
                                 tjg::color_t>::value,
                "unspecified and variant conversions preserve their result types");
  static_assert(!decltype(can_to_rgb<int>(0))::value &&
                    !decltype(can_to_rgb<const char*>(0))::value &&
                    !decltype(can_to_rgb<tjg::emphasis>(0))::value &&
                    !decltype(can_to_rgb<tjg::text_style>(0))::value,
                "to_rgb rejects non-color arguments");
  static_assert(noexcept(tjg::to_rgb(tjg::rgb{})) &&
                    noexcept(tjg::to_rgb(tjg::term_color::red)) &&
                    noexcept(tjg::to_rgb(std::declval<tjg::gray_color>())) &&
                    noexcept(tjg::to_rgb(std::declval<tjg::cube_color>())) &&
                    noexcept(tjg::to_rgb(tjg::no_color)) &&
                    noexcept(tjg::to_rgb(tjg::color_t{})),
                "all to_rgb overloads must be noexcept");
  static_assert(tjg::to_rgb(tjg::rgb{0x123456}) == tjg::rgb{0x123456} &&
                    tjg::to_rgb(tjg::term_color::red) == tjg::rgb{205, 0, 0} &&
                    tjg::to_rgb(tjg::term_color::bright_blue) == tjg::rgb{92, 92, 255} &&
                    tjg::to_rgb(tjg::gray_color{0}) == tjg::rgb{8, 8, 8} &&
                    tjg::to_rgb(tjg::gray_color{23}) == tjg::rgb{238, 238, 238} &&
                    tjg::to_rgb(tjg::cube_color{0}) == tjg::rgb{} &&
                    tjg::to_rgb(tjg::cube_color{215}) == tjg::rgb{255, 255, 255},
                "plain conversion supports C++11 constant evaluation");
  constexpr auto unspecified = tjg::to_rgb(tjg::no_color);
  static_assert(tjg::color_t{unspecified} == tjg::color_t{},
                "the unspecified sentinel remains unspecified");
  static_assert(tjg::to_rgb(tjg::color_t{}) == tjg::color_t{} &&
                    tjg::to_rgb(tjg::color_t{tjg::rgb{1, 2, 3}}) ==
                        tjg::color_t{tjg::rgb{1, 2, 3}} &&
                    tjg::to_rgb(tjg::color_t{tjg::term_color::black}) ==
                        tjg::color_t{tjg::rgb{}} &&
                    tjg::to_rgb(tjg::color_t{tjg::gray_color{12}}) ==
                        tjg::color_t{tjg::rgb{128, 128, 128}} &&
                    tjg::to_rgb(tjg::color_t{tjg::cube_color{1, 2, 3}}) ==
                        tjg::color_t{tjg::rgb{95, 135, 175}},
                "all variant alternatives support C++11 constant conversion");
  for (const auto& sample : composition_samples) {
    const auto source = sample.color;
    const auto converted = tjg::to_rgb(source);
    EXPECT_EQ(converted, source.index() == 0 ? tjg::color_t{}
                                             : tjg::color_t{sample.promoted});
    EXPECT_EQ(source, sample.color);
    EXPECT_EQ(tjg::to_rgb(converted), converted);
  }
  EXPECT_EQ(tjg::to_rgb(tjg::gray_color{12}), (tjg::rgb{128, 128, 128}));
  EXPECT_EQ(tjg::to_rgb(tjg::cube_color{1, 2, 3}), (tjg::rgb{95, 135, 175}));
}

TEST(color_test, blend_overloads) {
  static_assert(std::is_same<decltype(tjg::blend(tjg::term_color::red,
                                                tjg::term_color::red)),
                             tjg::rgb>::value &&
                    std::is_same<decltype(tjg::blend(tjg::cube_color{0},
                                                     tjg::gray_color{0})),
                                 tjg::rgb>::value,
                "plain palette arguments return RGB");
  static_assert(std::is_same<decltype(tjg::blend(tjg::rgb{}, tjg::color_t{})),
                             tjg::color_t>::value &&
                    std::is_same<decltype(tjg::blend(tjg::color_t{}, tjg::rgb{})),
                                 tjg::color_t>::value &&
                    std::is_same<decltype(tjg::blend(tjg::rgb{}, tjg::no_color)),
                                 tjg::color_t>::value,
                "a variant or unspecified argument selects variant blending");
  static_assert(!decltype(can_blend<>(0))::value &&
                    !decltype(can_blend<int>(0))::value &&
                    !decltype(can_blend<tjg::rgb, int>(0))::value &&
                    !decltype(can_blend<int, tjg::rgb>(0))::value &&
                    !decltype(can_blend<tjg::color_t, const char*>(0))::value,
                "blending requires valid color arguments");
  static_assert(noexcept(tjg::blend(tjg::term_color::red,
                                    std::declval<tjg::gray_color>())) &&
                    noexcept(tjg::blend(tjg::color_t{}, tjg::rgb{})),
                "both blend overloads must be noexcept");
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
  static_assert(tjg::Color<const tjg::rgb&> && tjg::Color<tjg::term_color> &&
                    tjg::Color<tjg::cube_color> && tjg::Color<tjg::gray_color> &&
                    !tjg::Color<tjg::color_t> && !tjg::Color<tjg::no_color_t> &&
                    !tjg::Color<int>,
                "Color identifies only plain color types");
#endif
  constexpr auto mixed = tjg::blend(tjg::term_color::red,
                                    tjg::cube_color{1, 2, 3},
                                    tjg::gray_color{12});
  static_assert(mixed == tjg::rgb{142, 87, 101},
                "plain palette promotion and averaging must be constexpr");
  constexpr auto terminal = tjg::blend(tjg::term_color::red,
                                       tjg::term_color::red);
  static_assert(terminal == tjg::rgb{205, 0, 0},
                "equal plain terminal colors return their RGB equivalent");
}

TEST(color_test, blend_minimum_arity) {
  static_assert(!decltype(can_blend<>(0))::value &&
                    !decltype(can_blend<tjg::rgb>(0))::value &&
                    !decltype(can_blend<tjg::term_color>(0))::value &&
                    !decltype(can_blend<tjg::cube_color>(0))::value &&
                    !decltype(can_blend<tjg::gray_color>(0))::value &&
                    !decltype(can_blend<tjg::color_t>(0))::value &&
                    !decltype(can_blend<tjg::no_color_t>(0))::value &&
                    !decltype(can_blend<tjg::text_style>(0))::value,
                "public blend requires at least two arguments");
  static_assert(decltype(can_blend<tjg::rgb, tjg::rgb>(0))::value &&
                    decltype(can_blend<tjg::rgb, tjg::color_t>(0))::value &&
                    decltype(can_blend<tjg::color_t, tjg::rgb>(0))::value &&
                    decltype(can_blend<tjg::no_color_t,
                                       tjg::no_color_t>(0))::value &&
                    decltype(can_blend<tjg::text_style,
                                       tjg::text_style>(0))::value,
                "two-argument color and style blends remain available");
}

TEST(color_test, color_variant_blend) {
  constexpr auto empty = tjg::blend(tjg::no_color, tjg::color_t{}, tjg::no_color);
  static_assert(empty == tjg::color_t{}, "all unspecified colors remain empty");
  constexpr auto single = tjg::blend(tjg::no_color, tjg::gray_color{12},
                                     tjg::color_t{});
  static_assert(single == tjg::color_t{tjg::gray_color{12}},
                "one specified color retains its palette kind");
  constexpr auto same = tjg::blend(tjg::color_t{tjg::gray_color{12}},
                                   tjg::gray_color{12});
  static_assert(same == tjg::color_t{tjg::rgb{128, 128, 128}},
                "two specified palette colors produce RGB");
  constexpr auto average = tjg::blend(
      tjg::rgb{0, 1, 254}, tjg::no_color, tjg::rgb{0, 2, 255},
      tjg::color_t{tjg::rgb{2, 4, 255}}, tjg::no_color);
  static_assert(average == tjg::color_t{tjg::rgb{0, 2, 254}},
                "ignore unspecified colors and divide once after summation");
  constexpr auto palette = tjg::blend(tjg::cube_color{1, 2, 3},
                                      tjg::color_t{tjg::gray_color{12}});
  static_assert(palette == tjg::color_t{tjg::rgb{111, 131, 151}},
                "a trailing variant selects RGB promotion of plain arguments");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("blend kinds {} and {}", i, j));
      const auto lhs = composition_samples[i].color;
      const auto rhs = composition_samples[j].color;
      const auto a = composition_samples[i].promoted;
      const auto b = composition_samples[j].promoted;
      const auto expected = i == 0 ? rhs : j == 0 ? lhs : tjg::color_t{tjg::rgb{
          static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
          static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
          static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      EXPECT_EQ(tjg::blend(lhs, rhs), expected);
      EXPECT_EQ(tjg::blend(tjg::no_color, lhs, tjg::no_color, rhs,
                           tjg::no_color), expected);
      EXPECT_EQ(tjg::blend(lhs, tjg::no_color), lhs);
      EXPECT_EQ(lhs, composition_samples[i].color);
      EXPECT_EQ(rhs, composition_samples[j].color);
    }
  }
}

TEST(color_test, text_style_blend) {
  static_assert(std::is_same<decltype(tjg::blend(tjg::text_style{},
                                                tjg::text_style{})),
                             tjg::text_style>::value &&
                    decltype(can_blend<const tjg::text_style&,
                                       tjg::text_style>(0))::value,
                "style blending accepts style values and returns a style");
  static_assert(!decltype(can_blend<tjg::text_style, tjg::rgb>(0))::value &&
                    !decltype(can_blend<tjg::rgb, tjg::text_style>(0))::value &&
                    !decltype(can_blend<tjg::text_style, int>(0))::value &&
                    !decltype(can_blend<tjg::emphasis>(0))::value,
                "style blending requires style arguments without color overlap");
  static_assert(noexcept(tjg::blend(tjg::text_style{}, tjg::text_style{})),
                "style blending must be noexcept");
  constexpr auto empty = tjg::blend(tjg::text_style{}, tjg::text_style{});
  static_assert(empty == tjg::text_style{}, "empty styles blend to an empty style");
  constexpr auto single = tjg::text_style{tjg::cube_color{1, 2, 3},
                                         tjg::gray_color{12},
                                         tjg::emphasis::bold};
  static_assert(tjg::blend(tjg::text_style{}, single) == single &&
                    tjg::blend(tjg::text_style{}, single, tjg::text_style{}) == single,
                "one specified color per channel retains its kind");
  constexpr auto first = tjg::text_style{tjg::term_color::red, tjg::no_color,
                                        tjg::emphasis::bold};
  constexpr auto second = tjg::text_style{tjg::gray_color{12},
                                         tjg::cube_color{1, 2, 3},
                                         tjg::emphasis::underline};
  constexpr auto third = tjg::text_style{tjg::rgb{}, tjg::no_color,
                                        tjg::emphasis::strike};
  constexpr auto averaged = tjg::blend(first, second, third);
  static_assert(averaged.fg() == tjg::color_t{tjg::rgb{111, 42, 42}} &&
                    averaged.bg() == tjg::color_t{tjg::cube_color{1, 2, 3}} &&
                    averaged.emph() == (tjg::emphasis::bold |
                                        tjg::emphasis::underline |
                                        tjg::emphasis::strike),
                "channels use independent counts and emphasis bits are ORed");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(std::format("style blend kinds {} and {}", i, j));
      const auto a = composition_samples[i].promoted;
      const auto b = composition_samples[j].promoted;
      const auto foreground = i == 0 ? composition_samples[j].color
          : j == 0 ? composition_samples[i].color : tjg::color_t{tjg::rgb{
              static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
              static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
              static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      const auto lhs = tjg::text_style{composition_samples[i].color,
                                       tjg::gray_color{12}, tjg::emphasis::bold};
      const auto rhs = tjg::text_style{composition_samples[j].color, tjg::no_color,
                                       tjg::emphasis::underline};
      EXPECT_EQ(tjg::blend(lhs, rhs),
                (tjg::text_style{foreground, tjg::gray_color{12},
                                 tjg::emphasis::bold | tjg::emphasis::underline}));
      EXPECT_EQ(tjg::blend(lhs, rhs, tjg::bg(tjg::cube_color{1, 2, 3})),
                (tjg::text_style{foreground, tjg::rgb{111, 131, 151},
                                 tjg::emphasis::bold | tjg::emphasis::underline}));
      EXPECT_EQ(lhs.fg(), composition_samples[i].color);
      EXPECT_EQ(lhs.bg(), tjg::color_t{tjg::gray_color{12}});
      EXPECT_EQ(rhs.fg(), composition_samples[j].color);
      EXPECT_FALSE(rhs.has_bg());
    }
  }
  for (unsigned mask = 0; mask != 256; ++mask) {
    auto lhs = tjg::text_style{static_cast<tjg::emphasis>(mask)};
    auto rhs = tjg::text_style{static_cast<tjg::emphasis>(mask ^ 0xFFu)};
    EXPECT_EQ(tjg::blend(lhs, rhs),
              tjg::text_style{static_cast<tjg::emphasis>(0xFF)});
  }
}

TEST(color_test, text_style_color_constructor) {
  using tjg::color_t;
  static_assert(std::is_default_constructible<tjg::text_style>::value,
                "default construction must remain unambiguous");
  static_assert(std::is_convertible<tjg::emphasis, tjg::text_style>::value,
                "emphasis conversion must remain implicit");
  static_assert(std::is_convertible<color_t, tjg::text_style>::value,
                "the color constructor must not be explicit");
  static_assert(noexcept(tjg::text_style{tjg::color::red,
                                         tjg::term_color::black,
                                         tjg::emphasis::bold}),
                "color construction must be noexcept");
  static_assert(noexcept(tjg::text_style{tjg::emphasis::bold,
                                         tjg::color::red,
                                         tjg::term_color::black}),
                "emphasis-first construction must be noexcept");
  constexpr auto emphasis_only = tjg::text_style{tjg::emphasis::bold};
  static_assert(tjg::to_underlying(emphasis_only) == (uint64_t{1} << 54),
                "emphasis-only construction must remain constexpr");
  constexpr auto empty = tjg::text_style{tjg::no_color};
  static_assert(tjg::to_underlying(empty) == 0,
                "an unspecified foreground must produce an empty style");

  const color_t colors[] = {
      tjg::no_color, tjg::rgb{}, tjg::rgb{0xFFFFFF},
      tjg::term_color::black, tjg::term_color::bright_white,
      tjg::color::coral, tjg::cube_color{0}, tjg::cube_color{215},
      tjg::gray_color{0}, tjg::gray_color{23}};
  const tjg::emphasis attributes[] = {
      tjg::emphasis{}, tjg::emphasis::bold | tjg::emphasis::faint,
      tjg::emphasis::strike, static_cast<tjg::emphasis>(0xFF)};
  for (auto foreground : colors) {
    tjg::text_style implicit = foreground;
    EXPECT_EQ(implicit, tjg::fg(foreground));
    for (auto emphasis : attributes) {
      tjg::text_style implicit_emphasis = emphasis;
      EXPECT_EQ(implicit_emphasis, tjg::text_style{emphasis});
      EXPECT_EQ((tjg::text_style{emphasis, foreground}),
                tjg::fg(foreground) | emphasis);
    }
    for (auto background : colors) {
      EXPECT_EQ((tjg::text_style{foreground, background}),
                tjg::fg(foreground) | tjg::bg(background));
      for (auto emphasis : attributes) {
        auto style = tjg::text_style{foreground, background, emphasis};
        EXPECT_EQ((tjg::text_style{emphasis, foreground, background}), style);
        EXPECT_EQ(style, tjg::fg(foreground) | tjg::bg(background) | emphasis);
        EXPECT_EQ(style.get_foreground(), foreground);
        EXPECT_EQ(style.get_background(), background);
        EXPECT_EQ(style.get_emphasis(), emphasis);
      }
    }
  }

  EXPECT_EQ(tjg::text_style{}, tjg::text_style{tjg::emphasis{}});
  EXPECT_EQ(tjg::text_style{tjg::color::red}, tjg::fg(tjg::color::red));
  EXPECT_TRUE(tjg::text_style{tjg::rgb{}}.has_foreground());
  EXPECT_TRUE(tjg::text_style{tjg::term_color::black}.has_foreground());
  tjg::text_style listed = {tjg::color::red, tjg::color::blue,
                           tjg::emphasis::bold};
  EXPECT_EQ(listed, tjg::fg(tjg::color::red) | tjg::bg(tjg::color::blue) |
                        tjg::emphasis::bold);
  tjg::text_style attributes_first = {tjg::emphasis::bold, tjg::color::red,
                                     tjg::color::blue};
  EXPECT_EQ(attributes_first, listed);
  EXPECT_EQ((tjg::text_style{tjg::emphasis::bold, tjg::no_color,
                             tjg::term_color::black}),
            tjg::bg(tjg::term_color::black) | tjg::emphasis::bold);
#if 1
  constexpr auto style = tjg::text_style{tjg::color::coral,
                                         tjg::term_color::black,
                                         tjg::emphasis::bold};
  static_assert(tjg::to_underlying(style.get_foreground()) ==
                    ((uint32_t{1} << 24) | 0xFF7F50),
                "constant evaluation must preserve the RGB kind and payload");
  static_assert(tjg::to_underlying(style.get_background()) ==
                    (uint32_t{2} << 24),
                "constant evaluation must preserve specified terminal black");
  static_assert(style.get_emphasis() == tjg::emphasis::bold,
                "constant evaluation must preserve emphasis");
  constexpr auto constexpr_attributes_first = tjg::text_style{
      tjg::emphasis::bold, tjg::color::coral, tjg::term_color::black};
  static_assert(constexpr_attributes_first == style,
                "both constructor orders must agree in constant evaluation");
#endif
}

void check_fluent_emphasis(tjg::emphasis attribute,
                          tjg::text_style& (tjg::text_style::*set)(bool),
                          bool (tjg::text_style::*get)() const) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = tjg::text_style{static_cast<tjg::emphasis>(bits),
                                    tjg::color::coral,
                                    tjg::term_color::black};
    auto mask = static_cast<unsigned>(attribute);
    EXPECT_EQ((original.*get)(), (bits & mask) != 0);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      auto style = original;
      auto expected = tjg::text_style{
          static_cast<tjg::emphasis>(enabled ? bits | mask : bits & ~mask),
          tjg::color::coral, tjg::term_color::black};
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);
      EXPECT_EQ((style.*get)(), enabled);
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);  // Repeating a setter is idempotent.
    }
  }
}

void check_free_emphasis(tjg::emphasis attribute,
                        tjg::text_style (*set)(tjg::text_style, bool)) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = tjg::text_style{static_cast<tjg::emphasis>(bits),
                                    tjg::cube_color{1, 2, 3},
                                    tjg::gray_color{12}};
    const auto saved = original;
    const auto mask = static_cast<unsigned>(attribute);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      const auto result = set(original, enabled);
      EXPECT_EQ(result, (tjg::text_style{
                            static_cast<tjg::emphasis>(
                                enabled ? bits | mask : bits & ~mask),
                            tjg::cube_color{1, 2, 3}, tjg::gray_color{12}}));
      EXPECT_EQ(original, saved);
      EXPECT_EQ(set(result, enabled), result);
    }
  }
}

TEST(color_test, emphasis_factories) {
#define CHECK_EMPHASIS_FACTORY(name)                                      \
  static_assert(std::is_same<decltype(tjg::name()),                        \
                             tjg::text_style>::value,                    \
                "attribute factories must return text_style");           \
  static_assert(noexcept(tjg::name()) && noexcept(tjg::name(true)) &&      \
                    noexcept(tjg::name(false)),                          \
                "attribute factories must be noexcept");                 \
  static_assert(tjg::name() == tjg::text_style{tjg::emphasis::name} &&      \
                    tjg::name(true) == tjg::name(),                       \
                "default and true must set exactly one attribute");       \
  static_assert(tjg::name(false).empty(),                                 \
                "false must produce an empty style");                    \
  for (bool enabled : {false, true}) {                                    \
    const auto style = tjg::name(enabled);                               \
    EXPECT_EQ(style.emph(), enabled ? tjg::emphasis::name                  \
                                   : tjg::emphasis{});                   \
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
  EXPECT_EQ(std::format("{}", tjg::styled(42, tjg::bold())),
            "\x1b[1m42\x1b[0m");
  EXPECT_EQ(std::format("{}", tjg::styled(42, tjg::bold(false))), "42");
}

TEST(color_test, free_emphasis) {
#define CHECK_FREE_EMPHASIS(name)                                            \
  static_assert(noexcept(tjg::name(tjg::text_style{})) &&                    \
                    noexcept(tjg::name(tjg::text_style{}, false)),          \
                "free attribute functions must be noexcept");               \
  static_assert(std::is_same<decltype(tjg::name(tjg::text_style{})),          \
                             tjg::text_style>::value,                      \
                "free attribute functions must return a style value");      \
  EXPECT_EQ(tjg::name(tjg::text_style{}),                                    \
            (tjg::text_style{tjg::emphasis::name}));                         \
  EXPECT_EQ(tjg::name(tjg::text_style{}),                                    \
            tjg::name(tjg::text_style{}, true));                            \
  check_free_emphasis(tjg::emphasis::name, &tjg::name)
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

#if 1
TEST(color_test, free_emphasis_constexpr) {
  constexpr auto base = tjg::text_style{tjg::cube_color{1, 2, 3},
                                       tjg::gray_color{12}};
#define CHECK_FREE_CONSTEXPR(name)                                          \
  constexpr auto name##_style = tjg::name(base);                            \
  static_assert(name##_style.is_##name(), "default must enable attribute"); \
  static_assert(tjg::name(name##_style, false) == base,                     \
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
#endif

TEST(color_test, fluent_emphasis) {
  using tjg::text_style;
#define CHECK_EMPHASIS(name)                                                 \
  static_assert(noexcept(text_style{}.name()), "setter must be noexcept");   \
  static_assert(noexcept(text_style{}.is_##name()),                          \
                "getter must be noexcept");                                \
  static_assert(!text_style{}.is_##name(), "empty getter must be constexpr"); \
  check_fluent_emphasis(tjg::emphasis::name, &text_style::name,                \
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
  EXPECT_EQ(style.emph(), static_cast<tjg::emphasis>(0xFF));
  EXPECT_EQ(&style.bold(false).faint(false).italic(false).underline(false)
                 .blink(false).reverse(false).conceal(false)
                 .strike(false), &style);
  EXPECT_EQ(style, text_style{});

  style.bold().faint();
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  EXPECT_EQ(tjg::format(style, "both"), "\x1b[1;2mboth\x1b[0m");
  style.bold(false);
  EXPECT_FALSE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  style.bold().faint(false);
  EXPECT_TRUE(style.is_bold());
  EXPECT_FALSE(style.is_faint());
}

TEST(color_test, fluent_components) {
  using tjg::color_t;
  using tjg::emphasis;
  using tjg::text_style;
  constexpr auto empty = text_style{};
  static_assert(tjg::to_underlying(empty.fg()) == 0,
                "empty foreground getter must be constexpr");
  static_assert(tjg::to_underlying(empty.bg()) == 0,
                "empty background getter must be constexpr");
  static_assert(empty.emph() == emphasis{},
                "empty emphasis getter must be constexpr");
  constexpr auto filled = text_style{emphasis::strike};
  static_assert(filled.emph() == emphasis::strike &&
                    filled.get_emphasis() == emphasis::strike,
                "both emphasis getters must be constexpr in C++11");
  static_assert(noexcept(empty.fg()) && noexcept(empty.bg()) &&
                    noexcept(empty.emph()), "getters must be noexcept");
  static_assert(noexcept(text_style{}.fg(tjg::no_color)) &&
                    noexcept(text_style{}.bg(tjg::no_color)) &&
                    noexcept(text_style{}.emph(emphasis{})),
                "setters must be noexcept");

  const color_t colors[] = {
      tjg::no_color, tjg::rgb{}, tjg::color::coral,
      tjg::term_color::black, tjg::term_color::bright_white};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto original = text_style{
          static_cast<emphasis>(0xFF), foreground, background};
      EXPECT_EQ(original.fg(), original.get_foreground());
      EXPECT_EQ(original.bg(), original.get_background());
      EXPECT_EQ(original.emph(), original.get_emphasis());
      for (auto replacement : colors) {
        auto style = original;
        EXPECT_EQ(&style.fg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     replacement, background}));
        EXPECT_EQ(style.fg(), replacement);
        EXPECT_EQ(style.has_foreground(), replacement != color_t{});
        style = original;
        EXPECT_EQ(&style.bg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     foreground, replacement}));
        EXPECT_EQ(style.bg(), replacement);
        EXPECT_EQ(style.has_background(), replacement != color_t{});
      }
      auto style = original;
      auto selected = emphasis::bold | emphasis::blink | emphasis::strike;
      EXPECT_EQ(&style.emph(selected), &style);
      EXPECT_EQ(style, (text_style{selected, foreground, background}));
      EXPECT_EQ(style.emph(), selected);
      EXPECT_EQ(&style.emph(emphasis{}), &style);
      EXPECT_EQ(style, (text_style{foreground, background}));
      EXPECT_FALSE(style.has_emphasis());
    }
  }

  auto style = text_style{};
  EXPECT_EQ(&style.fg(tjg::color::coral).bg(tjg::term_color::black)
                 .emph(emphasis::italic).underline(), &style);
  EXPECT_EQ(style, (text_style{emphasis::italic | emphasis::underline,
                               tjg::color::coral, tjg::term_color::black}));
  EXPECT_EQ(&style.fg(tjg::no_color).bg(tjg::no_color).emph(emphasis{}), &style);
  EXPECT_EQ(style, empty);
  EXPECT_EQ(tjg::format(style, "plain"), "plain");
}

TEST(color_test, fluent_presence) {
  using tjg::emphasis;
  using tjg::text_style;
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
    tjg::color_t value;
    bool specified;
  } colors[] = {{tjg::no_color, false}, {tjg::rgb{}, true},
                {tjg::term_color::black, true}, {tjg::color::coral, true},
                {tjg::term_color::bright_white, true}};
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
        EXPECT_EQ(style.has_fg(), style.has_foreground());
        EXPECT_EQ(style.has_bg(), style.has_background());
        EXPECT_EQ(style.has_emph(), style.has_emphasis());
      }
    }
  }

  auto style = text_style{emphasis::bold, tjg::rgb{}, tjg::term_color::black};
  style.fg(tjg::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bg(tjg::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bold(false);
  EXPECT_FALSE(style.has_emph());
  style.emph(emphasis::faint).fg(tjg::term_color::black).bg(tjg::rgb{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.emph(emphasis{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
}

#if 1
TEST(color_test, fluent_constexpr) {
  constexpr auto all = tjg::text_style{}.bold().faint().italic().underline()
                          .blink().reverse().conceal().strike();
  static_assert(all.is_bold() && all.is_faint() && all.is_italic() &&
                    all.is_underline() && all.is_blink() && all.is_reverse() &&
                    all.is_conceal() && all.is_strike(),
                "all emphasis setters must support constant evaluation");
  constexpr auto cleared = tjg::text_style{all}.bold(false).faint(false)
                              .italic(false).underline(false).blink(false)
                              .reverse(false).conceal(false).strike(false);
  static_assert(!cleared.has_emphasis(),
                "disabling emphasis must support constant evaluation");
  constexpr auto replaced = tjg::text_style{all}.fg(tjg::color::coral)
                               .bg(tjg::term_color::black)
                               .emph(tjg::emphasis::underline);
  static_assert(replaced == tjg::text_style{tjg::emphasis::underline,
                                             tjg::color::coral,
                                             tjg::term_color::black},
                "component replacement must support constant evaluation");
  static_assert(replaced.has_fg() && replaced.has_bg() && replaced.has_emph(),
                "presence queries must detect constexpr component replacement");
  constexpr auto empty = tjg::text_style{replaced}.fg(tjg::no_color)
                            .bg(tjg::no_color).emph(tjg::emphasis{});
  static_assert(empty == tjg::text_style{},
                "clearing components must support constant evaluation");
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "presence queries must detect constexpr component clearing");
  constexpr auto cleared_style = tjg::text_style{replaced}.clear();
  static_assert(cleared_style.empty() && cleared_style == tjg::text_style{},
                "clear must restore an empty style during constant evaluation");
  constexpr auto reused = tjg::text_style{replaced}.clear().strike();
  static_assert(reused == tjg::text_style{tjg::emphasis::strike},
                "clear must support fluent reuse during constant evaluation");
}
#endif

TEST(color_test, text_style) {
  EXPECT_FALSE(tjg::text_style().has_foreground());
  EXPECT_FALSE(tjg::text_style().has_background());
  EXPECT_FALSE(tjg::text_style().has_emphasis());

  EXPECT_TRUE(fg(tjg::rgb(0)).has_foreground());
  EXPECT_FALSE(fg(tjg::rgb(0)).has_background());
  EXPECT_FALSE(fg(tjg::rgb(0)).has_emphasis());
  EXPECT_TRUE(bg(tjg::rgb(0)).has_background());
  EXPECT_FALSE(bg(tjg::rgb(0)).has_foreground());
  EXPECT_FALSE(bg(tjg::rgb(0)).has_emphasis());

  EXPECT_TRUE(
      (fg(tjg::rgb(0xFFFFFF)) | bg(tjg::rgb(0xFFFFFF))).has_foreground());
  EXPECT_TRUE(
      (fg(tjg::rgb(0xFFFFFF)) | bg(tjg::rgb(0xFFFFFF))).has_background());
  EXPECT_FALSE(
      (fg(tjg::rgb(0xFFFFFF)) | bg(tjg::rgb(0xFFFFFF))).has_emphasis());

  EXPECT_EQ(fg(tjg::rgb(0x000000)) | fg(tjg::rgb(0x000000)),
            fg(tjg::rgb(0x000000)));
  EXPECT_EQ(fg(tjg::rgb(0x00000F)) | fg(tjg::rgb(0x00000F)),
            fg(tjg::rgb(0x00000F)));
  EXPECT_EQ(fg(tjg::rgb(0xC0F000)) | fg(tjg::rgb(0x000FEE)),
            fg(tjg::rgb(0xC0FFEE)));

  EXPECT_EQ(fg(tjg::terminal_color::black) | fg(tjg::terminal_color::black),
            fg(tjg::rgb(0)));
  EXPECT_EQ(fg(tjg::terminal_color::black) | fg(tjg::terminal_color::white),
            fg(tjg::rgb(0xE5E5E5)));
  EXPECT_EQ(bg(tjg::terminal_color::black) | bg(tjg::terminal_color::black),
            bg(tjg::rgb(0)));
  EXPECT_EQ(bg(tjg::terminal_color::black) | bg(tjg::terminal_color::white),
            bg(tjg::rgb(0xE5E5E5)));
  EXPECT_EQ(fg(tjg::terminal_color::black) | fg(tjg::color::black),
            fg(tjg::rgb(0)));
  EXPECT_EQ(bg(tjg::terminal_color::black) | bg(tjg::color::black),
            bg(tjg::rgb(0)));

  EXPECT_NO_THROW((void) (fg(tjg::terminal_color::white) |
                          bg(tjg::terminal_color::white)));
  EXPECT_NO_THROW(
      (void) (fg(tjg::terminal_color::white) | bg(tjg::rgb(0xFFFFFF))));
  EXPECT_NO_THROW((void) (fg(tjg::terminal_color::white) | tjg::text_style()));
  EXPECT_NO_THROW((void) (bg(tjg::terminal_color::white) | tjg::text_style()));
}

TEST(color_test, short_names_compatibility) {
  using tjg::emphasis;
  using tjg::term_color;
  using tjg::text_style;
  static_assert(std::is_same<term_color, tjg::terminal_color>::value,
                "terminal_color must remain an alias");
  static_assert(emphasis::strike == emphasis::strikethrough,
                "both names must select the same emphasis bit");
  static_assert(text_style{emphasis::strike}.is_strikethrough() &&
                    text_style{emphasis::strikethrough}.is_strike(),
                "both queries must recognize either emphasis name");

  for (unsigned i = 0; i < 16; ++i) {
    const auto color = static_cast<term_color>(i);
    const auto legacy = static_cast<tjg::terminal_color>(i);
    const auto stored = tjg::color_t{color};
    EXPECT_EQ(tjg::get<tjg::terminal_color>(stored), color);
    EXPECT_TRUE(tjg::holds_alternative<tjg::terminal_color>(stored));
    EXPECT_EQ(tjg::get<term_color>(
                  tjg::color_t{legacy}), color);
    EXPECT_EQ(tjg::format(fg(color), "fg"), tjg::format(fg(legacy), "fg"));
    EXPECT_EQ(tjg::format(bg(color), "bg"), tjg::format(bg(legacy), "bg"));
  }

  auto style = text_style{emphasis::bold, term_color::red,
                         tjg::terminal_color::black};
  EXPECT_EQ(&style.strike(), &style);
  EXPECT_TRUE(style.is_strikethrough());
  EXPECT_EQ(tjg::format(style, "mixed"),
            FMT_SGR_EXPECTED(
                "\x1b[1;9m\x1b[31m\x1b[40mmixed\x1b[0m",
                "\x1b[1;9;31;40mmixed\x1b[0m"));
  EXPECT_EQ(&style.strikethrough(false), &style);
  EXPECT_FALSE(style.is_strike());
  style.strikethrough().strike(false);
  EXPECT_FALSE(style.is_strikethrough());
  EXPECT_EQ(style, (text_style{emphasis::bold, term_color::red,
                              tjg::terminal_color::black}));
  EXPECT_EQ(tjg::format(emphasis::strike, "strike"),
            "\x1b[9mstrike\x1b[0m");
#if 1
  constexpr auto cleared = text_style{}.strike().strikethrough(false);
  static_assert(!cleared.is_strike() && !cleared.is_strikethrough(),
                "mixed setters must support constant evaluation");
#endif
}

TEST(color_test, term_color_boundaries) {
  using tjg::term_color;
  EXPECT_EQ(static_cast<int>(term_color::black), 0);
  EXPECT_EQ(static_cast<int>(term_color::white), 7);
  EXPECT_EQ(static_cast<int>(term_color::bright_black), 8);
  EXPECT_EQ(static_cast<int>(term_color::bright_white), 15);

  EXPECT_TRUE(fg(term_color::black).has_foreground());
  EXPECT_TRUE(bg(term_color::black).has_background());

  EXPECT_EQ(tjg::format(fg(term_color::black), "black"),
            "\x1b[30mblack\x1b[0m");
  EXPECT_EQ(tjg::format(bg(term_color::black), "black"),
            "\x1b[40mblack\x1b[0m");
  EXPECT_EQ(tjg::format(fg(term_color::white), "white"),
            "\x1b[37mwhite\x1b[0m");
  EXPECT_EQ(tjg::format(bg(term_color::white), "white"),
            "\x1b[47mwhite\x1b[0m");
  EXPECT_EQ(tjg::format(fg(term_color::bright_black), "bright black"),
            "\x1b[90mbright black\x1b[0m");
  EXPECT_EQ(tjg::format(bg(term_color::bright_black), "bright black"),
            "\x1b[100mbright black\x1b[0m");
  EXPECT_EQ(tjg::format(fg(term_color::bright_white), "bright white"),
            "\x1b[97mbright white\x1b[0m");
  EXPECT_EQ(tjg::format(bg(term_color::bright_white), "bright white"),
            "\x1b[107mbright white\x1b[0m");
}

TEST(color_test, format) {
  EXPECT_EQ(tjg::format(tjg::text_style(), "no style"), "no style");
  EXPECT_EQ(tjg::format(fg(tjg::rgb(255, 20, 30)), "rgb(255,20,30)"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
  EXPECT_EQ(tjg::format(fg(tjg::rgb(255, 0, 0)) | fg(tjg::rgb(0, 20, 30)),
                        "rgb(255,20,30)"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
  EXPECT_EQ(
      tjg::format(fg(tjg::rgb(0, 0, 0)) | fg(tjg::rgb(0, 0, 0)), "rgb(0,0,0)"),
      FMT_SGR_EXPECTED(
          "\x1b[38;2;000;000;000mrgb(0,0,0)\x1b[0m",
          "\x1b[38;2;0;0;0mrgb(0,0,0)\x1b[0m"));
  EXPECT_EQ(tjg::format(fg(tjg::color::blue), "blue"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;000;000;255mblue\x1b[0m",
                "\x1b[38;2;0;0;255mblue\x1b[0m"));
  EXPECT_EQ(
      tjg::format(fg(tjg::color::blue) | bg(tjg::color::red), "two color"),
      FMT_SGR_EXPECTED(
          "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m",
          "\x1b[38;2;0;0;255;48;2;255;0;0mtwo color\x1b[0m"));
  EXPECT_EQ(tjg::format(tjg::emphasis::bold, "bold"), "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::faint, "faint"), "\x1b[2mfaint\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::blink, "blink"), "\x1b[5mblink\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::reverse, "reverse"),
            "\x1b[7mreverse\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::conceal, "conceal"),
            "\x1b[8mconceal\x1b[0m");
  EXPECT_EQ(tjg::format(tjg::emphasis::strikethrough, "strikethrough"),
            "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      tjg::format(fg(tjg::color::blue) | tjg::emphasis::bold, "blue/bold"),
      FMT_SGR_EXPECTED(
          "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m",
          "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m"));
  EXPECT_EQ(tjg::format(tjg::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(tjg::format(fg(tjg::color::blue), "blue log"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;000;000;255mblue log\x1b[0m",
                "\x1b[38;2;0;0;255mblue log\x1b[0m"));
  EXPECT_EQ(tjg::format(tjg::text_style(), "hi"), "hi");
  EXPECT_EQ(tjg::format(fg(tjg::terminal_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(tjg::format(bg(tjg::terminal_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(tjg::format(fg(tjg::terminal_color::bright_green), "tbgreen"),
            "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(tjg::format(bg(tjg::terminal_color::bright_magenta), "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(tjg::format(fg(tjg::terminal_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(std::format("{}{}", tjg::styled("red", fg(tjg::color::red)),
                        tjg::styled("bold", tjg::emphasis::bold)),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;000;000mred\x1b[0m\x1b[1mbold\x1b[0m",
                "\x1b[38;2;255;0;0mred\x1b[0m\x1b[1mbold\x1b[0m"));
  EXPECT_EQ(std::format("{}", tjg::styled("bar", fg(tjg::color::blue) |
                                                     tjg::emphasis::underline)),
            FMT_SGR_EXPECTED(
                "\x1b[4m\x1b[38;2;000;000;255mbar\x1b[0m",
                "\x1b[4;38;2;0;0;255mbar\x1b[0m"));
  EXPECT_EQ(
      std::format(
          "{}", tjg::styled(
                    "all", tjg::emphasis::bold | tjg::emphasis::faint |
                               tjg::emphasis::italic |
                               tjg::emphasis::underline | tjg::emphasis::blink |
                               tjg::emphasis::reverse | tjg::emphasis::conceal |
                               tjg::emphasis::strikethrough)),
      "\x1b[1;2;3;4;5;7;8;9mall\x1b[0m");
}

TEST(color_test, format_to) {
  auto out = std::string();
  tjg::format_to(std::back_inserter(out), fg(tjg::rgb(255, 20, 30)),
                 "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(std::string(out.data(), out.size()),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)123\x1b[0m"));
}

TEST(color_test, print) {
  EXPECT_WRITE(stdout, tjg::print(fg(tjg::rgb(255, 20, 30)), "rgb(255,20,30)"),
               FMT_SGR_EXPECTED(
                   "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                   "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
}

TEST(color_test, println) {
  EXPECT_WRITE(stdout,
               tjg::println(fg(tjg::rgb(255, 20, 30)), "rgb(255,20,30)"),
               FMT_SGR_EXPECTED(
                   "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m\n",
                   "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m\n"));
  EXPECT_WRITE(stdout, tjg::println(tjg::emphasis::bold, "bold"),
               "\x1b[1mbold\x1b[0m\n");
  EXPECT_WRITE(
      stdout,
      tjg::println(fg(tjg::color::blue) | tjg::emphasis::bold, "blue/bold"),
      FMT_SGR_EXPECTED(
          "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m\n",
          "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m\n"));
}

#if TJG_COLOR_COMPACT_SGR == 1
TEST(color_test, compact_sgr_combined_style) {
  const auto style = tjg::text_style{static_cast<tjg::emphasis>(0xff),
                                    tjg::rgb{0, 9, 255}, tjg::cube_color{0}};
  const auto sgr = std::string{
      "\x1b[1;2;3;4;5;7;8;9;38;2;0;9;255;48;5;16m"};
  check_to_sgr(style, sgr);
  EXPECT_EQ(tjg::format(style, "value"), sgr + "value\x1b[0m");
  EXPECT_EQ(std::format("{:>5}", tjg::styled(42, style)),
            sgr + "   42\x1b[0m");
  EXPECT_EQ(std::format("{}plain", tjg::styled("value", style)),
            sgr + "value\x1b[0mplain");

  // Appending a style must preserve existing buffer contents.
  auto buf = tjg::memory_buffer();
  const auto prefix = std::string{"prefix"};
  buf.append(prefix.data(), prefix.data() + prefix.size());
  tjg::detail::append_sgr(buf, style);
  EXPECT_EQ(std::string(buf.data(), buf.size()), prefix + sgr);
}

TEST(color_test, compact_sgr_empty_and_single_components) {
  check_to_sgr(tjg::text_style{}, "");
  EXPECT_EQ(tjg::format(tjg::text_style{}, "plain"), "plain");
  EXPECT_EQ(std::format("{:>5}", tjg::styled(42, tjg::text_style{})), "   42");
  check_to_sgr(tjg::text_style{tjg::emphasis::bold}, "\x1b[1m");
  check_to_sgr(tjg::fg(tjg::term_color::black), "\x1b[30m");
  check_to_sgr(tjg::bg(tjg::term_color::bright_white), "\x1b[107m");
  check_to_sgr(tjg::fg(tjg::gray_color{0}), "\x1b[38;5;232m");
  check_to_sgr(tjg::bg(tjg::gray_color{23}), "\x1b[48;5;255m");
}

TEST(color_test, compact_sgr_decimal_channels) {
  for (unsigned n = 0; n < 256; ++n) {
    const auto channel = static_cast<uint8_t>(n);
    const auto rgb = tjg::rgb{channel, channel, channel};
    const auto value = std::to_string(n);
    const auto parameters = value + ";" + value + ";" + value;
    check_to_sgr(tjg::fg(rgb), "\x1b[38;2;" + parameters + "m");
    check_to_sgr(tjg::bg(rgb), "\x1b[48;2;" + parameters + "m");
  }
}
#endif

namespace {
void check_wrap_sgr(const tjg::text_style& style, const std::string& text,
                   const std::string& expected) {
  const auto original_style = style;
  const auto original_text = text;
  EXPECT_EQ(tjg::wrap_sgr(style, text), expected);
  EXPECT_EQ(tjg::wrap_sgr<char>(style, std::string_view{text.data(), text.size()}),
            expected);
  const auto wide_text = std::wstring{text.begin(), text.end()};
  EXPECT_EQ(tjg::wrap_sgr<wchar_t>(
                style, std::basic_string_view<wchar_t>{wide_text.data(),
                                                     wide_text.size()}),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original_style);
  EXPECT_EQ(text, original_text);
}
}  // namespace

TEST(color_test, wrap_sgr) {
  static_assert(std::is_same<decltype(tjg::wrap_sgr(tjg::text_style{}, "text")),
                             std::string>::value,
                "the convenience overload must return std::string");
  static_assert(std::is_same<decltype(tjg::wrap_sgr<wchar_t>(
                                tjg::text_style{},
                                std::basic_string_view<wchar_t>{L"text"})),
                             std::wstring>::value,
                "wide wrapping must return std::wstring");
  EXPECT_EQ(tjg::wrap_sgr(tjg::text_style{tjg::emphasis::bold}, "literal"),
            "\x1b[1mliteral\x1b[0m");
  check_wrap_sgr(tjg::text_style{}, "value", "value");
  check_wrap_sgr(tjg::text_style{tjg::emphasis::bold}, "value",
                 "\x1b[1mvalue\x1b[0m");
  check_wrap_sgr(tjg::text_style{tjg::rgb{1, 2, 3}}, "value",
                 FMT_SGR_EXPECTED("\x1b[38;2;001;002;003mvalue\x1b[0m",
                                  "\x1b[38;2;1;2;3mvalue\x1b[0m"));
  check_wrap_sgr(tjg::text_style{tjg::term_color::black,
                                 tjg::term_color::bright_white},
                 "value", FMT_SGR_EXPECTED("\x1b[30m\x1b[107mvalue\x1b[0m",
                                            "\x1b[30;107mvalue\x1b[0m"));
  check_wrap_sgr(tjg::text_style{tjg::cube_color{0}, tjg::gray_color{23}},
                 "value", FMT_SGR_EXPECTED(
                              "\x1b[38;5;016m\x1b[48;5;255mvalue\x1b[0m",
                              "\x1b[38;5;16;48;5;255mvalue\x1b[0m"));
  check_wrap_sgr(tjg::text_style{static_cast<tjg::emphasis>(0xff),
                                 tjg::rgb{255, 255, 255},
                                 tjg::rgb{255, 255, 255}},
                 "value", FMT_SGR_EXPECTED(
                              "\x1b[1;2;3;4;5;7;8;9m\x1b[38;2;255;255;255m"
                              "\x1b[48;2;255;255;255mvalue\x1b[0m",
                              "\x1b[1;2;3;4;5;7;8;9;38;2;255;255;255;"
                              "48;2;255;255;255mvalue\x1b[0m"));
}

TEST(color_test, wrap_sgr_empty) {
  const tjg::text_style styles[] = {tjg::text_style{},
                                    tjg::text_style{tjg::emphasis::bold}};
  for (const auto& style : styles) {
    EXPECT_EQ(tjg::wrap_sgr(style, std::string_view{}), "");
    EXPECT_EQ(tjg::wrap_sgr(style, ""), "");
    EXPECT_EQ(tjg::wrap_sgr<wchar_t>(style, std::basic_string_view<wchar_t>{}),
              L"");
    EXPECT_EQ(tjg::wrap_sgr<wchar_t>(style, L""), L"");
  }
}

TEST(color_test, wrap_sgr_literal_payload) {
  const auto style = tjg::text_style{tjg::emphasis::bold};
  const char bytes[] = "before\0{}\x1b[31mafter";
  const auto text = std::string{bytes, sizeof(bytes) - 1};
  check_wrap_sgr(style, text, "\x1b[1m" + text + "\x1b[0m");
  check_wrap_sgr(tjg::text_style{}, text, text);
  const auto long_text = std::string(4096, 'x');
  check_wrap_sgr(style, long_text, "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(tjg::wrap_sgr<wchar_t>(style, L"wide \u03a9"),
            L"\x1b[1mwide \u03a9\x1b[0m");

  auto source = std::string{"owned"};
  const auto wrapped = tjg::wrap_sgr(style, source);
  source.assign("changed");
  EXPECT_EQ(wrapped, "\x1b[1mowned\x1b[0m");
}

TEST(color_test, absolute_style_sgr) {
  const auto reset = tjg::text_style{}.absolute();
  check_to_sgr(reset, "\x1b[0m");
  const auto style = tjg::underline().absolute();
  const auto prefix = std::string{FMT_SGR_EXPECTED("\x1b[0m\x1b[4m",
                                                  "\x1b[0;4m")};
  check_to_sgr(style, prefix);
  EXPECT_EQ(tjg::format(style, "text"), prefix + "text\x1b[0m");
  // Styled arguments honor the explicit absolute reset in both modes.
  EXPECT_EQ(std::format("{}", tjg::styled("text", style)),
            prefix + "text\x1b[0m");
  EXPECT_EQ(tjg::wrap_sgr(reset, "text"), "\x1b[0mtext\x1b[0m");
  EXPECT_EQ(tjg::wrap_sgr<wchar_t>(reset, L"text"),
            L"\x1b[0mtext\x1b[0m");
  const auto maximal = tjg::text_style{tjg::rgb{255, 255, 255},
      tjg::rgb{255, 255, 255}, static_cast<tjg::emphasis>(0xFF)}.absolute();
  const auto expected = std::string{FMT_SGR_EXPECTED(
      "\x1b[0m\x1b[1;2;3;4;5;7;8;9m"
      "\x1b[38;2;255;255;255m\x1b[48;2;255;255;255m",
      "\x1b[0;1;2;3;4;5;7;8;9;38;2;255;255;255;48;2;255;255;255m")};
  check_to_sgr(maximal, expected);
  EXPECT_EQ(expected.size(), FMT_SGR_EXPECTED(size_t{60}, size_t{54}));
  EXPECT_LE(expected.size(), tjg::sgr_max_length);
}

#undef FMT_SGR_EXPECTED


TEST(color_test, text_style_underlay) {
  using tc = tjg::term_color;
  const tjg::color_t colors[] = {tjg::no_color, tc::black, tc::red,
      tjg::rgb{1, 2, 3}, tjg::cube_color{3}, tjg::gray_color{12}};
  for (auto foreground : colors) {
    for (auto background : colors) {
      for (auto supplied : colors) {
        const auto base = tjg::text_style{foreground, background}.bold();
        const auto defaults = tjg::text_style{supplied, supplied}.underline().absolute();
        const auto result = tjg::underlay(base, defaults);
        EXPECT_EQ(result.fg(), base.has_fg() ? foreground : supplied);
        EXPECT_EQ(result.bg(), base.has_bg() ? background : supplied);
        EXPECT_TRUE(result.is_bold() && result.is_underline() && result.is_absolute());
        auto member = base;
        EXPECT_EQ(&member.underlay(defaults), &member);
        EXPECT_EQ(member, result);
        EXPECT_EQ(tjg::underlay(base, tjg::text_style{}), base);
        EXPECT_EQ(tjg::underlay(base, base), base);
      }
    }
  }
}

#if 1
TEST(color_test, text_style_underlay_constexpr) {
  constexpr auto base = tjg::fg(tjg::term_color::yellow);
  constexpr auto defaults = tjg::fg(tjg::term_color::white).bg(tjg::term_color::blue);
  constexpr auto result = tjg::underlay(base, defaults);
  static_assert(result.fg() == tjg::term_color::yellow, "preserve foreground");
  static_assert(result.bg() == tjg::term_color::blue, "supply background");
  constexpr auto member = tjg::text_style{base}.underlay(defaults);
  static_assert(member == result, "member and free underlay agree");
}
#endif

TEST(color_test, standard_sgr_reset_view) {
  static_assert(std::is_same_v<decltype(tjg::sgr_reset()), std::string_view>);
  static_assert(std::is_same_v<decltype(tjg::sgr_reset<wchar_t>()),
                               std::wstring_view>);
  static_assert(tjg::sgr_reset() == std::string_view{"\x1b[0m"});
  static_assert(tjg::sgr_reset<wchar_t>() == std::wstring_view{L"\x1b[0m"});
}
