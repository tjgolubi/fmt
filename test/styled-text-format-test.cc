#include "fmt/styled_text.h"
#include "fmt/compile.h"
#include "fmt/xchar.h"
#include "fmt/color-ostream.h"
#include "gtest-extra.h"

#include <iomanip>
#include <sstream>

namespace {
auto render(const tjg::styled_text& value) -> std::string {
  const auto expected = fmt::format(FMT_STRING("{}"), value);
  EXPECT_EQ(std::format("{}", value), expected);
  return expected;
}
auto render(const tjg::wstyled_text& value) -> std::wstring {
  const auto expected = fmt::format(FMT_STRING(L"{}"), value);
  EXPECT_EQ(std::format(L"{}", value), expected);
  return expected;
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
                  const std::vector<tjg::text_style>& expected) {
  ASSERT_EQ(text.size(), expected.size());
  auto state = tjg::text_style{};
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
        case 39: state.fg(tjg::no_color); break;
        case 49: state.bg(tjg::no_color); break;
        case 38: case 48: {
          ASSERT_LT(p + 1, parameters.size());
          const auto mode = parameters[++p];
          auto color = tjg::color_t{};
          if (mode == 2) {
            ASSERT_LT(p + 3, parameters.size());
            color = tjg::rgb{static_cast<int>(parameters[p + 1]),
                             static_cast<int>(parameters[p + 2]),
                             static_cast<int>(parameters[p + 3])};
            p += 3;
          } else {
            ASSERT_EQ(mode, 5u);
            ASSERT_LT(p + 1, parameters.size());
            const auto index = parameters[++p];
            if (index >= 232)
              color = tjg::gray_color{static_cast<int>(index - 232)};
            else {
              ASSERT_GE(index, 16u);
              color = tjg::cube_color{index - 16};
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
          if (background) state.bg(static_cast<tjg::term_color>(index));
          else state.fg(static_cast<tjg::term_color>(index));
        }
      }
    }
  }
  EXPECT_EQ(position, text.size());
  EXPECT_EQ(state, tjg::text_style{});
}

template<typename Char>
void check_nested_formatting() {
  using node = tjg::basic_styled_text<Char>;
  const auto outer = tjg::text_style{tjg::rgb{1, 2, 3}, tjg::gray_color{12},
                                     tjg::emphasis::bold};
  auto inner_style = tjg::text_style{tjg::term_color::green, tjg::no_color,
                                    tjg::emphasis::italic};
  auto inner = node(chars<Char>("b")) +
               node(chars<Char>("c"), tjg::underline());
  inner.style(inner_style);
  auto tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  const auto saved = tree;
  auto b = outer;
  b.fg(tjg::term_color::green).italic();
  auto c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});
  EXPECT_EQ(tree, saved);

  // An absolute group drops outer colors/emphasis, its relative children
  // inherit from that group, and a later sibling resumes the outer state.
  inner_style = tjg::absolute(inner_style);
  inner = tjg::absolute(inner);
  tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  b = tjg::text_style{tjg::term_color::green, tjg::no_color,
                       tjg::emphasis::italic};
  c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});

  auto plain_absolute = tjg::text_style{};
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
  // The absolute root resets even from assumed defaults, in both modes.
  const auto expected = tjg::to_sgr<Char>(absolute_outer) + chars<Char>("ab") +
                        tjg::to_sgr<Char>(tjg::text_style{}, outer);
  EXPECT_EQ(render(tree), expected);

  // Equal complete states still honor an explicit absolute child boundary.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), absolute_outer);
  tree.style(outer);
  const auto boundary = tjg::to_sgr<Char>(outer, tjg::text_style{}) +
      chars<Char>("a") + tjg::to_sgr<Char>(absolute_outer) + chars<Char>("b") +
      tjg::to_sgr<Char>(tjg::text_style{}, outer);
  EXPECT_EQ(render(tree), boundary);
}
} // namespace

TEST(styled_text_format_test, plain_empty_and_output_iterators) {
  EXPECT_EQ(render(tjg::styled_text{}), "");
  EXPECT_EQ(render(tjg::styled_text{"", tjg::bold()}), "");
  EXPECT_EQ(render(tjg::styled_text{"literal {}"}), "literal {}");
  EXPECT_EQ(fmt::format(FMT_STRING("{:}"), tjg::styled_text{"plain"}), "plain");
  EXPECT_EQ(render(tjg::wstyled_text{L"wide"}), L"wide");
  EXPECT_EQ(fmt::format(FMT_STRING("before{}after{}"), tjg::bold("x"), 42),
            "before\x1b[1mx\x1b[0mafter42");
  auto output = std::vector<char>{'p'};
  fmt::format_to(std::back_inserter(output), FMT_STRING("{}!"), tjg::bold("x"));
  EXPECT_EQ((std::string{output.begin(), output.end()}), "p\x1b[1mx\x1b[0m!");
  const auto binary = std::string{"a\0{}b", 5};
  EXPECT_EQ(render(tjg::styled_text{binary}), binary);
  const auto long_text = std::string(2000, 'x');
  EXPECT_EQ(render(tjg::bold(long_text)), "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(fmt::format(FMT_COMPILE("before{}after"), tjg::bold("x")),
            "before\x1b[1mx\x1b[0mafter");
  const auto raw = std::string{"\x1b[4mraw\x1b[24m"};
  EXPECT_EQ(render(tjg::styled_text{raw}), raw);
}

TEST(styled_text_format_test, nested_inheritance_and_absolute_boundaries) {
  check_nested_formatting<char>();
  check_nested_formatting<wchar_t>();
}

namespace {
template<typename Char, typename... T>
auto format_in_style(tjg::text_style style, const char* pattern, const T&... args)
    -> std::basic_string<Char> {
  auto buffer = tjg::basic_memory_buffer<Char>{};
  const auto text = chars<Char>(pattern);
  tjg::detail::vformat_to(buffer, style, std::basic_string_view<Char>{text},
      fmt::make_format_args<fmt::buffered_context<Char>>(args...));
  return {buffer.data(), buffer.size()};
}

template<typename Char>
void check_context_styles() {
  const auto outer = tjg::underline(tjg::bg(tjg::gray_color{12}));
  const auto red = tjg::fg(tjg::term_color::bright_red);
  auto inherited = outer;
  inherited.fg(tjg::term_color::bright_red);
  const auto number = 42;
  const auto value = tjg::styled(number, red);
  // Both the exact-"{}" shortcut and replacement fields without specs create
  // contexts for custom formatters. Explicit specs use the handler's context.
  check_states(format_in_style<Char>(outer, "{}", value), chars<Char>("42"),
               {inherited, inherited});
  check_states(format_in_style<Char>(outer, "{:}", value), chars<Char>("42"),
               {inherited, inherited});
  check_states(format_in_style<Char>(outer, "a{}z{}", value, 7),
               chars<Char>("a42z7"), {outer, inherited, inherited, outer, outer});
  check_states(format_in_style<Char>(outer, "a{:>{}}z", value, 4),
               chars<Char>("a  42z"),
               {outer, inherited, inherited, inherited, inherited, outer});

  const auto absolute = tjg::styled(number, tjg::absolute(red));
  check_states(format_in_style<Char>(outer, "a{}z", absolute),
               chars<Char>("a42z"), {outer, red, red, outer});
  const auto clear = tjg::styled(number, tjg::absolute(tjg::text_style{}));
  check_states(format_in_style<Char>(outer, "a{}z", clear),
               chars<Char>("a42z"), {outer, {}, {}, outer});
  check_states(format_in_style<Char>(outer, "a{}z", tjg::styled(42, {})),
               chars<Char>("a42z"), {outer, outer, outer, outer});

  const auto nested = tjg::styled(value, tjg::bold());
  const auto bold_red = tjg::bold(inherited);
  check_states(format_in_style<Char>(outer, "a{}z", nested),
               chars<Char>("a42z"), {outer, bold_red, bold_red, outer});

  using node = tjg::basic_styled_text<Char>;
  auto tree = tjg::overlay(red, node{chars<Char>("x")}) +
              tjg::absolute(tjg::bold(node{chars<Char>("y")}));
  check_states(format_in_style<Char>(outer, "a{}z{}", tree, 7),
               chars<Char>("axyz7"), {outer, inherited, tjg::bold(), outer, outer});
  check_states(format_in_style<Char>(outer, "a{:>4.1}z", tree),
               chars<Char>("a   xz"),
               {outer, outer, outer, outer, inherited, outer});
  check_states(format_in_style<Char>(outer, "a{:>2.0}z", tree),
               chars<Char>("a  z"), {outer, outer, outer, outer});
  check_states(format_in_style<Char>(outer, "a{}z", node{}),
               chars<Char>("az"), {outer, outer});
  check_states(format_in_style<Char>(tjg::absolute(outer), "a{}z", tree),
               chars<Char>("axyz"), {outer, inherited, tjg::bold(), outer});

  const auto wrapped = tjg::styled(tree, tjg::italic());
  check_states(format_in_style<Char>(outer, "a{}z", wrapped),
               chars<Char>("axyz"),
               {outer, tjg::italic(inherited), tjg::bold(), outer});
}
} // namespace

struct color_context_nested {};
struct color_context_throwing {};
struct terminal_state_nested {};

FMT_BEGIN_NAMESPACE
template<> struct formatter<color_context_nested> : nested_formatter<int> {
  template<typename Context>
  auto format(color_context_nested, Context& ctx) const
      -> decltype(ctx.out()) {
    return write(ctx, "a", tjg::styled(42, tjg::fg(tjg::term_color::red)), "z");
  }
};
template<> struct formatter<terminal_state_nested> : formatter<int> {
  template<typename Context>
  auto format(terminal_state_nested, Context& ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), FMT_STRING("a{}z"),
                         tjg::styled(42, tjg::fg(tjg::term_color::red)));
  }
};
template<> struct formatter<color_context_throwing> : formatter<int> {
  template<typename Context>
  auto format(color_context_throwing, Context& ctx) const -> decltype(ctx.out()) {
    report_error("color context test");
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(styled_text_format_test, nested_buffer_context_inherits_style) {
  const auto outer = tjg::underline();
  const auto red = tjg::underline(tjg::fg(tjg::term_color::red));
  check_states(tjg::format(outer, FMT_STRING("{}"), color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  // Nonzero outer width creates a separate buffer context. Its custom child
  // must still inherit underline, even though the complete text exceeds width.
  check_states(tjg::format(outer, FMT_STRING("{:>1}"), color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  const auto nested = color_context_nested{};
  const auto wrapped = tjg::styled(nested, outer);
  check_states(fmt::format(FMT_COMPILE("{:>1}"), wrapped),
               std::string{"a42z"}, {outer, red, red, outer});
}

TEST(styled_text_format_test, separate_format_call_inherits_shared_state) {
  const auto outer = tjg::underline();
  const auto red = tjg::underline(tjg::fg(tjg::term_color::red));
  check_states(tjg::format(outer, FMT_STRING("{}"), terminal_state_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW((void)tjg::format(outer, FMT_STRING("{}"),
                                tjg::styled(color_context_throwing{}, red)),
               fmt::format_error);
  EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
#endif
}

TEST(styled_text_format_test, terminal_state_is_shared_and_exception_safe) {
  auto buffer = fmt::memory_buffer{};
  auto ctx = fmt::format_context{fmt::appender{buffer}, {}};
  tjg::detail::style_scope scope{};
  auto& terminal = tjg::get_terminal_state();
  EXPECT_EQ(terminal, tjg::text_style{});
  const auto outer = tjg::underline(tjg::bg(tjg::term_color::blue));
  terminal = outer;
  const auto number = 42;
  auto inner = tjg::styled(number, tjg::fg(tjg::term_color::red));
  auto formatter = fmt::formatter<decltype(inner)>{};
  auto parse_inner = fmt::parse_context<char>{fmt::string_view{}};
  formatter.parse(parse_inner);
  ctx.advance_to(formatter.format(inner, ctx));
  EXPECT_EQ(&tjg::get_terminal_state(), &terminal);
  const auto wide_text = tjg::styled(42, tjg::fg(tjg::term_color::red));
  EXPECT_FALSE(fmt::format(FMT_STRING(L"{}"), wide_text).empty());
  EXPECT_EQ(tjg::get_terminal_state(), outer);
#if FMT_USE_EXCEPTIONS
  const auto value = color_context_throwing{};
  auto styled = tjg::styled(value, tjg::bold());
  auto throwing = fmt::formatter<decltype(styled)>{};
  auto parse = fmt::parse_context<char>{fmt::string_view{}};
  throwing.parse(parse);
  EXPECT_THROW((void)throwing.format(styled, ctx), fmt::format_error);
  EXPECT_EQ(tjg::get_terminal_state(), outer);
#endif
}

TEST(styled_text_format_test, enclosing_format_context) {
  check_context_styles<char>();
  check_context_styles<wchar_t>();
  const auto outer = tjg::underline();
  const auto red = tjg::fg(tjg::term_color::bright_red);
  const auto expected = std::vector<tjg::text_style>{
      outer, tjg::underline(red), tjg::underline(red), outer};
  check_states(tjg::format(outer, FMT_STRING("a{}z"), tjg::styled(42, red)),
               std::string{"a42z"}, expected);
  check_states(tjg::format(outer, FMT_STRING("a{}z"),
                          tjg::overlay(red, std::to_string(42))),
               std::string{"a42z"}, expected);
  std::ostringstream os{};
  tjg::print(os, outer, FMT_STRING("a{}z"), tjg::styled(42, red));
  check_states(os.str(), std::string{"a42z"}, expected);
  std::wostringstream wide{};
  tjg::println(wide, outer, FMT_STRING(L"a{:}z"), tjg::styled(42, red));
  check_states(wide.str(), std::wstring{L"a42z\n"},
               {outer, tjg::underline(red), tjg::underline(red), outer, {}});
  EXPECT_WRITE(stdout, tjg::print(outer, FMT_STRING("a{}z"),
                                 tjg::styled(42, red)),
      format_in_style<char>(outer, "a{}z", tjg::styled(42, red)));
  auto output = std::vector<char>{};
  tjg::format_to(std::back_inserter(output), outer, FMT_STRING("a{}z"),
                 tjg::overlay(red, "42"));
  check_states(std::string{output.begin(), output.end()}, std::string{"a42z"},
               expected);
  // Independent operations begin at defaults, even after styled output.
  EXPECT_EQ(fmt::format(FMT_STRING("{}"), 42), "42");
}

TEST(styled_text_format_test, compiled_nested_style_and_pointer_output) {
  const auto inner = tjg::bold("x");
  const auto wrapped = tjg::styled(inner, tjg::underline());
  const auto both = tjg::underline(tjg::bold());
  check_states(fmt::format(FMT_COMPILE("a{}z"), wrapped),
               std::string{"axz"}, {{}, both, {}});
  auto buffer = std::string(128, '\0');
  const auto end = fmt::format_to(&buffer[0], FMT_COMPILE("a{:>3}z"), wrapped);
  buffer.resize(static_cast<size_t>(end - &buffer[0]));
  check_states(buffer, std::string{"a  xz"},
               {{}, both, both, both, {}});
}

TEST(styled_text_format_test, unsupported_specifications) {
#if FMT_USE_EXCEPTIONS
  const auto value = tjg::bold("text");
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:+}"), value), fmt::format_error);
#endif
}

TEST(styled_text_format_test, stream_insertion_options) {
  const auto tree = tjg::bold(tjg::styled_text{"ab"} +
                             tjg::styled_text{"cdefgh", tjg::fg(tjg::term_color::red)});
  const auto bold_red = tjg::bold(tjg::fg(tjg::term_color::red));
#if defined(_MSC_VER) && _MSC_VER < 1930
  // Older MSVC STL alignment constants and fmtflags have different types.
  const std::ios_base::fmtflags alignments[] = {
      std::ios_base::left, std::ios_base::right,
      std::ios_base::internal, std::ios_base::fmtflags{}};
  for (auto alignment : alignments) {
#else
  for (auto alignment : {std::ios_base::left, std::ios_base::right,
                         std::ios_base::internal, std::ios_base::fmtflags{}}) {
#endif
    std::ostringstream os;
    os.setf(alignment, std::ios_base::adjustfield);
    os << std::hex << std::setprecision(4) << std::setfill('*') << std::setw(11);
    const auto flags = os.flags();
    auto& result = (os << tree);
    EXPECT_EQ(&result, &os);
    if (alignment == std::ios_base::left) {
      check_states(os.str(), std::string{"abcdefgh***"},
                   {tjg::bold(), tjg::bold(), bold_red, bold_red,
                    bold_red, bold_red, bold_red, bold_red,
                    tjg::bold(), tjg::bold(), tjg::bold()});
    } else {
      check_states(os.str(), std::string{"***abcdefgh"},
                   {tjg::bold(), tjg::bold(), tjg::bold(), tjg::bold(),
                    tjg::bold(), bold_red, bold_red, bold_red, bold_red,
                    bold_red, bold_red});
    }
    EXPECT_EQ(os.width(), 0);
    EXPECT_EQ(os.precision(), 4);
    EXPECT_EQ(os.fill(), '*');
    EXPECT_EQ(os.flags(), flags);
    os << '!';
    EXPECT_EQ(os.str().back(), '!');
  }
  std::ostringstream default_stream;
  default_stream << tree;
  check_states(default_stream.str(), std::string{"abcdefgh"},
               {tjg::bold(), tjg::bold(), bold_red, bold_red, bold_red, bold_red,
                bold_red, bold_red});
  std::ostringstream zero;
  zero << std::setprecision(0) << std::setfill('*') << std::setw(3) << tree;
  check_states(zero.str(), std::string{"abcdefgh"},
               {tjg::bold(), tjg::bold(), bold_red, bold_red, bold_red, bold_red,
                bold_red, bold_red});
  EXPECT_EQ(zero.precision(), 0);
  std::ostringstream empty;
  empty << std::setfill('*') << std::setw(3) << tjg::bold("");
  check_states(empty.str(), std::string{"***"},
               {tjg::bold(), tjg::bold(), tjg::bold()});
}

TEST(styled_text_format_test, stream_insertion_wide_and_utf8) {
  const auto style = tjg::bold();
  std::wostringstream wide;
  wide << std::left << std::setfill(L'\u2603') << std::setprecision(2)
       << std::setw(5) << tjg::bold(tjg::wstyled_text{L"abcd"});
  check_states(wide.str(), std::wstring{L"abcd\u2603"},
               {style, style, style, style, style});
  EXPECT_EQ(wide.width(), 0);
  EXPECT_EQ(wide.fill(), L'\u2603');
  const auto tree = tjg::bold(tjg::styled_text{"\xe6\x9d\xb1\xe4\xba\xac"} + "abc");
  std::ostringstream utf8;
  utf8 << std::left << std::setprecision(3) << std::setw(9) << tree;
  EXPECT_EQ(utf8.str(), fmt::format("{:<9}", tree));
  check_states(utf8.str(), std::string{"\xe6\x9d\xb1\xe4\xba\xac" "abc  "},
               {style, style, style, style, style, style, style, style, style,
                style, style});
  std::ostringstream absolute;
  const auto protected_tree = tjg::bold(
      tjg::styled_text{"a"} + tjg::absolute(tjg::underline("b")) + "c");
  absolute << protected_tree;
  check_states(absolute.str(), std::string{"abc"},
               {style, tjg::underline(), style});
}

TEST(styled_text_format_test, stream_insertion_failure) {
  std::ostringstream os;
  os.setstate(std::ios_base::failbit);
  os << std::setw(3) << tjg::bold("text");
  EXPECT_EQ(os.str(), "");
  EXPECT_EQ(os.width(), 3);
  std::wostringstream wide;
  wide.setstate(std::ios_base::failbit);
  wide << tjg::bold(L"text");
  EXPECT_EQ(wide.str(), L"");
  struct failing_buffer : std::streambuf {
    std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
  } buffer;
  std::ostream failing{&buffer};
  failing << tjg::bold("text");
  EXPECT_TRUE(failing.bad());
#if FMT_USE_EXCEPTIONS
  failing.clear();
  failing.exceptions(std::ios_base::badbit);
  EXPECT_THROW(failing << tjg::bold("text"), std::ios_base::failure);
#endif
}

TEST(styled_text_format_test, all_parent_child_emphasis_pairs) {
  const auto colors = tjg::text_style{tjg::term_color::red,
                                      tjg::cube_color{1, 2, 3}};
  for (unsigned parent = 0; parent < 256; ++parent) {
    auto enclosing = colors;
    enclosing.emph(static_cast<tjg::emphasis>(parent));
    for (unsigned child = 0; child < 256; ++child) {
      auto relative = tjg::text_style{static_cast<tjg::emphasis>(child)};
      auto absolute = relative;
      absolute.absolute();
      auto tree = tjg::styled_text{"a"} + tjg::styled_text{"b", relative} +
                  tjg::styled_text{"c", absolute} + tjg::styled_text{"d"};
      tree.style(enclosing);
      auto inherited = colors;
      inherited.emph(static_cast<tjg::emphasis>(parent | child));
      check_states(render(tree), std::string{"abcd"},
                   {enclosing, inherited, relative, enclosing});
    }
  }
}

namespace {
template<typename Char>
void check_string_specifications() {
  using node = tjg::basic_styled_text<Char>;
  const char* specifications[] = {
      "{}", "{:s}", "{:8}", "{:>8}", "{:_<8}", "{:*^9}",
      "{:.0}", "{:.1}", "{:.3}", "{:.20}", "{:>8.3s}", "{:*^8.0}"};
  const auto plain = chars<Char>("abcdef");
  const auto tree = node{chars<Char>("ab")} + node{chars<Char>("cde")} +
                    node{chars<Char>("f")};
  for (const auto spec : specifications) {
    const auto pattern = chars<Char>(spec);
    EXPECT_EQ(fmt::format(fmt::runtime(pattern), tree),
              fmt::format(fmt::runtime(pattern), plain));
    EXPECT_EQ(fmt::format(fmt::runtime(pattern), node{}),
              fmt::format(fmt::runtime(pattern), std::basic_string<Char>{}));
  }
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:>{}.{}}")), tree, 8, 3),
            fmt::format(fmt::runtime(chars<Char>("{:>{}.{}}")), plain, 8, 3));
  const auto root = tjg::fg(tjg::term_color::red);
  const auto child = tjg::bold(root);
  const auto absolute = tjg::underline();
  auto styled = node{chars<Char>("ab"), tjg::bold()} +
      tjg::absolute(node{chars<Char>("cd"), absolute}) +
      node{chars<Char>("ef")};
  styled.style(root);
  const auto saved = styled;
  check_states(fmt::format(fmt::runtime(chars<Char>("{:*^8.3}")), styled),
      chars<Char>("**abc***"), {root, root, child, child, absolute, root, root, root});
  check_states(fmt::format(fmt::runtime(chars<Char>("{:>3.0}")), styled),
      chars<Char>("   "), {root, root, root});
  check_states(fmt::format(fmt::runtime(chars<Char>("{:>3}")), node{std::basic_string<Char>{}, root}),
      chars<Char>("   "), {root, root, root});
  EXPECT_EQ(styled, saved);
}
} // namespace

TEST(styled_text_format_test, string_specifications) {
  check_string_specifications<char>();
  check_string_specifications<wchar_t>();
  EXPECT_EQ(fmt::format(FMT_COMPILE("{:>6.3}"), tjg::styled_text{"abcdef"}),
            "   abc");
  auto output = std::vector<char>{};
  fmt::format_to(std::back_inserter(output), FMT_STRING("{:>6.3}"),
                 tjg::styled_text{"abcdef"});
  EXPECT_EQ((std::string{output.begin(), output.end()}), "   abc");
  EXPECT_EQ(fmt::format(FMT_STRING("{0:>{1}.{2}}"), tjg::styled_text{"abcdef"}, 6, 3),
            "   abc");
}

TEST(styled_text_format_test, unicode_precision) {
  const std::string samples[] = {
      "caf\xc3\xa9", "\xe7\x95\x8c!", "\xf0\x9f\x98\x80!", "e\xcc\x81",
      "\xff!", std::string{"a\0b", 3}};
  for (const auto& sample : samples) {
    const auto tree = tjg::styled_text{"x"} + tjg::styled_text{sample} + "later";
    for (int precision = 0; precision < 12; ++precision) {
      const auto expected = fmt::format(FMT_STRING("{:*^14.{}}"), "x" + sample + "later", precision);
      EXPECT_EQ(fmt::format(FMT_STRING("{:*^14.{}}"), tree, precision), expected);
      EXPECT_EQ(std::format("{:*^14.{}}", tree, precision), expected);
    }
  }
  // A wide code point that cannot fit stops the traversal, including later leaves.
  const auto tree = tjg::styled_text{"a"} + "\xe7\x95\x8c" + "b";
  EXPECT_EQ(fmt::format(FMT_STRING("{:.2}"), tree), "a");
  EXPECT_EQ(fmt::format(FMT_STRING("{:\xc2\xb7^5.1}"), tree),
            fmt::format(FMT_STRING("{:\xc2\xb7^5.1}"), "a"));
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:>{}}"), tree, -1), fmt::format_error);
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:.{}}"), tree, -1), fmt::format_error);
#endif
}

namespace {
template<typename Char>
void check_deferred_transitions() {
  using node = tjg::basic_styled_text<Char>;
  const auto root = tjg::fg(tjg::term_color::red);
  const auto child = tjg::bold(root);
  const auto inner = tjg::underline(child);
  auto tree = tjg::overlay(root, tjg::bold(
      tjg::underline(node{chars<Char>("abc")}) + chars<Char>("later")) +
      chars<Char>("tail"));
  const auto opening = tjg::to_sgr<Char>(inner, tjg::text_style{});
  const auto reset = tjg::to_sgr<Char>(tjg::text_style{}, inner);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.1}")), tree),
            opening + chars<Char>("a") + reset);
  const auto padded = opening + chars<Char>("a") +
      tjg::to_sgr<Char>(root, inner) + chars<Char>("  ") +
      tjg::to_sgr<Char>(tjg::text_style{}, root);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:<3.1}")), tree), padded);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.0}")), tree),
            std::basic_string<Char>{});

  // The enclosing group never prints: only its absolute child needs an entry.
  const auto absolute = tjg::absolute(tjg::underline());
  const auto group = tjg::overlay(root, tjg::absolute(tjg::underline(
      node{chars<Char>("abc")})) + chars<Char>("later"));
  auto relative = absolute;
  relative.absolute(false);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.1}")), group),
      tjg::to_sgr<Char>(absolute, tjg::text_style{}) + chars<Char>("a") +
      tjg::to_sgr<Char>(tjg::text_style{}, relative));
}
} // namespace

TEST(styled_text_format_test, deferred_transitions) {
  check_deferred_transitions<char>();
  check_deferred_transitions<wchar_t>();
}


namespace {
template <typename Char> void check_defaults_and_overrides() {
  using node = tjg::basic_styled_text<Char>;
  using tc = tjg::term_color;
  const auto defaults = tjg::fg(tc::white).bg(tc::blue);
  const auto yellow = tjg::fg(tc::yellow).bg(tc::blue);
  const auto white = defaults;
  auto protected_group = node{chars<Char>("C"), tjg::fg(tc::red)}
      + node{chars<Char>("D")};
  protected_group.style(tjg::absolute(tjg::text_style{}));
  const auto text = node{chars<Char>("A"), tjg::fg(tc::yellow)}
      + node{chars<Char>("B")} + protected_group;
  check_states(render(tjg::underlay(defaults, text)), chars<Char>("ABCD"),
               {yellow, white, tjg::fg(tc::red), tjg::text_style{}});
  check_states(render(tjg::overlay(defaults, text)), chars<Char>("ABCD"),
               {white, white, tjg::fg(tc::red), tjg::text_style{}});
  const auto leaf = node{chars<Char>("A"), tjg::fg(tc::yellow)};
  const auto branch = leaf + node{chars<Char>("B")};
  check_states(fmt::format(fmt::runtime(chars<Char>("{:.1}")),
                          tjg::underlay(defaults, leaf)), chars<Char>("A"), {yellow});
  check_states(fmt::format(fmt::runtime(chars<Char>("{:.1}")),
                          tjg::underlay(defaults, branch)), chars<Char>("A"), {yellow});
}
}  // namespace
TEST(styled_text_format_test, defaults_and_recursive_overrides) {
  check_defaults_and_overrides<char>();
  check_defaults_and_overrides<wchar_t>();
}

TEST(styled_text_format_test, standard_tree_specs_and_debug) {
  const auto tree = tjg::bold(tjg::styled_text{"hello "} +
      tjg::styled_text{"world", tjg::fg(tjg::term_color::blue)});
  for (const char* spec : {"{}", "{:s}", "{:>15}", "{:*^15.9}", "{:.0}",
                           "{:*>8.0}", "{:?}", "{:>20.12?}"}) {
    EXPECT_EQ(std::vformat(spec, std::make_format_args(tree)),
              fmt::vformat(spec, fmt::make_format_args(tree)));
  }
  EXPECT_EQ(std::format("{:>{}.{}s} {}", tree, 12, 9, 42),
            fmt::format(FMT_STRING("{:>{}.{}s} {}"), tree, 12, 9, 42));
  EXPECT_EQ(std::format("{0:*^{1}.{2}?}", tree, 15, 9),
            fmt::format(FMT_STRING("{0:*^{1}.{2}?}"), tree, 15, 9));
  const auto wide = tjg::underline(tjg::wstyled_text{L"wide"} + L" tree");
  EXPECT_EQ(std::format(L"{:*^15.6}", wide),
            fmt::format(FMT_STRING(L"{:*^15.6}"), wide));
  EXPECT_EQ(std::format(L"{:?}", wide), fmt::format(FMT_STRING(L"{:?}"), wide));
  EXPECT_EQ(std::formatted_size("{}", tree), std::format("{}", tree).size());
  char output[256] = {};
  const auto end = std::format_to(output, "{:*>12.9}", tree);
  EXPECT_EQ(std::string(output, end), std::format("{:*>12.9}", tree));
  const auto result = std::format_to_n(output, 7, "{}", tree);
  EXPECT_EQ(result.size, static_cast<std::ptrdiff_t>(std::formatted_size("{}", tree)));
  EXPECT_EQ(std::string(output, output + 7), std::format("{}", tree).substr(0,7));
}

TEST(styled_text_format_test, standard_tree_inherits_and_restores_style) {
  const auto outer = tjg::underline(tjg::bg(tjg::gray_color{12}));
  const auto tree = tjg::styled_text{"a"} +
      tjg::styled_text{"b", tjg::fg(tjg::term_color::red)} +
      tjg::absolute(tjg::bold("c")) + "d";
  auto red = outer; red.fg(tjg::term_color::red);
  check_states(std::format("{}", tjg::styled(tree, outer)), std::string{"abcd"},
               {outer, red, tjg::bold(), outer});
  EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
  const auto empty = tjg::absolute(tjg::styled_text{});
  EXPECT_EQ(std::format("{}", empty), "");
  EXPECT_EQ(std::format("{:.0}", tree), "");
  EXPECT_EQ(std::format("{:>4.0}", tree), "    ");
  auto width = -1;
  EXPECT_THROW((void)std::vformat("{:>{}}", std::make_format_args(tree, width)),
               std::format_error);
  EXPECT_EQ(tjg::get_terminal_state(), tjg::text_style{});
  for (const char* spec : {"{:08}", "{:+}", "{:.}", "{:d}", "{:s?}"}) {
    EXPECT_THROW((void)std::vformat(spec, std::make_format_args(tree)), std::format_error);
  }
}

namespace {
struct tree_throwing_output {
  using difference_type = std::ptrdiff_t;
  int* remaining = nullptr;
  auto operator*() -> tree_throwing_output& { return *this; }
  auto operator++() -> tree_throwing_output& { return *this; }
  auto operator++(int) -> tree_throwing_output { return *this; }
  auto operator=(char) -> tree_throwing_output& {
    if (--*remaining == 0) throw std::runtime_error("output failed");
    return *this;
  }
};
}
TEST(styled_text_format_test, standard_tree_output_failure_restores_state) {
  tjg::detail::style_scope scope{};
  const auto outer = tjg::underline();
  tjg::get_terminal_state() = outer;
  const auto tree = tjg::bold(std::string(4000, 'x'));
  int remaining = 20;
  EXPECT_THROW(std::format_to(tree_throwing_output{&remaining}, "{}", tree), std::runtime_error);
  EXPECT_EQ(tjg::get_terminal_state(), outer);
}
TEST(styled_text_format_test, legacy_named_dynamic_specs) {
  const auto tree = tjg::styled_text{"hello world"};
  EXPECT_EQ(fmt::format(FMT_STRING("{:>{width}.{prec}}"), tree,
                       fmt::arg("width", 9), fmt::arg("prec", 5)), "    hello");
}
