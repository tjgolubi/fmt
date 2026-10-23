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
#include <utility>  // std::declval, std::move

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
  using fmt::color_t;
  constexpr auto low = fmt::detail::unpack_color(uint32_t{1} << 24);
  constexpr auto high = fmt::detail::unpack_color(uint32_t{3} << 24);
  static_assert(color_t{} < low, "unspecified differs from RGB black");
  static_assert(low < high, "RGB black differs from terminal black");
  const color_t values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::term_color::black,
      fmt::rgb{0xFFFFFF}, fmt::term_color::bright_white,
      fmt::rgb{0x123456}, fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_comparisons) {
  constexpr auto low = fmt::text_style{};
  constexpr auto high = fmt::text_style{fmt::emphasis::bold};
  static_assert(low < high, "an empty style sorts before bold");
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::term_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::term_color::black), fmt::fg(fmt::rgb{0xFFFFFF}),
      fmt::emphasis::bold, fmt::emphasis::strike,
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::term_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strike};
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
  using fmt::color_t;
  const color_t values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::term_color::black,
      fmt::rgb{0xFFFFFF}, fmt::term_color::bright_white,
      fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_hash) {
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::term_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::term_color::black), fmt::emphasis::bold,
      fmt::emphasis::strike, fmt::fg(fmt::color::coral),
      fmt::fg(fmt::rgb{0xFF7F50}),
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::term_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strike};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, underlying) {
  static_assert(std::is_same<fmt::underlying_type<fmt::term_color>::type,
                             uint8_t>::value, "terminal color underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::emphasis>::type,
                             uint8_t>::value, "emphasis underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::rgb>::type,
                             uint32_t>::value, "RGB underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::text_style>::type,
                             uint64_t>::value, "text style underlying type");

  static_assert(std::is_same<decltype(fmt::to_underlying(
                                 fmt::term_color::black)),
                             uint8_t>::value, "terminal color return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::emphasis::bold)),
                             uint8_t>::value, "emphasis return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::rgb{})),
                             uint32_t>::value, "RGB return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::text_style{})),
                             uint64_t>::value, "text style return type");

  static_assert(noexcept(fmt::to_underlying(fmt::term_color::black)),
                "terminal color conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::emphasis::bold)),
                "emphasis conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::rgb{})),
                "RGB conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::text_style{})),
                "text style conversion must be noexcept");

  static_assert(fmt::to_underlying(fmt::term_color::black) ==
                    static_cast<uint8_t>(fmt::term_color::black),
                "terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::term_color::bright_white) ==
                    static_cast<uint8_t>(fmt::term_color::bright_white),
                "bright terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::emphasis::bold) == 1,
                "bold must retain its bit");
  static_assert(fmt::to_underlying(fmt::emphasis::strike) == 128,
                "strike must retain its bit");
  static_assert(fmt::to_underlying(fmt::rgb{0xAB123456}) == 0x123456,
                "packed RGB must have a zero high byte");
  static_assert(fmt::to_underlying(fmt::rgb{}) == 0,
                "default RGB must be black");
  static_assert(fmt::to_underlying(fmt::text_style{}) == 0,
                "default text style must be empty");
  static_assert(fmt::to_underlying(
                    fmt::text_style{fmt::emphasis::strike}) ==
                    (uint64_t{128} << 54),
                "text style conversion must preserve high emphasis bits");

  auto foreground = fmt::fg(fmt::rgb{0x123456});
  auto background = fmt::bg(fmt::term_color::black);
  auto attributes = fmt::text_style{fmt::emphasis::bold |
                                    fmt::emphasis::strike};
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
  using fmt::color_t;
  static_assert(std::is_same<fmt::underlying_type<color_t>::type,
                             uint32_t>::value, "packed color underlying type");
  static_assert(std::is_same<decltype(fmt::to_underlying(color_t{})),
                             uint32_t>::value, "packed color return type");
  static_assert(noexcept(fmt::to_underlying(color_t{})),
                "packed color conversion must be noexcept");
  static_assert(fmt::to_underlying(color_t{}) == 0,
                "default packed color must be empty");

  // Check the internal packed representation against independent expected values.
  const color_t colors[] = {
      color_t{}, color_t{fmt::rgb{0}},
      color_t{fmt::rgb{0x123456}},
      color_t{fmt::term_color::black}};
  const uint32_t expected[] = {
      0u, (uint32_t{1} << 24),
      (uint32_t{1} << 24) | 0x123456u, (uint32_t{3} << 24)};
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
    EXPECT_EQ(fmt::to_underlying(colors[i]), expected[i]);
}

TEST(color_test, color_type_composition) {
  using fmt::color_t;
  // The reserved discriminator is invalid and is not a supported input.
  const color_t colors[] = {
      color_t{}, color_t{fmt::rgb{0}},
      color_t{fmt::term_color::black}};
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
        EXPECT_EQ(fmt::to_underlying(result),
                  fmt::to_underlying(lhs) | fmt::to_underlying(rhs));
        EXPECT_EQ(fmt::to_underlying(lhs),
                  fmt::to_underlying(colors[i]));
        EXPECT_EQ(&(lhs |= rhs), &lhs);
        EXPECT_EQ(fmt::to_underlying(lhs), fmt::to_underlying(result));
      } else {
        EXPECT_THROW((void) (lhs | rhs), fmt::format_error);
        EXPECT_THROW((void) (lhs |= rhs), fmt::format_error);
      }
      if (!allowed[i][j]) {
        EXPECT_EQ(fmt::to_underlying(lhs), fmt::to_underlying(colors[i]));
      }
      EXPECT_EQ(fmt::to_underlying(rhs), fmt::to_underlying(colors[j]));
    }
  }

  auto lhs = color_t{fmt::rgb{0xC0F000}};
  auto rhs = color_t{fmt::rgb{0x000FEE}};
  EXPECT_EQ(fmt::to_underlying(lhs | rhs), (uint32_t{1} << 24) | 0xC0FFEE);
  EXPECT_EQ(fmt::to_underlying(lhs), (uint32_t{1} << 24) | 0xC0F000);
  EXPECT_EQ(fmt::to_underlying(rhs), (uint32_t{1} << 24) | 0x000FEE);
}

TEST(color_test, color_sentinels) {
  using fmt::no_color_t;
  using fmt::rsvd_color;
  static_assert(std::is_empty<no_color_t>::value &&
                    std::is_empty<rsvd_color>::value,
                "sentinels have no payload");
  static_assert(std::is_default_constructible<no_color_t>::value,
                "the unspecified sentinel must be constructible");
  static_assert(!std::is_default_constructible<rsvd_color>::value,
                "the reserved alternative must not be constructible");
  // GCC 4.9's standard library does not provide this trait.
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
  static_assert(std::is_trivially_copyable<rsvd_color>::value &&
                    std::is_trivially_copyable<fmt::color_t>::value,
                "alternative replacement relies on trivial union copying");
#endif
  static_assert(std::is_same<fmt::underlying_type<no_color_t>::type,
                             unsigned>::value &&
                    std::is_same<fmt::underlying_type<rsvd_color>::type,
                                 unsigned>::value,
                "sentinels have an unsigned numeric representation");
  static_assert(fmt::to_underlying(fmt::no_color) == 0,
                "the unspecified sentinel has zero payload");
  static_assert(noexcept(fmt::to_underlying(fmt::no_color)) &&
                    noexcept(fmt::to_underlying(std::declval<rsvd_color>())),
                "sentinel conversion must be noexcept");
  // Do not manufacture a reserved object just to test its numeric conversion.
  static_assert(std::is_same<decltype(fmt::to_underlying(
                                 std::declval<rsvd_color>())), unsigned>::value,
                "reserved conversion must be available to generic visitors");
}

namespace {
template<typename T, size_t Index>
void check_color_access(T value) {
  using fmt::color_t;
  static_assert(std::is_convertible<T, color_t>::value,
                "color construction must be implicit");
  static_assert(noexcept(color_t{value}) &&
                    noexcept(std::declval<color_t&>() = value),
                "construction and alternative assignment must be noexcept");
#define CHECK_COLOR_GET(Color, Result)                                      \
  static_assert(std::is_same<decltype(fmt::get<T>(std::declval<Color>())),    \
                             Result>::value &&                             \
                    std::is_same<decltype(fmt::get<Index>(                  \
                                     std::declval<Color>())),              \
                                 Result>::value,                           \
                "type and index access must preserve cv/ref qualifiers")
  CHECK_COLOR_GET(color_t&, T&);
  CHECK_COLOR_GET(const color_t&, const T&);
  CHECK_COLOR_GET(color_t&&, T&&);
  CHECK_COLOR_GET(const color_t&&, const T&&);
#undef CHECK_COLOR_GET
  static_assert(std::is_same<decltype(fmt::get_if<T>(
                                 std::declval<color_t*>())), T*>::value &&
                    std::is_same<decltype(fmt::get_if<Index>(
                                     std::declval<const color_t*>())),
                                 const T*>::value,
                "pointer access must preserve constness");

  color_t color = value;
  const color_t& constant = color;
  EXPECT_EQ(color.index(), Index);
  EXPECT_TRUE(fmt::holds_alternative<T>(constant));
  EXPECT_FALSE(fmt::holds_alternative<fmt::rsvd_color>(constant));
  EXPECT_EQ(&fmt::get<T>(color), &fmt::get<Index>(color));
  EXPECT_EQ(&fmt::get<T>(constant), &fmt::get<Index>(constant));
  EXPECT_EQ(fmt::get_if<T>(&color), &fmt::get<T>(color));
  EXPECT_EQ(fmt::get_if<Index>(&color), &fmt::get<T>(color));
  EXPECT_EQ(fmt::get_if<T>(&constant), &fmt::get<T>(constant));
  EXPECT_EQ(fmt::get_if<Index>(&constant), &fmt::get<T>(constant));
  EXPECT_EQ(fmt::get_if<T>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<T>(static_cast<const color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<Index>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<Index>(static_cast<const color_t*>(nullptr)), nullptr);
  static_assert(noexcept(fmt::holds_alternative<T>(constant)) &&
                    noexcept(fmt::get_if<T>(&color)) &&
                    noexcept(fmt::get_if<T>(&constant)) &&
                    noexcept(fmt::get_if<Index>(&color)) &&
                    noexcept(fmt::get_if<Index>(&constant)),
                "classification and pointer access must be noexcept");
  EXPECT_EQ(fmt::to_underlying(fmt::get<T>(std::move(color))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<Index>(std::move(color))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<T>(std::move(constant))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<Index>(std::move(constant))),
            fmt::to_underlying(value));

  const color_t alternatives[] = {fmt::no_color, fmt::rgb{},
                                 fmt::term_color::black};
  for (auto other : alternatives) {
    if (other.index() == Index) continue;
    const auto& const_other = other;
    EXPECT_FALSE(fmt::holds_alternative<T>(other));
    EXPECT_EQ(fmt::get_if<T>(&other), nullptr);
    EXPECT_EQ(fmt::get_if<T>(&const_other), nullptr);
    EXPECT_EQ(fmt::get_if<Index>(&other), nullptr);
    EXPECT_EQ(fmt::get_if<Index>(&const_other), nullptr);
    EXPECT_THROW_MSG((void)fmt::get<T>(other), fmt::format_error,
                     "wrong color_t alternative");
    EXPECT_THROW((void)fmt::get<T>(const_other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<T>(std::move(other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<T>(std::move(const_other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(const_other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(std::move(other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(std::move(const_other)), fmt::format_error);
  }
}

template<typename T>
void check_color_assignment(T replacement) {
  const fmt::color_t original[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                 fmt::term_color::bright_white};
  for (auto color : original) {
    const fmt::color_t expected = replacement;
    EXPECT_EQ(&(color = replacement), &color);
    EXPECT_EQ(color, expected);
    // Assignment from the active member must read it before replacing storage.
    EXPECT_EQ(&(color = fmt::get<T>(color)), &color);
    EXPECT_EQ(color, expected);
  }
}

struct color_kind_visitor {
#if FMT_MSC_VERSION >= 1929 && FMT_MSC_VERSION <= 1944 && !FMT_CLANG_VERSION
  // MSVC 19.29 and 19.44 reject copying the empty union member in constexpr.
  constexpr size_t operator()(const fmt::no_color_t&) const { return 0; }
#else
  constexpr size_t operator()(fmt::no_color_t) const { return 0; }
#endif
  constexpr size_t operator()(fmt::rgb) const { return 1; }
  constexpr size_t operator()(fmt::rsvd_color) const { return 2; }
  constexpr size_t operator()(fmt::term_color) const { return 3; }
};

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
  void operator()(fmt::no_color_t&) const { ++calls; }
  void operator()(fmt::rgb& value) const {
    ++calls;
    value.r = 42;
  }
  void operator()(fmt::rsvd_color&) const { ++calls; }
  void operator()(fmt::term_color& value) const {
    ++calls;
    value = fmt::term_color::bright_white;
  }
};

struct color_reference_visitor {
  int& value;
  template<typename T> int& operator()(T&&) const { return value; }
};

struct color_throwing_visitor {
  template<typename T> void operator()(T&&) const { throw 42; }
};

#if FMT_USE_CONSTEXPR
constexpr bool check_color_constexpr_assignment() {
  fmt::color_t color;
  color = fmt::rgb{1, 2, 3};
  fmt::get<fmt::rgb>(color).r = 4;
  fmt::get_if<1>(&color)->b = 5;
  bool rgb_ok = fmt::get<1>(color) == fmt::rgb{4, 2, 5};
  color = fmt::term_color::black;
  bool terminal_ok = fmt::holds_alternative<fmt::term_color>(color) &&
                     fmt::get<3>(color) == fmt::term_color::black;
  color = fmt::no_color;
  return rgb_ok && terminal_ok &&
         fmt::holds_alternative<fmt::no_color_t>(color);
}
#endif
}  // namespace

TEST(color_test, color_variant_access) {
  check_color_access<fmt::no_color_t, 0>(fmt::no_color);
  check_color_access<fmt::rgb, 1>(fmt::rgb{});
  check_color_access<fmt::rgb, 1>(fmt::color::coral);
  check_color_access<fmt::term_color, 3>(fmt::term_color::black);
  check_color_access<fmt::term_color, 3>(fmt::term_color::bright_white);
  auto color = fmt::color_t{fmt::rgb{1, 2, 3}};
  fmt::get<fmt::rgb>(color).r = 4;
  fmt::get<1>(color).g = 5;
  fmt::get_if<fmt::rgb>(&color)->b = 6;
  fmt::get_if<1>(&color)->r = 7;
  EXPECT_EQ(fmt::get<fmt::rgb>(color), (fmt::rgb{7, 5, 6}));
  EXPECT_EQ(fmt::get_if<2>(&color), nullptr);
  EXPECT_EQ(fmt::get_if<fmt::rsvd_color>(&color), nullptr);
  EXPECT_THROW((void)fmt::get<2>(color), fmt::format_error);
}

TEST(color_test, color_variant_assignment) {
  check_color_assignment(fmt::no_color);
  check_color_assignment(fmt::rgb{});
  check_color_assignment(fmt::color::coral);
  check_color_assignment(fmt::term_color::black);
  check_color_assignment(fmt::term_color::bright_white);
  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                fmt::term_color::black};
  for (const auto& source : colors) {
    fmt::color_t copy(source);
    fmt::color_t moved(std::move(copy));
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
  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                fmt::term_color::black};
  const fmt::color_t changed[] = {fmt::no_color, fmt::rgb{42, 2, 3},
                                fmt::term_color::bright_white};
  for (size_t i = 0; i < 3; ++i) {
    auto color = colors[i];
    const auto& constant = color;
    EXPECT_EQ(fmt::visit(color_kind_visitor{}, constant), color.index());
    color_category_visitor visitor;
    EXPECT_EQ(fmt::visit(visitor, color), 0u);
    EXPECT_EQ(fmt::visit(visitor, constant), 1u);
    EXPECT_EQ(fmt::visit(visitor, std::move(color)), 2u);
    EXPECT_EQ(fmt::visit(visitor, std::move(constant)), 3u);
    EXPECT_EQ(fmt::visit(std::move(visitor), color), 4u);
    EXPECT_EQ(fmt::visit(std::move(visitor), constant), 5u);
    EXPECT_EQ(fmt::visit(std::move(visitor), std::move(color)), 6u);
    EXPECT_EQ(fmt::visit(std::move(visitor), std::move(constant)), 7u);
    int calls = 0;
    fmt::visit(color_mutating_visitor{calls}, color);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(color, changed[i]);
    int result = 0;
    EXPECT_EQ(&fmt::visit(color_reference_visitor{result}, color), &result);
    fmt::visit(color_reference_visitor{result}, color) = 9;
    EXPECT_EQ(result, 9);
    EXPECT_THROW(fmt::visit(color_throwing_visitor{}, color), int);
    EXPECT_EQ(color, changed[i]);
  }
}

TEST(color_test, color_variant_constexpr) {
#if FMT_GCC_VERSION && FMT_GCC_VERSION < 500
  // GCC 4.9 requires static storage for the constexpr pointer checks below.
  static constexpr fmt::color_t empty{};
  static constexpr fmt::color_t rgb = fmt::rgb{1, 2, 3};
  static constexpr fmt::color_t terminal = fmt::term_color::black;
#else
  constexpr fmt::color_t empty;
  constexpr fmt::color_t rgb = fmt::rgb{1, 2, 3};
  constexpr fmt::color_t terminal = fmt::term_color::black;
#endif
  static_assert(empty.index() == 0 && fmt::holds_alternative<fmt::no_color_t>(empty),
                "default construction must hold the unspecified alternative");
  static_assert(rgb.index() == 1 && fmt::get<fmt::rgb>(rgb).g == 2 &&
                    fmt::get<1>(rgb).b == 3,
                "RGB type and index access must be constexpr in C++11");
  static_assert(terminal.index() == 3 &&
                    fmt::get<fmt::term_color>(terminal) == fmt::term_color::black,
                "terminal black must remain specified in constant evaluation");
  static_assert(fmt::get_if<1>(&rgb) == &fmt::get<fmt::rgb>(rgb) &&
                    fmt::get_if<fmt::term_color>(&rgb) == nullptr &&
                    fmt::get_if<0>(static_cast<const fmt::color_t*>(nullptr)) ==
                        nullptr,
                "pointer access must be constexpr in C++11");
  static_assert(fmt::visit(color_kind_visitor{}, empty) == 0 &&
                    fmt::visit(color_kind_visitor{}, rgb) == 1 &&
                    fmt::visit(color_kind_visitor{}, terminal) == 3,
                "visitation must be constexpr in C++11");
#if FMT_USE_CONSTEXPR
  static_assert(check_color_constexpr_assignment(),
                "replacement and mutable access must support constant evaluation");
#endif
}

TEST(color_test, color_variant_style_values) {
  static_assert(std::is_same<decltype(fmt::text_style{}.fg()),
                             fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.bg()),
                                 fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.get_foreground()),
                                 fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.get_background()),
                                 fmt::color_t>::value,
                "short and compatibility getters must return color values");
  auto foreground = fmt::color_t{fmt::rgb{1, 2, 3}};
  auto background = fmt::color_t{fmt::term_color::black};
  const auto style =
      fmt::text_style{foreground, background, fmt::emphasis::strike};
  fmt::get<1>(foreground).r = 255;
  fmt::get<3>(background) = fmt::term_color::white;
  EXPECT_EQ(fmt::get<fmt::rgb>(style.fg()), (fmt::rgb{1, 2, 3}));
  EXPECT_EQ(fmt::get<fmt::term_color>(style.bg()), fmt::term_color::black);
  auto extracted = style.fg();
  fmt::get<1>(extracted).b = 255;
  EXPECT_EQ(fmt::get<1>(style.fg()).b, 3);
  // Copying an alternative from a temporary color is safe.
  auto copy = fmt::get<fmt::rgb>(style.fg());
  copy.g = 255;
  EXPECT_EQ(copy, (fmt::rgb{1, 255, 3}));
  EXPECT_EQ(fmt::get<1>(style.fg()).g, 2);
  EXPECT_EQ(fmt::format(style, "value"),
            "\x1b[9m\x1b[38;2;001;002;003m\x1b[40mvalue\x1b[0m");
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
  using fmt::color_t;
  static_assert(std::is_default_constructible<fmt::text_style>::value,
                "default construction must remain unambiguous");
  static_assert(std::is_convertible<fmt::emphasis, fmt::text_style>::value,
                "emphasis conversion must remain implicit");
  static_assert(std::is_convertible<color_t, fmt::text_style>::value,
                "the color constructor must not be explicit");
  static_assert(noexcept(fmt::text_style{fmt::color::red,
                                         fmt::term_color::black,
                                         fmt::emphasis::bold}),
                "color construction must be noexcept");
  static_assert(noexcept(fmt::text_style{fmt::emphasis::bold,
                                         fmt::color::red,
                                         fmt::term_color::black}),
                "emphasis-first construction must be noexcept");
  constexpr auto emphasis_only = fmt::text_style{fmt::emphasis::bold};
  static_assert(fmt::to_underlying(emphasis_only) == (uint64_t{1} << 54),
                "emphasis-only construction must remain constexpr");
  constexpr auto empty = fmt::text_style{fmt::no_color};
  static_assert(fmt::to_underlying(empty) == 0,
                "an unspecified foreground must produce an empty style");

  const color_t colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::rgb{0xFFFFFF},
      fmt::term_color::black, fmt::term_color::bright_white,
      fmt::color::coral};
  const fmt::emphasis attributes[] = {
      fmt::emphasis{}, fmt::emphasis::bold | fmt::emphasis::faint,
      fmt::emphasis::strike, static_cast<fmt::emphasis>(0xFF)};
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
  EXPECT_TRUE(fmt::text_style{fmt::term_color::black}.has_foreground());
  fmt::text_style listed = {fmt::color::red, fmt::color::blue,
                           fmt::emphasis::bold};
  EXPECT_EQ(listed, fmt::fg(fmt::color::red) | fmt::bg(fmt::color::blue) |
                        fmt::emphasis::bold);
  fmt::text_style attributes_first = {fmt::emphasis::bold, fmt::color::red,
                                     fmt::color::blue};
  EXPECT_EQ(attributes_first, listed);
  EXPECT_EQ((fmt::text_style{fmt::emphasis::bold, fmt::no_color,
                             fmt::term_color::black}),
            fmt::bg(fmt::term_color::black) | fmt::emphasis::bold);
#if FMT_USE_CONSTEXPR
  constexpr auto style = fmt::text_style{fmt::color::coral,
                                         fmt::term_color::black,
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
      fmt::emphasis::bold, fmt::color::coral, fmt::term_color::black};
  static_assert(constexpr_attributes_first == style,
                "both constructor orders must agree in constant evaluation");
#endif
}

void check_fluent_emphasis(fmt::emphasis attribute,
                          fmt::text_style& (fmt::text_style::*set)(bool),
                          bool (fmt::text_style::*get)() const) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = fmt::text_style{static_cast<fmt::emphasis>(bits),
                                    fmt::color::coral,
                                    fmt::term_color::black};
    auto mask = static_cast<unsigned>(attribute);
    EXPECT_EQ((original.*get)(), (bits & mask) != 0);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      auto style = original;
      auto expected = fmt::text_style{
          static_cast<fmt::emphasis>(enabled ? bits | mask : bits & ~mask),
          fmt::color::coral, fmt::term_color::black};
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);
      EXPECT_EQ((style.*get)(), enabled);
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);  // Repeating a setter is idempotent.
    }
  }
}

TEST(color_test, fluent_emphasis) {
  using fmt::text_style;
#define CHECK_EMPHASIS(name)                                                 \
  static_assert(noexcept(text_style{}.name()), "setter must be noexcept");   \
  static_assert(noexcept(text_style{}.is_##name()),                          \
                "getter must be noexcept");                                \
  static_assert(!text_style{}.is_##name(), "empty getter must be constexpr"); \
  check_fluent_emphasis(fmt::emphasis::name, &text_style::name,                \
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
  EXPECT_EQ(style.emph(), static_cast<fmt::emphasis>(0xFF));
  EXPECT_EQ(&style.bold(false).faint(false).italic(false).underline(false)
                 .blink(false).reverse(false).conceal(false)
                 .strike(false), &style);
  EXPECT_EQ(style, text_style{});

  style.bold().faint();
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  EXPECT_EQ(fmt::format(style, "both"), "\x1b[1;2mboth\x1b[0m");
  style.bold(false);
  EXPECT_FALSE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  style.bold().faint(false);
  EXPECT_TRUE(style.is_bold());
  EXPECT_FALSE(style.is_faint());
}

TEST(color_test, fluent_components) {
  using fmt::color_t;
  using fmt::emphasis;
  using fmt::text_style;
  constexpr auto empty = text_style{};
  static_assert(fmt::to_underlying(empty.fg()) == 0,
                "empty foreground getter must be constexpr");
  static_assert(fmt::to_underlying(empty.bg()) == 0,
                "empty background getter must be constexpr");
  static_assert(empty.emph() == emphasis{},
                "empty emphasis getter must be constexpr");
  constexpr auto filled = text_style{emphasis::strike};
  static_assert(filled.emph() == emphasis::strike &&
                    filled.get_emphasis() == emphasis::strike,
                "both emphasis getters must be constexpr in C++11");
  static_assert(noexcept(empty.fg()) && noexcept(empty.bg()) &&
                    noexcept(empty.emph()), "getters must be noexcept");
  static_assert(noexcept(text_style{}.fg(fmt::no_color)) &&
                    noexcept(text_style{}.bg(fmt::no_color)) &&
                    noexcept(text_style{}.emph(emphasis{})),
                "setters must be noexcept");

  const color_t colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::color::coral,
      fmt::term_color::black, fmt::term_color::bright_white};
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
  EXPECT_EQ(&style.fg(fmt::color::coral).bg(fmt::term_color::black)
                 .emph(emphasis::italic).underline(), &style);
  EXPECT_EQ(style, (text_style{emphasis::italic | emphasis::underline,
                               fmt::color::coral, fmt::term_color::black}));
  EXPECT_EQ(&style.fg(fmt::no_color).bg(fmt::no_color).emph(emphasis{}), &style);
  EXPECT_EQ(style, empty);
  EXPECT_EQ(fmt::format(style, "plain"), "plain");
}

TEST(color_test, fluent_presence) {
  using fmt::emphasis;
  using fmt::text_style;
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
    fmt::color_t value;
    bool specified;
  } colors[] = {{fmt::no_color, false}, {fmt::rgb{}, true},
                {fmt::term_color::black, true}, {fmt::color::coral, true},
                {fmt::term_color::bright_white, true}};
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

  auto style = text_style{emphasis::bold, fmt::rgb{}, fmt::term_color::black};
  style.fg(fmt::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bg(fmt::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bold(false);
  EXPECT_FALSE(style.has_emph());
  style.emph(emphasis::faint).fg(fmt::term_color::black).bg(fmt::rgb{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.emph(emphasis{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
}

#if FMT_USE_CONSTEXPR
TEST(color_test, fluent_constexpr) {
  constexpr auto all = fmt::text_style{}.bold().faint().italic().underline()
                          .blink().reverse().conceal().strike();
  static_assert(all.is_bold() && all.is_faint() && all.is_italic() &&
                    all.is_underline() && all.is_blink() && all.is_reverse() &&
                    all.is_conceal() && all.is_strike(),
                "all emphasis setters must support constant evaluation");
  constexpr auto cleared = fmt::text_style{all}.bold(false).faint(false)
                              .italic(false).underline(false).blink(false)
                              .reverse(false).conceal(false).strike(false);
  static_assert(!cleared.has_emphasis(),
                "disabling emphasis must support constant evaluation");
  constexpr auto replaced = fmt::text_style{all}.fg(fmt::color::coral)
                               .bg(fmt::term_color::black)
                               .emph(fmt::emphasis::underline);
  static_assert(replaced == fmt::text_style{fmt::emphasis::underline,
                                             fmt::color::coral,
                                             fmt::term_color::black},
                "component replacement must support constant evaluation");
  static_assert(replaced.has_fg() && replaced.has_bg() && replaced.has_emph(),
                "presence queries must detect constexpr component replacement");
  constexpr auto empty = fmt::text_style{replaced}.fg(fmt::no_color)
                            .bg(fmt::no_color).emph(fmt::emphasis{});
  static_assert(empty == fmt::text_style{},
                "clearing components must support constant evaluation");
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "presence queries must detect constexpr component clearing");
}
#endif

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

TEST(color_test, short_names_compatibility) {
  using fmt::emphasis;
  using fmt::term_color;
  using fmt::text_style;
  static_assert(std::is_same<term_color, fmt::terminal_color>::value,
                "terminal_color must remain an alias");
  static_assert(emphasis::strike == emphasis::strikethrough,
                "both names must select the same emphasis bit");
  static_assert(text_style{emphasis::strike}.is_strikethrough() &&
                    text_style{emphasis::strikethrough}.is_strike(),
                "both queries must recognize either emphasis name");

  for (unsigned i = 0; i < 16; ++i) {
    const auto color = static_cast<term_color>(i);
    const auto legacy = static_cast<fmt::terminal_color>(i);
    const auto stored = fmt::color_t{color};
    EXPECT_EQ(fmt::get<fmt::terminal_color>(stored), color);
    EXPECT_TRUE(fmt::holds_alternative<fmt::terminal_color>(stored));
    EXPECT_EQ(fmt::get<term_color>(
                  fmt::color_t{legacy}), color);
    EXPECT_EQ(fmt::format(fg(color), "fg"), fmt::format(fg(legacy), "fg"));
    EXPECT_EQ(fmt::format(bg(color), "bg"), fmt::format(bg(legacy), "bg"));
  }

  auto style = text_style{emphasis::bold, term_color::red,
                         fmt::terminal_color::black};
  EXPECT_EQ(&style.strike(), &style);
  EXPECT_TRUE(style.is_strikethrough());
  EXPECT_EQ(fmt::format(style, "mixed"),
            "\x1b[1;9m\x1b[31m\x1b[40mmixed\x1b[0m");
  EXPECT_EQ(&style.strikethrough(false), &style);
  EXPECT_FALSE(style.is_strike());
  style.strikethrough().strike(false);
  EXPECT_FALSE(style.is_strikethrough());
  EXPECT_EQ(style, (text_style{emphasis::bold, term_color::red,
                              fmt::terminal_color::black}));
  EXPECT_EQ(fmt::format(emphasis::strike, "strike"),
            "\x1b[9mstrike\x1b[0m");
#if FMT_USE_CONSTEXPR
  constexpr auto cleared = text_style{}.strike().strikethrough(false);
  static_assert(!cleared.is_strike() && !cleared.is_strikethrough(),
                "mixed setters must support constant evaluation");
#endif
}

TEST(color_test, term_color_boundaries) {
  using fmt::term_color;
  EXPECT_EQ(static_cast<int>(term_color::black), 0);
  EXPECT_EQ(static_cast<int>(term_color::white), 7);
  EXPECT_EQ(static_cast<int>(term_color::bright_black), 8);
  EXPECT_EQ(static_cast<int>(term_color::bright_white), 15);

  EXPECT_TRUE(fg(term_color::black).has_foreground());
  EXPECT_TRUE(bg(term_color::black).has_background());

  EXPECT_EQ(fmt::format(fg(term_color::black), "black"),
            "\x1b[30mblack\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::black), "black"),
            "\x1b[40mblack\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::white), "white"),
            "\x1b[37mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::white), "white"),
            "\x1b[47mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::bright_black), "bright black"),
            "\x1b[90mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::bright_black), "bright black"),
            "\x1b[100mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::bright_white), "bright white"),
            "\x1b[97mbright white\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::bright_white), "bright white"),
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
