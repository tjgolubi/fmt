#include "standard-format-test.h"
// Formatting library for C++ - styled std::ostream tests
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "tjg/color-ostream.h"

#include <iomanip>
#include <sstream>

#include "test-support.h"

TEST(color_ostream_test, emphasis_insertion) {
  using tjg::emphasis;
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
      {emphasis::strikethrough, "{strike}"},
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
     << tjg::emphasis::bold << '!';
  EXPECT_EQ(os.str(), "**{bold}!");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.precision(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::left << std::setw(5) << tjg::emphasis{};
  EXPECT_EQ(os.str(), "**{bold}!{}***");
  std::wostringstream wide;
  wide << std::left << std::setfill(L'*') << std::setw(8)
       << tjg::emphasis::faint << L'!';
  EXPECT_EQ(wide.str(), L"{faint}*!");
  EXPECT_EQ(wide.width(), 0);
}

TEST(color_ostream_test, insertion) {
  std::ostringstream os;
  const auto style = tjg::text_style{tjg::emphasis::bold, tjg::rgb{1, 2, 3},
                                    tjg::term_color::black};
  const std::string text("a\0{}b", 5);
  auto& result = (os << tjg::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << ' ' << tjg::styled(42, tjg::emphasis::underline) << '!';
#if TJG_COLOR_COMPACT_SGR == 1
  const std::string opening = "\x1b[1;38;2;1;2;3;40m";
#else
  const std::string opening = "\x1b[1m\x1b[38;2;001;002;003m\x1b[40m";
#endif
  EXPECT_EQ(os.str(), opening + text + "\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_wide) {
  std::wostringstream os;
  const auto style = tjg::text_style{tjg::emphasis::bold, tjg::rgb{1, 2, 3},
                                    tjg::term_color::black};
  const std::wstring text(L"a\0{}b", 5);
  auto& result = (os << tjg::styled(text, style));
  EXPECT_EQ(&result, &os);
  result << L' ' << tjg::styled(42, tjg::emphasis::underline) << L'!';
#if TJG_COLOR_COMPACT_SGR == 1
  const std::wstring opening = L"\x1b[1;38;2;1;2;3;40m";
#else
  const std::wstring opening = L"\x1b[1m\x1b[38;2;001;002;003m\x1b[40m";
#endif
  EXPECT_EQ(os.str(), opening + text + L"\x1b[0m \x1b[4m42\x1b[0m!");
}

TEST(color_ostream_test, insertion_empty_style_and_text) {
  std::ostringstream os;
  os << tjg::styled("plain", tjg::text_style{})
     << tjg::styled("", tjg::text_style{});
  EXPECT_EQ(os.str(), "plain");
  os << tjg::styled("", tjg::emphasis::bold);
  EXPECT_EQ(os.str(), "plain\x1b[1m\x1b[0m");

  std::wostringstream wide;
  wide << tjg::styled(L"plain", tjg::text_style{})
       << tjg::styled(L"", tjg::text_style{})
       << tjg::styled(L"", tjg::emphasis::bold);
  EXPECT_EQ(wide.str(), L"plain\x1b[1m\x1b[0m");
}

TEST(color_ostream_test, insertion_stream_formatting) {
  std::ostringstream os;
  os << std::hex << std::setfill('*') << std::setw(4)
     << tjg::styled(42, tjg::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(os.width(), 0);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
  os << std::fixed << std::setprecision(2)
     << tjg::styled(1.25, tjg::emphasis::underline);
  EXPECT_EQ(os.str(), "\x1b[1m**2a\x1b[0m\x1b[4m1.25\x1b[0m");
  EXPECT_EQ(os.precision(), 2);

  std::wostringstream wide;
  wide << std::hex << std::setfill(L'*') << std::setw(4)
       << tjg::styled(42, tjg::emphasis::bold);
  EXPECT_EQ(wide.str(), L"\x1b[1m**2a\x1b[0m");
  EXPECT_EQ(wide.width(), 0);
  wide << std::setw(4) << tjg::styled(42, tjg::text_style{});
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
  os << tjg::styled(value, tjg::emphasis::bold);
  EXPECT_EQ(os.str(), "\x1b[1mcustom\x1b[0m");
}
TEST(color_ostream_test, wide_print_and_println) {
  std::wostringstream os;
  const auto style = tjg::text_style{tjg::emphasis::bold, tjg::rgb{1, 2, 3},
                                    tjg::term_color::black};
  tjg::print(os, style, L"Result: {:04x} {}", 42, L"done");
#if TJG_COLOR_COMPACT_SGR == 1
  EXPECT_EQ(os.str(), L"\x1b[1;38;2;1;2;3;40mResult: 002a done\x1b[0m");
#else
  EXPECT_EQ(os.str(),
            L"\x1b[1m\x1b[38;2;001;002;003m\x1b[40mResult: 002a done\x1b[0m");
#endif
  os.str(L"");
  tjg::println(os, tjg::emphasis::underline, L"{}", 42);
  EXPECT_EQ(os.str(), L"\x1b[4m42\x1b[0m\n");
  os.str(L"");
  tjg::print(os, tjg::text_style{}, L"{}", L"plain");
  tjg::println(os, tjg::text_style{}, L"{}", 42);
  EXPECT_EQ(os.str(), L"plain42\n");
  os.str(L"");
  tjg::print(os, style, L"");
  EXPECT_EQ(os.str(), tjg::to_sgr<wchar_t>(style) + L"\x1b[0m");
}

TEST(color_ostream_test, wide_argument_packs) {
  std::wostringstream os;
  int value = 42;
  auto args = std::make_wformat_args(value);
  tjg::vprint(os, tjg::emphasis::bold, L"{:04}", args);
  EXPECT_EQ(os.str(), L"\x1b[1m0042\x1b[0m");
  os.str(L"");
  tjg::vprintln(os, tjg::emphasis::bold, L"{}", args);
  EXPECT_EQ(os.str(), L"\x1b[1m42\x1b[0m\n");
}

TEST(color_ostream_test, wide_print_preserves_stream_formatting) {
  std::wostringstream os;
  os << std::hex << std::setprecision(2) << std::setfill(L'*') << std::setw(30);
  const std::wstring text(L"a\0{}b", 5);
  tjg::print(os, tjg::emphasis::bold, L"{} {}", text, 42);
  const wchar_t expected[] = L"\x1b[1ma\0{}b 42\x1b[0m";
  EXPECT_EQ(os.str(), std::wstring(expected, sizeof(expected) / sizeof(*expected) - 1));
  EXPECT_EQ(os.width(), 30);
  EXPECT_EQ(os.fill(), L'*');
  EXPECT_EQ(os.precision(), 2);
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, wide_formatting_error) {
  std::wostringstream os;
  os << L"original";
  EXPECT_THROW(tjg::vprint(os, tjg::emphasis::bold, L"{", std::make_wformat_args()),
               std::format_error);
  EXPECT_EQ(os.str(), L"original");
}

namespace {
struct failing_wstreambuf : std::wstreambuf {
  std::streamsize xsputn(const wchar_t*, std::streamsize) override { return 0; }
};
}  // namespace

TEST(color_ostream_test, wide_stream_failure) {
  failing_wstreambuf buffer;
  std::wostream os(&buffer);
  tjg::print(os, tjg::emphasis::bold, L"{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(tjg::println(os, tjg::emphasis::bold, L"{}", 42),
               std::ios_base::failure);
}
TEST(color_ostream_test, print) {
  std::ostringstream os;
  const auto style = tjg::text_style{tjg::emphasis::bold, tjg::rgb{1, 2, 3},
                                    tjg::term_color::black};
  tjg::print(os, style, "Result: {:04x} {}", 42, "done");
#if TJG_COLOR_COMPACT_SGR == 1
  EXPECT_EQ(os.str(), "\x1b[1;38;2;1;2;3;40mResult: 002a done\x1b[0m");
#else
  EXPECT_EQ(os.str(),
            "\x1b[1m\x1b[38;2;001;002;003m\x1b[40mResult: 002a done\x1b[0m");
#endif
  os << " plain";
  EXPECT_EQ(os.str().substr(os.str().size() - 10), "\x1b[0m plain");
}

TEST(color_ostream_test, println) {
  std::ostringstream os;
  tjg::println(os, tjg::emphasis::underline, "Result: {}", 42);
  EXPECT_EQ(os.str(), "\x1b[4mResult: 42\x1b[0m\n");
}

TEST(color_ostream_test, argument_packs) {
  std::ostringstream os;
  int value = 42;
  auto args = std::make_format_args(value);
  tjg::vprint(os, tjg::emphasis::bold, "{:04}", args);
  EXPECT_EQ(os.str(), "\x1b[1m0042\x1b[0m");
  os.str("");
  tjg::vprintln(os, tjg::emphasis::bold, "{}", args);
  EXPECT_EQ(os.str(), "\x1b[1m42\x1b[0m\n");
}

TEST(color_ostream_test, empty_style_and_text) {
  std::ostringstream os;
  tjg::print(os, tjg::text_style{}, "{}", "plain");
  tjg::println(os, tjg::text_style{}, "{}", 42);
  EXPECT_EQ(os.str(), "plain42\n");
  os.str("");
  tjg::print(os, tjg::text_style{}, "");
  EXPECT_EQ(os.str(), "");
  tjg::print(os, tjg::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m");
  os.str("");
  tjg::println(os, tjg::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m\n");
}

TEST(color_ostream_test, literal_text_and_stream_state) {
  std::ostringstream os;
  os.width(30);
  os.fill('*');
  os << std::hex;
  const char text[] = "a\0{}b";
  tjg::print(os, tjg::emphasis::bold, "{} {}",
             std::string_view(text, sizeof(text) - 1), 42);
  const char expected[] = "\x1b[1ma\0{}b 42\x1b[0m";
  EXPECT_EQ(os.str(), std::string(expected, sizeof(expected) - 1));
  EXPECT_EQ(os.width(), 30);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, long_output) {
  std::ostringstream os;
  const auto text = std::string(4096, 'x');
  tjg::print(os, tjg::emphasis::bold, "{}", text);
  EXPECT_EQ(os.str(), "\x1b[1m" + text + "\x1b[0m");
}

TEST(color_ostream_test, formatting_error) {
  std::ostringstream os;
  os << "original";
  EXPECT_THROW(tjg::vprint(os, tjg::emphasis::bold, "{", std::make_format_args()),
               std::format_error);
  EXPECT_EQ(os.str(), "original");
}

namespace {
struct failing_streambuf : std::streambuf {
  std::streamsize xsputn(const char*, std::streamsize) override { return 0; }
};
}  // namespace

TEST(color_ostream_test, stream_failure) {
  failing_streambuf buf;
  std::ostream os(&buf);
  tjg::print(os, tjg::emphasis::bold, "{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(tjg::println(os, tjg::emphasis::bold, "{}", 42),
               std::ios_base::failure);
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
    EXPECT_THROW(os << tjg::styled(value, tjg::emphasis::bold), insertion_error);
    EXPECT_EQ(buffer.attempts, 3);
    EXPECT_EQ(buffer.str(), fail_reset ? "\x1b[1mpartial"
                                       : "\x1b[1mpartial\x1b[0m");
    EXPECT_EQ(os.bad(), fail_reset);
    EXPECT_EQ(os.exceptions(), std::ios_base::badbit);
  }
}
#endif
