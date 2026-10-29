// Owned structured styled text.
#ifndef FMT_STYLED_TEXT_H_
#define FMT_STYLED_TEXT_H_

#include "color.h"

#include <memory>
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
  using leaf = std::shared_ptr<const string_type>;
  using branch = std::vector<basic_styled_text>;
  friend struct formatter<basic_styled_text<Char>, Char>;
  text_style _style;
  // A branch is created only by concatenating nonempty values.
  bool _is_branch;
  union storage {
    leaf _text;
    branch _children;
    storage() {}
    ~storage() {}
  } _data;

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

  auto style() const noexcept -> text_style { return _style; }
  auto style(text_style value) noexcept -> basic_styled_text& {
    _style = value;
    return *this;
  }
  // Empty leaves are concatenation identities.
  auto empty() const noexcept -> bool
  { return !_is_branch && !_data._text; }

  auto operator+=(basic_styled_text rhs) -> basic_styled_text& {
    if (rhs.empty()) return *this;
    if (empty()) return *this = std::move(rhs);
    if (!_is_branch) {
      auto children = branch{};
      // Allocate before moving either operand. The following moves cannot throw.
      children.reserve(2);
      children.push_back(std::move(*this));
      children.push_back(std::move(rhs));
      destroy();
      new (&_data._children) branch{std::move(children)};
      _is_branch = true;
      _style = text_style{};
      return *this;
    }
    _data._children.push_back(std::move(rhs));
    return *this;
  }

  friend bool operator==(const basic_styled_text& lhs,
                         const basic_styled_text& rhs) {
    if (lhs._style != rhs._style || lhs._is_branch != rhs._is_branch) return false;
    if (lhs._is_branch) return lhs._data._children == rhs._data._children;
    return lhs._data._text == rhs._data._text ||
           (lhs._data._text && rhs._data._text &&
            *lhs._data._text == *rhs._data._text);
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
inline basic_styled_text<Char>::basic_styled_text()
    : _style{}, _is_branch{false} {
  new (&_data._text) leaf{};
}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(string_type text, text_style style)
    : _style{style}, _is_branch{false} {
  new (&_data._text) leaf{text.empty() ? leaf{} :
      std::make_shared<const string_type>(std::move(text))};
}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(const Char* text, text_style style)
    : basic_styled_text{string_type{text}, style} {}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(const basic_styled_text& other)
    : _style{other._style}, _is_branch{other._is_branch} {
  if (_is_branch)
    new (&_data._children) branch{other._data._children};
  else
    new (&_data._text) leaf{other._data._text};
}
template<typename Char>
inline basic_styled_text<Char>::basic_styled_text(basic_styled_text&& other) noexcept
    : _style{other._style}, _is_branch{other._is_branch} {
  if (_is_branch)
    new (&_data._children) branch{std::move(other._data._children)};
  else
    new (&_data._text) leaf{std::move(other._data._text)};
  other.destroy();
  new (&other._data._text) leaf{};
  other._is_branch = false;
  other._style = text_style{};
}
template<typename Char>
inline void basic_styled_text<Char>::destroy() noexcept {
  if (_is_branch)
    _data._children.~branch();
  else
    _data._text.~leaf();
}
template<typename Char>
inline basic_styled_text<Char>::~basic_styled_text() { destroy(); }
template<typename Char>
inline auto basic_styled_text<Char>::operator=(const basic_styled_text& other)
    -> basic_styled_text& {
  if (this == &other) return *this;
  auto copy = basic_styled_text{other};
  return *this = std::move(copy);
}
template<typename Char>
inline auto basic_styled_text<Char>::operator=(basic_styled_text&& other) noexcept
    -> basic_styled_text& {
  if (this == &other) return *this;
  // Preserve the value before replacing the active union alternative.
  auto moved = basic_styled_text{std::move(other)};
  this->~basic_styled_text();
  new (this) basic_styled_text{std::move(moved)};
  return *this;
}

using styled_text = basic_styled_text<char>;
using wstyled_text = basic_styled_text<wchar_t>;

FMT_END_NAMESPACE

#endif // FMT_STYLED_TEXT_H_
