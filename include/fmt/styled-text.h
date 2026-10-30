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
  using char_type   = Char;
  using string_type = std::basic_string<Char>;

 private:
  using leaf   = std::shared_ptr<const string_type>;
  using branch = std::vector<basic_styled_text>;
  friend struct formatter<basic_styled_text<Char>, Char>;
  text_style _style{};
  // A branch is created only by concatenating nonempty values.
  bool _is_branch = false;
  union storage {
    leaf text;
    branch children;
    storage() {}
    storage(leaf value) noexcept : text{std::move(value)} {}
    ~storage() {}
  } _data;

  void destroy() noexcept {
    if (_is_branch)
      _data.children.~branch();
    else
      _data.text.~leaf();
  }

  void append_plain_text(string_type& result) const {
    if (_is_branch) {
      for (const auto& child : _data.children)
        child.append_plain_text(result);
    } else if (_data.text) {
      result.append(*_data.text);
    }
  }

 public:
  basic_styled_text() : _style{}, _is_branch{false}, _data{leaf{}} {}

  basic_styled_text(string_type text, text_style style = {})
    : _style{style}, _is_branch{false},
      _data{text.empty() ? leaf{} :
          std::make_shared<const string_type>(std::move(text))} {}

  basic_styled_text(const Char* text, text_style style = {})
    : basic_styled_text{string_type{text}, style} {}

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
      new (&_data.text) leaf{std::move(other._data.text)};
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
    // Preserve the value before replacing the active union alternative.
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

  /**
   * Overlays this node's style in place, leaving its text and children intact.
   * Specified colors replace matching channels; emphasis bits are combined
   * by OR. Returns a reference to this node for chaining.
   */
  auto overlay(text_style style) noexcept -> basic_styled_text& {
    _style.overlay(style);
    return *this;
  }

  // Empty leaves are concatenation identities.
  auto empty() const noexcept -> bool
  { return !_is_branch && !_data.text; }

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
    if (!_is_branch) {
      auto children = branch{};
      // Allocate before moving either operand. The following moves cannot throw.
      children.reserve(2);
      children.push_back(std::move(*this));
      children.push_back(std::move(rhs));
      destroy();
      new (&_data.children) branch{std::move(children)};
      _is_branch = true;
      _style = text_style{};
      return *this;
    }
    _data.children.push_back(std::move(rhs));
    return *this;
  }

  friend
  bool operator==(const basic_styled_text& lhs, const basic_styled_text& rhs) {
    if (lhs._style != rhs._style || lhs._is_branch != rhs._is_branch)
      return false;
    if (lhs._is_branch)
      return lhs._data.children == rhs._data.children;
    return lhs._data.text == rhs._data.text
       || (lhs._data.text && rhs._data.text
           && *lhs._data.text == *rhs._data.text);
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

namespace detail {
template<typename Char>
auto to_styled_text(basic_styled_text<Char> text) -> basic_styled_text<Char>
{ return text; }

template<typename S, typename Char = char_t<S>,
         FMT_ENABLE_IF(
             std::is_constructible<basic_styled_text<Char>, S&&>::value)>
auto to_styled_text(S&& text) -> basic_styled_text<Char>
{ return basic_styled_text<Char>{std::forward<S>(text)}; }
} // namespace detail

/**
 * Returns styled text with `style` overlaid on its root. Accepts styled text,
 * character strings, and std::basic_string. String inputs become owned leaves.
 * Styled-text lvalues copy their structure; rvalues move it. Children retain
 * their styles. Only the returned value is modified.
 */
template<typename T>
FMT_NODISCARD auto overlay(text_style style, T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  auto result = detail::to_styled_text(std::forward<T>(text));
  result.overlay(style);
  return result;
}

/// Returns styled text with bold enabled on its root, preserving other style.
template<typename T>
FMT_NODISCARD auto bold(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::bold(), std::forward<T>(text)); }

/// Returns styled text with faint enabled on its root, preserving other style.
template<typename T>
FMT_NODISCARD auto faint(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::faint(), std::forward<T>(text)); }

/// Returns styled text with italic enabled on its root, preserving other style.
template<typename T>
FMT_NODISCARD auto italic(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::italic(), std::forward<T>(text)); }

/// Enables underline on the returned root, preserving its other style.
template<typename T>
FMT_NODISCARD auto underline(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::underline(), std::forward<T>(text)); }

/// Returns styled text with blink enabled on its root, preserving other style.
template<typename T>
FMT_NODISCARD auto blink(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::blink(), std::forward<T>(text)); }

/// Enables reverse on the returned root, preserving its other style.
template<typename T>
FMT_NODISCARD auto reverse(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::reverse(), std::forward<T>(text)); }

/// Enables conceal on the returned root, preserving its other style.
template<typename T>
FMT_NODISCARD auto conceal(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::conceal(), std::forward<T>(text)); }

/// Returns styled text with strike enabled on its root, preserving other style.
template<typename T>
FMT_NODISCARD auto strike(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text)))
{ return fmt::overlay(fmt::strike(), std::forward<T>(text)); }

FMT_END_NAMESPACE

#endif // FMT_STYLED_TEXT_H_
