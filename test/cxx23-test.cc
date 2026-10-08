// Color/style integration regressions. Checks remain active in Release.
#include "fmt/color.h"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

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
    const auto ts = fmt::emphasis::bold | fmt::fg(fmt::color::blue);
    point value{7};
    auto styled_value = fmt::styled(value, ts);
    check(std::format("{:04}", styled_value) ==
          "\x1b[1m\x1b[38;2;000;000;255m0007\x1b[0m", "styled custom formatter");
    check(std::format("{:{}}", fmt::styled(7, fmt::emphasis::bold), 3) ==
          "\x1b[1m  7\x1b[0m", "dynamic width on styled value");
    check(std::format("{}", fmt::styled("red", fmt::fg(fmt::color::red))) ==
          "\x1b[38;2;255;000;000mred\x1b[0m", "styled string literal");
    check(std::format("A{}Z", fmt::styled(7, fmt::emphasis::bold)) ==
          "A\x1b[1m7\x1b[0mZ", "style iterator position");
    check(std::format(L"{}", fmt::styled(L"x", fmt::emphasis::bold)) ==
          L"\x1b[1mx\x1b[0m", "wide styled value");
    check(std::format("{}", fmt::styled("x", fmt::text_style{})) == "x",
          "empty styled value");
    check(fmt::format(ts, "{}", "x") ==
          "\x1b[1m\x1b[38;2;000;000;255mx\x1b[0m", "whole-output styling");
    check(fmt::format(fmt::text_style{}, "{}", "x") == "x", "empty whole-output style");
    std::string output;
    fmt::format_to(std::back_inserter(output), fmt::emphasis::bold, "{:04}", 7);
    check(output == "\x1b[1m0007\x1b[0m", "styled output iterator");
    output.clear();
    std::format_to(std::back_inserter(output), "A{}Z", fmt::styled(7, fmt::emphasis::bold));
    check(output == "A\x1b[1m7\x1b[0mZ", "standard output iterator");
    int number = 42;
    auto arguments = std::make_format_args(number);
    check(fmt::vformat(fmt::emphasis::bold, "{}", arguments) ==
          "\x1b[1m42\x1b[0m", "runtime whole-output styling");
    output.clear();
    fmt::vformat_to(std::back_inserter(output), fmt::emphasis::bold, "{}", arguments);
    check(output == "\x1b[1m42\x1b[0m", "runtime styled output iterator");
    bool threw = false;
    try { (void)fmt::vformat(ts, "{:s}", arguments); }
    catch (const std::format_error&) { threw = true; }
    check(threw, "invalid runtime format must throw");
    auto file = std::unique_ptr<std::FILE, decltype(&std::fclose)>(std::tmpfile(), &std::fclose);
    check(file != nullptr, "temporary output file");
    std::print(file.get(), "{}", fmt::styled(42, fmt::emphasis::bold));
    fmt::print(file.get(), fmt::emphasis::bold, "{}", 7);
    fmt::println(file.get(), fmt::emphasis::bold, "{}", "x");
    fmt::vprint(file.get(), fmt::emphasis::bold, "{}", arguments);
    fmt::vprintln(file.get(), fmt::emphasis::bold, "{}", arguments);
    std::rewind(file.get());
    char bytes[128]{};
    const auto count = std::fread(bytes, 1, sizeof(bytes), file.get());
    check(std::string(bytes, count) ==
          "\x1b[1m42\x1b[0m\x1b[1m7\x1b[0m\x1b[1mx\x1b[0m\n"
          "\x1b[1m42\x1b[0m\x1b[1m42\x1b[0m\n", "standard and styled FILE printing");
    std::cout << "Color/style integration regressions passed\n";
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
