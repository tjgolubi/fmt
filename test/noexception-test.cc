#include "tjg/color.hpp"

int main() {
  const auto value = tjg::color::styled("hello", tjg::color::bold());
  const auto narrow = tjg::color::format(tjg::color::underline(), "{}", value);
  const auto wide = tjg::color::format(tjg::color::bold(), L"{}", L"wide");
  return narrow != "\x1b[4m\x1b[1mhello\x1b[22m\x1b[0m" ||
         wide != L"\x1b[1mwide\x1b[0m" ||
         tjg::color::detail::get_style_state() != tjg::color::text_style{};
}
