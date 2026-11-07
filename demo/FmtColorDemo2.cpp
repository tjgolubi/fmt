#include <tjg/color.h>
#include <tjg/styled_text.h>
#include <print>

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

  std::println("{}", line);

  auto line2 = "This is part1.  " + line + "  This is part2.";
  constexpr auto surrounding = fg(white).bg(gray_color{12}).strike();

  // Defaults preserve each part's explicit colors and absolute scopes.
  std::println("Underlay:");
  std::println("{}", underlay(surrounding, line2));

  // Overrides replace colors throughout relative parts; absolute scopes stay.
  std::println("Overlay:");
  std::println("{}", overlay(surrounding, line2));
}
