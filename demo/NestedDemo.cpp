#include <fmt/color.h>
#include <tjg/color.hpp>

#include <print>

int main() {
  // C++23 with raw SGR sequences.
  {
    std::println("\x1b[4mThe result is \x1b[31m{}\x1b[39m bytes.\x1b[0m", 42);
  }

  // C++23 with named SGR sequences.
  {
    constexpr char Underline[] = "\x1b[4m";
    constexpr char Red[]       = "\x1b[31m";
    constexpr char Reset[]     = "\x1b[0m";

    std::println("{}The result is {}{}{}{} bytes.{}",
                 Underline, Red, 42, Reset, Underline, Reset);
  }

  // {fmt}: the red argument resets the underline before the suffix.
  {
    const auto line = fmt::emphasis::underline;
    fmt::print(line, "The result is {} bytes.\n",
               fmt::styled(42, fmt::fg(fmt::terminal_color::red)));
  }

  // {fmt}: explicitly reapply the underline to the suffix.
  {
    const auto line = fmt::emphasis::underline;
    fmt::print(line, "The result is {}{}\n",
               fmt::styled(42, fmt::fg(fmt::terminal_color::red)),
               fmt::styled(" bytes.", line));
  }

  // {color}: the enclosing underline resumes after the red argument.
  {
    tjg::color::println(tjg::color::underline(), "The result is {} bytes.",
                 tjg::color::styled(42, tjg::color::fg(tjg::color::term_color::red)));
  }
} // main
