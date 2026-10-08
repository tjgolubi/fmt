// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information see LICENSE.

#include "fmt/color.h"

#include <iterator>  // std::back_inserter

#include <stdexcept>
#include <iostream>
#define TEST(suite, name) void suite##_##name()
#define EXPECT_TRUE(x) do { if (!(x)) throw std::runtime_error(#x); } while (false)
#define EXPECT_FALSE(x) EXPECT_TRUE(!(x))
#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))
#define EXPECT_NO_THROW(x) do { (void)(x); } while (false)
#define EXPECT_THROW_MSG(x, exception, message) do { \
  bool caught = false; \
  try { (void)(x); } catch (const exception& error) { \
    caught = true; EXPECT_EQ(std::string(error.what()), std::string(message)); \
  } \
  EXPECT_TRUE(caught); \
} while (false)


constexpr auto fluent_attributes() {
  return fmt::text_style{}.faint().bold().blink().flash()
      .italic().underline().inverse().strike();
}
constexpr auto composed_style = fmt::fg(fmt::terminal_color::black) |
    fmt::bg(fmt::color::blue) | fmt::emphasis::bold | fmt::emphasis::flash;
static_assert(composed_style.has_foreground() && composed_style.has_background());
static_assert(composed_style.is_bold() && composed_style.is_flash());
static_assert(sizeof(fmt::text_style) == sizeof(std::uint64_t));
static_assert(fluent_attributes().is_bold() && !fluent_attributes().is_faint());
static_assert(fluent_attributes().is_flash() && !fluent_attributes().is_blink());
static_assert(fluent_attributes().is_italic() && fluent_attributes().is_underline());
static_assert(fluent_attributes().is_inverse() && fluent_attributes().is_strike());
static_assert((fmt::emphasis::faint | fmt::emphasis::bold) ==
              fmt::text_style{}.bold());
static_assert((fmt::emphasis::blink | fmt::emphasis::flash) ==
              fmt::text_style{}.flash());

TEST(color_test, text_style) {
  auto style = fmt::text_style{};
  EXPECT_FALSE(style.has_foreground());
  EXPECT_FALSE(style.has_background());
  EXPECT_FALSE(style.has_emphasis());
  EXPECT_TRUE(&style.bold().italic() == &style);
  EXPECT_TRUE(style.is_bold());
  style.faint();
  EXPECT_TRUE(style.is_faint());
  EXPECT_FALSE(style.is_bold());
  style.bold(false);
  EXPECT_TRUE(style.is_faint()); // Disabling an inactive mode leaves the other mode alone.
  style.faint(false);
  EXPECT_FALSE(style.is_faint());
  style.bold().bold(false);
  EXPECT_FALSE(style.is_bold());
  style.flash().blink();
  EXPECT_TRUE(style.is_blink());
  EXPECT_FALSE(style.is_flash());
  style.flash(false);
  EXPECT_TRUE(style.is_blink());
  style.blink(false);
  EXPECT_FALSE(style.is_blink());
  style.flash().flash(false);
  EXPECT_FALSE(style.is_flash());
  style.italic(false).underline().underline(false)
      .reverse().reverse(false).strikethrough().strikethrough(false);
  EXPECT_FALSE(style.has_emphasis());
  EXPECT_FALSE(style.is_italic());
  EXPECT_FALSE(style.is_underline());
  EXPECT_FALSE(style.is_reverse());
  EXPECT_FALSE(style.is_strikethrough());
  style.inverse().strike();
  EXPECT_TRUE(style.is_reverse() && style.is_inverse());
  EXPECT_TRUE(style.is_strike() && style.is_strikethrough());

  style = fmt::text_style{}.fg(fmt::terminal_color::black)
      .bg(fmt::color::blue).bold().flash();
  EXPECT_TRUE(style.has_foreground() && style.has_background());
  EXPECT_EQ(style.get_foreground().value(), 0u);
  EXPECT_TRUE(style.get_foreground().is_terminal_color());
  style.fg(fmt::rgb(0x123456)).bg(fmt::terminal_color::bright_white);
  EXPECT_FALSE(style.get_foreground().is_terminal_color());
  EXPECT_EQ(style.get_foreground().value(), 0x123456u);
  EXPECT_TRUE(style.get_background().is_terminal_color());
  EXPECT_EQ(style.get_background().value(), 15u);
  EXPECT_TRUE(style.is_bold() && style.is_flash());
  style.fg(fmt::terminal_color::red).bg(fmt::rgb(0xABCDEF));
  EXPECT_TRUE(style.get_foreground().is_terminal_color());
  EXPECT_EQ(style.get_foreground().value(), 1u);
  EXPECT_FALSE(style.get_background().is_terminal_color());
  EXPECT_EQ(style.get_background().value(), 0xABCDEFu);

  const fmt::text_style foregrounds[] = {
    fmt::fg(fmt::terminal_color::black), fmt::fg(fmt::color::black),
    fmt::fg(fmt::rgb(0x123456))
  };
  const fmt::text_style backgrounds[] = {
    fmt::bg(fmt::terminal_color::black), fmt::bg(fmt::color::black),
    fmt::bg(fmt::rgb(0xABCDEF))
  };
  for (const auto& foreground : foregrounds) {
    for (const auto& background : backgrounds) {
      const auto combined = foreground | background;
      EXPECT_EQ(combined, background | foreground);
      EXPECT_EQ(combined.get_foreground().value_, foreground.get_foreground().value_);
      EXPECT_EQ(combined.get_background().value_, background.get_background().value_);
      EXPECT_EQ(combined | fmt::emphasis::bold, fmt::text_style(combined).bold());
      EXPECT_EQ(fmt::emphasis::bold | combined, fmt::text_style(combined).bold());
      EXPECT_EQ(combined | fmt::text_style{}, combined);
      EXPECT_EQ(fmt::text_style{} | combined, combined);
      auto lhs = foreground;
      lhs |= background;
      EXPECT_EQ(lhs, combined);
      EXPECT_THROW_MSG(lhs |= foreground, std::format_error, "can't OR two foreground colors");
      EXPECT_EQ(lhs, combined);
      EXPECT_THROW_MSG(lhs |= background, std::format_error, "can't OR two background colors");
      EXPECT_EQ(lhs, combined);
    }
    for (const auto& other : foregrounds) {
      EXPECT_THROW_MSG(foreground | other, std::format_error, "can't OR two foreground colors");
      auto lhs = foreground;
      // Reject a duplicate foreground before merging rhs's new background/attributes.
      const auto rhs = fmt::text_style(other).bg(fmt::color::blue).flash();
      EXPECT_THROW_MSG(lhs |= rhs, std::format_error, "can't OR two foreground colors");
      EXPECT_EQ(lhs, foreground);
    }
  }
  for (const auto& background : backgrounds) {
    for (const auto& other : backgrounds) {
      EXPECT_THROW_MSG(background | other, std::format_error, "can't OR two background colors");
      auto lhs = background;
      const auto rhs = fmt::text_style(other).fg(fmt::color::red).bold();
      EXPECT_THROW_MSG(lhs |= rhs, std::format_error, "can't OR two background colors");
      EXPECT_EQ(lhs, background);
    }
  }
  EXPECT_EQ(fmt::fg(fmt::color::red) | fmt::bg(fmt::color::blue),
            fmt::fg(fmt::color::red).bg(fmt::color::blue));
  EXPECT_EQ(fmt::text_style{}.bold() | fmt::text_style{}.faint(), fmt::text_style{}.bold());
  EXPECT_EQ(fmt::text_style{}.faint() | fmt::text_style{}.bold(), fmt::text_style{}.bold());
  EXPECT_EQ(fmt::text_style{}.blink() | fmt::text_style{}.flash(), fmt::text_style{}.flash());
  EXPECT_EQ(fmt::text_style{}.flash() | fmt::text_style{}.blink(), fmt::text_style{}.flash());
  EXPECT_EQ(fmt::text_style{}.bold() | fmt::emphasis::italic,
            fmt::text_style{}.bold().italic());
}

TEST(color_test, format) {
  EXPECT_EQ(fmt::format(fmt::text_style{}, "x"), "x");
  EXPECT_EQ(fmt::format(fmt::fg(fmt::rgb(255, 20, 30)), "x"),
            "\x1b[38;2;255;020;030mx\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::fg(fmt::color::blue) | fmt::bg(fmt::color::red), "x"),
            "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mx\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::fg(fmt::terminal_color::black).bg(fmt::terminal_color::white), "x"),
            "\x1b[30m\x1b[47mx\x1b[0m");
  const struct { fmt::emphasis attribute; const char* expected; } cases[] = {
    {fmt::emphasis::bold, "\x1b[1mx\x1b[0m"},
    {fmt::emphasis::faint, "\x1b[2mx\x1b[0m"},
    {fmt::emphasis::italic, "\x1b[3mx\x1b[0m"},
    {fmt::emphasis::underline, "\x1b[4mx\x1b[0m"},
    {fmt::emphasis::blink, "\x1b[5mx\x1b[0m"},
    {fmt::emphasis::flash, "\x1b[6mx\x1b[0m"},
    {fmt::emphasis::reverse, "\x1b[7mx\x1b[0m"},
    {fmt::emphasis::strikethrough, "\x1b[9mx\x1b[0m"},
  };
  for (const auto& entry : cases) {
    EXPECT_EQ(fmt::format(entry.attribute, "x"), entry.expected);
    EXPECT_EQ(std::format("{}", fmt::styled("x", entry.attribute)), entry.expected);
  }
  EXPECT_EQ(fmt::format(fluent_attributes(), "x"), "\x1b[1;3;4;6;7;9mx\x1b[0m");
  EXPECT_EQ(std::format(L"{}", fmt::styled(L"x", fluent_attributes())),
            L"\x1b[1;3;4;6;7;9mx\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::faint | fmt::emphasis::bold, "x"),
            "\x1b[1mx\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::blink | fmt::emphasis::flash, "x"),
            "\x1b[6mx\x1b[0m");
  EXPECT_EQ(std::format("{}{}", fmt::styled("red", fmt::fg(fmt::color::red)),
                       fmt::styled("bold", fmt::emphasis::bold)),
            "\x1b[38;2;255;000;000mred\x1b[0m\x1b[1mbold\x1b[0m");
}

TEST(color_test, terminal_palette) {
  struct palette_case {
    fmt::terminal_color color;
    const char* foreground;
    const char* background;
  };
  constexpr palette_case cases[] = {
    {fmt::terminal_color::black, "\x1b[30mx\x1b[0m", "\x1b[40mx\x1b[0m"},
    {fmt::terminal_color::red, "\x1b[31mx\x1b[0m", "\x1b[41mx\x1b[0m"},
    {fmt::terminal_color::green, "\x1b[32mx\x1b[0m", "\x1b[42mx\x1b[0m"},
    {fmt::terminal_color::yellow, "\x1b[33mx\x1b[0m", "\x1b[43mx\x1b[0m"},
    {fmt::terminal_color::blue, "\x1b[34mx\x1b[0m", "\x1b[44mx\x1b[0m"},
    {fmt::terminal_color::magenta, "\x1b[35mx\x1b[0m", "\x1b[45mx\x1b[0m"},
    {fmt::terminal_color::cyan, "\x1b[36mx\x1b[0m", "\x1b[46mx\x1b[0m"},
    {fmt::terminal_color::white, "\x1b[37mx\x1b[0m", "\x1b[47mx\x1b[0m"},
    {fmt::terminal_color::bright_black, "\x1b[90mx\x1b[0m", "\x1b[100mx\x1b[0m"},
    {fmt::terminal_color::bright_red, "\x1b[91mx\x1b[0m", "\x1b[101mx\x1b[0m"},
    {fmt::terminal_color::bright_green, "\x1b[92mx\x1b[0m", "\x1b[102mx\x1b[0m"},
    {fmt::terminal_color::bright_yellow, "\x1b[93mx\x1b[0m", "\x1b[103mx\x1b[0m"},
    {fmt::terminal_color::bright_blue, "\x1b[94mx\x1b[0m", "\x1b[104mx\x1b[0m"},
    {fmt::terminal_color::bright_magenta, "\x1b[95mx\x1b[0m", "\x1b[105mx\x1b[0m"},
    {fmt::terminal_color::bright_cyan, "\x1b[96mx\x1b[0m", "\x1b[106mx\x1b[0m"},
    {fmt::terminal_color::bright_white, "\x1b[97mx\x1b[0m", "\x1b[107mx\x1b[0m"},
  };
  for (std::size_t index = 0; index != std::size(cases); ++index) {
    const auto& entry = cases[index];
    EXPECT_EQ(std::to_underlying(entry.color), index);
    const auto foreground = fmt::fg(entry.color);
    const auto background = fmt::bg(entry.color);
    const auto combined = fmt::fg(entry.color).bg(entry.color);
    EXPECT_TRUE(combined.has_foreground());
    EXPECT_TRUE(combined.has_background());
    EXPECT_TRUE(combined.get_foreground().is_terminal_color());
    EXPECT_TRUE(combined.get_background().is_terminal_color());
    EXPECT_EQ(combined.get_foreground().value(), index);
    EXPECT_EQ(combined.get_background().value(), index);
    EXPECT_EQ(fmt::format(foreground, "x"), entry.foreground);
    EXPECT_EQ(fmt::format(background, "x"), entry.background);
    EXPECT_EQ(std::format("{}", fmt::styled("x", foreground)), entry.foreground);
    EXPECT_EQ(std::format("{}", fmt::styled("x", background)), entry.background);
    EXPECT_EQ(std::format(L"{}", fmt::styled(L"x", foreground)),
              std::wstring(entry.foreground, entry.foreground + std::char_traits<char>::length(entry.foreground)));
    EXPECT_EQ(std::format(L"{}", fmt::styled(L"x", background)),
              std::wstring(entry.background, entry.background + std::char_traits<char>::length(entry.background)));
  }
}

TEST(color_test, format_to) {
  auto out = std::string();
  fmt::format_to(std::back_inserter(out), fg(fmt::rgb(255, 20, 30)),
                 "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(out,
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

int main() {
  try {
    color_test_text_style();
    color_test_format();
    color_test_format_to();
    color_test_terminal_palette();
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
