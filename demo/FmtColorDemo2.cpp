#include <fmt/color.h>
#include <fmt/styled_text.h>
#include <fmt/format.h>

int main() {
  using namespace tjg;
  using enum tjg::term_color;

  // Starting with two styled children creates an unstyled parent. Appending
  // later spans then preserves their own colors and the plain separators.
  auto red_text = underlay(fg(red), "red text with "
      + bold("nested bold") + " still red");

  auto line = red_text
      + " | " + underlay(bg(blue), "blue background")
      + " | " + absolute(underline("reset+underline only"))
      + " | " + reverse(overlay(fg(red), "reverse red"));

  fmt::println("{}", line);

  auto line2 = "This is part1.  " + line + "  This is part2.";
  constexpr auto surrounding = fg(white).bg(gray_color{12}).strike();

  // Defaults preserve each part's explicit colors and absolute scopes.
  fmt::println("Underlay:");
  fmt::println("{}", underlay(surrounding, line2));

  // Overrides replace colors throughout relative parts; absolute scopes stay.
  fmt::println("Overlay:");
  fmt::println("{}", overlay(surrounding, line2));
}
