#include "tjg/color.h"
#include "standard-format-test.h"
#include "test-support.h"

#include <format>
#include <string>

#include <vector>

namespace {
struct native_nested { bool absolute = false; };
struct native_throwing {};
struct not_formattable {};
}

namespace std {
template <typename Char>
struct formatter<native_nested, Char> : formatter<int, Char> {
  template <typename Context>
  auto format(native_nested value, Context& ctx) const -> decltype(ctx.out()) {
    auto style = tjg::fg(tjg::term_color::red);
    style.absolute(value.absolute);
    if constexpr (same_as<Char, char>) {
      return std::format_to(ctx.out(), "a{}z", tjg::styled(42, style));
    } else {
      return std::format_to(ctx.out(), L"a{}z", tjg::styled(42, style));
    }
  }
};
template <typename Char>
struct formatter<native_throwing, Char> : formatter<int, Char> {
  template <typename Context>
  auto format(native_throwing, Context& ctx) const -> decltype(ctx.out()) {
    (void)ctx;
    throw format_error("native formatter failure");
  }
};
}

TEST(color_std_format_test, standard_specs_and_argument_indices) {
  const auto red = tjg::term_color::red;
  EXPECT_EQ(std::format("{:*>8}", red), "*****red");
  EXPECT_EQ(std::format("{:*>8?}", red), "*****red");
  EXPECT_EQ(std::format(L"{:*>8?}", red), L"*****red");
  EXPECT_EQ(std::format("{:?>8}", red), "?????red");
  EXPECT_EQ(std::format("{:?>8?}", red), "?????red");
  EXPECT_EQ(std::format("{0:*^{1}.{2}?}", red, 7, 2), "**re***");
  EXPECT_EQ(std::format(L"{0:*^{1}.{2}?}", red, 7, 2), L"**re***");
  EXPECT_EQ(std::format("{:>{}?} {}", red, 7, 42), "    red 42");
  EXPECT_EQ(std::format("{} {:>{}.{}?} {}", 9, red, 6, 2, 7), "9     re 7");
  EXPECT_EQ(std::format("{0:>{1}.{2}s}", tjg::emphasis::bold, 5, 3), "  {bo");
  EXPECT_EQ(std::format("{:.3}", tjg::rgb{1, 2, 3}), "#01");
  EXPECT_EQ(std::format("{:.0?}", tjg::text_style{}), "");
#if defined(__cpp_lib_format_ranges)
  const auto values = std::vector<tjg::emphasis>{tjg::emphasis::bold, {}};
  EXPECT_EQ(std::format("{}", values), "[{bold}, {}]");
#endif
}

TEST(color_std_format_test, invalid_specs) {
  const auto red = tjg::term_color::red;
  for (const char* pattern : {"{:d}", "{:s?}", "{:??}", "{:08}", "{:+}"}) {
    EXPECT_THROW((void)std::vformat(pattern, std::make_format_args(red)),
                 std::format_error);
  }
  auto width = -1;
  EXPECT_THROW((void)std::vformat("{:>{}?}", std::make_format_args(red, width)),
               std::format_error);
  const auto wrong_width = std::string{"bad"};
  EXPECT_THROW((void)std::vformat("{:>{}?}",
                                std::make_format_args(red, wrong_width)),
               std::format_error);
  for (const char* pattern : {"{:>{1}?}", "{0:>{}?}", "{0:>{2}?}"}) {
    EXPECT_THROW((void)std::vformat(pattern, std::make_format_args(red, width)),
                 std::format_error);
  }
}

TEST(color_std_format_test, output_iterators_and_counts) {
  const auto style = tjg::absolute(tjg::bold(tjg::fg(tjg::term_color::red)));
  const auto expected = std::string{"=b red"};
  auto text = std::string{"prefix:"};
  std::format_to(std::back_inserter(text), "{}", style);
  EXPECT_EQ(text, "prefix:" + expected);
  EXPECT_EQ(std::formatted_size("{}", style), expected.size());
  char output[32] = {};
  const auto end = std::format_to(output, "{:>10?}", style);
  EXPECT_EQ(std::string(output, end), "    =b red");
  const auto bounded = std::format_to_n(output, 3, "{}", style);
  EXPECT_EQ(bounded.size, static_cast<std::ptrdiff_t>(expected.size()));
  EXPECT_EQ(std::string(output, output + 3), "=b ");
}

TEST(color_std_format_test, styled_underlying_specs_and_padding) {
  const auto style = tjg::bold(tjg::fg(tjg::term_color::red));
  const int integer = 42;
  const auto number = tjg::styled(integer, style);
  EXPECT_EQ(std::format("{:04x}", number),
            tjg::wrap_sgr(style, "002a"));
  EXPECT_EQ(std::format("{:>{}}", number, 8),
            tjg::wrap_sgr(style, "      42"));
  EXPECT_EQ(std::format(L"{:04x}", number),
            tjg::wrap_sgr<wchar_t>(style, L"002a"));
  const auto word = tjg::styled("abcdef", style);
  EXPECT_EQ(std::format("{:*>8.3}", word),
            tjg::wrap_sgr(style, "*****abc"));
  auto text = std::string{};
  std::format_to(std::back_inserter(text), "{}", number);
  EXPECT_EQ(text, std::format("{}", number));
  EXPECT_EQ(std::formatted_size("{}", number), text.size());
  char output[64] = {};
  const auto bounded = std::format_to_n(output, 5, "{}", number);
  EXPECT_EQ(bounded.size, static_cast<std::ptrdiff_t>(text.size()));
  EXPECT_EQ(std::string(output, output + 5), text.substr(0, 5));
  EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
  static_assert(!std::formattable<decltype(tjg::styled(not_formattable{}, {})),
                                  char>);
}

TEST(color_std_format_test, nested_native_formatting_and_absolute_styles) {
  const auto outer = tjg::underline();
  for (bool absolute : {false, true}) {
    const auto body = native_nested{absolute};
    const auto value = tjg::styled(body, outer);
    const auto red = tjg::fg(tjg::term_color::red);
    auto effective = absolute ? tjg::absolute(red) : tjg::underline(red);
    const auto expected = tjg::to_sgr(outer, {}) + "a" +
                          tjg::to_sgr(effective, outer) + "42" +
                          tjg::to_sgr(outer, effective) + "z" +
                          tjg::to_sgr(tjg::text_style{}, outer);
    EXPECT_EQ(std::format("{}", value), expected);
    EXPECT_EQ(std::format(L"{}", value),
              (std::wstring{expected.begin(), expected.end()}));
    EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
  }
  const int integer = 42;
  const auto inner = tjg::styled(integer, tjg::fg(tjg::term_color::red));
  const auto nested = tjg::styled(inner, outer);
  EXPECT_EQ(std::format("{}", nested), tjg::to_sgr(outer) + tjg::to_sgr(tjg::overlay(outer, tjg::fg(tjg::term_color::red)), outer) + "42" + tjg::to_sgr(outer, tjg::overlay(outer, tjg::fg(tjg::term_color::red))) + tjg::to_sgr(tjg::text_style{}, outer));
}

TEST(color_std_format_test, exception_restores_shared_state) {
  tjg::detail::style_scope scope{};
  const auto outer = tjg::underline(tjg::bg(tjg::term_color::blue));
  tjg::get_terminal_state() = outer;
  const auto body = native_throwing{};
  const auto value = tjg::styled(body, tjg::bold());
  EXPECT_THROW((void)std::format("{}", value), std::format_error);
  EXPECT_EQ(tjg::get_terminal_state(), outer);
  auto output = std::string{};
  EXPECT_THROW(std::format_to(std::back_inserter(output), "{}", value),
               std::format_error);
  EXPECT_EQ(tjg::get_terminal_state(), outer);
}

TEST(color_std_format_test, styled_character_arrays_use_c_string_semantics) {
  const auto style = tjg::bold();
  EXPECT_EQ(std::format("{}", tjg::styled("text", style)), "\x1b[1mtext\x1b[0m");
  EXPECT_EQ(std::format(L"{}", tjg::styled(L"text", style)), L"\x1b[1mtext\x1b[0m");
  const char text[] = "a\0b";
  EXPECT_EQ(std::format("{}", tjg::styled(text, style)), "\x1b[1ma\x1b[0m");
  const auto view = std::string_view{text, 3};
  EXPECT_EQ(std::format("{}", tjg::styled(view, style)), std::string("\x1b[1ma\0b\x1b[0m", 11));
}

TEST(color_std_format_test, multiple_question_specs_share_argument_indices) {
  const auto red = tjg::term_color::red;
  const auto blue = tjg::term_color::blue;
  EXPECT_EQ(std::format("{:>{}?}|{:*^{}.{}?}|{}", red, 5, blue, 8, 2, 99),
            "  red|***bl***|99");
  EXPECT_EQ(std::format(L"{:>{}?}|{:*^{}.{}?}|{}", red, 5, blue, 8, 2, 99),
            L"  red|***bl***|99");
  EXPECT_EQ(std::format("{0:*^{2}.{3}?}|{1:>{4}?}|{5}", red, blue, 8, 2, 6, 99),
            "***re***|  blue|99");
  EXPECT_EQ(std::format(L"{0:*^{2}.{3}?}|{1:>{4}?}|{5}", red, blue, 8, 2, 6, 99),
            L"***re***|  blue|99");
}
