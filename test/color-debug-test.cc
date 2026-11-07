#include "standard-format-test.h"
#include <iomanip>
#include <sstream>

#include "tjg/color-ostream.h"


#include "tjg/strip_ansi.h"
#include "tjg/styled_text.h"
#include "gtest-extra.h"

namespace {
template <typename T>
void check_output(const T& value, const std::string& expected) {
  EXPECT_EQ(std::format("{}", value), expected);
  EXPECT_EQ(std::format("{:?}", value), expected);
  const auto wide = std::wstring(expected.begin(), expected.end());
  EXPECT_EQ(std::format(L"{}", value), wide);
  EXPECT_EQ(std::format(L"{:?}", value), wide);
  std::ostringstream os;
  os << value;
  EXPECT_EQ(os.str(), expected);
  std::wostringstream wos;
  wos << value;
  EXPECT_EQ(wos.str(), wide);
}
}  // namespace

TEST(color_debug_test, color_notation) {
  check_output(tjg::no_color, "no_color");
  check_output(tjg::color_t{}, "no_color");
  const char* names[] = {"black",        "red",
                         "green",        "yellow",
                         "blue",         "magenta",
                         "cyan",         "white",
                         "bright_black", "bright_red",
                         "bright_green", "bright_yellow",
                         "bright_blue",  "bright_magenta",
                         "bright_cyan",  "bright_white"};
  for (int i = 0; i < 16; ++i) {
    const auto color = static_cast<tjg::term_color>(i);
    check_output(color, names[i]);
    check_output(tjg::color_t{color}, names[i]);
  }
  for (int i = 0; i < 24; ++i) {
    const auto color = tjg::gray_color{i};
    const auto expected = std::format("gray{}", i);
    check_output(color, expected);
    check_output(tjg::color_t{color}, expected);
  }
  for (int r = 0; r < 6; ++r) {
    for (int g = 0; g < 6; ++g) {
      for (int b = 0; b < 6; ++b) {
        const auto color = tjg::cube_color{r, g, b};
        const auto expected = std::format("cube{}{}{}", r, g, b);
        check_output(color, expected);
        check_output(tjg::color_t{color}, expected);
      }
    }
  }
  check_output(tjg::rgb{}, "#000000");
  check_output(tjg::rgb{1, 10, 255}, "#010aff");
  check_output(tjg::color::red, "#ff0000");
  check_output(tjg::color_t{tjg::rgb{171, 205, 239}}, "#abcdef");
  check_output(tjg::rgb{255, 255, 255}, "#ffffff");
}

TEST(color_debug_test, style_notation) {
  check_output(tjg::text_style{}, "{}");
  check_output(tjg::absolute(tjg::text_style{}), "=");
  for (int mask = 0; mask < 256; ++mask) {
    auto marks = std::string{};
    for (int i = 0; i < 8; ++i) {
      if ((mask & (1 << i)) != 0) marks += "bfi_!r?-"[i];
    }
    const auto style = tjg::text_style{static_cast<tjg::emphasis>(mask)};
    check_output(style, marks.empty() ? "{}" : marks);
    check_output(tjg::absolute(style), "=" + marks);
    for (int channels = 1; channels < 4; ++channels) {
      auto colored = style;
      auto colors = std::string{};
      if ((channels & 1) != 0) {
        colored.fg(tjg::term_color::bright_red);
        colors += "bright_red";
      }
      if ((channels & 2) != 0) {
        colored.bg(tjg::rgb{26, 43, 60});
        colors += "/#1a2b3c";
      }
      check_output(colored, marks + (marks.empty() ? "" : " ") + colors);
      check_output(tjg::absolute(colored), "=" + marks + " " + colors);
    }
  }
  check_output(tjg::fg(tjg::cube_color{5, 0, 3}).bg(tjg::gray_color{12}),
               "cube503/gray12");
}

TEST(color_debug_test, emphasis_notation) {
  using tjg::emphasis;
  check_output(emphasis{}, "{}");
  check_output(emphasis::bold, "{bold}");
  check_output(emphasis::faint, "{faint}");
  check_output(emphasis::italic, "{italic}");
  check_output(emphasis::underline, "{underline}");
  check_output(emphasis::blink, "{blink}");
  check_output(emphasis::reverse, "{reverse}");
  check_output(emphasis::conceal, "{conceal}");
  check_output(emphasis::strike, "{strike}");
  check_output(emphasis::strikethrough, "{strike}");
  check_output(emphasis::strike | emphasis::bold | emphasis::underline,
               "{bold underline strike}");
  check_output(static_cast<emphasis>(255),
               "{bold faint italic underline blink reverse conceal strike}");
  EXPECT_EQ(std::format("{:*>8}", emphasis::bold), "**{bold}");
  EXPECT_EQ(std::format(L"{:*<8?}", emphasis::faint), L"{faint}*");
  EXPECT_EQ(std::format("{0:>{1}.{2}s}", emphasis::bold, 5, 3), "  {bo");
#if defined(__cpp_lib_format_ranges)
  EXPECT_EQ(std::format("{}", std::vector<emphasis>{emphasis{}, emphasis::bold}), "[{}, {bold}]");
#endif
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW((void)runtime_format("{:d}", emphasis::bold),
               std::format_error);
#endif
}

TEST(color_debug_test, string_specifications) {
  const auto red = tjg::term_color::red;
  EXPECT_EQ(std::format("{:*>8?}", red), "*****red");
  EXPECT_EQ(std::format("{:.3}", tjg::rgb{1, 2, 3}), "#01");
  EXPECT_EQ(std::format("{0:*^{1}.{2}?}", red, 7, 2), "**re***");
  EXPECT_EQ(std::format(L"{:*>8?}", red), L"*****red");
  EXPECT_EQ(std::format("{:>4}", tjg::bold()), "   b");
#if defined(__cpp_lib_format_ranges)
  EXPECT_EQ(std::format("{}", std::vector<tjg::color_t>{red, tjg::no_color}), "[red, no_color]");
#endif
  std::ostringstream os;
  os << std::hex << std::uppercase << std::setfill('*') << std::setw(10)
     << tjg::rgb{171, 205, 239};
  EXPECT_EQ(os.str(), "***#abcdef");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::left << std::setw(8) << tjg::bold();
  EXPECT_EQ(os.str(), "***#abcdefb*******");
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW((void)runtime_format("{:x}", red), std::format_error);
  EXPECT_THROW((void)runtime_format("{:+}", tjg::bold()),
               std::format_error);
  EXPECT_THROW((void)runtime_format("{:>{}}", red, -1),
               std::format_error);
#endif
}

TEST(color_debug_test, tree_layout) {
  check_output(tjg::text_style{}, "{}");
  EXPECT_EQ(std::format("{:?}", tjg::styled_text{}), "\"\"");
  EXPECT_EQ(std::format("{:?}", tjg::bold("hello")), "b \"hello\"");
  const auto child =
      tjg::underline("world") +
      tjg::absolute(tjg::styled_text{"!", tjg::bg(tjg::cube_color{5, 0, 3})});
  const auto tree = tjg::bold(
      tjg::styled_text{"hello\n", tjg::fg(tjg::term_color::red)} + child);
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
  EXPECT_EQ(bounded.size, expected.size());
  EXPECT_EQ(std::string(output, output + 7), expected.substr(0, 7));
  EXPECT_EQ(std::format("{:>10.4?}", tree), "      b {\n");
}

TEST(color_debug_test, tree_strings_and_stored_styles) {
  const auto raw = std::string{"a\0\n\t\"\\\x1b", 8};
  EXPECT_EQ(std::format("{:?}", tjg::styled_text{raw}),
            "\"a\\x00\\n\\t\\\"\\\\\\x1b\\x00\"");
  const auto wide = std::wstring{L"a\0\n\t\"\\\x1b", 8};
  EXPECT_EQ(std::format(L"{:?}", tjg::wstyled_text{wide}),
            L"\"a\\x00\\n\\t\\\"\\\\\\x1b\\x00\"");
  const auto tree = tjg::bold(tjg::styled_text{"a"} + tjg::underline("b"));
  const auto copy = tree;
  EXPECT_EQ(std::format("{:?}", tree), "b {\n  \"a\"\n  _ \"b\"\n}\n");
  EXPECT_EQ(tree, copy);
  const auto wtree = tjg::bold(tjg::wstyled_text{L"a"} + tjg::underline(L"b"));
  EXPECT_EQ(std::format(L"{:?}", wtree), L"b {\n  \"a\"\n  _ \"b\"\n}\n");
  const auto empty_children =
      tjg::strip_ansi(tjg::styled_text{"\x1b[m"} + tjg::bold("\x1b[m"));
  EXPECT_EQ(std::format("{:?}", empty_children), "{\n  \"\"\n  b \"\"\n}\n");
  auto moved = tjg::styled_text{"old"};
  const auto owner = std::move(moved);
  EXPECT_EQ(std::format("{:?}", moved), "\"\"");
  EXPECT_EQ(std::format("{:?}", owner), "\"old\"");
}
