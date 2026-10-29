#include "fmt/styled-text.h"
#include "gtest-extra.h"

TEST(styled_text_test, construction_and_access) {
  using node = fmt::styled_text;
  const auto empty = node{};
  EXPECT_TRUE(fmt::holds_alternative<node::leaf>(empty));
  EXPECT_EQ(fmt::get<node::leaf>(empty), "");
  EXPECT_TRUE(empty.style().empty());
  node literal = "hello";
  node owned = std::string("hello");
  EXPECT_EQ(literal, owned);
  auto group = node(node::branch{literal, owned}, fmt::bold());
  EXPECT_TRUE(fmt::holds_alternative<node::branch>(group));
  EXPECT_EQ(fmt::get<node::branch>(group).size(), size_t{2});
  EXPECT_EQ(group.style(), fmt::bold());
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW(fmt::get<node::leaf>(group), fmt::format_error);
  EXPECT_THROW(fmt::get<node::branch>(literal), fmt::format_error);
#endif
}

TEST(styled_text_test, concatenation_and_exact_equality) {
  using node = fmt::styled_text;
  const auto first = node("a", fmt::bold());
  const auto second = node("b", fmt::underline());
  auto joined = first + second;
  EXPECT_TRUE(joined.style().empty());
  const auto& children = fmt::get<node::branch>(joined);
  ASSERT_EQ(children.size(), size_t{2});
  EXPECT_EQ(children[0], first);
  EXPECT_EQ(children[1], second);
  EXPECT_NE(joined, second + first);
  EXPECT_NE(node("ab"), node("a") + "b");
  EXPECT_EQ("a" + second, std::string("a") + second);
  EXPECT_EQ(first + "b", first + std::string("b"));
  EXPECT_NE(node("a"), first);
  auto colored = first;
  colored.style(fmt::fg(fmt::color::red));
  EXPECT_NE(colored, first);
  joined.style(fmt::italic());
  EXPECT_EQ(fmt::get<node::branch>(joined)[0], first);
  const auto nested = first + second + "c";
  EXPECT_TRUE(fmt::holds_alternative<node::branch>(
      fmt::get<node::branch>(nested)[0]));
}

TEST(styled_text_test, ownership_copy_move_and_assignment) {
  using node = fmt::styled_text;
  auto source = std::string(200, 'x');
  auto original = node(source, fmt::bold()) + "tail";
  source[0] = 'y';
  auto copy = original;
  auto& copy_text = fmt::get<node::leaf>(fmt::get<node::branch>(copy)[0]);
  copy_text[0] = 'z';
  EXPECT_NE(copy, original);
  EXPECT_EQ(fmt::get<node::leaf>(fmt::get<node::branch>(original)[0])[0], 'x');
  auto moved = node(std::move(copy));
  node assigned = "old leaf";
  assigned = moved;
  EXPECT_EQ(assigned, moved);
  assigned = node("new leaf");
  EXPECT_EQ(assigned, node("new leaf"));
  assigned = std::move(moved);
  EXPECT_TRUE(fmt::holds_alternative<node::branch>(assigned));
  auto& self = assigned;
  assigned = self;
  assigned = std::move(self);
  EXPECT_TRUE(fmt::holds_alternative<node::branch>(assigned));
  // Assignment from a descendant must survive destruction of the old branch.
  assigned = std::move(fmt::get<node::branch>(assigned)[0]);
  EXPECT_TRUE(fmt::holds_alternative<node::leaf>(assigned));
  EXPECT_EQ(fmt::get<node::leaf>(assigned)[0], 'z');
}
