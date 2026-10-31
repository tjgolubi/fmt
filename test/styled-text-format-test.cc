#include "fmt/styled-text.h"
#include "fmt/compile.h"
#include "fmt/xchar.h"
#include "gtest-extra.h"

namespace {
auto render(const fmt::styled_text& value) -> std::string {
  return fmt::format(FMT_STRING("{}"), value);
}
auto render(const fmt::wstyled_text& value) -> std::wstring {
  return fmt::format(FMT_STRING(L"{}"), value);
}
template<typename Char>
auto chars(const char* value) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  for (; *value; ++value) result.push_back(static_cast<Char>(*value));
  return result;
}

// A small ANSI interpreter checks the state of each output code unit, without
// using overlay or to_sgr to construct the expected terminal state.
template<typename Char>
void check_states(const std::basic_string<Char>& output,
                  const std::basic_string<Char>& text,
                  const std::vector<fmt::text_style>& expected) {
  ASSERT_EQ(text.size(), expected.size());
  auto state = fmt::text_style{};
  size_t position = 0;
  for (size_t i = 0; i < output.size();) {
    if (output[i] != Char('\x1b')) {
      ASSERT_LT(position, text.size());
      EXPECT_EQ(output[i++], text[position]);
      EXPECT_EQ(state, expected[position]);
      ++position;
      continue;
    }
    ASSERT_LT(i + 1, output.size());
    ASSERT_EQ(output[i + 1], Char('['));
    i += 2;
    auto parameters = std::vector<unsigned>{};
    unsigned number = 0;
    for (;;) {
      ASSERT_LT(i, output.size());
      auto c = output[i++];
      if (c >= Char('0') && c <= Char('9')) {
        number = number * 10 + static_cast<unsigned>(c - Char('0'));
        continue;
      }
      ASSERT_TRUE(c == Char(';') || c == Char('m'));
      parameters.push_back(number);
      number = 0;
      if (c == Char('m')) break;
    }
    for (size_t p = 0; p < parameters.size(); ++p) {
      const auto code = parameters[p];
      switch (code) {
        case 0: state.clear(); break;
        case 1: state.bold(); break;
        case 2: state.faint(); break;
        case 3: state.italic(); break;
        case 4: state.underline(); break;
        case 5: state.blink(); break;
        case 7: state.reverse(); break;
        case 8: state.conceal(); break;
        case 9: state.strike(); break;
        case 22: state.bold(false).faint(false); break;
        case 23: state.italic(false); break;
        case 24: state.underline(false); break;
        case 25: state.blink(false); break;
        case 27: state.reverse(false); break;
        case 28: state.conceal(false); break;
        case 29: state.strike(false); break;
        case 39: state.fg(fmt::no_color); break;
        case 49: state.bg(fmt::no_color); break;
        case 38: case 48: {
          ASSERT_LT(p + 1, parameters.size());
          const auto mode = parameters[++p];
          auto color = fmt::color_t{};
          if (mode == 2) {
            ASSERT_LT(p + 3, parameters.size());
            color = fmt::rgb{static_cast<int>(parameters[p + 1]),
                             static_cast<int>(parameters[p + 2]),
                             static_cast<int>(parameters[p + 3])};
            p += 3;
          } else {
            ASSERT_EQ(mode, 5u);
            ASSERT_LT(p + 1, parameters.size());
            const auto index = parameters[++p];
            if (index >= 232)
              color = fmt::gray_color{static_cast<int>(index - 232)};
            else {
              ASSERT_GE(index, 16u);
              color = fmt::cube_color{index - 16};
            }
          }
          if (code == 38) state.fg(color);
          else state.bg(color);
          break;
        }
        default: {
          const bool background = (code >= 40 && code <= 47) ||
                                  (code >= 100 && code <= 107);
          const auto base = background ? (code >= 100 ? 92u : 40u)
                                       : (code >= 90 ? 82u : 30u);
          ASSERT_GE(code, base);
          const auto index = code - base;
          ASSERT_LT(index, 16u);
          if (background) state.bg(static_cast<fmt::term_color>(index));
          else state.fg(static_cast<fmt::term_color>(index));
        }
      }
    }
  }
  EXPECT_EQ(position, text.size());
  EXPECT_EQ(state, fmt::text_style{});
}

template<typename Char>
void check_nested_formatting() {
  using node = fmt::basic_styled_text<Char>;
  const auto outer = fmt::text_style{fmt::rgb{1, 2, 3}, fmt::gray_color{12},
                                     fmt::emphasis::bold};
  auto inner_style = fmt::text_style{fmt::term_color::green, fmt::no_color,
                                    fmt::emphasis::italic};
  auto inner = node(chars<Char>("b")) +
               node(chars<Char>("c"), fmt::underline());
  inner.style(inner_style);
  auto tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  const auto saved = tree;
  auto b = outer;
  b.fg(fmt::term_color::green).italic();
  auto c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});
  EXPECT_EQ(tree, saved);

  // An absolute group drops outer colors/emphasis, its relative children
  // inherit from that group, and a later sibling resumes the outer state.
  inner_style.absolute();
  inner.style(inner_style);
  tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  b = fmt::text_style{fmt::term_color::green, fmt::no_color,
                       fmt::emphasis::italic};
  c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});

  auto plain_absolute = fmt::text_style{};
  plain_absolute.absolute();
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), plain_absolute) +
         node(chars<Char>("c"));
  tree.style(outer);
  check_states(render(tree), chars<Char>("abc"), {outer, {}, outer});

  // An absolute outer group does not force a reset at each relative child.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"));
  auto absolute_outer = outer;
  absolute_outer.absolute();
  tree.style(absolute_outer);
  check_states(render(tree), chars<Char>("ab"), {outer, outer});
  const auto expected = fmt::to_sgr<Char>(absolute_outer) + chars<Char>("ab") +
                        fmt::to_sgr<Char>(fmt::text_style{}, outer);
  EXPECT_EQ(render(tree), expected);

  // Equal complete states still honor an explicit absolute child boundary.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), absolute_outer);
  tree.style(outer);
  const auto boundary = fmt::to_sgr<Char>(outer, fmt::text_style{}) +
      chars<Char>("a") + fmt::to_sgr<Char>(absolute_outer) + chars<Char>("b") +
      fmt::to_sgr<Char>(fmt::text_style{}, outer);
  EXPECT_EQ(render(tree), boundary);
}
} // namespace

TEST(styled_text_format_test, plain_empty_and_output_iterators) {
  EXPECT_EQ(render(fmt::styled_text{}), "");
  EXPECT_EQ(render(fmt::styled_text{"", fmt::bold()}), "");
  EXPECT_EQ(render(fmt::styled_text{"literal {}"}), "literal {}");
  EXPECT_EQ(fmt::format(FMT_STRING("{:}"), fmt::styled_text{"plain"}), "plain");
  EXPECT_EQ(render(fmt::wstyled_text{L"wide"}), L"wide");
  EXPECT_EQ(fmt::format(FMT_STRING("before{}after{}"), fmt::bold("x"), 42),
            "before\x1b[1mx\x1b[0mafter42");
  auto output = std::vector<char>{'p'};
  fmt::format_to(std::back_inserter(output), FMT_STRING("{}!"), fmt::bold("x"));
  EXPECT_EQ((std::string{output.begin(), output.end()}), "p\x1b[1mx\x1b[0m!");
  const auto binary = std::string{"a\0{}b", 5};
  EXPECT_EQ(render(fmt::styled_text{binary}), binary);
  const auto long_text = std::string(2000, 'x');
  EXPECT_EQ(render(fmt::bold(long_text)), "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(fmt::format(FMT_COMPILE("before{}after"), fmt::bold("x")),
            "before\x1b[1mx\x1b[0mafter");
  const auto raw = std::string{"\x1b[4mraw\x1b[24m"};
  EXPECT_EQ(render(fmt::styled_text{raw}), raw);
}

TEST(styled_text_format_test, nested_inheritance_and_absolute_boundaries) {
  check_nested_formatting<char>();
  check_nested_formatting<wchar_t>();
}

TEST(styled_text_format_test, unsupported_specifications) {
#if FMT_USE_EXCEPTIONS
  const auto value = fmt::bold("text");
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:>8}"), value), fmt::format_error);
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:.2}"), value), fmt::format_error);
#endif
}

TEST(styled_text_format_test, all_parent_child_emphasis_pairs) {
  const auto colors = fmt::text_style{fmt::term_color::red,
                                      fmt::cube_color{1, 2, 3}};
  for (unsigned parent = 0; parent < 256; ++parent) {
    auto enclosing = colors;
    enclosing.emph(static_cast<fmt::emphasis>(parent));
    for (unsigned child = 0; child < 256; ++child) {
      auto relative = fmt::text_style{static_cast<fmt::emphasis>(child)};
      auto absolute = relative;
      absolute.absolute();
      auto tree = fmt::styled_text{"a"} + fmt::styled_text{"b", relative} +
                  fmt::styled_text{"c", absolute} + fmt::styled_text{"d"};
      tree.style(enclosing);
      auto inherited = colors;
      inherited.emph(static_cast<fmt::emphasis>(parent | child));
      check_states(render(tree), std::string{"abcd"},
                   {enclosing, inherited, relative, enclosing});
    }
  }
}
