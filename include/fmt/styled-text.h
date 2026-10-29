// Owned structured styled text.
#ifndef FMT_STYLED_TEXT_H_
#define FMT_STYLED_TEXT_H_

#include "color.h"

#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

FMT_BEGIN_NAMESPACE

/** An owned string or sequence of styled children, with a style for this node. */
class styled_text {
 public:
  using leaf = std::string;
  using branch = std::vector<styled_text>;

 private:
  text_style style_;
  bool is_branch_;
  union storage {
    leaf text;
    branch children;
    storage() {}
    ~storage() {}
  } data_;

  void destroy() noexcept;

 public:
  styled_text();
  styled_text(leaf text, text_style style = {});
  styled_text(const char* text, text_style style = {});
  styled_text(branch children, text_style style = {});
  styled_text(const styled_text& other);
  styled_text(styled_text&& other) noexcept;
  ~styled_text();
  auto operator=(const styled_text& other) -> styled_text&;
  auto operator=(styled_text&& other) noexcept -> styled_text&;

  auto style() const noexcept -> text_style { return style_; }
  auto style(text_style value) noexcept -> styled_text& {
    style_ = value;
    return *this;
  }
  bool is_branch() const noexcept { return is_branch_; }

  auto as_leaf() -> leaf&;
  auto as_leaf() const -> const leaf&;
  auto as_branch() -> branch&;
  auto as_branch() const -> const branch&;

  friend bool operator==(const styled_text& lhs, const styled_text& rhs);
  friend bool operator!=(const styled_text& lhs, const styled_text& rhs) {
    return !(lhs == rhs);
  }
  friend auto operator+(styled_text lhs, styled_text rhs) -> styled_text;
};

inline styled_text::styled_text() : styled_text(leaf{}) {}
inline styled_text::styled_text(leaf text, text_style style)
    : style_(style), is_branch_(false) {
  new (&data_.text) leaf(std::move(text));
}
inline styled_text::styled_text(const char* text, text_style style)
    : styled_text(leaf(text), style) {}
inline styled_text::styled_text(branch children, text_style style)
    : style_(style), is_branch_(true) {
  new (&data_.children) branch(std::move(children));
}
inline styled_text::styled_text(const styled_text& other)
    : style_(other.style_), is_branch_(other.is_branch_) {
  if (is_branch_)
    new (&data_.children) branch(other.data_.children);
  else
    new (&data_.text) leaf(other.data_.text);
}
inline styled_text::styled_text(styled_text&& other) noexcept
    : style_(other.style_), is_branch_(other.is_branch_) {
  if (is_branch_)
    new (&data_.children) branch(std::move(other.data_.children));
  else
    new (&data_.text) leaf(std::move(other.data_.text));
}
inline void styled_text::destroy() noexcept {
  if (is_branch_)
    data_.children.~branch();
  else
    data_.text.~leaf();
}
inline styled_text::~styled_text() { destroy(); }
inline auto styled_text::operator=(const styled_text& other) -> styled_text& {
  if (this == &other) return *this;
  auto copy = styled_text(other);
  return *this = std::move(copy);
}
inline auto styled_text::operator=(styled_text&& other) noexcept -> styled_text& {
  if (this == &other) return *this;
  // Move out before destroying this node: other may be one of its descendants.
  auto moved = styled_text(std::move(other));
  destroy();
  style_ = moved.style_;
  is_branch_ = moved.is_branch_;
  if (is_branch_)
    new (&data_.children) branch(std::move(moved.data_.children));
  else
    new (&data_.text) leaf(std::move(moved.data_.text));
  return *this;
}
inline auto styled_text::as_leaf() -> leaf& {
  if (is_branch_) FMT_THROW(format_error("styled_text does not hold a leaf"));
  return data_.text;
}
inline auto styled_text::as_leaf() const -> const leaf& {
  if (is_branch_) FMT_THROW(format_error("styled_text does not hold a leaf"));
  return data_.text;
}
inline auto styled_text::as_branch() -> branch& {
  if (!is_branch_) FMT_THROW(format_error("styled_text does not hold a branch"));
  return data_.children;
}
inline auto styled_text::as_branch() const -> const branch& {
  if (!is_branch_) FMT_THROW(format_error("styled_text does not hold a branch"));
  return data_.children;
}
inline bool operator==(const styled_text& lhs, const styled_text& rhs) {
  if (lhs.style_ != rhs.style_ || lhs.is_branch_ != rhs.is_branch_) return false;
  return lhs.is_branch_ ? lhs.data_.children == rhs.data_.children
                        : lhs.data_.text == rhs.data_.text;
}
inline auto operator+(styled_text lhs, styled_text rhs) -> styled_text {
  auto children = styled_text::branch{};
  children.reserve(2);
  children.push_back(std::move(lhs));
  children.push_back(std::move(rhs));
  return styled_text(std::move(children));
}

template<typename T>
inline bool holds_alternative(const styled_text& value) noexcept {
  static_assert(std::is_same<T, styled_text::leaf>::value ||
                std::is_same<T, styled_text::branch>::value,
                "T must be styled_text::leaf or styled_text::branch");
  return value.is_branch() == std::is_same<T, styled_text::branch>::value;
}

namespace detail {
template<typename T> struct styled_text_access;
template<> struct styled_text_access<styled_text::leaf> {
  template<typename Node>
  static auto get(Node& node) -> decltype(node.as_leaf()) {
    return node.as_leaf();
  }
};
template<> struct styled_text_access<styled_text::branch> {
  template<typename Node>
  static auto get(Node& node) -> decltype(node.as_branch()) {
    return node.as_branch();
  }
};
} // namespace detail

template<typename T> inline auto get(styled_text& value) -> T& {
  return detail::styled_text_access<T>::get(value);
}
template<typename T> inline auto get(const styled_text& value) -> const T& {
  return detail::styled_text_access<T>::get(value);
}
template<typename T> inline auto get(styled_text&& value) -> T&& {
  return std::move(get<T>(value));
}
template<typename T> inline auto get(const styled_text&& value) -> const T&& {
  return std::move(get<T>(value));
}

FMT_END_NAMESPACE

#endif // FMT_STYLED_TEXT_H_
