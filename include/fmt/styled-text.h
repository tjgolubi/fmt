// Structured styled text with borrowed character storage.
#ifndef FMT_STYLED_TEXT_H_
#define FMT_STYLED_TEXT_H_

#include "color.h"

#include "core.h"

#include <new>
#include <string>
#include <utility>
#include <vector>

FMT_BEGIN_NAMESPACE

/**
 * A borrowed text view or owned sequence of styled children.
 * Referenced characters must outlive this value and every copy that uses them.
 * Copies own their child nodes but continue to borrow the same characters.
 */
template<typename Char>
class basic_styled_text {
 public:
  using char_type   = Char;
  using string_type = std::basic_string<Char>;
  using view_type   = basic_string_view<Char>;

 private:
  using leaf   = view_type;
  using branch = std::vector<basic_styled_text>;
  friend struct formatter<basic_styled_text<Char>, Char>;
  text_style _style{};
  // A branch is created only by concatenating nonempty values.
  bool _is_branch = false;
  union storage {
    leaf text;
    branch children;
    storage() {}
    ~storage() {}
  } _data;

  void destroy() noexcept {
    if (_is_branch)
      _data.children.~branch();
    else
      _data.text.~leaf();
  }

 public:
  basic_styled_text() : _style{}, _is_branch{false}
  { new (&_data.text) leaf{}; }

  basic_styled_text(view_type text, text_style style = {})
    : _style{style}, _is_branch{false}
  { new (&_data.text) leaf{text}; }

  // Borrows the string's characters; does not extend its lifetime.
  basic_styled_text(const string_type& text, text_style style = {})
    : basic_styled_text{view_type{text}, style} {}

  basic_styled_text(const Char* text, text_style style = {})
    : basic_styled_text{view_type{text}, style} {}

  basic_styled_text(const basic_styled_text& other)
    : _style{other._style}, _is_branch{other._is_branch}
  {
    if (_is_branch)
      new (&_data.children) branch{other._data.children};
    else
      new (&_data.text) leaf{other._data.text};
  }

  basic_styled_text(basic_styled_text&& other) noexcept
    : _style{other._style}, _is_branch{other._is_branch}
  {
    if (_is_branch)
      new (&_data.children) branch{std::move(other._data.children)};
    else
      new (&_data.text) leaf{other._data.text};
    other.destroy();
    new (&other._data.text) leaf{};
    other._is_branch = false;
    other._style = text_style{};
  }

  ~basic_styled_text() { destroy(); }

  auto operator=(const basic_styled_text& other) -> basic_styled_text& {
    if (this == &other) return *this;
    auto copy = basic_styled_text{other};
    return *this = std::move(copy);
  }

  auto operator=(basic_styled_text&& other) noexcept -> basic_styled_text& {
    if (this == &other) return *this;
    auto moved = basic_styled_text{std::move(other)};
    this->~basic_styled_text();
    new (this) basic_styled_text{std::move(moved)};
    return *this;
  }

  auto style() const noexcept -> text_style { return _style; }
  auto style(text_style value) noexcept -> basic_styled_text& {
    _style = value;
    return *this;
  }

  // Empty leaves are concatenation identities.
  auto empty() const noexcept -> bool
  { return !_is_branch && _data.text.empty(); }

private:

  void append_plain_text(string_type& result) const {
    if (_is_branch) {
      for (const auto& child : _data.children)
        child.append_plain_text(result);
    } else if (!_data.text.empty()) {
      result.append(_data.text.data(), _data.text.size());
    }
  }

public:

  /**
   * Returns an independent string containing all leaf text in order, without
   * applying styles. Preserves embedded nulls and escapes without interpreting
   * them. Does not modify this value. Allocation can throw.
   */
  FMT_NODISCARD auto plain_text() const -> string_type {
    auto result = string_type{};
    append_plain_text(result);
    return result;
  }

  auto operator+=(basic_styled_text rhs) -> basic_styled_text& {
    if (rhs.empty()) return *this;
    if (empty()) return *this = std::move(rhs);
    if (_is_branch) {
      _data.children.push_back(std::move(rhs));
    }
    else {
      auto children = branch{};
      // Allocate before moving either operand. The following moves cannot throw.
      children.reserve(2);
      children.push_back(std::move(*this));
      children.push_back(std::move(rhs));
      destroy();
      new (&_data.children) branch{std::move(children)};
      _is_branch = true;
      _style = text_style{};
    }
    return *this;
  } // +=

  friend
  bool operator==(const basic_styled_text& lhs, const basic_styled_text& rhs) {
    if (lhs._style != rhs._style || lhs._is_branch != rhs._is_branch)
      return false;
    if (lhs._is_branch)
      return lhs._data.children == rhs._data.children;
    return lhs._data.text == rhs._data.text;
  }

  friend
  bool operator!=(const basic_styled_text& lhs, const basic_styled_text& rhs)
  { return !(lhs == rhs); }

  friend auto operator+(basic_styled_text lhs, basic_styled_text rhs)
      -> basic_styled_text
  {
    lhs += std::move(rhs);
    return lhs;
  }
}; // basic_styled_text

using styled_text  = basic_styled_text<char>;
using wstyled_text = basic_styled_text<wchar_t>;

FMT_END_NAMESPACE

#endif // FMT_STYLED_TEXT_H_
