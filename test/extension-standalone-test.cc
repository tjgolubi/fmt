// This target deliberately has no {fmt} library or test harness dependency.
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include "fmt/backslash.h"
#include "fmt/color-ostream.h"
#include "fmt/parse_styled_text.h"
#include "fmt/strip_ansi.h"
#include "fmt/styled_text.h"

#ifdef FMT_VERSION
#  error "The extension must not include the fmt engine"
#endif

auto terminal_state_from_other_translation_unit() -> tjg::text_style&;

namespace {
void check(bool value) {
  if (!value) std::abort();
}
}  // namespace

int main() {
  const auto red = tjg::fg(tjg::term_color::red);
  check(&terminal_state_from_other_translation_unit() ==
        &tjg::get_terminal_state());
  const auto tree = tjg::bold("hello");
  const auto rendered = std::format("{}", tree);
  check(rendered == "\x1b[1mhello\x1b[0m");
  check(tjg::strip_ansi(rendered) == "hello");
  check(tjg::parse_styled_text(rendered).plain_text() == "hello");
  check(tjg::backslash_unescape(tjg::backslash_escape(rendered)) == rendered);
  check(tjg::format(red, "{} {}", 42, tree) ==
        "\x1b[31m42 \x1b[1mhello\x1b[22m\x1b[0m");
  auto output = std::string{};
  tjg::format_to(std::back_inserter(output), red, "{:04x}", 42);
  check(output == "\x1b[31m002a\x1b[0m");
  auto wide = std::wstring{};
  tjg::format_to(std::back_inserter(wide), red, L"{}", 42);
  check(wide == tjg::format(red, L"{}", 42));
  const int value = 42;
  check(tjg::vformat(red, "{}", std::make_format_args(value)) ==
        "\x1b[31m42\x1b[0m");
  std::ostringstream stream;
  tjg::println(stream, red, "{}", value);
  check(stream.str() == "\x1b[31m42\x1b[0m\n");
  std::wostringstream wide_stream;
  tjg::print(wide_stream, red, L"{}", value);
  check(wide_stream.str() == wide);
  stream.str("");
  stream << tree;
  check(stream.str() == rendered);
  auto file = std::tmpfile();
  check(file);
  tjg::println(file, red, "{}", value);
  std::rewind(file);
  char bytes[64]{};
  const auto count = std::fread(bytes, 1, sizeof(bytes), file);
  check(std::string(bytes, count) == "\x1b[31m42\x1b[0m\n");
  std::fclose(file);
  try {
    (void)tjg::vformat(red, "{", std::make_format_args());
    return 1;
  } catch (const std::format_error&) {
    check(tjg::get_terminal_state() == tjg::text_style{});
  }
  const auto utf8 = tjg::styled_text{"a\xe7\x95\x8c"};
  check(utf8.display_width() == 3);
  check(std::format("{:.2}", utf8) == "a");
}
