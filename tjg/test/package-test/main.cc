#include <tjg/backslash.h>
#include <tjg/color-ostream.h>
#include <tjg/parse_styled_text.h>
#include <tjg/strip_ansi.h>
#include <tjg/styled_text.h>

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
static_assert(TJG_COLOR_COMPACT_SGR == TJG_EXPECTED_COMPACT_SGR);

int main() {
  const auto tree = tjg::bold("hello") + " world";
  const auto rendered = std::format("{}", tree);
  if (rendered != "\x1b[1mhello\x1b[0m world") return 1;
  if (tjg::strip_ansi(rendered) != "hello world") return 2;
  if (tjg::parse_styled_text(rendered).plain_text() != "hello world") return 3;
  if (tjg::backslash_unescape(tjg::backslash_escape(rendered)) != rendered)
    return 4;
  std::ostringstream output;
  output << tree;
  if (output.str() != rendered) return 5;
  if (tjg::format(tjg::underline(), L"{}", tjg::wstyled_text{L"wide"}) !=
      L"\x1b[4mwide\x1b[0m")
    return 6;
  std::println("{}", tree);
}
