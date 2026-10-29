// Owned structured styled text.
#ifndef FMT_STYLED_TEXT_H_
#define FMT_STYLED_TEXT_H_

#include "color.h"

#include <new>
#include <string>
#include <utility>
#include <vector>

FMT_BEGIN_NAMESPACE

/** An owned string or sequence of styled children, with a style for this node. */
template<typename Char>
class basic_styled_text {
 public:
  using char_type = Char;
  using string_type = std::basic_string<Char>;

 private:
  using leaf = string_type;
  using branch = std::vector<basic_styled_text>;
  friend struct formatter<basic_styled_text<Char>, Char>;
  text_style style_;
  // A branch is created only by concatenating nonempty values.
  bool is_branch_;
  union storage {
    leaf text;
    branch children;
    storage() {}
    ~storage() {}
  } data_;

  void destroy() noexcept;

 public:
  basic_styled_text();
  basic_styled_text(string_type text, text_style style = {});
  basic_styled_text(const Char* text, text_style style = {});
  basic_styled_text(const basic_styled_text& other);
  basic_styled_text(basic_styled_text&& other) noexcept;
  ~basic_styled_text();
  auto operator=(const basic_styled_text& other) -> basic_styled_text&;
  auto operator=(basic_styled_text&& other) noexcept -> basic_styled_text&;

  auto style() const noexcept -> text_style { return style_; }
  auto style(text_style value) noexcept -> basic_styled_text& {
    style_ = value;
    return *this;
  }
  // Empty leaves are concatenation identities.
  auto empty() const noexcept -> bool
  { return !is_branch_ && data_.text.empty(); }

  auto operator+=(basic_styled_text rhs) -> basic_styled_text& {
    if (rhs.empty()) return *this;
    if (empty()) return *this = std::move(rhs);
    if (!is_branch_) {
      auto children = branch{};
      // Allocate before moving either operand. The following moves cannot throw.
      children.reserve(2);
      children.push_back(std::move(*this));
      children.push_back(std::move(rhs));
      destroy();
      new (&data_.children) branch(std::move(children));
      is_branch_ = true;
      style_ = text_style{};
      return *this;
    }
    data_.children.push_back(std::move(rhs));
    return *this;
  }

  friend bool operator==(const basic_styled_text& lhs,
                         const basic_styled_text& rhs) {
    if (lhs.style_ != rhs.style_ || lhs.is_branch_ != rhs.is_branch_) return false;
    return lhs.is_branch_ ? lhs.data_.children == rhs.data_.children
                          : lhs.data_.text == rhs.data_.text;
  }
  friend bool operator!=(const basic_styled_text& lhs,
                         const basic_styled_text& rhs) {
    return !(lhs == rhs);
  }
  friend auto operator+(basic_styled_text lhs, basic_styled_text rhs)
      -> basic_styled_text {
    lhs += std::move(rhs);
    return lhs;
  }
}; // basic_styled_text

template<typename Char>
inline basic_styled_text<Char>::basic_styled_text() : basic_styled_text(leaf{}) {}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(leaf text, text_style style)
    : style_(style), is_branch_(false) {
  new (&data_.text) leaf(std::move(text));
}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(const Char* text, text_style style)
    : basic_styled_text(leaf(text), style) {}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(const basic_styled_text& other)
    : style_(other.style_), is_branch_(other.is_branch_) {
  if (is_branch_)
    new (&data_.children) branch(other.data_.children);
  else
    new (&data_.text) leaf(other.data_.text);
}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(basic_styled_text&& other) noexcept
    : style_(other.style_), is_branch_(other.is_branch_) {
  if (is_branch_)
    new (&data_.children) branch(std::move(other.data_.children));
  else
    new (&data_.text) leaf(std::move(other.data_.text));
  other.destroy();
  new (&other.data_.text) leaf();
  other.is_branch_ = false;
  other.style_ = text_style{};
}
template<typename Char>
inline void basic_styled_text<Char>::destroy() noexcept {
  if (is_branch_)
    data_.children.~branch();
  else
    data_.text.~leaf();
}
template<typename Char>
inline basic_styled_text<Char>::~basic_styled_text() { destroy(); }
template<typename Char>
inline auto basic_styled_text<Char>::operator=(const basic_styled_text& other)
    -> basic_styled_text& {
  if (this == &other) return *this;
  auto copy = basic_styled_text(other);
  return *this = std::move(copy);
}
template<typename Char>
inline auto basic_styled_text<Char>::operator=(basic_styled_text&& other) noexcept
    -> basic_styled_text& {
  if (this == &other) return *this;
  // Preserve the value before replacing the active union alternative.
  auto moved = basic_styled_text(std::move(other));
  this->~basic_styled_text();
  new (this) basic_styled_text(std::move(moved));
  return *this;
}

using styled_text = basic_styled_text<char>;
using wstyled_text = basic_styled_text<wchar_t>;

FMT_END_NAMESPACE

#endif // FMT_STYLED_TEXT_H_
