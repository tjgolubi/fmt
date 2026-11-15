#include <tjg/text_style.hpp>
#include <tjg/text_style-format.hpp>

#include <print>

int main() {
  using namespace tjg;
  using enum tjg::term_color;
  using tjg::styled;

  std::println("{} {} {} | {} | {} | {}",
            styled("red text with", fg(red)),
            styled("nested bold", fg(red).bold()),
            styled("still red", fg(red)),
            styled("blue background", bg(blue)),
            styled("reset+underline only", text_style{}.absolute().underline()),
            styled("reverse red", fg(red).reverse()));

  tjg::println(emphasis::strike, "{} {} {} | {} | {} | {}",
            styled("red text with", fg(red)),
            styled("nested bold", fg(red).bold()),
            styled("still red", fg(red)),
            styled("blue background", bg(blue)),
            styled("reset+underline only", text_style{}.absolute().underline()),
            styled("reverse red", fg(red).reverse()));
} // main
