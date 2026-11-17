#include <tjg/color.hpp>

#include <print>
#include <sstream>

#ifdef FMT_VERSION
#  error "TJG must not include the fmt engine"
#endif
#if defined(_MSVC_LANG)
// /std:c++latest can report a C++20 draft value on MSVC. CMake also
// checks that the imported target explicitly requests cxx_std_23.
static_assert(_MSVC_LANG >= 202002L);
#else
static_assert(__cplusplus >= 202100L);
#endif

int main() {
  const auto hello = tjg::color::styled("hello", tjg::color::bold());
  const auto rendered = std::format("{} world", hello);
  if (rendered != "\x1b[1mhello\x1b[0m world") return 1;
  if (tjg::strip_ansi(rendered) != "hello world") return 2;
  if (tjg::backslash_unescape(tjg::backslash_escape(rendered)) != rendered)
    return 3;
  std::ostringstream output;
  output << hello << " world";
  if (output.str() != rendered) return 4;
  if (tjg::color::format(tjg::color::underline(), L"{}", L"wide") !=
      L"\x1b[4mwide\x1b[0m")
    return 5;
  std::println("{}", hello);
}
