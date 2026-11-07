#include "tjg/color.h"

void test_text_style() {
  tjg::print(fg(tjg::rgb(255, 20, 30)), "{}", "rgb(255,20,30)");
  (void)tjg::format(fg(tjg::rgb(255, 20, 30)), "{}", "rgb(255,20,30)");

  tjg::text_style ts = fg(tjg::rgb(255, 20, 30));
  std::string out;
  tjg::format_to(std::back_inserter(out), ts, "rgb(255,20,30){}{}{}", 1, 2, 3);
}

int main() { test_text_style(); }
