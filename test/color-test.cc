// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/color.h"

#include <iterator>  // std::back_inserter
#include <map>       // std::map
#include <type_traits>  // std::is_convertible, std::is_same
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set

#include "gtest-extra.h"  // EXPECT_WRITE, EXPECT_THROW_MSG

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
    integers[static_cast<uint64_t>(fmt::to_underlying(values[i]))] = i;
    for (size_t j = 0; j < count; ++j) {
      auto lhs = fmt::to_underlying(values[i]);
      auto rhs = fmt::to_underlying(values[j]);
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
    EXPECT_EQ(fmt::to_underlying(entry.first), expected->first);
    EXPECT_EQ(entry.second, expected->second);
    ++expected;
  }
}

TEST(color_test, rgb_comparisons) {
  constexpr auto low = fmt::rgb{0, 255, 255};
  constexpr auto high = fmt::rgb{1, 0, 0};
  static_assert(low < high, "red precedes green and blue in packed ordering");
  static_assert(fmt::rgb{0xAB123456} == fmt::rgb{0x123456},
                "the discarded high byte must not affect equality");
  const fmt::rgb values[] = {
      {}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 0, 0},
      {0, 255, 255}, {255, 255, 255},
      fmt::rgb{0xAB123456}, fmt::rgb{0x123456}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, color_type_comparisons) {
  using fmt::detail::color_type;
  constexpr auto low = color_type{uint32_t{1} << 24};
  constexpr auto high = color_type{uint32_t{3} << 24};
  static_assert(color_type{} < low, "unspecified differs from RGB black");
  static_assert(low < high, "RGB black differs from terminal black");
  const color_type values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::terminal_color::black,
      fmt::rgb{0xFFFFFF}, fmt::terminal_color::bright_white,
      fmt::rgb{0x123456}, fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_comparisons) {
  constexpr auto low = fmt::text_style{};
  constexpr auto high = fmt::text_style{fmt::emphasis::bold};
  static_assert(low < high, "an empty style sorts before bold");
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::terminal_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::terminal_color::black), fmt::fg(fmt::rgb{0xFFFFFF}),
      fmt::emphasis::bold, fmt::emphasis::strikethrough,
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::terminal_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strikethrough};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

template<typename T>
void check_color_hashes(const T* values, size_t count) {
  using value_type = typename fmt::underlying_type<T>::type;
  static_assert(noexcept(std::hash<T>{}(T{})), "hashing must be noexcept");
  static_assert(std::is_same<decltype(std::hash<T>{}(T{})), size_t>::value,
                "hashing must return size_t");
  auto hasher = std::hash<T>();
  auto integer_hasher = std::hash<value_type>();
  auto keys = std::unordered_set<T>();
  auto entries = std::unordered_map<T, size_t>();
  auto integers = std::map<value_type, size_t>();
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(hasher(values[i]), integer_hasher(fmt::to_underlying(values[i])));
    keys.insert(values[i]);
    entries[values[i]] = i;
    integers[fmt::to_underlying(values[i])] = i;
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
    EXPECT_EQ(entries.at(values[i]), integers.at(fmt::to_underlying(values[i])));
  }
}

TEST(color_test, rgb_hash) {
  const fmt::rgb values[] = {
      {}, {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 255, 255},
      fmt::rgb{0xAB123456}, fmt::rgb{0x123456},
      fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, color_type_hash) {
  using fmt::detail::color_type;
  const color_type values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::terminal_color::black,
      fmt::rgb{0xFFFFFF}, fmt::terminal_color::bright_white,
      fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_hash) {
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::terminal_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::terminal_color::black), fmt::emphasis::bold,
      fmt::emphasis::strikethrough, fmt::fg(fmt::color::coral),
      fmt::fg(fmt::rgb{0xFF7F50}),
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::terminal_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strikethrough};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, underlying) {
  static_assert(std::is_same<fmt::underlying_type<fmt::terminal_color>::type,
                             uint8_t>::value, "terminal color underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::emphasis>::type,
                             uint8_t>::value, "emphasis underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::rgb>::type,
                             uint32_t>::value, "RGB underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::text_style>::type,
                             uint64_t>::value, "text style underlying type");

  static_assert(std::is_same<decltype(fmt::to_underlying(
                                 fmt::terminal_color::black)),
                             uint8_t>::value, "terminal color return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::emphasis::bold)),
                             uint8_t>::value, "emphasis return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::rgb{})),
                             uint32_t>::value, "RGB return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::text_style{})),
                             uint64_t>::value, "text style return type");

  static_assert(noexcept(fmt::to_underlying(fmt::terminal_color::black)),
                "terminal color conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::emphasis::bold)),
                "emphasis conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::rgb{})),
                "RGB conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::text_style{})),
                "text style conversion must be noexcept");

  static_assert(fmt::to_underlying(fmt::terminal_color::black) ==
                    static_cast<uint8_t>(fmt::terminal_color::black),
                "terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::terminal_color::bright_white) ==
                    static_cast<uint8_t>(fmt::terminal_color::bright_white),
                "bright terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::emphasis::bold) == 1,
                "bold must retain its bit");
  static_assert(fmt::to_underlying(fmt::emphasis::strikethrough) == 128,
                "strikethrough must retain its bit");
  static_assert(fmt::to_underlying(fmt::rgb{0xAB123456}) == 0x123456,
                "packed RGB must have a zero high byte");
  static_assert(fmt::to_underlying(fmt::rgb{}) == 0,
                "default RGB must be black");
  static_assert(fmt::to_underlying(fmt::text_style{}) == 0,
                "default text style must be empty");
  static_assert(fmt::to_underlying(
                    fmt::text_style{fmt::emphasis::strikethrough}) ==
                    (uint64_t{128} << 54),
                "text style conversion must preserve high emphasis bits");

  auto foreground = fmt::fg(fmt::rgb{0x123456});
  auto background = fmt::bg(fmt::terminal_color::black);
  auto attributes = fmt::text_style{fmt::emphasis::bold |
                                    fmt::emphasis::strikethrough};
  auto style = foreground | background | attributes;
  EXPECT_EQ(fmt::to_underlying(style),
            fmt::to_underlying(foreground) | fmt::to_underlying(background) |
                fmt::to_underlying(attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(background | attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(foreground | attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(foreground | background));
}

TEST(color_test, color_type_underlying) {
  using fmt::detail::color_type;
  static_assert(std::is_same<fmt::underlying_type<color_type>::type,
                             uint32_t>::value, "packed color underlying type");
  static_assert(std::is_same<decltype(fmt::to_underlying(color_type{})),
                             uint32_t>::value, "packed color return type");
  static_assert(noexcept(fmt::to_underlying(color_type{})),
                "packed color conversion must be noexcept");
  static_assert(fmt::to_underlying(color_type{}) == 0,
                "default packed color must be empty");

  // Check internal packing consistency without specifying a public encoding.
  const color_type colors[] = {
      color_type{}, color_type{fmt::rgb{0}},
      color_type{fmt::rgb{0x123456}},
      color_type{fmt::terminal_color::black}};
  for (auto color : colors)
    EXPECT_EQ(fmt::to_underlying(color), static_cast<uint32_t>(color));
}

TEST(color_test, color_type_composition) {
  using fmt::detail::color_type;
  // The reserved discriminator is invalid and is not a supported input.
  const color_type colors[] = {
      color_type{}, color_type{fmt::rgb{0}},
      color_type{fmt::terminal_color::black}};
  const bool allowed[3][3] = {
      {true, true, true},
      {true, true, false},
      {true, false, false}};

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      SCOPED_TRACE(fmt::format("color kinds {} and {}", i, j));
      auto lhs = colors[i];
      auto rhs = colors[j];
      if (allowed[i][j]) {
        auto result = lhs | rhs;
        EXPECT_EQ(static_cast<uint32_t>(result),
                  static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
        EXPECT_EQ(static_cast<uint32_t>(lhs),
                  static_cast<uint32_t>(colors[i]));
        EXPECT_EQ(&(lhs |= rhs), &lhs);
        EXPECT_EQ(static_cast<uint32_t>(lhs), static_cast<uint32_t>(result));
      } else {
        EXPECT_THROW((void) (lhs | rhs), fmt::format_error);
        EXPECT_THROW((void) (lhs |= rhs), fmt::format_error);
      }
      if (!allowed[i][j]) {
        EXPECT_EQ(static_cast<uint32_t>(lhs), static_cast<uint32_t>(colors[i]));
      }
      EXPECT_EQ(static_cast<uint32_t>(rhs), static_cast<uint32_t>(colors[j]));
    }
  }

  auto lhs = color_type{fmt::rgb{0xC0F000}};
  auto rhs = color_type{fmt::rgb{0x000FEE}};
  EXPECT_EQ(static_cast<uint32_t>(lhs | rhs), (uint32_t{1} << 24) | 0xC0FFEE);
  EXPECT_EQ(static_cast<uint32_t>(lhs), (uint32_t{1} << 24) | 0xC0F000);
  EXPECT_EQ(static_cast<uint32_t>(rhs), (uint32_t{1} << 24) | 0x000FEE);
}

TEST(color_test, rgb) {
  static_assert(!std::is_convertible<uint32_t, fmt::rgb>::value,
                "integer to rgb conversion must be explicit");
  static_assert(!std::is_convertible<fmt::rgb, uint32_t>::value,
                "rgb to integer conversion must be explicit");
  static_assert(std::is_same<decltype(fmt::color::red), const fmt::rgb>::value,
                "named colors must be rgb constants");
  constexpr auto c = fmt::rgb{0xAB123456};
  static_assert(c.r == 0x12 && c.g == 0x34 && c.b == 0x56,
                "the high byte must be ignored");
  static_assert(static_cast<uint32_t>(c) == 0x123456,
                "packed rgb must have a zero high byte");
  static_assert(static_cast<uint32_t>(fmt::color::alice_blue) == 0xF0F8FF,
                "named colors must preserve their values");
  auto components = fmt::rgb{0x12, 0x34, 0x56};
  EXPECT_EQ(static_cast<uint32_t>(components), 0x123456u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::rgb{}), 0u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::color::black), 0u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::color::white), 0xFFFFFFu);
}

TEST(color_test, rgb_blend) {
  static_assert(std::is_same<decltype(fmt::blend(fmt::rgb{})), fmt::rgb>::value,
                "blend must return an RGB value");
  static_assert(noexcept(fmt::blend(fmt::rgb{}, fmt::rgb{})),
                "blend must be noexcept");
  auto black = fmt::rgb{};
  auto white = fmt::rgb{255, 255, 255};
  EXPECT_EQ(fmt::blend(black), black);
  EXPECT_EQ(fmt::blend(white), white);
  EXPECT_EQ(fmt::blend(black, black, black), black);
  EXPECT_EQ(fmt::blend(white, white, white, white), white);
  EXPECT_EQ(fmt::blend(black, white), (fmt::rgb{127, 127, 127}));
  EXPECT_EQ(fmt::blend(black, white, white), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(fmt::blend(white, black, white), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(fmt::blend(white, white, black), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(black, fmt::rgb{});
  EXPECT_EQ(white, (fmt::rgb{255, 255, 255}));

  auto first  = fmt::rgb{0, 1, 254};
  auto second = fmt::rgb{0, 2, 255};
  auto third  = fmt::rgb{2, 4, 255};
  EXPECT_EQ(fmt::blend(first, second, third), (fmt::rgb{0, 2, 254}));
  EXPECT_EQ(fmt::blend(third, second, first), (fmt::rgb{0, 2, 254}));
  EXPECT_EQ(fmt::blend(fmt::color::red, fmt::color::green, fmt::color::blue),
            (fmt::rgb{85, 42, 85}));
#if FMT_USE_CONSTEXPR
  constexpr auto single  = fmt::blend(fmt::rgb{1, 2, 3});
  constexpr auto average = fmt::blend(fmt::rgb{255, 0, 0}, fmt::rgb{0, 255, 0},
                                     fmt::rgb{0, 0, 255});
  static_assert(single.r == 1 && single.g == 2 && single.b == 3,
                "one-argument blending must preserve all channels");
  static_assert(average.r == 85 && average.g == 85 && average.b == 85,
                "blend must support constant evaluation");
#endif
}

TEST(color_test, text_style_color_constructor) {
  using fmt::detail::color_type;
  static_assert(std::is_default_constructible<fmt::text_style>::value,
                "default construction must remain unambiguous");
  static_assert(std::is_convertible<fmt::emphasis, fmt::text_style>::value,
                "emphasis conversion must remain implicit");
  static_assert(std::is_convertible<color_type, fmt::text_style>::value,
                "the color constructor must not be explicit");
  static_assert(noexcept(fmt::text_style{fmt::color::red,
                                         fmt::terminal_color::black,
                                         fmt::emphasis::bold}),
                "color construction must be noexcept");
  static_assert(noexcept(fmt::text_style{fmt::emphasis::bold,
                                         fmt::color::red,
                                         fmt::terminal_color::black}),
                "emphasis-first construction must be noexcept");
  constexpr auto emphasis_only = fmt::text_style{fmt::emphasis::bold};
  static_assert(fmt::to_underlying(emphasis_only) == (uint64_t{1} << 54),
                "emphasis-only construction must remain constexpr");
  constexpr auto empty = fmt::text_style{fmt::no_color};
  static_assert(fmt::to_underlying(empty) == 0,
                "an unspecified foreground must produce an empty style");

  const color_type colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::rgb{0xFFFFFF},
      fmt::terminal_color::black, fmt::terminal_color::bright_white,
      fmt::color::coral};
  const fmt::emphasis attributes[] = {
      fmt::emphasis{}, fmt::emphasis::bold | fmt::emphasis::faint,
      fmt::emphasis::strikethrough, static_cast<fmt::emphasis>(0xFF)};
  for (auto foreground : colors) {
    fmt::text_style implicit = foreground;
    EXPECT_EQ(implicit, fmt::fg(foreground));
    for (auto emphasis : attributes) {
      fmt::text_style implicit_emphasis = emphasis;
      EXPECT_EQ(implicit_emphasis, fmt::text_style{emphasis});
      EXPECT_EQ((fmt::text_style{emphasis, foreground}),
                fmt::fg(foreground) | emphasis);
    }
    for (auto background : colors) {
      EXPECT_EQ((fmt::text_style{foreground, background}),
                fmt::fg(foreground) | fmt::bg(background));
      for (auto emphasis : attributes) {
        auto style = fmt::text_style{foreground, background, emphasis};
        EXPECT_EQ((fmt::text_style{emphasis, foreground, background}), style);
        EXPECT_EQ(style, fmt::fg(foreground) | fmt::bg(background) | emphasis);
        EXPECT_EQ(style.get_foreground(), foreground);
        EXPECT_EQ(style.get_background(), background);
        EXPECT_EQ(style.get_emphasis(), emphasis);
      }
    }
  }

  EXPECT_EQ(fmt::text_style{}, fmt::text_style{fmt::emphasis{}});
  EXPECT_EQ(fmt::text_style{fmt::color::red}, fmt::fg(fmt::color::red));
  EXPECT_TRUE(fmt::text_style{fmt::rgb{}}.has_foreground());
  EXPECT_TRUE(fmt::text_style{fmt::terminal_color::black}.has_foreground());
  fmt::text_style listed = {fmt::color::red, fmt::color::blue,
                           fmt::emphasis::bold};
  EXPECT_EQ(listed, fmt::fg(fmt::color::red) | fmt::bg(fmt::color::blue) |
                        fmt::emphasis::bold);
  fmt::text_style attributes_first = {fmt::emphasis::bold, fmt::color::red,
                                     fmt::color::blue};
  EXPECT_EQ(attributes_first, listed);
  EXPECT_EQ((fmt::text_style{fmt::emphasis::bold, fmt::no_color,
                             fmt::terminal_color::black}),
            fmt::bg(fmt::terminal_color::black) | fmt::emphasis::bold);
#if FMT_USE_CONSTEXPR
  constexpr auto style = fmt::text_style{fmt::color::coral,
                                         fmt::terminal_color::black,
                                         fmt::emphasis::bold};
  static_assert(fmt::to_underlying(style.get_foreground()) ==
                    ((uint32_t{1} << 24) | 0xFF7F50),
                "constant evaluation must preserve the RGB kind and payload");
  static_assert(fmt::to_underlying(style.get_background()) ==
                    (uint32_t{3} << 24),
                "constant evaluation must preserve specified terminal black");
  static_assert(style.get_emphasis() == fmt::emphasis::bold,
                "constant evaluation must preserve emphasis");
  constexpr auto constexpr_attributes_first = fmt::text_style{
      fmt::emphasis::bold, fmt::color::coral, fmt::terminal_color::black};
  static_assert(constexpr_attributes_first == style,
                "both constructor orders must agree in constant evaluation");
#endif
}

TEST(color_test, text_style) {
  EXPECT_FALSE(fmt::text_style().has_foreground());
  EXPECT_FALSE(fmt::text_style().has_background());
  EXPECT_FALSE(fmt::text_style().has_emphasis());

  EXPECT_TRUE(fg(fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(fg(fmt::rgb(0)).has_background());
  EXPECT_FALSE(fg(fmt::rgb(0)).has_emphasis());
  EXPECT_TRUE(bg(fmt::rgb(0)).has_background());
  EXPECT_FALSE(bg(fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(bg(fmt::rgb(0)).has_emphasis());

  EXPECT_TRUE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_foreground());
  EXPECT_TRUE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_background());
  EXPECT_FALSE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_emphasis());

  EXPECT_EQ(fg(fmt::rgb(0x000000)) | fg(fmt::rgb(0x000000)),
            fg(fmt::rgb(0x000000)));
  EXPECT_EQ(fg(fmt::rgb(0x00000F)) | fg(fmt::rgb(0x00000F)),
            fg(fmt::rgb(0x00000F)));
  EXPECT_EQ(fg(fmt::rgb(0xC0F000)) | fg(fmt::rgb(0x000FEE)),
            fg(fmt::rgb(0xC0FFEE)));

  EXPECT_THROW_MSG(
      (void) (fg(fmt::terminal_color::black) | fg(fmt::terminal_color::black)),
      fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      (void) (fg(fmt::terminal_color::black) | fg(fmt::terminal_color::white)),
      fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      (void) (bg(fmt::terminal_color::black) | bg(fmt::terminal_color::black)),
      fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      (void) (bg(fmt::terminal_color::black) | bg(fmt::terminal_color::white)),
      fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      (void) (fg(fmt::terminal_color::black) | fg(fmt::color::black)),
      fmt::format_error, "can't OR a terminal color");
  EXPECT_THROW_MSG(
      (void) (bg(fmt::terminal_color::black) | bg(fmt::color::black)),
      fmt::format_error, "can't OR a terminal color");

  EXPECT_NO_THROW((void) (fg(fmt::terminal_color::white) |
                          bg(fmt::terminal_color::white)));
  EXPECT_NO_THROW(
      (void) (fg(fmt::terminal_color::white) | bg(fmt::rgb(0xFFFFFF))));
  EXPECT_NO_THROW((void) (fg(fmt::terminal_color::white) | fmt::text_style()));
  EXPECT_NO_THROW((void) (bg(fmt::terminal_color::white) | fmt::text_style()));
}

TEST(color_test, terminal_color_boundaries) {
  using fmt::terminal_color;
  EXPECT_EQ(static_cast<int>(terminal_color::black), 0);
  EXPECT_EQ(static_cast<int>(terminal_color::white), 7);
  EXPECT_EQ(static_cast<int>(terminal_color::bright_black), 8);
  EXPECT_EQ(static_cast<int>(terminal_color::bright_white), 15);

  EXPECT_TRUE(fg(terminal_color::black).has_foreground());
  EXPECT_TRUE(bg(terminal_color::black).has_background());

  EXPECT_EQ(fmt::format(fg(terminal_color::black), "black"),
            "\x1b[30mblack\x1b[0m");
  EXPECT_EQ(fmt::format(bg(terminal_color::black), "black"),
            "\x1b[40mblack\x1b[0m");
  EXPECT_EQ(fmt::format(fg(terminal_color::white), "white"),
            "\x1b[37mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(bg(terminal_color::white), "white"),
            "\x1b[47mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(fg(terminal_color::bright_black), "bright black"),
            "\x1b[90mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(bg(terminal_color::bright_black), "bright black"),
            "\x1b[100mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(fg(terminal_color::bright_white), "bright white"),
            "\x1b[97mbright white\x1b[0m");
  EXPECT_EQ(fmt::format(bg(terminal_color::bright_white), "bright white"),
            "\x1b[107mbright white\x1b[0m");
}

TEST(color_test, format) {
  EXPECT_EQ(fmt::format(fmt::text_style(), "no style"), "no style");
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
            "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 0, 0)) | fg(fmt::rgb(0, 20, 30)),
                        "rgb(255,20,30)"),
            "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::rgb(0, 0, 0)) | fg(fmt::rgb(0, 0, 0)), "rgb(0,0,0)"),
      "\x1b[38;2;000;000;000mrgb(0,0,0)\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue"),
            "\x1b[38;2;000;000;255mblue\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | bg(fmt::color::red), "two color"),
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold"), "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::faint, "faint"), "\x1b[2mfaint\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::blink, "blink"), "\x1b[5mblink\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::reverse, "reverse"),
            "\x1b[7mreverse\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::conceal, "conceal"),
            "\x1b[8mconceal\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::strikethrough, "strikethrough"),
            "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue log"),
            "\x1b[38;2;000;000;255mblue log\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::text_style(), "hi"), "hi");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::bright_green), "tbgreen"),
            "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::bright_magenta), "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(fmt::format("{}{}", fmt::styled("red", fg(fmt::color::red)),
                        fmt::styled("bold", fmt::emphasis::bold)),
            "\x1b[38;2;255;000;000mred\x1b[0m\x1b[1mbold\x1b[0m");
  EXPECT_EQ(fmt::format("{}", fmt::styled("bar", fg(fmt::color::blue) |
                                                     fmt::emphasis::underline)),
            "\x1b[4m\x1b[38;2;000;000;255mbar\x1b[0m");
  EXPECT_EQ(
      fmt::format(
          "{}", fmt::styled(
                    "all", fmt::emphasis::bold | fmt::emphasis::faint |
                               fmt::emphasis::italic |
                               fmt::emphasis::underline | fmt::emphasis::blink |
                               fmt::emphasis::reverse | fmt::emphasis::conceal |
                               fmt::emphasis::strikethrough)),
      "\x1b[1;2;3;4;5;7;8;9mall\x1b[0m");
}

TEST(color_test, format_to) {
  auto out = std::string();
  fmt::format_to(std::back_inserter(out), fg(fmt::rgb(255, 20, 30)),
                 "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(fmt::to_string(out),
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

TEST(color_test, print) {
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
}

TEST(color_test, println) {
  EXPECT_WRITE(stdout,
               fmt::println(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m\n");
  EXPECT_WRITE(stdout, fmt::println(fmt::emphasis::bold, "bold"),
               "\x1b[1mbold\x1b[0m\n");
  EXPECT_WRITE(
      stdout,
      fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m\n");
}
