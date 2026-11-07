#include "standard-format-test.h"
#include "tjg/parse_styled_text.h"

#include "test-support.h"

namespace {
template<typename Char>
auto chars(const char* text) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  while (*text) result.push_back(static_cast<Char>(*text++));
  return result;
}
template<typename Char>
auto parse(const std::basic_string<Char>& text) -> tjg::basic_styled_text<Char>
{ return tjg::parse_styled_text<Char>({text.data(), text.size()}); }

template<typename Char>
void check_structure() {
  using node = tjg::basic_styled_text<Char>;
  EXPECT_EQ(parse(chars<Char>("")), node{});
  EXPECT_EQ(parse(chars<Char>("plain")), (node{chars<Char>("plain")}));
  EXPECT_EQ(parse(chars<Char>("\x1b[31mred\x1b[0m")),
      (node{chars<Char>("red"), tjg::fg(tjg::term_color::red)}));
  const auto red = tjg::fg(tjg::term_color::red);
  const auto expected = node{chars<Char>("plain")} +
      node{chars<Char>("red"), red} + tjg::absolute(node{chars<Char>("plain")});
  EXPECT_EQ(parse(chars<Char>("plain\x1b[31mred\x1b[0mplain")), expected);
  EXPECT_EQ(parse(chars<Char>("\x1b[31ma\x1b[32m\x1b[31mb")),
      (node{chars<Char>("ab"), red}));
  // Consecutive or redundant sequences never introduce empty spans.
  EXPECT_EQ(parse(chars<Char>("\x1b[1m\x1b[31m\x1b[31mred")),
      (node{chars<Char>("red"), tjg::bold(red)}));
  EXPECT_EQ(parse(chars<Char>("a\x1b[999m\x1b[0mb")),
      (node{chars<Char>("a")} + tjg::absolute(node{chars<Char>("b")})));
  EXPECT_EQ(parse(chars<Char>("\x1b[31m\x1b[0m")), node{});
  const auto binary = std::basic_string<Char>{Char('a'), Char{}, Char('b')};
  EXPECT_EQ(parse(binary).plain_text(), binary);
  auto owned_input = chars<Char>("owned");
  const auto owned = parse(owned_input);
  owned_input.assign(100, Char('x'));
  EXPECT_EQ(owned.plain_text(), chars<Char>("owned"));
}

template<typename Char>
void check_sgr() {
  using node = tjg::basic_styled_text<Char>;
  const auto check = [](const char* sequence, tjg::text_style style) {
    EXPECT_EQ(parse(chars<Char>(sequence) + chars<Char>("x")),
        (node{chars<Char>("x"), style}));
  };
  check("\x1b[m", tjg::absolute(tjg::text_style{}));
  check("\x1b[000m", tjg::absolute(tjg::text_style{}));
  check("\x1b[1;2;3;4;5;7;8;9m", tjg::text_style{static_cast<tjg::emphasis>(255)});
  check("\x1b[1;2;3;4;5;7;8;9;22;23;24;25;27;28;29m", {});
  check("\x1b[1;2;22m", {}); // 22 clears bold and faint together.
  check("\x1b[31;44;39;49m", {});
  check("\x1b[31;0;4m", tjg::absolute(tjg::underline()));
  check("\x1b[31;;4m", tjg::absolute(tjg::underline()));
  check("\x1b[31;m", tjg::absolute(tjg::text_style{}));
  check("\x1b[999;31;888;4m", tjg::underline(tjg::fg(tjg::term_color::red)));
  for (unsigned i = 0; i < 16; ++i) {
    const auto fg = i < 8 ? 30 + i : 90 + i - 8;
    const auto bg = i < 8 ? 40 + i : 100 + i - 8;
    const auto sequence = std::format("\x1b[{};{}m", fg, bg);
    check(sequence.c_str(), tjg::text_style{static_cast<tjg::term_color>(i),
                                           static_cast<tjg::term_color>(i)});
  }
  for (unsigned i = 0; i < 256; ++i) {
    auto color = tjg::color_t{};
    if (i < 16) color = static_cast<tjg::term_color>(i);
    else if (i < 232) color = tjg::cube_color{i - 16};
    else color = tjg::gray_color{static_cast<int>(i - 232)};
    const auto sequence = std::format("\x1b[38;5;{};48;5;{}m", i, i);
    check(sequence.c_str(), tjg::text_style{color, color});
  }
  const auto rgb = tjg::text_style{tjg::rgb{1, 2, 255}, tjg::rgb{254, 0, 3}};
  check("\x1b[38;2;1;2;255;48;2;254;0;3m", rgb);
  check("\x1b[38:2::1:2:255;48:2:0:254:0:3m", rgb);
  check("\x1b[38:2:1:2:255;48:2:254:0:3m", rgb);
  check("\x1b[38:5:1;4m", tjg::underline(tjg::fg(tjg::term_color::red)));
  check("\x1b[1;38;99;4;31m", tjg::bold()); // Unknown payload is opaque.
  check("\x1b[4:2;31m", tjg::fg(tjg::term_color::red));
}

template<typename Char>
void check_literal_escapes() {
  const char* inputs[] = {"\x1b", "\x1b[", "\x1b[31", "\x1b[2J", "\x1b[31M",
      "\x1b[?31m", "\x1b[1;38;2;1;2m", "\x1b[1;38;5;256m",
      "\x1b[38:2::1:2:999m", "\x1b[9999999999999999999999m",
      "\x1b]title\x1b[31minside\a", "\x1bPdata\x1b[1m\x1b\\", "\x1bXdata\x1b[31m\x1b\\", "\x1b(B"};
  for (const auto input : inputs) {
    const auto text = chars<Char>(input);
    EXPECT_EQ(parse(text), (tjg::basic_styled_text<Char>{text}));
  }
  const auto text = chars<Char>("\x1b[1;38;2;1;2m");
  const auto parsed = parse(chars<Char>("\x1b[31m") + text);
  EXPECT_EQ(parsed, (tjg::basic_styled_text<Char>{text,
      tjg::fg(tjg::term_color::red)})); // Bad sequence changes nothing.
}
} // namespace

TEST(parse_styled_text_test, structure) { check_structure<char>(); check_structure<wchar_t>(); }
TEST(parse_styled_text_test, sgr) { check_sgr<char>(); check_sgr<wchar_t>(); }
TEST(parse_styled_text_test, literal_escapes) { check_literal_escapes<char>(); check_literal_escapes<wchar_t>(); }

TEST(parse_styled_text_test, overloads_and_round_trip) {
  EXPECT_EQ(tjg::parse_styled_text("plain").plain_text(), "plain");
  EXPECT_EQ(tjg::parse_styled_text(L"wide").plain_text(), L"wide");
  EXPECT_EQ(tjg::parse_styled_text(std::string{"owned"}).plain_text(), "owned");
  EXPECT_EQ(tjg::parse_styled_text(std::wstring{L"owned"}).plain_text(), L"owned");
  for (unsigned mask = 0; mask < 256; ++mask) {
    auto source = tjg::styled_text{"a", tjg::text_style{static_cast<tjg::emphasis>(mask)}} +
        tjg::styled_text{"b", tjg::fg(tjg::rgb{12, 34, 56})} + "c";
    const auto rendered = std::format("{}", source);
    const auto parsed = tjg::parse_styled_text(rendered);
    EXPECT_EQ(parsed.plain_text(), "abc");
    // Rendering may split a combined reset and color sequence. Preserve the
    // parsed text, styles, and reset-group boundaries, not the escape bytes.
    const auto rerendered = std::format("{}", parsed);
    EXPECT_EQ(tjg::parse_styled_text(rerendered), parsed);
  }
}

namespace {
template<typename Char>
void check_reset_boundaries() {
  using node = tjg::basic_styled_text<Char>;
  const auto plain = node{chars<Char>("a")};
  const auto reset = tjg::absolute(plain);
  const auto bold = node{chars<Char>("b"), tjg::bold()};
  // One absolute parent supplies defaults to all following relative children.
  EXPECT_EQ(parse(chars<Char>("\x1b[ma\x1b[1mb")), tjg::absolute(plain + bold));
  EXPECT_EQ(parse(chars<Char>("a\x1b[0mb\x1b[31mc")),
      (plain + tjg::absolute(node{chars<Char>("b")} +
       node{chars<Char>("c"), tjg::fg(tjg::term_color::red)})));
  EXPECT_EQ(parse(chars<Char>("\x1b[ma\x1b[m")), reset);
  EXPECT_EQ(parse(chars<Char>("\x1b[ma\x1b[mb")),
      tjg::absolute(node{chars<Char>("ab")}));
  // Repeated resets reuse the same flat absolute group, never nested groups.
  const auto red = tjg::fg(tjg::term_color::red);
  const auto repeated = tjg::absolute(node{chars<Char>("a"), red} +
      node{chars<Char>("b")} + node{chars<Char>("c"), tjg::bold()} +
      node{chars<Char>("d")});
  EXPECT_EQ(parse(chars<Char>("\x1b[0;31ma\x1b[mb\x1b[1mc\x1b[0md")), repeated);
  EXPECT_EQ(parse(chars<Char>("prefix\x1b[0;31ma\x1b[mb\x1b[1mc\x1b[0md")),
      node{chars<Char>("prefix")} + repeated);
  // Non-SGR literal chunks remain together within the absolute group.
  EXPECT_EQ(parse(chars<Char>("\x1b[ma\x1b[2Jb")),
      tjg::absolute(node{chars<Char>("a\x1b[2Jb")}));
  // Consecutive parameters/sequences after reset still modify its next span.
  EXPECT_EQ(parse(chars<Char>("\x1b[0m\x1b[31m\x1b[1mx")),
      tjg::absolute(node{chars<Char>("x"), tjg::bold(tjg::fg(tjg::term_color::red))}));
  EXPECT_EQ(parse(chars<Char>("\x1b[1ma\x1b[0mb")),
      (node{chars<Char>("a"), tjg::bold()} + tjg::absolute(node{chars<Char>("b")})));
}
} // namespace

TEST(parse_styled_text_test, reset_boundaries) {
  check_reset_boundaries<char>();
  check_reset_boundaries<wchar_t>();
}

TEST(parse_styled_text_test, enclosing_style) {
  const auto outer = tjg::underline();
  EXPECT_EQ(tjg::format(outer, "A{}B", tjg::parse_styled_text("plain")),
            tjg::format(outer, "AplainB"));
  const auto parsed = tjg::parse_styled_text("a\x1b[mb\x1b[1mc");
  const auto bold = tjg::bold();
  const auto expected = tjg::to_sgr(outer) + "a" +
      tjg::to_sgr(tjg::absolute(tjg::text_style{}), outer) + "b" +
      tjg::to_sgr(bold, tjg::text_style{}) + "c" +
      tjg::to_sgr(outer, bold) + tjg::to_sgr(tjg::text_style{}, outer);
  EXPECT_EQ(tjg::format(outer, "{}", parsed), expected);
}

TEST(parse_styled_text_test, standard_view_slice_owns_text) {
  auto input = std::string{"!\x1b[31m"} + std::string{"a\0b", 3} + "?";
  const auto view = std::string_view{input}.substr(1, input.size() - 2);
  const auto parsed = tjg::parse_styled_text(view);
  input.assign(200, 'x');
  EXPECT_EQ(parsed.plain_text(), (std::string{"a\0b", 3}));
  EXPECT_EQ(parsed.style().fg(), tjg::color_t{tjg::term_color::red});
  EXPECT_EQ(tjg::parse_styled_text(std::wstring_view{L"wide"}).plain_text(),
            L"wide");
}
