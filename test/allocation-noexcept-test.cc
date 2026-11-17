#include "tjg/color.hpp"
#include "test-support.h"

#include <type_traits>
#include <utility>

namespace {
#ifdef TJG_TEST_ALLOCATION_NOEXCEPT
constexpr bool allocation_noexcept = true;
#else
constexpr bool allocation_noexcept = false;
#endif

template <typename Char> constexpr bool check_policy() {
  static_assert(noexcept(tjg::color::to_sgr(tjg::color::text_style{})) ==
                allocation_noexcept);
  static_assert(noexcept(tjg::color::to_sgr(tjg::color::text_style{},
                                         tjg::color::text_style{})) ==
                allocation_noexcept);
  static_assert(!noexcept(tjg::backslash_unescape(
      std::basic_string_view<Char>{})));
  static_assert(!noexcept(tjg::color::wrap_sgr<Char>(tjg::color::text_style{},
                                           std::basic_string_view<Char>{})));
  return true;
}
static_assert(check_policy<char>());
static_assert(check_policy<wchar_t>());
}  // namespace

TEST(allocation_noexcept_test, bounded_sgr) {
  const auto style = tjg::color::bold().fg(tjg::color::rgb::red);
  EXPECT_FALSE(tjg::color::to_sgr(style).empty());
  EXPECT_FALSE(tjg::color::to_sgr(style, {}).empty());
}

TEST(allocation_noexcept_test, decoding_errors_still_throw) {
  EXPECT_THROW((void)tjg::backslash_unescape("\\xzz"), std::invalid_argument);
}

template <typename Char> constexpr bool check_tree_policy() {
  using text = tjg::color::basic_styled_text<Char>;
  using string = std::basic_string<Char>;
  static_assert(std::is_nothrow_constructible_v<text, string> ==
                allocation_noexcept);
  static_assert(std::is_nothrow_copy_constructible_v<text> ==
                allocation_noexcept);
  static_assert(std::is_nothrow_copy_assignable_v<text> == allocation_noexcept);
  static_assert(noexcept(tjg::color::strip_ansi(std::declval<text>())) ==
                allocation_noexcept);
  // Existing unconditional guarantees stay intact.
  static_assert(std::is_nothrow_move_constructible_v<text>);
  static_assert(std::is_nothrow_move_assignable_v<text>);
  static_assert(!noexcept(std::declval<const text&>().plain_text()));
  static_assert(!noexcept(std::declval<text&>() += std::declval<text>()));
  return true;
}
static_assert(check_tree_policy<char>());
static_assert(check_tree_policy<wchar_t>());

TEST(allocation_noexcept_test, copy_and_strip_tree) {
  auto original = tjg::color::styled_text{std::string{"left\x1b[31m"}, tjg::color::bold()};
  original += tjg::color::styled_text{std::string{"right"}, tjg::color::underline()};
  auto copy = tjg::color::styled_text{original};
  tjg::color::styled_text assigned;
  assigned = copy;
  EXPECT_EQ(assigned, original);
  const auto stripped = tjg::color::strip_ansi(std::move(assigned));
  EXPECT_EQ(stripped.plain_text(), "leftright");
  EXPECT_EQ(original.plain_text(), "left\x1b[31mright");
}
