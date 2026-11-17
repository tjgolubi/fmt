#include "tjg/color.hpp"
#include "standard-format-test.h"
#include "test-support.h"

#include <sstream>
#include <iomanip>

namespace {
template <typename T>
void check_output(const T& value, const std::string& expected,
                  const std::string& debug) {
  EXPECT_EQ(std::format("{}", value), expected);
  EXPECT_EQ(std::format("{:?}", value), debug);
  const auto wide = std::wstring(expected.begin(), expected.end());
  EXPECT_EQ(std::format(L"{}", value), wide);
  EXPECT_EQ(std::format(L"{:?}", value), (std::wstring{debug.begin(), debug.end()}));
  std::ostringstream os;
  os << value;
  EXPECT_EQ(os.str(), expected);
  std::wostringstream wos;
  wos << value;
  EXPECT_EQ(wos.str(), wide);
}
template <typename T>
void check_output(const T& value, const std::string& expected) {
  check_output(value, expected, expected);
}

void check_style(const tjg::color::text_style& value, const std::string& debug) {
  const auto sgr = tjg::color::to_sgr(value);
  EXPECT_EQ(std::format("{}", value), sgr);
  EXPECT_EQ(std::format(L"{}", value), (std::wstring{sgr.begin(), sgr.end()}));
  EXPECT_EQ(std::format("{:?}", value), debug);
  EXPECT_EQ(std::format(L"{:?}", value), (std::wstring{debug.begin(), debug.end()}));
}
}  // namespace

TEST(color_debug_test, color_notation) {
  check_output(tjg::color::no_color, "no_color");
  check_output(tjg::color::color_t{}, "no_color");
  const char* names[] = {"black",        "red",
                         "green",        "yellow",
                         "blue",         "magenta",
                         "cyan",         "white",
                         "bright_black", "bright_red",
                         "bright_green", "bright_yellow",
                         "bright_blue",  "bright_magenta",
                         "bright_cyan",  "bright_white"};
  for (int i = 0; i < 16; ++i) {
    const auto color = static_cast<tjg::color::term_color>(i);
    check_output(color, names[i]);
    check_output(tjg::color::color_t{color}, names[i]);
  }
  for (int i = 0; i < 24; ++i) {
    const auto color = tjg::color::gray_color{i};
    const auto expected = std::format("gray{}", i);
    check_output(color, expected);
    check_output(tjg::color::color_t{color}, expected);
  }
  for (int r = 0; r < 6; ++r) {
    for (int g = 0; g < 6; ++g) {
      for (int b = 0; b < 6; ++b) {
        const auto color = tjg::color::cube_color{r, g, b};
        const auto expected = std::format("cube{}{}{}", r, g, b);
        check_output(color, expected);
        check_output(tjg::color::color_t{color}, expected);
      }
    }
  }
  check_output(tjg::color::rgb{}, "#000000");
  check_output(tjg::color::rgb{1, 10, 255}, "#010aff");
  check_output(tjg::color::rgb::red, "#ff0000");
  check_output(tjg::color::color_t{tjg::color::rgb{171, 205, 239}}, "#abcdef");
  check_output(tjg::color::rgb{255, 255, 255}, "#ffffff");
}

TEST(color_debug_test, style_notation) {
  check_style(tjg::color::text_style{}, "{}");
  check_style(tjg::color::absolute(tjg::color::text_style{}), "=");
  for (int mask = 0; mask < 256; ++mask) {
    auto marks = std::string{};
    for (int i = 0; i < 8; ++i) {
      if ((mask & (1 << i)) != 0) marks += "bfi_!r?-"[i];
    }
    const auto style = tjg::color::text_style{static_cast<tjg::color::emphasis>(mask)};
    check_style(style, marks.empty() ? "{}" : marks);
    check_style(tjg::color::absolute(style), "=" + marks);
    for (int channels = 1; channels < 4; ++channels) {
      auto colored = style;
      auto colors = std::string{};
      if ((channels & 1) != 0) {
        colored.fg(tjg::color::term_color::bright_red);
        colors += "bright_red";
      }
      if ((channels & 2) != 0) {
        colored.bg(tjg::color::rgb{26, 43, 60});
        colors += "/#1a2b3c";
      }
      check_style(colored, marks + (marks.empty() ? "" : " ") + colors);
      check_style(tjg::color::absolute(colored), "=" + marks + " " + colors);
    }
  }
  check_style(tjg::color::fg(tjg::color::cube_color{5, 0, 3}).bg(tjg::color::gray_color{12}),
               "cube503/gray12");
}

TEST(color_debug_test, emphasis_notation) {
  using tjg::color::emphasis;
  check_output(emphasis::none, "{}");
  check_output(emphasis::bold, "{bold}", "b");
  check_output(emphasis::faint, "{faint}", "f");
  check_output(emphasis::italic, "{italic}", "i");
  check_output(emphasis::underline, "{underline}", "_");
  check_output(emphasis::blink, "{blink}", "!");
  check_output(emphasis::reverse, "{reverse}", "r");
  check_output(emphasis::conceal, "{conceal}", "?");
  check_output(emphasis::strike, "{strike}", "-");
  check_output(emphasis::strike | emphasis::bold | emphasis::underline,
               "{bold underline strike}", "b_-");
  check_output(static_cast<emphasis>(255),
               "{bold faint italic underline blink reverse conceal strike}",
               "bfi_!r?-");
  EXPECT_EQ(std::format("{:*>8}", emphasis::bold), "**{bold}");
  EXPECT_EQ(std::format(L"{:*<8?}", emphasis::faint), L"f*******");
  EXPECT_EQ(std::format("{0:>{1}.{2}s}", emphasis::bold, 5, 3), "  {bo");
#if defined(__cpp_lib_format_ranges)
  EXPECT_EQ(std::format("{}", std::vector<emphasis>{emphasis{}, emphasis::bold}), "[{}, {bold}]");
#endif
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  EXPECT_THROW((void)runtime_format("{:d}", emphasis::bold),
               std::format_error);
#endif
}

TEST(color_debug_test, string_specifications) {
  const auto red = tjg::color::term_color::red;
  EXPECT_EQ(std::format("{:*>8?}", red), "*****red");
  EXPECT_EQ(std::format("{:.3}", tjg::color::rgb{1, 2, 3}), "#01");
  EXPECT_EQ(std::format("{0:*^{1}.{2}?}", red, 7, 2), "**re***");
  EXPECT_EQ(std::format(L"{:*>8?}", red), L"*****red");
  EXPECT_EQ(std::format("{:>4?}", tjg::color::bold()), "   b");
#if defined(__cpp_lib_format_ranges)
  EXPECT_EQ(std::format("{}", std::vector<tjg::color::color_t>{red, tjg::color::no_color}), "[red, no_color]");
#endif
  std::ostringstream os;
  os << std::hex << std::uppercase << std::setfill('*') << std::setw(10)
     << tjg::color::rgb{171, 205, 239};
  EXPECT_EQ(os.str(), "***#abcdef");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::left << std::setw(8) << tjg::color::emphasis::bold;
  EXPECT_EQ(os.str(), "***#abcdef{bold}**");
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  EXPECT_THROW((void)runtime_format("{:x}", red), std::format_error);
  EXPECT_THROW((void)runtime_format("{:+}", tjg::color::bold()),
               std::format_error);
  EXPECT_THROW((void)runtime_format("{:>{}}", red, -1),
               std::format_error);
#endif
}

TEST(color_debug_test, tree_layout) {
  check_style(tjg::color::text_style{}, "{}");
  EXPECT_EQ(std::format("{:?}", tjg::color::styled_text{}), "\"\"");
  EXPECT_EQ(std::format("{:?}", tjg::color::bold("hello")), "b \"hello\"");
  const auto child =
      tjg::color::underline("world") +
      tjg::color::absolute(tjg::color::styled_text{"!", tjg::color::bg(tjg::color::cube_color{5, 0, 3})});
  const auto tree = tjg::color::bold(
      tjg::color::styled_text{"hello\n", tjg::color::fg(tjg::color::term_color::red)} + child);
  const auto expected = std::string{
      "b {\n"
      "  red \"hello\\n\"\n"
      "  {\n"
      "    _ \"world\"\n"
      "    = /cube503 \"!\"\n"
      "  }\n"
      "}\n"};
  EXPECT_EQ(std::format("{:?}", tree), expected);
  EXPECT_EQ(std::formatted_size("{:?}", tree), expected.size());
  char output[512] = {};
  const auto end = std::format_to(&output[0], "{:?}", tree);
  EXPECT_EQ(std::string(output, end), expected);
  const auto bounded = std::format_to_n(output, 7, "{:?}", tree);
  EXPECT_EQ(bounded.size, static_cast<std::ptrdiff_t>(expected.size()));
  EXPECT_EQ(std::string(output, output + 7), expected.substr(0, 7));
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  EXPECT_THROW((void)runtime_format("{:>10.4?}", tree), std::format_error);
#endif
}

TEST(color_debug_test, tree_strings_and_stored_styles) {
  const auto raw = std::string{"a\0\n\t\"\\\x1b", 8};
  EXPECT_EQ(std::format("{:?}", tjg::color::styled_text{raw}),
            "\"a\\x00\\n\\t\\\"\\\\\\x1b\\x00\"");
  const auto wide = std::wstring{L"a\0\n\t\"\\\x1b", 8};
  EXPECT_EQ(std::format(L"{:?}", tjg::color::wstyled_text{wide}),
            L"\"a\\x00\\n\\t\\\"\\\\\\x1b\\x00\"");
  const auto tree = tjg::color::bold(tjg::color::styled_text{"a"} + tjg::color::underline("b"));
  const auto copy = tree;
  EXPECT_EQ(std::format("{:?}", tree), "b {\n  \"a\"\n  _ \"b\"\n}\n");
  EXPECT_EQ(tree, copy);
  const auto wtree = tjg::color::bold(tjg::color::wstyled_text{L"a"} + tjg::color::underline(L"b"));
  EXPECT_EQ(std::format(L"{:?}", wtree), L"b {\n  \"a\"\n  _ \"b\"\n}\n");
  const auto empty_children =
      tjg::color::strip_ansi(tjg::color::styled_text{"\x1b[m"} + tjg::color::bold("\x1b[m"));
  EXPECT_EQ(std::format("{:?}", empty_children), "{\n  \"\"\n  b \"\"\n}\n");
  auto moved = tjg::color::styled_text{"old"};
  const auto owner = std::move(moved);
  EXPECT_EQ(std::format("{:?}", moved), "\"\"");
  EXPECT_EQ(std::format("{:?}", owner), "\"old\"");
}
