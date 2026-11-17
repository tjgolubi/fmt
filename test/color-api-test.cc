#include "tjg/color.hpp"

void test_text_style() {
  tjg::color::print(fg(tjg::color::rgb(255, 20, 30)), "{}", "rgb(255,20,30)");
  (void)tjg::color::format(fg(tjg::color::rgb(255, 20, 30)), "{}", "rgb(255,20,30)");

  tjg::color::text_style ts = fg(tjg::color::rgb(255, 20, 30));
  std::string out;
  tjg::color::format_to(std::back_inserter(out), ts, "rgb(255,20,30){}{}{}", 1, 2, 3);
}

int main() { test_text_style(); }
