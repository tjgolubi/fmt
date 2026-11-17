#include "tjg/color.hpp"
#include "standard-format-test.h"
#include "test-support.h"

#include <sstream>
#include <format>
#include <iomanip>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace {
template<typename Char, typename... Styles>
auto sgr(const Styles&... styles) -> std::basic_string<Char> {
  const auto narrow = tjg::color::to_sgr(styles...);
  return {narrow.begin(), narrow.end()};
}

auto render(const tjg::color::styled_text& value) -> std::string {
  return std::format("{}", value);
}
auto render(const tjg::color::wstyled_text& value) -> std::wstring {
  return std::format(L"{}", value);
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
                  const std::vector<tjg::color::text_style>& expected) {
  ASSERT_EQ(text.size(), expected.size());
  auto state = tjg::color::text_style{};
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
        case 39: state.fg(tjg::color::no_color); break;
        case 49: state.bg(tjg::color::no_color); break;
        case 38: case 48: {
          ASSERT_LT(p + 1, parameters.size());
          const auto mode = parameters[++p];
          auto color = tjg::color::color_t{};
          if (mode == 2) {
            ASSERT_LT(p + 3, parameters.size());
            color = tjg::color::rgb{static_cast<int>(parameters[p + 1]),
                             static_cast<int>(parameters[p + 2]),
                             static_cast<int>(parameters[p + 3])};
            p += 3;
          } else {
            ASSERT_EQ(mode, 5u);
            ASSERT_LT(p + 1, parameters.size());
            const auto index = parameters[++p];
            if (index >= 232)
              color = tjg::color::gray_color{static_cast<int>(index - 232)};
            else {
              ASSERT_GE(index, 16u);
              color = tjg::color::cube_color{static_cast<int>(index - 16)};
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
          if (background) state.bg(static_cast<tjg::color::term_color>(index));
          else state.fg(static_cast<tjg::color::term_color>(index));
        }
      }
    }
  }
  EXPECT_EQ(position, text.size());
  EXPECT_EQ(state, tjg::color::text_style{});
}

template<typename Char>
void check_nested_formatting() {
  using node = tjg::color::basic_styled_text<Char>;
  const auto outer = tjg::color::text_style{tjg::color::rgb{1, 2, 3}, tjg::color::gray_color{12},
                                     tjg::color::emphasis::bold};
  auto inner_style = tjg::color::text_style{tjg::color::term_color::green, tjg::color::no_color,
                                    tjg::color::emphasis::italic};
  auto inner = node(chars<Char>("b")) +
               node(chars<Char>("c"), tjg::color::underline());
  inner.style(inner_style);
  auto tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  const auto saved = tree;
  auto b = outer;
  b.fg(tjg::color::term_color::green).italic();
  auto c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});
  EXPECT_EQ(tree, saved);

  // An absolute group drops outer colors/emphasis, its relative children
  // inherit from that group, and a later sibling resumes the outer state.
  inner_style = tjg::color::absolute(inner_style);
  inner = tjg::color::absolute(inner);
  tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  b = tjg::color::text_style{tjg::color::term_color::green, tjg::color::no_color,
                       tjg::color::emphasis::italic};
  c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});

  auto plain_absolute = tjg::color::text_style{};
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
  // The absolute root resets even from assumed defaults.
  const auto expected = sgr<Char>(absolute_outer) + chars<Char>("ab") +
                        sgr<Char>(tjg::color::text_style{}, outer);
  EXPECT_EQ(render(tree), expected);

  // Equal complete states still honor an explicit absolute child boundary.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), absolute_outer);
  tree.style(outer);
  const auto boundary = sgr<Char>(outer, tjg::color::text_style{}) +
      chars<Char>("a") + sgr<Char>(absolute_outer) + chars<Char>("b") +
      sgr<Char>(tjg::color::text_style{}, outer);
  EXPECT_EQ(render(tree), boundary);
}
} // namespace

TEST(styled_text_format_test, plain_empty_and_output_iterators) {
  EXPECT_EQ(render(tjg::color::styled_text{}), "");
  EXPECT_EQ(render(tjg::color::styled_text{"", tjg::color::bold()}), "");
  EXPECT_EQ(render(tjg::color::styled_text{"literal {}"}), "literal {}");
  EXPECT_EQ(std::format("{:}", tjg::color::styled_text{"plain"}), "plain");
  EXPECT_EQ(render(tjg::color::wstyled_text{L"wide"}), L"wide");
  EXPECT_EQ(std::format("before{}after{}", tjg::color::bold("x"), 42),
            "before\x1b[1mx\x1b[0mafter42");
  auto output = std::vector<char>{'p'};
  std::format_to(std::back_inserter(output), "{}!", tjg::color::bold("x"));
  EXPECT_EQ((std::string{output.begin(), output.end()}), "p\x1b[1mx\x1b[0m!");
  const auto binary = std::string{"a\0{}b", 5};
  EXPECT_EQ(render(tjg::color::styled_text{binary}), binary);
  const auto long_text = std::string(2000, 'x');
  EXPECT_EQ(render(tjg::color::bold(long_text)), "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(std::format("before{}after", tjg::color::bold("x")),
            "before\x1b[1mx\x1b[0mafter");
  const auto raw = std::string{"\x1b[4mraw\x1b[24m"};
  EXPECT_EQ(render(tjg::color::styled_text{raw}), raw);
}

TEST(styled_text_format_test, nested_inheritance_and_absolute_boundaries) {
  check_nested_formatting<char>();
  check_nested_formatting<wchar_t>();
}

namespace {
template<typename Char, typename... T>
auto format_in_style(tjg::color::text_style style, const char* pattern, const T&... args)
    -> std::basic_string<Char> {
  auto buffer = std::basic_string<Char>{};
  const auto text = chars<Char>(pattern);
  tjg::color::vformat_to(std::back_inserter(buffer), style,
      std::basic_string_view<Char>{text},
      std::make_format_args<tjg::color::detail::format_context<Char>>(args...));
  return buffer;
}

template<typename Char>
void check_context_styles() {
  const auto outer = tjg::color::underline(tjg::color::bg(tjg::color::gray_color{12}));
  const auto red = tjg::color::fg(tjg::color::term_color::bright_red);
  auto inherited = outer;
  inherited.fg(tjg::color::term_color::bright_red);
  const auto number = 42;
  const auto value = tjg::color::styled(number, red);
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

  const auto absolute = tjg::color::styled(number, tjg::color::absolute(red));
  check_states(format_in_style<Char>(outer, "a{}z", absolute),
               chars<Char>("a42z"), {outer, red, red, outer});
  const auto clear = tjg::color::styled(number, tjg::color::absolute(tjg::color::text_style{}));
  check_states(format_in_style<Char>(outer, "a{}z", clear),
               chars<Char>("a42z"), {outer, {}, {}, outer});
  check_states(format_in_style<Char>(outer, "a{}z", tjg::color::styled(42, {})),
               chars<Char>("a42z"), {outer, outer, outer, outer});

  const auto nested = tjg::color::styled(value, tjg::color::bold());
  const auto bold_red = tjg::color::bold(inherited);
  check_states(format_in_style<Char>(outer, "a{}z", nested),
               chars<Char>("a42z"), {outer, bold_red, bold_red, outer});

  using node = tjg::color::basic_styled_text<Char>;
  auto tree = tjg::color::overlay(red, node{chars<Char>("x")}) +
              tjg::color::absolute(tjg::color::bold(node{chars<Char>("y")}));
  check_states(format_in_style<Char>(outer, "a{}z{}", tree, 7),
               chars<Char>("axyz7"), {outer, inherited, tjg::color::bold(), outer, outer});
  check_states(format_in_style<Char>(outer, "a{:>4.1}z", tree),
               chars<Char>("a   xz"),
               {outer, outer, outer, outer, inherited, outer});
  check_states(format_in_style<Char>(outer, "a{:>2.0}z", tree),
               chars<Char>("a  z"), {outer, outer, outer, outer});
  check_states(format_in_style<Char>(outer, "a{}z", node{}),
               chars<Char>("az"), {outer, outer});
  check_states(format_in_style<Char>(tjg::color::absolute(outer), "a{}z", tree),
               chars<Char>("axyz"), {outer, inherited, tjg::color::bold(), outer});

  const auto wrapped = tjg::color::styled(tree, tjg::color::italic());
  check_states(format_in_style<Char>(outer, "a{}z", wrapped),
               chars<Char>("axyz"),
               {outer, tjg::color::italic(inherited), tjg::color::bold(), outer});
}
} // namespace

struct color_context_nested {};
// A volatile condition keeps the normal return path reachable to the compiler.
struct color_context_throwing { volatile bool should_throw = true; };
struct terminal_state_nested {};

namespace std {
template<> struct formatter<color_context_nested> : formatter<string_view> {
  template<typename Context>
  auto format(color_context_nested, Context& ctx) const
      -> decltype(ctx.out()) {
    const auto text = std::format("a{}z", tjg::color::styled(42, tjg::color::fg(tjg::color::term_color::red)));
    return formatter<string_view>::format(text, ctx);
  }
};
template<> struct formatter<terminal_state_nested> : formatter<int> {
  template<typename Context>
  auto format(terminal_state_nested, Context& ctx) const -> decltype(ctx.out()) {
    return std::format_to(ctx.out(), "a{}z",
                         tjg::color::styled(42, tjg::color::fg(tjg::color::term_color::red)));
  }
};
template<> struct formatter<color_context_throwing> : formatter<int> {
  template<typename Context>
  auto format(const color_context_throwing& value, Context& ctx) const
      -> decltype(ctx.out()) {
    if (value.should_throw) throw format_error("color context test");
    return ctx.out();
  }
};
}

TEST(styled_text_format_test, nested_buffer_context_inherits_style) {
  const auto outer = tjg::color::underline();
  const auto red = tjg::color::underline(tjg::color::fg(tjg::color::term_color::red));
  check_states(tjg::color::format(outer, "{}", color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  // Nonzero outer width creates a separate buffer context. Its custom child
  // must still inherit underline, even though the complete text exceeds width.
  check_states(tjg::color::format(outer, "{:>1}", color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  const auto nested = color_context_nested{};
  const auto wrapped = tjg::color::styled(nested, outer);
  check_states(std::format("{:>1}", wrapped),
               std::string{"a42z"}, {outer, red, red, outer});
}

TEST(styled_text_format_test, separate_format_call_inherits_shared_state) {
  const auto outer = tjg::color::underline();
  const auto red = tjg::color::underline(tjg::color::fg(tjg::color::term_color::red));
  check_states(tjg::color::format(outer, "{}", terminal_state_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  EXPECT_EQ(tjg::color::detail::get_style_state(), tjg::color::text_style{});
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  EXPECT_THROW((void)tjg::color::format(outer, "{}",
                                tjg::color::styled(color_context_throwing{}, red)),
               std::format_error);
  EXPECT_EQ(tjg::color::detail::get_style_state(), tjg::color::text_style{});
#endif
}

TEST(styled_text_format_test, terminal_state_is_shared_and_exception_safe) {
  tjg::color::detail::style_scope scope{};
  auto& terminal = tjg::color::detail::get_style_state();
  EXPECT_EQ(terminal, tjg::color::text_style{});
  const auto outer = tjg::color::underline(tjg::color::bg(tjg::color::term_color::blue));
  terminal = outer;
  const auto number = 42;
  auto inner = tjg::color::styled(number, tjg::color::fg(tjg::color::term_color::red));
  (void)std::format("{}", inner);
  EXPECT_EQ(&tjg::color::detail::get_style_state(), &terminal);
  const auto wide_text = tjg::color::styled(number, tjg::color::fg(tjg::color::term_color::red));
  EXPECT_FALSE(std::format(L"{}", wide_text).empty());
  EXPECT_EQ(tjg::color::detail::get_style_state(), outer);
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  const auto value = color_context_throwing{};
  auto styled = tjg::color::styled(value, tjg::color::bold());
  EXPECT_THROW((void)std::format("{}", styled), std::format_error);
  EXPECT_EQ(tjg::color::detail::get_style_state(), outer);
#endif
}

TEST(styled_text_format_test, enclosing_format_context) {
  check_context_styles<char>();
  check_context_styles<wchar_t>();
  const auto outer = tjg::color::underline();
  const auto red = tjg::color::fg(tjg::color::term_color::bright_red);
  const auto expected = std::vector<tjg::color::text_style>{
      outer, tjg::color::underline(red), tjg::color::underline(red), outer};
  check_states(tjg::color::format(outer, "a{}z", tjg::color::styled(42, red)),
               std::string{"a42z"}, expected);
  check_states(tjg::color::format(outer, "a{}z",
                          tjg::color::overlay(red, std::to_string(42))),
               std::string{"a42z"}, expected);
  std::ostringstream os{};
  tjg::color::print(os, outer, "a{}z", tjg::color::styled(42, red));
  check_states(os.str(), std::string{"a42z"}, expected);
  std::wostringstream wide{};
  wide << tjg::color::format(outer, L"a{:}z", tjg::color::styled(42, red)) << L'\n';
  check_states(wide.str(), std::wstring{L"a42z\n"},
               {outer, tjg::color::underline(red), tjg::color::underline(red), outer, {}});
  EXPECT_WRITE(stdout, tjg::color::print(outer, "a{}z",
                                 tjg::color::styled(42, red)),
      format_in_style<char>(outer, "a{}z", tjg::color::styled(42, red)));
  auto output = std::vector<char>{};
  tjg::color::format_to(std::back_inserter(output), outer, "a{}z",
                 tjg::color::overlay(red, "42"));
  check_states(std::string{output.begin(), output.end()}, std::string{"a42z"},
               expected);
  // Independent operations begin at defaults, even after styled output.
  EXPECT_EQ(std::format("{}", 42), "42");
}

TEST(styled_text_format_test, compiled_nested_style_and_pointer_output) {
  const auto inner = tjg::color::bold("x");
  const auto wrapped = tjg::color::styled(inner, tjg::color::underline());
  const auto both = tjg::color::underline(tjg::color::bold());
  check_states(std::format("a{}z", wrapped),
               std::string{"axz"}, {{}, both, {}});
  auto buffer = std::string(128, '\0');
  const auto end = std::format_to(&buffer[0], "a{:>3}z", wrapped);
  buffer.resize(static_cast<size_t>(end - &buffer[0]));
  check_states(buffer, std::string{"a  xz"},
               {{}, both, both, both, {}});
}

TEST(styled_text_format_test, unsupported_specifications) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  const auto value = tjg::color::bold("text");
  EXPECT_THROW((void)runtime_format("{:+}", value), std::format_error);
#endif
}

TEST(styled_text_format_test, stream_insertion_options) {
  const auto tree = tjg::color::bold(tjg::color::styled_text{"ab"} +
                             tjg::color::styled_text{"cdefgh", tjg::color::fg(tjg::color::term_color::red)});
  const auto bold_red = tjg::color::bold(tjg::color::fg(tjg::color::term_color::red));
  for (auto alignment : {std::ios_base::left, std::ios_base::right,
                         std::ios_base::internal, std::ios_base::fmtflags{}}) {
    std::ostringstream os;
    os.setf(alignment, std::ios_base::adjustfield);
    os << std::hex << std::setprecision(4) << std::setfill('*') << std::setw(11);
    const auto flags = os.flags();
    auto& result = (os << tree);
    EXPECT_EQ(&result, &os);
    if (alignment == std::ios_base::left) {
      check_states(os.str(), std::string{"abcdefgh***"},
                   {tjg::color::bold(), tjg::color::bold(), bold_red, bold_red,
                    bold_red, bold_red, bold_red, bold_red,
                    tjg::color::bold(), tjg::color::bold(), tjg::color::bold()});
    } else {
      check_states(os.str(), std::string{"***abcdefgh"},
                   {tjg::color::bold(), tjg::color::bold(), tjg::color::bold(), tjg::color::bold(),
                    tjg::color::bold(), bold_red, bold_red, bold_red, bold_red,
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
               {tjg::color::bold(), tjg::color::bold(), bold_red, bold_red, bold_red, bold_red,
                bold_red, bold_red});
  std::ostringstream zero;
  zero << std::setprecision(0) << std::setfill('*') << std::setw(3) << tree;
  check_states(zero.str(), std::string{"abcdefgh"},
               {tjg::color::bold(), tjg::color::bold(), bold_red, bold_red, bold_red, bold_red,
                bold_red, bold_red});
  EXPECT_EQ(zero.precision(), 0);
  std::ostringstream empty;
  empty << std::setfill('*') << std::setw(3) << tjg::color::bold("");
  check_states(empty.str(), std::string{"***"},
               {tjg::color::bold(), tjg::color::bold(), tjg::color::bold()});
}

TEST(styled_text_format_test, stream_insertion_wide_and_utf8) {
  const auto style = tjg::color::bold();
  std::wostringstream wide;
  wide << std::left << std::setfill(L'\u2603') << std::setprecision(2)
       << std::setw(5) << tjg::color::bold(tjg::color::wstyled_text{L"abcd"});
  check_states(wide.str(), std::wstring{L"abcd\u2603"},
               {style, style, style, style, style});
  EXPECT_EQ(wide.width(), 0);
  EXPECT_EQ(wide.fill(), L'\u2603');
  const auto tree = tjg::color::bold(tjg::color::styled_text{"\xe6\x9d\xb1\xe4\xba\xac"} + "abc");
  std::ostringstream utf8;
  utf8 << std::left << std::setprecision(3) << std::setw(9) << tree;
  EXPECT_EQ(utf8.str(), std::format("{:<9}", tree));
  check_states(utf8.str(), std::string{"\xe6\x9d\xb1\xe4\xba\xac" "abc  "},
               {style, style, style, style, style, style, style, style, style,
                style, style});
  std::ostringstream absolute;
  const auto protected_tree = tjg::color::bold(
      tjg::color::styled_text{"a"} + tjg::color::absolute(tjg::color::underline("b")) + "c");
  absolute << protected_tree;
  check_states(absolute.str(), std::string{"abc"},
               {style, tjg::color::underline(), style});
}

TEST(styled_text_format_test, stream_insertion_failure) {
  std::ostringstream os;
  os.setstate(std::ios_base::failbit);
  os << std::setw(3) << tjg::color::bold("text");
  EXPECT_EQ(os.str(), "");
  EXPECT_EQ(os.width(), 3);
  std::wostringstream wide;
  wide.setstate(std::ios_base::failbit);
  wide << tjg::color::bold(L"text");
  EXPECT_EQ(wide.str(), L"");
  struct failing_buffer : std::streambuf {
    std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
  } buffer;
  std::ostream failing{&buffer};
  failing << tjg::color::bold("text");
  EXPECT_TRUE(failing.bad());
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  failing.clear();
  failing.exceptions(std::ios_base::badbit);
  EXPECT_THROW(failing << tjg::color::bold("text"), std::ios_base::failure);
#endif
}

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
TEST(styled_text_format_test, stream_insertion_buffer_exception) {
  struct write_error {};
  struct throwing_buffer : std::streambuf {
    int_type overflow(int_type) override { throw write_error{}; }
    std::streamsize xsputn(const char*, std::streamsize) override {
      throw write_error{};
    }
  } buffer;
  std::ostream os{&buffer};
  const auto text = tjg::color::bold("text");

  EXPECT_NO_THROW(os << text);
  EXPECT_TRUE(os.bad());
  EXPECT_TRUE(tjg::color::detail::get_style_state().empty());

  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(os << text, write_error);
  EXPECT_TRUE(os.bad());
  EXPECT_TRUE(tjg::color::detail::get_style_state().empty());
}
#endif

TEST(styled_text_format_test, all_parent_child_emphasis_pairs) {
  const auto colors = tjg::color::text_style{tjg::color::term_color::red,
                                      tjg::color::cube_color{1, 2, 3}};
  for (unsigned parent = 0; parent < 256; ++parent) {
    auto enclosing = colors;
    enclosing.emph(static_cast<tjg::color::emphasis>(parent));
    for (unsigned child = 0; child < 256; ++child) {
      auto relative = tjg::color::text_style{static_cast<tjg::color::emphasis>(child)};
      auto absolute = relative;
      absolute.absolute();
      auto tree = tjg::color::styled_text{"a"} + tjg::color::styled_text{"b", relative} +
                  tjg::color::styled_text{"c", absolute} + tjg::color::styled_text{"d"};
      tree.style(enclosing);
      auto inherited = colors;
      inherited.emph(static_cast<tjg::color::emphasis>(parent | child));
      check_states(render(tree), std::string{"abcd"},
                   {enclosing, inherited, relative, enclosing});
    }
  }
}

namespace {
template<typename Char>
void check_string_specifications() {
  using node = tjg::color::basic_styled_text<Char>;
  const char* specifications[] = {
      "{}", "{:s}", "{:8}", "{:>8}", "{:_<8}", "{:*^9}",
      "{:.0}", "{:.1}", "{:.3}", "{:.20}", "{:>8.3s}", "{:*^8.0}"};
  const auto plain = chars<Char>("abcdef");
  const auto tree = node{chars<Char>("ab")} + node{chars<Char>("cde")} +
                    node{chars<Char>("f")};
  for (const auto spec : specifications) {
    const auto pattern = chars<Char>(spec);
    EXPECT_EQ(runtime_format(pattern, tree),
              runtime_format(pattern, plain));
    EXPECT_EQ(runtime_format(pattern, node{}),
              runtime_format(pattern, std::basic_string<Char>{}));
  }
  EXPECT_EQ(runtime_format(chars<Char>("{:>{}.{}}"), tree, 8, 3),
            runtime_format(chars<Char>("{:>{}.{}}"), plain, 8, 3));
  const auto root = tjg::color::fg(tjg::color::term_color::red);
  const auto child = tjg::color::bold(root);
  const auto absolute = tjg::color::underline();
  auto styled = node{chars<Char>("ab"), tjg::color::bold()} +
      tjg::color::absolute(node{chars<Char>("cd"), absolute}) +
      node{chars<Char>("ef")};
  styled.style(root);
  const auto saved = styled;
  check_states(runtime_format(chars<Char>("{:*^8.3}"), styled),
      chars<Char>("**abc***"), {root, root, child, child, absolute, root, root, root});
  check_states(runtime_format(chars<Char>("{:>3.0}"), styled),
      chars<Char>("   "), {root, root, root});
  check_states(runtime_format(chars<Char>("{:>3}"), node{std::basic_string<Char>{}, root}),
      chars<Char>("   "), {root, root, root});
  EXPECT_EQ(styled, saved);
}
} // namespace

TEST(styled_text_format_test, string_specifications) {
  check_string_specifications<char>();
  check_string_specifications<wchar_t>();
  EXPECT_EQ(std::format("{:>6.3}", tjg::color::styled_text{"abcdef"}),
            "   abc");
  auto output = std::vector<char>{};
  std::format_to(std::back_inserter(output), "{:>6.3}",
                 tjg::color::styled_text{"abcdef"});
  EXPECT_EQ((std::string{output.begin(), output.end()}), "   abc");
  EXPECT_EQ(std::format("{0:>{1}.{2}}", tjg::color::styled_text{"abcdef"}, 6, 3),
            "   abc");
}

TEST(styled_text_format_test, unicode_precision) {
  struct unicode_case {
    std::string text;
    int precision;
    std::string prefix;
    std::size_t width;
  };
  // Explicit code-point expectations retain our width contract independently
  // of the standard library's grapheme-aware string formatter.
  const unicode_case cases[] = {
      {"caf\xc3\xa9", 3, "caf", 3},
      {"caf\xc3\xa9", 4, "caf\xc3\xa9", 4},
      {"\xe7\x95\x8c!", 1, "", 0},
      {"\xe7\x95\x8c!", 2, "\xe7\x95\x8c", 2},
      {"\xe7\x95\x8c!", 3, "\xe7\x95\x8c!", 3},
      {"\xf0\x9f\x98\x80!", 1, "", 0},
      {"\xf0\x9f\x98\x80!", 2, "\xf0\x9f\x98\x80", 2},
      {"e\xcc\x81", 1, "e", 1},
      {"e\xcc\x81", 2, "e\xcc\x81", 2},
      {"\xff!", 1, "\xff", 1},
      {std::string{"a\0b", 3}, 2, std::string{"a\0", 2}, 2}};
  for (const auto& sample : cases) {
    const auto tree = tjg::color::styled_text{"x"} + tjg::color::styled_text{sample.text} + "later";
    const auto padding = std::size_t{14} - 1 - sample.width;
    const auto expected = std::string(padding / 2, '*') + "x" + sample.prefix +
                          std::string(padding - padding / 2, '*');
    EXPECT_EQ(std::format("{:*^14.{}}", tree, sample.precision + 1), expected);
  }
  // A wide code point that cannot fit stops the traversal, including later leaves.
  const auto tree = tjg::color::styled_text{"a"} + "\xe7\x95\x8c" + "b";
  EXPECT_EQ(std::format("{:.2}", tree), "a");
  EXPECT_EQ(std::format("{:\xc2\xb7^5.1}", tree),
            std::format("{:\xc2\xb7^5.1}", "a"));
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  EXPECT_THROW((void)runtime_format("{:>{}}", tree, -1), std::format_error);
  EXPECT_THROW((void)runtime_format("{:.{}}", tree, -1), std::format_error);
#endif
}

namespace {
template<typename Char>
void check_deferred_transitions() {
  using node = tjg::color::basic_styled_text<Char>;
  const auto root = tjg::color::fg(tjg::color::term_color::red);
  const auto child = tjg::color::bold(root);
  const auto inner = tjg::color::underline(child);
  auto tree = tjg::color::overlay(root, tjg::color::bold(
      tjg::color::underline(node{chars<Char>("abc")}) + chars<Char>("later")) +
      chars<Char>("tail"));
  const auto opening = sgr<Char>(inner, tjg::color::text_style{});
  const auto reset = sgr<Char>(tjg::color::text_style{}, inner);
  EXPECT_EQ(runtime_format(chars<Char>("{:.1}"), tree),
            opening + chars<Char>("a") + reset);
  const auto padded = opening + chars<Char>("a") +
      sgr<Char>(root, inner) + chars<Char>("  ") +
      sgr<Char>(tjg::color::text_style{}, root);
  EXPECT_EQ(runtime_format(chars<Char>("{:<3.1}"), tree), padded);
  EXPECT_EQ(runtime_format(chars<Char>("{:.0}"), tree),
            std::basic_string<Char>{});

  // The enclosing group never prints: only its absolute child needs an entry.
  const auto absolute = tjg::color::absolute(tjg::color::underline());
  const auto group = tjg::color::overlay(root, tjg::color::absolute(tjg::color::underline(
      node{chars<Char>("abc")})) + chars<Char>("later"));
  auto relative = absolute;
  relative.absolute(false);
  EXPECT_EQ(runtime_format(chars<Char>("{:.1}"), group),
      sgr<Char>(absolute, tjg::color::text_style{}) + chars<Char>("a") +
      sgr<Char>(tjg::color::text_style{}, relative));
}
} // namespace

TEST(styled_text_format_test, deferred_transitions) {
  check_deferred_transitions<char>();
  check_deferred_transitions<wchar_t>();
}


namespace {
template <typename Char> void check_defaults_and_overrides() {
  using node = tjg::color::basic_styled_text<Char>;
  using tc = tjg::color::term_color;
  const auto defaults = tjg::color::fg(tc::white).bg(tc::blue);
  const auto yellow = tjg::color::fg(tc::yellow).bg(tc::blue);
  const auto white = defaults;
  auto protected_group = node{chars<Char>("C"), tjg::color::fg(tc::red)}
      + node{chars<Char>("D")};
  protected_group.style(tjg::color::absolute(tjg::color::text_style{}));
  const auto text = node{chars<Char>("A"), tjg::color::fg(tc::yellow)}
      + node{chars<Char>("B")} + protected_group;
  check_states(render(tjg::color::underlay(defaults, text)), chars<Char>("ABCD"),
               {yellow, white, tjg::color::fg(tc::red), tjg::color::text_style{}});
  check_states(render(tjg::color::overlay(defaults, text)), chars<Char>("ABCD"),
               {white, white, tjg::color::fg(tc::red), tjg::color::text_style{}});
  const auto leaf = node{chars<Char>("A"), tjg::color::fg(tc::yellow)};
  const auto branch = leaf + node{chars<Char>("B")};
  check_states(runtime_format(chars<Char>("{:.1}"),
                          tjg::color::underlay(defaults, leaf)), chars<Char>("A"), {yellow});
  check_states(runtime_format(chars<Char>("{:.1}"),
                          tjg::color::underlay(defaults, branch)), chars<Char>("A"), {yellow});
}
}  // namespace
TEST(styled_text_format_test, defaults_and_recursive_overrides) {
  check_defaults_and_overrides<char>();
  check_defaults_and_overrides<wchar_t>();
}

TEST(styled_text_format_test, standard_tree_specs_and_debug) {
  const auto tree = tjg::color::bold(tjg::color::styled_text{"hello "} +
      tjg::color::styled_text{"world", tjg::color::fg(tjg::color::term_color::blue)});
  const auto child_open = std::string{"\x1b[34m"};
  const auto child_close = std::string{"\x1b[39m"};
  const auto rendered = "\x1b[1mhello " + child_open + "world\x1b[0m";
  EXPECT_EQ(std::format("{}", tree), rendered);
  EXPECT_EQ(std::format("{:s}", tree), rendered);
  EXPECT_EQ(std::format("{:>15}", tree), "\x1b[1m    hello " + child_open + "world\x1b[0m");
  EXPECT_EQ(std::format("{:*^15.9}", tree),
            "\x1b[1m***hello " + child_open + "wor" + child_close + "***\x1b[0m");
  EXPECT_EQ(std::format("{:.0}", tree), "");
  EXPECT_EQ(std::format("{:*>8.0}", tree), "\x1b[1m********\x1b[0m");
  EXPECT_EQ(std::format("{:?}", tree), "b {\n  \"hello \"\n  blue \"world\"\n}\n");
  EXPECT_EQ(std::format("{:>{}.{}s} {}", tree, 12, 9, 42),
            "\x1b[1m   hello " + child_open + "wor\x1b[0m 42");
  EXPECT_THROW((void)runtime_format("{0:*^{1}.{2}?}", tree, 15, 9),
               std::format_error);
  const auto wide = tjg::color::underline(tjg::color::wstyled_text{L"wide"} + L" tree");
  EXPECT_EQ(std::format(L"{:*^15.6}", wide), L"\x1b[4m****wide t*****\x1b[0m");
  EXPECT_EQ(std::format(L"{:?}", wide), L"_ {\n  \"wide\"\n  \" tree\"\n}\n");
  EXPECT_EQ(std::formatted_size("{}", tree), std::format("{}", tree).size());
  char output[256] = {};
  const auto end = std::format_to(output, "{:*>12.9}", tree);
  EXPECT_EQ(std::string(output, end), std::format("{:*>12.9}", tree));
  const auto result = std::format_to_n(output, 7, "{}", tree);
  EXPECT_EQ(result.size, static_cast<std::ptrdiff_t>(std::formatted_size("{}", tree)));
  EXPECT_EQ(std::string(output, output + 7), std::format("{}", tree).substr(0,7));
}

TEST(styled_text_format_test, standard_tree_inherits_and_restores_style) {
  const auto outer = tjg::color::underline(tjg::color::bg(tjg::color::gray_color{12}));
  const auto tree = tjg::color::styled_text{"a"} +
      tjg::color::styled_text{"b", tjg::color::fg(tjg::color::term_color::red)} +
      tjg::color::absolute(tjg::color::bold("c")) + "d";
  auto red = outer; red.fg(tjg::color::term_color::red);
  check_states(std::format("{}", tjg::color::styled(tree, outer)), std::string{"abcd"},
               {outer, red, tjg::color::bold(), outer});
  EXPECT_EQ(tjg::color::detail::get_style_state(), tjg::color::text_style{});
  const auto empty = tjg::color::absolute(tjg::color::styled_text{});
  EXPECT_EQ(std::format("{}", empty), "");
  EXPECT_EQ(std::format("{:.0}", tree), "");
  EXPECT_EQ(std::format("{:>4.0}", tree), "    ");
  auto width = -1;
  EXPECT_THROW((void)std::vformat("{:>{}}", std::make_format_args(tree, width)),
               std::format_error);
  EXPECT_EQ(tjg::color::detail::get_style_state(), tjg::color::text_style{});
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
  tjg::color::detail::style_scope scope{};
  const auto outer = tjg::color::underline();
  tjg::color::detail::get_style_state() = outer;
  const auto tree = tjg::color::bold(std::string(4000, 'x'));
  int remaining = 20;
  EXPECT_THROW(std::format_to(tree_throwing_output{&remaining}, "{}", tree), std::runtime_error);
  EXPECT_EQ(tjg::color::detail::get_style_state(), outer);
}
TEST(styled_text_format_test, named_dynamic_specs_are_rejected) {
  const auto tree = tjg::color::styled_text{"hello world"};
  EXPECT_THROW((void)runtime_format("{:>{width}.{prec}}", tree, 9, 5),
               std::format_error);
}
