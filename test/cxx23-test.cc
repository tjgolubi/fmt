// Standard-engine regression tests. These checks remain active in Release.
#include "fmt/color.h"

//#include "fmt/chrono.h"
//#include "fmt/ranges.h"
//#include "fmt/std.h"
//#include "fmt/xchar.h"

#include <array>
#include <vector>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>

void check(bool condition, const char* message) {
  if (!condition) throw std::runtime_error(message);
}
struct point { int x; };
template <> struct std::formatter<point> : std::formatter<int> {
  auto format(const point& value, std::format_context& ctx) const {
    return std::formatter<int>::format(value.x, ctx);
  }
};
int main() {
  try {
    check(fmt::format("{} {:#x} {:.2f}", "hello", 42, 1.25) ==
          "hello 0x2a 1.25", "basic formatting");
    check(fmt::format("{1} {0:0{2}}", 7, "x", 3) == "x 007", "dynamic width");
    check(fmt::format("{:04}", point{7}) == "0007", "standard custom formatter");
    check(fmt::format(fmt::runtime("{:.{}f}"), 1.25, 1) == "1.2", "runtime formatting");
    int number = 42;
    auto arguments = fmt::make_format_args(number);
    check(fmt::vformat("{}", arguments) == "42", "argument-store lifetime");
    bool threw = false;
    try { (void)fmt::format(fmt::runtime("{:d}"), "bad"); }
    catch (const fmt::format_error&) { threw = true; }
    check(threw, "invalid runtime format must throw");
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "{}", 42);
    check(fmt::to_string(buffer) == "42", "memory buffer");
    char truncated[2]{};
    auto result = fmt::format_to_n(truncated, 2, "{}", 12345);
    check(result.size == 5 && result.out == truncated + 2 &&
          std::string(truncated, 2) == "12", "truncation and total size");
    char bounded[2]{};
    auto bounded_result = fmt::format_to(bounded, "{}", 12345);
    check(bounded_result.truncated && bounded_result.out == bounded + 2 &&
          std::string(bounded, 2) == "12", "bounded character-array output");
    threw = false;
    try { char* ignored = bounded_result; (void)ignored; }
    catch (const fmt::format_error&) { threw = true; }
    check(threw, "bounded result must report truncation");
    auto runtime_result = fmt::format_to_n(bounded, 2, fmt::runtime("{}"), 12345);
    check(runtime_result.size == 5 && std::string(bounded, 2) == "12", "runtime truncation");
    check(fmt::format(L"{}", fmt::styled(L"x", fmt::emphasis::bold)) ==
          L"\x1b[1mx\x1b[0m", "wide styled value");
    check(fmt::formatted_size("{}", 12345) == 5, "formatted size");
    check(fmt::format(L"{}", 42) == L"42", "wide formatting");
    check(fmt::format(fmt::runtime(std::wstring_view(L"{}")), 42) == L"42",
          "wide runtime formatting");
    const auto ts = fmt::emphasis::bold | fmt::fg(fmt::color::blue);
    check(fmt::format(ts, "{}", "x") == "\x1b[1m\x1b[38;2;000;000;255mx\x1b[0m",
          "whole-string styling");
    check(fmt::format(fmt::text_style{}, "{}", "x") == "x", "empty style");
    check(fmt::format("{:04}", fmt::styled(point{7}, ts)) ==
          "\x1b[1m\x1b[38;2;000;000;255m0007\x1b[0m", "styled custom formatter");
    check(fmt::format("{}", fmt::styled("red", fmt::fg(fmt::color::red))) ==
          "\x1b[38;2;255;000;000mred\x1b[0m", "styled array");
    check(fmt::format("A{}Z", fmt::styled(7, fmt::emphasis::bold)) ==
          "A\x1b[1m7\x1b[0mZ", "style iterator position");
    check(std::format("{}", fmt::styled(7, fmt::emphasis::bold)) ==
          "\x1b[1m7\x1b[0m", "direct standard-engine integration");
    check(fmt::format(fmt::fg(fmt::terminal_color::bright_green), "x") ==
          "\x1b[92mx\x1b[0m", "terminal foreground");
    check(fmt::format(fmt::bg(fmt::terminal_color::bright_magenta), "x") ==
          "\x1b[105mx\x1b[0m", "terminal background");
    threw = false;
    try { (void)(fmt::fg(fmt::terminal_color::red) | fmt::fg(fmt::color::blue)); }
    catch (const fmt::format_error&) { threw = true; }
    check(threw, "conflicting terminal style");
    std::vector<int> values{1, 2, 3};
    check(fmt::format("{}", values) == "[1, 2, 3]", "standard range formatting");
    FILE* file = std::tmpfile();
    check(file != nullptr, "temporary output file");
    fmt::print(file, "{}", 42);
    fmt::println(file, fmt::emphasis::bold, "{}", "x");
    fmt::println(file, fmt::runtime("{}"), 7);
    std::rewind(file);
    char output[64]{};
    auto count = std::fread(output, 1, sizeof(output), file);
    std::fclose(file);
    check(std::string(output, count) == "42\x1b[1mx\x1b[0m\n7\n", "FILE printing");
    std::cout << "C++23 standard-engine regressions passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
