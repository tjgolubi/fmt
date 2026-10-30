#include "fmt/styled-text.h"
#include "gtest-extra.h"

namespace {
template<typename Char>
auto text(const char* value) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  for (; *value; ++value) result.push_back(static_cast<Char>(*value));
  return result;
}

#define CHECK_NO_MEMBER(name) \
  template<typename T> auto has_##name(int) \
      -> decltype(std::declval<T&>().name(), std::true_type{}); \
  template<typename> auto has_##name(...) -> std::false_type
CHECK_NO_MEMBER(is_branch);
CHECK_NO_MEMBER(as_branch);
CHECK_NO_MEMBER(as_leaf);
CHECK_NO_MEMBER(begin);
#undef CHECK_NO_MEMBER

template<typename T> auto has_get(int)
    -> decltype(fmt::get<typename T::string_type>(std::declval<T&>()),
                std::true_type{});
template<typename> auto has_get(...) -> std::false_type;
template<typename T> auto has_holds(int)
    -> decltype(fmt::holds_alternative<typename T::string_type>(
                    std::declval<const T&>()), std::true_type{});
template<typename> auto has_holds(...) -> std::false_type;

template<typename Char>
void check_public_surface() {
  using node = fmt::basic_styled_text<Char>;
  using sequence = std::vector<node>;
  static_assert(!std::is_constructible<node, sequence>::value,
                "vector construction must not be public");
  static_assert(!std::is_constructible<node, sequence, fmt::text_style>::value,
                "styled vector construction must not be public");
  static_assert(!decltype(has_is_branch<node>(0))::value, "no branch query");
  static_assert(!decltype(has_as_branch<node>(0))::value, "no child access");
  static_assert(!decltype(has_as_leaf<node>(0))::value, "no leaf access");
  static_assert(!decltype(has_begin<node>(0))::value, "no iteration");
  static_assert(!decltype(has_get<node>(0))::value, "no variant get");
  static_assert(!decltype(has_holds<node>(0))::value, "no variant query");
  static_assert(std::is_nothrow_move_constructible<node>::value,
                "move construction remains noexcept");
  static_assert(std::is_nothrow_move_assignable<node>::value,
                "move assignment remains noexcept");
}

template<typename Char>
void check_construction_and_composition() {
  using node = fmt::basic_styled_text<Char>;
  const auto empty = node{};
  EXPECT_TRUE(empty.empty());
  EXPECT_TRUE(empty.style().empty());
  const auto a_text = text<Char>("a");
  const auto b_text = text<Char>("b");
  const auto c_text = text<Char>("c");
  const auto empty_text = text<Char>("");
  const auto a = node(a_text, fmt::bold());
  const auto b = node(b_text, fmt::underline());
  const auto c = node(c_text);
  auto joined = a + b;
  EXPECT_FALSE(joined.empty());
  EXPECT_TRUE(joined.style().empty());
  EXPECT_NE(joined, b + a);
  EXPECT_NE(joined, node(text<Char>("ab")));
  auto appended = a;
  EXPECT_EQ(&(appended += b), &appended);
  EXPECT_EQ(appended, joined);
  appended += c;
  EXPECT_EQ(appended, a + b + c);
  EXPECT_NE(appended, a + (b + c));
  auto styled = joined;
  EXPECT_EQ(&styled.style(fmt::italic()), &styled);
  EXPECT_EQ(styled.style(), fmt::italic());
  EXPECT_NE(styled, joined);
  EXPECT_TRUE(joined.style().empty());
  EXPECT_EQ(empty + a, a);
  EXPECT_EQ(a + empty, a);
  EXPECT_EQ(empty + empty, empty);
  auto styled_empty = node(empty_text, fmt::bold());
  styled_empty += a;
  EXPECT_EQ(styled_empty, a);
  auto repeated = joined;
  const auto snapshot = repeated;
  repeated += repeated;
  EXPECT_EQ(repeated, snapshot + snapshot);
  const auto bytes = std::basic_string<Char>(3, Char{});
  EXPECT_FALSE(node(bytes).empty());
  auto binary = node(bytes);
  binary += a;
  EXPECT_EQ(binary, node(bytes) + a);
}

template<typename Char>
void check_copy_move_and_reuse() {
  using node = fmt::basic_styled_text<Char>;
  auto input = std::basic_string<Char>(200, static_cast<Char>('x'));
  const auto tail = text<Char>("tail");
  const auto reused = text<Char>("reused");
  const auto again = text<Char>("again");
  const auto extra = text<Char>("extra");
  const auto leaf = node(input, fmt::bold());
  input[0] = static_cast<Char>('y');
  EXPECT_EQ(leaf.plain_text(), input);
  const auto branch = leaf + tail;
  for (int alternative = 0; alternative != 2; ++alternative) {
    const auto expected = alternative == 0 ? leaf : branch;
    auto source = expected;
    auto moved = node(std::move(source));
    EXPECT_EQ(moved, expected);
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source, node{});
    source += reused;
    EXPECT_EQ(source, node(text<Char>("reused")));
    // Exercise both target alternatives for both source alternatives.
    for (int target = 0; target != 2; ++target) {
      auto assigned = target == 0 ? leaf : branch;
      auto rhs = expected;
      assigned = std::move(rhs);
      EXPECT_EQ(assigned, expected);
      EXPECT_EQ(rhs, node{});
      rhs += again;
      EXPECT_EQ(rhs, node(text<Char>("again")));
      auto& self = assigned;
      assigned = self;
      EXPECT_EQ(assigned, expected);
      assigned = std::move(self);
      EXPECT_EQ(assigned, expected);
      assigned = node{};
      EXPECT_TRUE(assigned.empty());
      assigned = expected;
      EXPECT_EQ(assigned, expected);
    }
  }
  auto copy = branch;
  copy.style(fmt::italic());
  copy += extra;
  EXPECT_EQ(branch, leaf + text<Char>("tail"));
  auto many = node{};
  for (int i = 0; i < 100; ++i) many += leaf;
  const auto snapshot = many;
  auto moved = node(std::move(many));
  EXPECT_EQ(moved, snapshot);
  EXPECT_EQ(many, node{});
  many += leaf;
  EXPECT_EQ(many, leaf);
}
} // namespace

TEST(styled_text_test, narrow_public_surface) { check_public_surface<char>(); }
TEST(styled_text_test, wide_public_surface) { check_public_surface<wchar_t>(); }
TEST(styled_text_test, narrow_composition) { check_construction_and_composition<char>(); }
TEST(styled_text_test, wide_composition) { check_construction_and_composition<wchar_t>(); }
TEST(styled_text_test, narrow_copy_move_reuse) { check_copy_move_and_reuse<char>(); }
TEST(styled_text_test, wide_copy_move_reuse) { check_copy_move_and_reuse<wchar_t>(); }

namespace {
template<typename Char>
void check_borrowed_text_lifetime_and_equality() {
  using node = fmt::basic_styled_text<Char>;
  const auto large = std::basic_string<Char>(100000, static_cast<Char>('x'));
  const auto tail = text<Char>("tail");
  const auto different_text = text<Char>("different");
  const auto more = text<Char>("more");
  const auto empty_text = std::basic_string<Char>{};
  auto surviving_leaf = node{};
  auto surviving_branch = node{};
  {
    auto source = node(large, fmt::bold());
    surviving_leaf = source;
    surviving_branch = source + tail;
    source.style(fmt::underline());
    source += different_text;
    EXPECT_EQ(surviving_leaf.style(), fmt::bold());
  }
  // Independently allocated strings compare by content, not pointer identity.
  const auto separate = node(large, fmt::bold());
  EXPECT_EQ(surviving_leaf, separate);
  EXPECT_EQ(surviving_branch, separate + text<Char>("tail"));
  auto different = large;
  different.back() = static_cast<Char>('y');
  EXPECT_NE(surviving_leaf, node(different, fmt::bold()));
  auto copy = surviving_branch;
  copy.style(fmt::italic());
  copy += more;
  EXPECT_EQ(surviving_branch, separate + text<Char>("tail"));
  const auto empty_string = node(empty_text);
  EXPECT_EQ(empty_string, node{});
  EXPECT_NE(empty_string, separate);
  EXPECT_NE(separate, empty_string);
  const auto nulls = std::basic_string<Char>(3, Char{});
  EXPECT_EQ(node(nulls), node(nulls));
  EXPECT_FALSE(node(nulls).empty());
}
} // namespace

TEST(styled_text_test, narrow_borrowed_text_lifetime) {
  check_borrowed_text_lifetime_and_equality<char>();
}
TEST(styled_text_test, wide_borrowed_text_lifetime) {
  check_borrowed_text_lifetime_and_equality<wchar_t>();
}

namespace {
template<typename Char>
void check_plain_text() {
  using node = fmt::basic_styled_text<Char>;
  using string_type = typename node::string_type;
  static_assert(std::is_same<decltype(std::declval<const node&>().plain_text()),
                             string_type>::value, "returns an owning string");
  EXPECT_EQ(node{}.plain_text(), string_type{});
  EXPECT_EQ((node{string_type{}, fmt::bold()}.plain_text()), string_type{});
  const auto a_text = text<Char>(" a ");
  const auto b_text = text<Char>("b");
  const auto c_text = text<Char>("\tc\n");
  const auto tail = text<Char>("tail");
  const auto a = node{a_text, fmt::bold()};
  auto nested = a + (node{b_text, fmt::underline()} + c_text);
  nested.style(fmt::italic());
  const auto snapshot = nested;
  const auto shared_copy = nested;
  const auto& read_only = nested;
  auto result = read_only.plain_text();
  EXPECT_EQ(result, text<Char>(" a b\tc\n"));
  EXPECT_EQ(nested, snapshot);
  EXPECT_EQ(nested.style(), fmt::italic());
  result[0] = static_cast<Char>('z');
  EXPECT_EQ(nested.plain_text(), text<Char>(" a b\tc\n"));
  EXPECT_EQ(shared_copy.plain_text(), text<Char>(" a b\tc\n"));
  const auto nulls = string_type(3, Char{});
  EXPECT_EQ((a + node{nulls} + tail).plain_text(),
            text<Char>(" a ") + nulls + text<Char>("tail"));
  const auto escapes = text<Char>("\x1b[31mred\x1b[0m");
  EXPECT_EQ((node{escapes, fmt::bold()}.plain_text()), escapes);
  const auto large = string_type(100000, static_cast<Char>('x'));
  EXPECT_EQ(node{large}.plain_text(), large);
  auto many = node{};
  for (int i = 0; i < 100; ++i) many += a;
  auto expected = string_type{};
  for (int i = 0; i < 100; ++i) expected.append(text<Char>(" a "));
  EXPECT_EQ(many.plain_text(), expected);
  auto moved = node{std::move(nested)};
  EXPECT_EQ(nested.plain_text(), string_type{});
  EXPECT_EQ(moved.plain_text(), text<Char>(" a b\tc\n"));
  nested = std::move(moved);
  EXPECT_EQ(moved.plain_text(), string_type{});
  EXPECT_EQ(nested.plain_text(), text<Char>(" a b\tc\n"));
  auto survivor = node{};
  {
    const auto source = node{large} + a;
    survivor = source;
  }
  EXPECT_EQ(survivor.plain_text(), large + text<Char>(" a "));
}
} // namespace

TEST(styled_text_test, narrow_plain_text) { check_plain_text<char>(); }
TEST(styled_text_test, wide_plain_text) { check_plain_text<wchar_t>(); }

namespace {
template<typename Char>
void check_empty_view_comparisons() {
  using view = fmt::basic_string_view<Char>;
  constexpr auto null_view = view{};
  static_assert(null_view.empty(), "default view is empty");
  const auto storage = text<Char>("abc");
  const auto backed_empty = view{storage.data(), 0};
  const auto nonempty = view{storage};
  EXPECT_EQ(null_view, view{});
  EXPECT_EQ(null_view, backed_empty);
  EXPECT_EQ(backed_empty, null_view);
  EXPECT_EQ(null_view.compare(backed_empty), 0);
  EXPECT_LT(null_view.compare(nonempty), 0);
  EXPECT_GT(nonempty.compare(null_view), 0);
  EXPECT_LT(null_view, nonempty);
  EXPECT_GT(nonempty, null_view);
  EXPECT_TRUE(null_view.starts_with(backed_empty));
  EXPECT_TRUE(nonempty.starts_with(null_view));
  EXPECT_NE(view(storage.data(), 1), nonempty);
  const auto separate = text<Char>("abd");
  EXPECT_LT(nonempty.compare(view{separate}), 0);
  EXPECT_GT(view{separate}.compare(nonempty), 0);
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
  constexpr Char literal[] = {static_cast<Char>('x'), Char{}};
  static_assert(view{}.compare(view{literal, 0}) == 0,
                "different empty pointers compare in constant evaluation");
#endif
}

template<typename Char>
void check_string_concatenation() {
  using node = fmt::basic_styled_text<Char>;
  using string = typename node::string_type;
  static_assert(std::is_convertible<const string&, node>::value,
                "strings convert directly to borrowing nodes");
  auto backing = string(200, static_cast<Char>('a'));
  const auto prefix = text<Char>("prefix ");
  const auto suffix = text<Char>(" suffix");
  node value = backing;
  const auto copy = value;
  const auto paragraph = prefix + value + suffix;
  EXPECT_EQ(paragraph.plain_text(), prefix + backing + suffix);
  EXPECT_EQ((value + suffix).plain_text(), backing + suffix);
  value += suffix;
  EXPECT_EQ(value.plain_text(), backing + suffix);
  // Changing a character does not invalidate storage: both views observe it.
  backing[0] = static_cast<Char>('z');
  EXPECT_EQ(copy.plain_text(), backing);
  EXPECT_EQ(value.plain_text(), backing + suffix);
  EXPECT_EQ(paragraph.plain_text(), prefix + backing + suffix);
  // A temporary string is safe when the entire tree is consumed immediately.
  EXPECT_EQ((node{prefix} + string(200, static_cast<Char>('b'))).plain_text(),
            prefix + string(200, static_cast<Char>('b')));
}
} // namespace

TEST(styled_text_test, narrow_empty_view_comparisons) {
  check_empty_view_comparisons<char>();
}
TEST(styled_text_test, wide_empty_view_comparisons) {
  check_empty_view_comparisons<wchar_t>();
}
TEST(styled_text_test, narrow_string_concatenation) {
  check_string_concatenation<char>();
}
TEST(styled_text_test, wide_string_concatenation) {
  check_string_concatenation<wchar_t>();
}

TEST(styled_text_test, aliases_and_literals) {
  static_assert(std::is_same<fmt::styled_text, fmt::basic_styled_text<char>>::value,
                "narrow alias");
  static_assert(std::is_same<fmt::wstyled_text, fmt::basic_styled_text<wchar_t>>::value,
                "wide alias");
  static_assert(!std::is_constructible<fmt::styled_text, fmt::wstyled_text>::value,
                "no implicit character conversion");
  fmt::styled_text narrow = "hello";
  fmt::wstyled_text wide = L"\u03b1";
  EXPECT_EQ("a" + narrow, fmt::styled_text("a") + narrow);
  EXPECT_EQ(wide + L"\u03b2", wide + std::wstring(L"\u03b2"));
  EXPECT_EQ((wide + L"\u03b2").plain_text(), std::wstring{L"\u03b1\u03b2"});
}

#if FMT_USE_CONSTEVAL && FMT_CPLUSPLUS >= 202002L
namespace {
template<typename Char>
struct constexpr_text {
  static constexpr Char value[] = {Char{'a'}, Char{}, Char{'b'}, Char{}};
};
template<typename Char> constexpr Char constexpr_text<Char>::value[];

template<typename Char>
constexpr auto check_constexpr_leaf() -> bool {
  using node = fmt::basic_styled_text<Char>;
  using view = typename node::view_type;
  using data = constexpr_text<Char>;
  const Char value[] = {Char{'a'}, Char{}, Char{'b'}, Char{}};
  const Char same[] = {Char{'a'}, Char{}, Char{'b'}, Char{}};
  const Char different[] = {Char{'a'}, Char{}, Char{'c'}, Char{}};
  constexpr auto empty = node{};
  constexpr auto literal = node{data::value, fmt::bold()};
  const auto backed_empty = node{view{value, 0}};
  const auto binary = node{view{value, 3}, fmt::underline()};
  const auto equal = node{view{same, 3}, fmt::underline()};
  const auto other_text = node{view{different, 3}, fmt::underline()};
  const auto other_style = node{view{value, 3}, fmt::italic()};
  const auto shorter = node{view{value, 1}, fmt::underline()};
  auto mutable_leaf = node{view{value, 3}};
  const auto& styled = mutable_leaf.style(fmt::italic());
  return empty.empty() && empty.style().empty() && backed_empty.empty() &&
         empty == backed_empty && backed_empty == empty && !literal.empty() &&
         literal.style() == fmt::bold() && binary.style() == fmt::underline() &&
         binary == equal && !(binary != equal) && binary != other_text &&
         binary != other_style && binary != shorter && binary != empty &&
         empty != binary && &styled == &mutable_leaf &&
         mutable_leaf.style() == fmt::italic() && mutable_leaf == other_style;
}

#if FMT_USE_CONSTEXPR_STRING
template<typename Char>
constexpr auto check_constexpr_plain_text() -> bool {
  using node = fmt::basic_styled_text<Char>;
  using view = typename node::view_type;
  using string = typename node::string_type;
  using data = constexpr_text<Char>;
  auto input = string(200, Char{'x'});
  input[1] = Char{};
  const auto borrowed = node{input, fmt::bold()};
  auto result = borrowed.plain_text();
  if (result != input) return false;
  result[0] = Char{'y'};
  const auto binary = node{view{data::value, 3}}.plain_text();
  return node{}.plain_text().empty() && input[0] == Char{'x'} &&
         borrowed.style() == fmt::bold() && binary.size() == 3 &&
         binary[0] == Char{'a'} && binary[1] == Char{} && binary[2] == Char{'b'};
}
#endif
} // namespace

TEST(styled_text_test, narrow_constexpr_leaf) {
  static_assert(check_constexpr_leaf<char>(), "narrow leaves are constexpr");
  EXPECT_TRUE(check_constexpr_leaf<char>());
}
TEST(styled_text_test, wide_constexpr_leaf) {
  static_assert(check_constexpr_leaf<wchar_t>(), "wide leaves are constexpr");
  EXPECT_TRUE(check_constexpr_leaf<wchar_t>());
}
#if FMT_USE_CONSTEXPR_STRING
TEST(styled_text_test, narrow_constexpr_plain_text) {
  static_assert(check_constexpr_plain_text<char>(),
                "narrow plain text is constexpr");
  EXPECT_TRUE(check_constexpr_plain_text<char>());
}
TEST(styled_text_test, wide_constexpr_plain_text) {
  static_assert(check_constexpr_plain_text<wchar_t>(),
                "wide plain text is constexpr");
  EXPECT_TRUE(check_constexpr_plain_text<wchar_t>());
}
#endif
#endif
