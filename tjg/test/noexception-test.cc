#include "tjg/text_style-format.hpp"

int main() {
  const auto value = tjg::styled("hello", tjg::bold());
  const auto narrow = tjg::format(tjg::underline(), "{}", value);
  const auto wide = tjg::format(tjg::bold(), L"{}", L"wide");
  return narrow != "\x1b[4m\x1b[1mhello\x1b[22m\x1b[0m" ||
         wide != L"\x1b[1mwide\x1b[0m" ||
         tjg::detail::get_style_state() != tjg::text_style{};
}
