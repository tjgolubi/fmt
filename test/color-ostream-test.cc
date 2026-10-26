// Formatting library for C++ - styled std::ostream tests
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/color-ostream.h"

#include <sstream>

#include "gtest-extra.h"

TEST(color_ostream_test, print) {
  std::ostringstream os;
  const auto style = fmt::text_style{fmt::emphasis::bold, fmt::rgb{1, 2, 3},
                                    fmt::term_color::black};
  fmt::print(os, style, "Result: {:04x} {}", 42, "done");
  EXPECT_EQ(os.str(),
            "\x1b[1m\x1b[38;2;001;002;003m\x1b[40mResult: 002a done\x1b[0m");
  fmt::print(os, " plain");
  EXPECT_EQ(os.str().substr(os.str().size() - 10), "\x1b[0m plain");
}

TEST(color_ostream_test, println) {
  std::ostringstream os;
  fmt::println(os, fmt::emphasis::underline, "Result: {}", 42);
  EXPECT_EQ(os.str(), "\x1b[4mResult: 42\x1b[0m\n");
}

TEST(color_ostream_test, argument_packs) {
  std::ostringstream os;
  int value = 42;
  auto args = fmt::make_format_args(value);
  fmt::vprint(os, fmt::emphasis::bold, "{:04}", args);
  EXPECT_EQ(os.str(), "\x1b[1m0042\x1b[0m");
  os.str("");
  fmt::vprintln(os, fmt::emphasis::bold, "{}", args);
  EXPECT_EQ(os.str(), "\x1b[1m42\x1b[0m\n");
}

TEST(color_ostream_test, empty_style_and_text) {
  std::ostringstream os;
  fmt::print(os, fmt::text_style{}, "{}", "plain");
  fmt::println(os, fmt::text_style{}, "{}", 42);
  EXPECT_EQ(os.str(), "plain42\n");
  os.str("");
  fmt::print(os, fmt::text_style{}, "");
  EXPECT_EQ(os.str(), "");
  fmt::print(os, fmt::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m");
  os.str("");
  fmt::println(os, fmt::emphasis::bold, "");
  EXPECT_EQ(os.str(), "\x1b[1m\x1b[0m\n");
}

TEST(color_ostream_test, literal_text_and_stream_state) {
  std::ostringstream os;
  os.width(30);
  os.fill('*');
  os << std::hex;
  const char text[] = "a\0{}b";
  fmt::print(os, fmt::emphasis::bold, "{} {}",
             fmt::string_view(text, sizeof(text) - 1), 42);
  const char expected[] = "\x1b[1ma\0{}b 42\x1b[0m";
  EXPECT_EQ(os.str(), std::string(expected, sizeof(expected) - 1));
  EXPECT_EQ(os.width(), 30);
  EXPECT_EQ(os.fill(), '*');
  EXPECT_EQ(os.flags() & std::ios_base::basefield, std::ios_base::hex);
}

TEST(color_ostream_test, long_output) {
  std::ostringstream os;
  const auto text = std::string(4096, 'x');
  fmt::print(os, fmt::emphasis::bold, "{}", text);
  EXPECT_EQ(os.str(), "\x1b[1m" + text + "\x1b[0m");
}

TEST(color_ostream_test, formatting_error) {
  std::ostringstream os;
  os << "original";
  EXPECT_THROW(fmt::print(os, fmt::emphasis::bold, fmt::runtime("{")),
               fmt::format_error);
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
  fmt::print(os, fmt::emphasis::bold, "{}", 42);
  EXPECT_TRUE(os.bad());
  os.clear();
  os.exceptions(std::ios_base::badbit);
  EXPECT_THROW(fmt::println(os, fmt::emphasis::bold, "{}", 42),
               std::ios_base::failure);
}
