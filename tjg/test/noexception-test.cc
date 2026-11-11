#include "tjg/styled_text.h"
#include "tjg/backslash.h"

int main() {
  const auto tree = tjg::bold("hello");
  const auto narrow = tjg::format(tjg::underline(), "{}", tree);
  const auto wide = tjg::format(tjg::bold(), L"{}", tjg::wstyled_text{L"wide"});
  return narrow.empty() || wide.empty() ||
         tjg::get_style_state() != tjg::text_style{};
}
