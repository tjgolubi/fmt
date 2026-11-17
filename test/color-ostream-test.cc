// Formatting library for C++ - styled std::ostream tests
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "tjg/color.hpp"

#include "standard-format-test.h"
#include "test-support.h"

#include <print>
#include <sstream>
#include <locale>
#include <iomanip>

namespace {
template<typename Char>
void check_style_insertion() {
  using namespace tjg::color;
  const auto widen = [](const char* value) {
    std::basic_string<Char> result;
    while (*value) result.push_back(static_cast<Char>(*value++));
    return result;
  };
  std::basic_ostringstream<Char> os;
  os << std::left << std::setfill(Char{'*'}) << std::setprecision(3)
     << std::setw(5);
  const auto flags = os.flags();
  const auto style = fg(term_color::red).bold();
  EXPECT_EQ(&(os << style), &os);
  EXPECT_EQ(os.str(), widen("\x1b[1;31m"));
  EXPECT_EQ(os.width(), 5);
  EXPECT_EQ(os.flags(), flags);
  EXPECT_EQ(os.fill(), Char{'*'});
  EXPECT_EQ(os.precision(), 3);
  EXPECT_EQ(detail::get_style_state(), text_style{});
  os << widen("x");
  EXPECT_EQ(os.str(), widen("\x1b[1;31mx****"));
  os << std::setw(7) << text_style{};
  EXPECT_EQ(os.str(), widen("\x1b[1;31mx****\x1b[0m"));
  EXPECT_EQ(os.width(), 7);
  os << absolute(text_style{});
  EXPECT_EQ(os.str(), widen("\x1b[1;31mx****\x1b[0m\x1b[0m"));
}
} // namespace

TEST(color_ostream_test, text_style_insertion) {
  check_style_insertion<char>();
  check_style_insertion<wchar_t>();
}

TEST(color_ostream_test, emphasis_insertion) {
  using tjg::color::emphasis;
  const struct {
    emphasis value;
    const char* expected;
  } cases[] = {
      {emphasis{}, "{}"},
      {emphasis::bold, "{bold}"},
      {emphasis::faint, "{faint}"},
      {emphasis::italic, "{italic}"},
      {emphasis::underline, "{underline}"},
      {emphasis::blink, "{blink}"},
      {emphasis::reverse, "{reverse}"},
      {emphasis::conceal, "{conceal}"},
      {emphasis::strike, "{strike}"},
      {emphasis::strike | emphasis::bold | emphasis::underline,
       "{bold underline strike}"},
      {static_cast<emphasis>(255),
       "{bold faint italic underline blink reverse conceal strike}"}};
  for (const auto& test : cases) {
    std::ostringstream os;
    EXPECT_EQ(&(os << test.value), &os);
    EXPECT_EQ(os.str(), test.expected);
    const auto expected = std::string{test.expected};
    std::wostringstream wide;
    EXPECT_EQ(&(wide << test.value), &wide);
    EXPECT_EQ(wide.str(), std::wstring(expected.begin(), expected.end()));
  }
}

TEST(color_ostream_test, emphasis_insertion_stream_settings) {
  std::ostringstream os;
  os << std::hex << std::setprecision(0) << std::setfill('*') << std::setw(8)
     << tjg::color::emphasis::bold << '!';
  EXPECT_EQ(os.str(), "**{bold}!");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.precision(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::left << std::setw(5) << tjg::color::emphasis{};
  EXPECT_EQ(os.str(), "**{bold}!{}***");
  std::wostringstream wide;
  wide << std::left << std::setfill(L'*') << std::setw(8)
       << tjg::color::emphasis::faint << L'!';
  EXPECT_EQ(wide.str(), L"{faint}*!");
  EXPECT_EQ(wide.width(), 0);
}

TEST(color_ostream_test, insertion) {
  std::ostringstream os;
  const auto style = tjg::color::text_style{tjg::color::emphasis::bold, tjg::color::rgb{1, 2, 3},
                                    tjg::color::term_color::black};
  const std::string text("a\0{}b", 5);
  auto& result = (os << tjg::color::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << ' ' << tjg::color::styled(42, tjg::color::emphasis::underline) << '!';
  const std::string opening = "\x1b[1;38;2;1;2;3;40m";
  EXPECT_EQ(os.str(), opening + text + "\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_wide) {
  std::wostringstream os;
  const auto style = tjg::color::text_style{tjg::color::emphasis::bold, tjg::color::rgb{1, 2, 3},
                                    tjg::color::term_color::black};
  const std::wstring text(L"a\0{}b", 5);
  auto& result = (os << tjg::color::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << L' ' << tjg::color::styled(42, tjg::color::emphasis::underline) << L'!';
  const std::wstring opening = L"\x1b[1;38;2;1;2;3;40m";
  EXPECT_EQ(os.str(), opening + text + L"\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_empty_style_and_text) {
  std::ostringstream os;
  os << tjg::color::styled("plain", tjg::color::text_style{})
     << tjg::color::styled("", tjg::color::text_style{});
  EXPECT_EQ(os.str(), "plain");
  os << tjg::color::styled("", tjg::color::emphasis::bold);
  EXPECT_EQ(os.str(), "plain\x1b[1m\x1b[0m");

  std::wostringstream wide;
  wide << tjg::color::styled(L"plain", tjg::color::text_style{})
       << tjg::color::styled(L"", tjg::color::text_style{})
       << tjg::color::styled(L"", tjg::color::emphasis::bold);
  EXPECT_EQ(wide.str(), L"plain\x1b[1m\x1b[0m");
}

TEST(color_ostream_test, insertion_stream_formatting) {
  std::ostringstream os;
  os << std::hex << std::setfill('*') << std::setw(4)
     << tjg::color::styled(42, tjg::color::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::fixed << std::setprecision(2)
     << tjg::color::styled(1.25, tjg::color::emphasis::underline);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m\x1b[4m1.25\x1b[0m");
  EXPECT_EQ(os.precision(), 2);

  std::wostringstream wide;
  wide << std::hex << std::setfill(L'*') << std::setw(4)
       << tjg::color::styled(42, tjg::color::emphasis::bold);
  EXPECT_EQ(wide.str(), L"\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(wide.width(), 0);
  wide << std::setw(4) << tjg::color::styled(42, tjg::color::text_style{});
  EXPECT_EQ(wide.str(), L"\x1b[1m**2a\x1b[0m**2a");
}

namespace {
struct stream_only_value {};

auto operator<<(std::ostream& os, const stream_only_value&) -> std::ostream& {
  return os << "custom";
}
}  // namespace

TEST(color_ostream_test, insertion_stream_only_value) {
  std::ostringstream os;
  const stream_only_value value{};
  os << tjg::color::styled(value, tjg::color::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1mcustom\x1b[0m");
}
TEST(color_ostream_test, print) {
  std::ostringstream os;
  const auto style = tjg::color::text_style{tjg::color::emphasis::bold, tjg::color::rgb{1, 2, 3},
                                    tjg::color::term_color::black};
  tjg::color::print(os, style, "Result: {:04x} {}", 42, "done");
  EXPECT_EQ(os.str(), "\x1b[1;38;2;1;2;3;40mResult: 002a done\x1b[0m");
  os << " plain";
  EXPECT_EQ(os.str().substr(os.str().size() - 10), "\x1b[0m plain");
}

TEST(color_ostream_test, println) {
  std::ostringstream os;
  tjg::color::println(os, tjg::color::emphasis::underline, "Result: {}", 42);
  EXPECT_EQ(os.str(), "\x1b[4mResult: 42\x1b[0m\n");
}

TEST(color_ostream_test, argument_packs) {
  std::ostringstream os;
  int value = 42;
  auto args = std::make_format_args(value);
  tjg::color::vprint_nonunicode(os, tjg::color::emphasis::bold, "{:04}", args);
  EXPECT_EQ(os.str(), "\x1b[1m0042\x1b[0m");
  os.str("");
  tjg::color::vprint_unicode(os, tjg::color::emphasis::bold, "{}\n", args);
  EXPECT_EQ(os.str(), "\x1b[1m42\n\x1b[0m");
}

TEST(color_ostream_test, localized_runtime_print) {
  struct punctuation : std::numpunct<char> {
    auto do_thousands_sep() const -> char override { return '_'; }
    auto do_grouping() const -> std::string override { return "\3"; }
  };
  auto os = std::ostringstream{};
  os.imbue(std::locale{std::locale::classic(), new punctuation});
  auto value = 1234;
  auto args = std::make_format_args(value);
  tjg::color::vprint_nonunicode(os, tjg::color::bold(), "{:L}", args);
  tjg::color::vprint_unicode(os, tjg::color::bold(), "{:L}", args);
  EXPECT_EQ(os.str(), "\x1b[1m1_234\x1b[0m\x1b[1m1_234\x1b[0m");
}

TEST(color_ostream_test, empty_style_and_text) {
  std::ostringstream os;
  tjg::color::print(os, tjg::color::text_style{}, "{}", "plain");
  tjg::color::println(os, tjg::color::text_style{}, "{}", 42);
  EXPECT_EQ(os.str(), "plain42\n");
  os.str("");
  tjg::color::print(os, tjg::color::text_style{}, "");
  EXPECT_EQ(os.str(), "");
  tjg::color::print(os, tjg::color::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m");
  os.str("");
  tjg::color::println(os, tjg::color::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m\n");
}

TEST(color_ostream_test, literal_text_and_stream_state) {
  std::ostringstream os;
  std::ostringstream plain;
  os << std::hex << std::setfill('*') << std::setw(30);
  plain.copyfmt(os);
  const char text[] = "a\0{}b";
  const auto value = std::string_view(text, sizeof(text) - 1);
  std::print(plain, "{} {}", value, 42);
  tjg::color::print(os, tjg::color::emphasis::bold, "{} {}", value, 42);
  EXPECT_EQ(os.str(), "\x1b[1m" + plain.str() + "\x1b[0m");
  EXPECT_EQ(os.width(), plain.width());
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, empty_style_matches_std_print) {
  const auto check = []<typename... T>(std::format_string<const T&...> fmt,
                                      const T&... values) {
    for (auto alignment : {std::ios_base::left, std::ios_base::right}) {
      SCOPED_TRACE(fmt.get());
      SCOPED_TRACE(alignment == std::ios_base::left ? "left" : "right");
      std::ostringstream plain;
      std::ostringstream styled;
      plain.setf(alignment, std::ios_base::adjustfield);
      plain << std::setfill('*') << std::setw(30);
      styled.copyfmt(plain);

      std::print(plain, fmt, values...);
      tjg::color::print(styled, tjg::color::text_style{}, fmt, values...);
      EXPECT_EQ(styled.str(), plain.str());
      EXPECT_EQ(styled.width(), plain.width());

      plain << '!';
      styled << '!';
      EXPECT_EQ(styled.str(), plain.str());
    }
  };

  const auto text = std::string{"ready"};
  check("{:04}", 42);
  check("{:.2f}", 1.25f);
  check("[{}]", text);
  check("Result: {:04} {:.2f} [{}]", 42, 1.25f, text);
}

TEST(color_ostream_test, long_output) {
  std::ostringstream os;
  const auto text = std::string(4096, 'x');
  tjg::color::print(os, tjg::color::emphasis::bold, "{}", text);
  EXPECT_EQ(os.str(), "\x1b[1m" + text + "\x1b[0m");
}

TEST(color_ostream_test, formatting_error) {
  std::ostringstream os;
  os << "original";
  EXPECT_THROW(tjg::color::vprint_nonunicode(os, tjg::color::emphasis::bold, "{", std::make_format_args()),
               std::format_error);
  EXPECT_EQ(os.str(), "original\x1b[1m\x1b[0m");
  EXPECT_EQ(tjg::color::detail::get_style_state(), tjg::color::text_style{});
}

namespace {
struct failing_streambuf : std::streambuf {
  std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
};
}  // namespace

TEST(color_ostream_test, stream_failure) {
  failing_streambuf buf;
  std::ostream os(&buf);
  tjg::color::print(os, tjg::color::emphasis::bold, "{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(tjg::color::println(os, tjg::color::emphasis::bold, "{}", 42),
               std::ios_base::failure);
}

TEST(color_ostream_test, text_style_insertion_failure) {
  failing_streambuf buf;
  std::ostream os(&buf);
  os << std::setw(5) << tjg::color::text_style{};
  EXPECT_TRUE(os.bad());
  EXPECT_EQ(os.width(), 5);
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(os << tjg::color::bold(), std::ios_base::failure);
  EXPECT_EQ(os.width(), 5);
}

#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
namespace {
struct insertion_error {};
struct reset_error {};

struct recovery_streambuf : std::stringbuf {
  bool fail = false;
  int attempts = 0;

  std::streamsize xsputn(const char* text, std::streamsize size) override {
    ++attempts;
    if (fail) throw reset_error{};
    return std::stringbuf::xsputn(text, size);
  }
};

struct throwing_value {
  recovery_streambuf& buffer;
  bool fail_reset;
};

auto operator<<(std::ostream& os, const throwing_value& value) -> std::ostream& {
  os << "partial";
  value.buffer.fail = value.fail_reset;
  throw insertion_error{};
}
}  // namespace

TEST(color_ostream_test, insertion_exception_reset) {
  for (bool fail_reset : {false, true}) {
    SCOPED_TRACE(fail_reset);
    recovery_streambuf buffer;
    std::ostream os(&buffer);
    os.exceptions(std::ios_base::badbit);
    const throwing_value value{buffer, fail_reset};
    EXPECT_THROW(os << tjg::color::styled(value, tjg::color::emphasis::bold), insertion_error);
    EXPECT_EQ(buffer.attempts, 3);
    EXPECT_EQ(buffer.str(), fail_reset ? "\x1b[1mpartial"
                                       : "\x1b[1mpartial\x1b[0m");
    EXPECT_EQ(os.bad(), fail_reset);
    EXPECT_EQ(os.exceptions(), std::ios_base::badbit);
  }
}
#endif
