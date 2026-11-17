#include <tjg/color.hpp>

#include <print>

int main() {
  using namespace tjg::color;
  using enum tjg::color::term_color;
  using tjg::color::styled;

  std::println("{} {} {} | {} | {} | {}",
            styled("red text with", fg(red)),
            styled("nested bold", fg(red).bold()),
            styled("still red", fg(red)),
            styled("blue background", bg(blue)),
            styled("reset+underline only", text_style{}.absolute().underline()),
            styled("reverse red", fg(red).reverse()));

  constexpr auto surrounding = fg(white).bg(gray_color{12}).strike();

  tjg::color::println(surrounding, "{} {} {} | {} | {} | {}",
            styled("red text with", fg(red)),
            styled("nested bold", fg(red).bold()),
            styled("still red", fg(red)),
            styled("blue background", bg(blue)),
            styled("reset+underline only", text_style{}.absolute().underline()),
            styled("reverse red", fg(red).reverse()));
} // main
