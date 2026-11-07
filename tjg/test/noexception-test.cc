#include "tjg/backslash.h"
#include "tjg/styled_text.h"

int main() {
  const auto tree = tjg::bold("hello");
  const auto narrow = tjg::format(tjg::underline(), "{}", tree);
  const auto wide = tjg::format(tjg::bold(), L"{}", tjg::wstyled_text{L"wide"});
  return narrow.empty() || wide.empty() ||
         tjg::get_terminal_state() != tjg::text_style{};
}
