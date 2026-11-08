#include "tjg/backslash.h"
#include "tjg/strip_ansi.h"

#include <type_traits>
#include <utility>

#include "test-support.h"

namespace {
#ifdef TJG_TEST_ALLOCATION_NOEXCEPT
constexpr bool allocation_noexcept = true;
#else
constexpr bool allocation_noexcept = false;
#endif

template <typename Char> constexpr bool check_policy() {
  using text = tjg::basic_styled_text<Char>;
  using string = std::basic_string<Char>;
  static_assert(std::is_nothrow_constructible_v<text, string> ==
                allocation_noexcept);
  static_assert(std::is_nothrow_copy_constructible_v<text> ==
                allocation_noexcept);
  static_assert(std::is_nothrow_copy_assignable_v<text> == allocation_noexcept);
  static_assert(noexcept(tjg::to_sgr<Char>(tjg::text_style{})) ==
                allocation_noexcept);
  static_assert(noexcept(tjg::to_sgr<Char>(tjg::text_style{},
                                         tjg::text_style{})) ==
                allocation_noexcept);
  static_assert(noexcept(tjg::strip_ansi(std::declval<text>())) ==
                allocation_noexcept);
  // Existing unconditional guarantees stay intact.
  static_assert(std::is_nothrow_move_constructible_v<text>);
  static_assert(std::is_nothrow_move_assignable_v<text>);
  // Decoding and unbounded growth retain their other exception paths.
  static_assert(!noexcept(tjg::backslash_unescape(
      std::basic_string_view<Char>{})));
  static_assert(!noexcept(std::declval<const text&>().plain_text()));
  static_assert(!noexcept(std::declval<text&>() += std::declval<text>()));
  static_assert(!noexcept(tjg::wrap_sgr<Char>(tjg::text_style{},
                                           std::basic_string_view<Char>{})));
  return true;
}
static_assert(check_policy<char>());
static_assert(check_policy<wchar_t>());
}  // namespace

TEST(allocation_noexcept_test, copy_and_strip_tree) {
  auto original = tjg::styled_text{std::string{"left\x1b[31m"}, tjg::bold()};
  original += tjg::styled_text{std::string{"right"}, tjg::underline()};
  auto copy = tjg::styled_text{original};
  tjg::styled_text assigned;
  assigned = copy;
  EXPECT_EQ(assigned, original);
  const auto stripped = tjg::strip_ansi(std::move(assigned));
  EXPECT_EQ(stripped.plain_text(), "leftright");
  EXPECT_EQ(original.plain_text(), "left\x1b[31mright");
}

TEST(allocation_noexcept_test, bounded_sgr) {
  const auto style = tjg::bold().fg(tjg::color::red);
  EXPECT_FALSE(tjg::to_sgr(style).empty());
  EXPECT_FALSE(tjg::to_sgr<wchar_t>(style, {}).empty());
}

TEST(allocation_noexcept_test, decoding_errors_still_throw) {
  EXPECT_THROW((void)tjg::backslash_unescape("\\xzz"), std::format_error);
}
