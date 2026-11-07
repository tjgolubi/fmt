// Owned structured styled text.
#ifndef FMT_STYLED_TEXT_H_
#define FMT_STYLED_TEXT_H_

#include <algorithm>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "color.h"

namespace tjg {

namespace detail {
struct styled_text_ansi_stripper;
template <typename Char> struct styled_text_formatter;

// Match ordinary string formatting: UTF-8 display width for char, code-unit
// count for other character types.
template <typename Char>
auto styled_text_width(std::basic_string_view<Char> text) noexcept
    -> std::size_t {
  return text.size();
}

inline auto styled_text_width(std::string_view text) noexcept -> std::size_t {
  auto width = std::size_t{0};
  fmt::detail::for_each_codepoint(fmt::string_view{text.data(), text.size()},
                                  [&](uint32_t cp, auto) {
                                    width += fmt::detail::display_width_of(cp);
                                    return true;
                                  });
  return width;
}

struct styled_text_render_state {
  std::size_t remaining;
  bool stopped = false;
  explicit styled_text_render_state(std::size_t limit) : remaining{limit} {}
};

struct styled_text_prefix {
  std::size_t size;
  std::size_t width;
};

template <typename Char>
auto styled_text_take(std::basic_string_view<Char> text,
                      styled_text_render_state& state) -> styled_text_prefix {
  const auto size = (std::min)(text.size(), state.remaining);
  state.remaining -= size;
  state.stopped = size < text.size() || state.remaining == 0;
  return {size, size};
}

inline auto styled_text_take(std::string_view text,
                             styled_text_render_state& state)
    -> styled_text_prefix {
  auto result = styled_text_prefix{0, 0};
  fmt::detail::for_each_codepoint(
      fmt::string_view{text.data(), text.size()}, [&](uint32_t cp, auto bytes) {
        const auto width = fmt::detail::display_width_of(cp);
        if (width > state.remaining) {
          state.stopped = true;
          return false;
        }
        result.size += bytes.size();
        result.width += width;
        state.remaining -= width;
        return true;
      });
  if (state.remaining == 0) state.stopped = true;
  return result;
}  // styled_text_take

}  // namespace detail

/** An owned string or sequence of styled children, with a style for this node.
 */
template <typename Char> class basic_styled_text {
 public:
  using char_type = Char;
  using string_type = std::basic_string<Char>;

 private:
  using leaf = std::shared_ptr<const string_type>;
  using branch = std::vector<basic_styled_text>;
  friend struct detail::styled_text_formatter<Char>;
  friend struct detail::styled_text_ansi_stripper;
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

#if FMT_GCC_VERSION >= 1300 && FMT_GCC_VERSION < 1400
  // GCC 13 can report an inactive union member as uninitialized when this
  // vector move is inlined through concatenation. Keep leaf moves inline.
  static FMT_NOINLINE void move_branch(branch* target,
                                       branch& source) noexcept {
    new (target) branch{std::move(source)};
  }
#endif

  void destroy() noexcept {
    if (_is_branch)
      _data.children.~branch();
    else
      _data.text.~leaf();
  }

  // Avoid GCC 13/14 recursive-inlining warnings about the inactive union
  // member.
  static FMT_NOINLINE auto branch_display_width(const branch& children) noexcept
      -> std::size_t {
    auto width = std::size_t{0};
    for (const auto& child : children) width += child.display_width();
    return width;
  }

  void append_plain_text(string_type& result) const {
    if (_is_branch) {
      for (const auto& child : _data.children) child.append_plain_text(result);
    } else if (_data.text) {
      result.append(*_data.text);
    }
  }

 public:
  basic_styled_text() : _style{}, _is_branch{false}, _data{leaf{}} {}

  basic_styled_text(string_type text, text_style style = {})
      : _style{style},
        _is_branch{false},
        _data{text.empty()
                  ? leaf{}
                  : std::make_shared<const string_type>(std::move(text))} {}

  basic_styled_text(const Char* text, text_style style = {})
      : basic_styled_text{string_type{text}, style} {}

  basic_styled_text(const basic_styled_text& other)
      : _style{other._style}, _is_branch{other._is_branch} {
    if (_is_branch)
      new (&_data.children) branch{other._data.children};
    else
      new (&_data.text) leaf{other._data.text};
  }

  basic_styled_text(basic_styled_text&& other) noexcept
      : _style{other._style}, _is_branch{other._is_branch} {
    if (_is_branch) {
#if FMT_GCC_VERSION >= 1300 && FMT_GCC_VERSION < 1400
      move_branch(&_data.children, other._data.children);
#else
      new (&_data.children) branch{std::move(other._data.children)};
#endif
    } else {
      new (&_data.text) leaf{std::move(other._data.text)};
    }
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
   * Applies `overrides` recursively in place, skipping absolute nodes and
   * their entire subtrees. Preserves text and tree structure. Specified
   * colors replace matching channels; emphasis bits are combined by OR.
   * Returns this node for chaining.
   * To overlay only this node, use style(style().overlay(overrides)).
   */
  auto overlay(text_style overrides) noexcept -> basic_styled_text& {
    if (_style.is_absolute()) return *this;
    _style.overlay(overrides);
    if (_is_branch) {
      for (auto& child : _data.children) child.overlay(overrides);
    }
    return *this;
  }

  /**
   * Underlays this node's style in place, leaving its text and children intact.
   * Specified colors replace unspecified matching channels; emphasis bits are
   * combined by OR. Returns a reference to this node for chaining.
   */
  auto underlay(text_style style) noexcept -> basic_styled_text& {
    _style.underlay(style);
    return *this;
  }

  // Empty leaves are concatenation identities.
  auto empty() const noexcept -> bool { return !_is_branch && !_data.text; }

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

  /**
   * Returns the sum of leaf widths using ordinary fmt string-formatting rules.
   * Narrow leaves are decoded as UTF-8; other character types count code units.
   * Styles do not contribute width. Raw escapes are not interpreted.
   */
  FMT_NODISCARD auto display_width() const noexcept -> std::size_t {
    if (!_is_branch)
      return _data.text
                 ? detail::styled_text_width(std::basic_string_view<Char>{
                       _data.text->data(), _data.text->size()})
                 : 0;
    return branch_display_width(_data.children);
  }  // display_width

  auto operator+=(basic_styled_text rhs) -> basic_styled_text& {
    if (rhs.empty()) return *this;
    if (empty()) return *this = std::move(rhs);
    if (_is_branch && _style.empty()) {
      _data.children.push_back(std::move(rhs));
      return *this;
    }
    auto children = branch{};
    // Allocate before moving either operand. The following moves cannot throw.
    children.reserve(2);
    children.push_back(std::move(*this));
    children.push_back(std::move(rhs));
    destroy();
    new (&_data.children) branch{std::move(children)};
    _is_branch = true;
    _style = text_style{};  // For exposition only.
    return *this;
  }  // operator+=

  friend bool operator==(const basic_styled_text& lhs,
                         const basic_styled_text& rhs) {
    if (lhs._style != rhs._style || lhs._is_branch != rhs._is_branch)
      return false;
    if (lhs._is_branch) return lhs._data.children == rhs._data.children;
    return lhs._data.text == rhs._data.text ||
           (lhs._data.text && rhs._data.text &&
            *lhs._data.text == *rhs._data.text);
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
};  // basic_styled_text

using styled_text = basic_styled_text<char>;
using wstyled_text = basic_styled_text<wchar_t>;

namespace detail {

template <typename Char, typename OutIter>
FMT_NODISCARD FMT_CONSTEXPR inline auto copy(const std::basic_string<Char>& str,
                                             OutIter out) -> OutIter {
  return std::copy(str.begin(), str.end(), out);
}

}  // namespace detail

/**
 * Formats an owning styled-text tree using ordinary ANSI SGR sequences.
 * Relative nodes inherit their enclosing style; absolute nodes start from
 * terminal defaults. Debug presentation (`?`) displays stored node styles and
 * quoted leaf strings, indenting children by two spaces per level.
 */
namespace detail {
template <typename Char> struct styled_text_formatter {
  FMT_CONSTEXPR auto parse(fmt::parse_context<Char>& ctx) -> const Char* {
    if (ctx.begin() == ctx.end() || *ctx.begin() == Char('}'))
      return ctx.begin();
    return fmt::detail::parse_format_specs(ctx.begin(), ctx.end(), _specs, ctx,
                                           fmt::detail::type::string_type);
  }

  FMT_CONSTEXPR void set_debug_format(bool set = true) {
    _specs.set_type(set ? fmt::presentation_type::debug
                        : fmt::presentation_type::none);
  }

 private:
  fmt::detail::dynamic_format_specs<Char> _specs;
  using render_state = detail::styled_text_render_state;

  template <typename OutputIt>
  static auto write_debug(const basic_styled_text<Char>& node, OutputIt out,
                          std::size_t indent = 0) -> OutputIt {
    for (std::size_t i = 0; i < indent; ++i) *out++ = Char(' ');
    if (!node._style.empty()) {
      out = detail::write_style_value<Char>(out, node._style);
      *out++ = Char(' ');
    }
    if (!node._is_branch) {
      const auto text = node._data.text
                            ? std::basic_string_view<Char>{*node._data.text}
                            : std::basic_string_view<Char>{};
      out = fmt::detail::write_escaped_string(
          out, fmt::basic_string_view<Char>{text.data(), text.size()});
      if (indent != 0) *out++ = Char('\n');
      return out;
    }
    *out++ = Char('{');
    *out++ = Char('\n');
    for (const auto& child : node._data.children)
      out = write_debug(child, out, indent + 2);
    for (std::size_t i = 0; i < indent; ++i) *out++ = Char(' ');
    *out++ = Char('}');
    *out++ = Char('\n');
    return out;
  }

  static auto measure(const basic_styled_text<Char>& node, render_state& state)
      -> std::size_t {
    if (node.empty() || state.stopped) return 0;
    if (!node._is_branch) {
      const auto& text = *node._data.text;
      return detail::styled_text_take(
                 std::basic_string_view<Char>{text.data(), text.size()}, state)
          .width;
    }
    auto width = std::size_t{0};
    for (const auto& child : node._data.children) {
      width += measure(child, state);
      if (state.stopped) break;
    }
    return width;
  }

  template <typename OutputIt>
  static auto transition(text_style style, text_style& terminal, OutputIt out)
      -> OutputIt {
    if (style == terminal) return out;
    const auto sgr = tjg::to_sgr<Char>(style, terminal);
    terminal = style.absolute(false);
    return detail::copy<Char>(sgr, out);
  }

  template <typename OutputIt>
  static auto write_node(const basic_styled_text<Char>& node,
                         text_style enclosing, text_style& terminal,
                         OutputIt out, render_state& state,
                         const fmt::format_specs& specs, std::size_t left = 0,
                         std::size_t right = 0) -> OutputIt {
    if ((node.empty() || state.stopped) && left == 0 && right == 0) return out;
    auto effective = node._style;
    // Output absolute style changes immediately; otherwise wait until before
    // text output.
    if (effective.is_absolute()) {
      out = transition(effective, terminal, out);
      effective = terminal;
    } else {
      effective = tjg::underlay(effective, enclosing);
    }
    if (left != 0) {
      out = transition(effective, terminal, out);
      out = fmt::detail::fill<Char>(out, left, specs);
    }
    if (node._is_branch) {
      for (const auto& child : node._data.children) {
        if (state.stopped) break;
        out = write_node(child, effective, terminal, out, state, specs);
      }
    } else if (!node.empty() && !state.stopped) {
      const auto& text = *node._data.text;
      const auto prefix = detail::styled_text_take(
          std::basic_string_view<Char>{text.data(), text.size()}, state);
      if (prefix.size != 0) {
        out = transition(effective, terminal, out);
        out = std::copy(text.data(), text.data() + prefix.size, out);
      }
    }
    if (right != 0) {
      out = transition(effective, terminal, out);
      out = fmt::detail::fill<Char>(out, right, specs);
    }
    return out;
  }

 public:
  template <typename FormatContext>
  auto format(const basic_styled_text<Char>& value, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto specs = fmt::format_specs{_specs};
    fmt::detail::handle_dynamic_spec(specs.dynamic_width(), specs.width,
                                     _specs.width_ref, ctx);
    fmt::detail::handle_dynamic_spec(specs.dynamic_precision(), specs.precision,
                                     _specs.precision_ref, ctx);
    return format_with_specs(value, ctx, specs);
  }

  // Shared by the formatter and ostream bridge. Options are already resolved;
  // stream insertion never builds or parses a format-specification string.
  template <typename FormatContext>
  static auto format_with_specs(const basic_styled_text<Char>& value,
                                FormatContext& ctx, fmt::format_specs specs)
      -> decltype(ctx.out()) {
    if (specs.type() == fmt::presentation_type::debug) {
      if (specs.width == 0 && specs.precision < 0)
        return write_debug(value, ctx.out());
      auto buffer = basic_memory_buffer<Char>{};
      write_debug(value, std::back_inserter(buffer));
      specs.set_type(fmt::presentation_type::string);
      return fmt::detail::write<Char>(
          ctx.out(), fmt::basic_string_view<Char>{buffer.data(), buffer.size()},
          specs);
    }
    const auto limit = (specs.precision < 0)
                           ? (std::numeric_limits<std::size_t>::max)()
                           : static_cast<std::size_t>(specs.precision);
    auto state = render_state{limit};
    auto padding = std::size_t{0};
    if (specs.width != 0 || specs.precision >= 0) {
      auto measuring = render_state{limit};
      const auto width = measure(value, measuring);
      if (static_cast<std::size_t>(specs.width) > width)
        padding = static_cast<std::size_t>(specs.width) - width;
      if (width == 0 && padding == 0) return ctx.out();
    }
    const auto left = specs.align() == fmt::align::right    ? padding
                      : specs.align() == fmt::align::center ? padding / 2
                                                            : 0;
    if (limit == 0) state.stopped = true;
    const auto initial = get_terminal_state();
    auto& terminal = get_terminal_state();
    detail::style_scope scope{};
    auto out = write_node(value, initial, terminal, ctx.out(), state, specs,
                          left, padding - left);
    return transition(initial, terminal, out);
  }
};  // styled-text formatter
}  // namespace detail

namespace detail {
template <typename Char>
auto to_styled_text(basic_styled_text<Char> text) -> basic_styled_text<Char> {
  return text;
}

template <
    typename S,
    typename Char = typename decltype(std::basic_string_view{
        std::declval<S&>()})::value_type,
    FMT_ENABLE_IF(std::is_constructible<basic_styled_text<Char>, S&&>::value)>
auto to_styled_text(S&& text) -> basic_styled_text<Char> {
  return basic_styled_text<Char>{std::forward<S>(text)};
}

}  // namespace detail

/**
 * Returns styled text with `overrides` applied recursively, skipping absolute
 * nodes and their entire subtrees. Specified colors replace matching channels;
 * Emphasis bits are combined by OR. Accepts styled text, character strings,
 * and std::basic_string. String
 * inputs become owned leaves. Styled-text lvalues copy their structure;
 * rvalues move it. Only the returned value is modified.
 */
template <typename T>
FMT_NODISCARD auto overlay(text_style overrides, T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  auto result = detail::to_styled_text(std::forward<T>(text));
  result.overlay(overrides);
  return result;
}

/**
 * Returns styled text with `style` underlaid on its root. Accepts styled text,
 * character strings, and std::basic_string. String inputs become owned leaves.
 * Styled-text lvalues copy their structure; rvalues move it. Children retain
 * their styles. Only the returned value is modified.
 */
template <typename T>
FMT_NODISCARD auto underlay(text_style style, T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  auto result = detail::to_styled_text(std::forward<T>(text));
  result.underlay(style);
  return result;
}

/**
 * Returns styled text with an absolute root, preserving its style and children.
 * Accepts the same owning text inputs as overlay. Lvalues copy; rvalues move.
 * Only the returned value is modified.
 */
template <typename T>
FMT_NODISCARD auto absolute(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::absolute(text_style{}), std::forward<T>(text));
}

/// Returns styled text with bold enabled on its root, preserving other style.
template <typename T>
FMT_NODISCARD auto bold(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::bold(), std::forward<T>(text));
}

/// Returns styled text with faint enabled on its root, preserving other style.
template <typename T>
FMT_NODISCARD auto faint(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::faint(), std::forward<T>(text));
}

/// Returns styled text with italic enabled on its root, preserving other style.
template <typename T>
FMT_NODISCARD auto italic(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::italic(), std::forward<T>(text));
}

/// Enables underline on the returned root, preserving its other style.
template <typename T>
FMT_NODISCARD auto underline(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::underline(), std::forward<T>(text));
}

/// Returns styled text with blink enabled on its root, preserving other style.
template <typename T>
FMT_NODISCARD auto blink(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::blink(), std::forward<T>(text));
}

/// Enables reverse on the returned root, preserving its other style.
template <typename T>
FMT_NODISCARD auto reverse(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::reverse(), std::forward<T>(text));
}

/// Enables conceal on the returned root, preserving its other style.
template <typename T>
FMT_NODISCARD auto conceal(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::conceal(), std::forward<T>(text));
}

/// Returns styled text with strike enabled on its root, preserving other style.
template <typename T>
FMT_NODISCARD auto strike(T&& text)
    -> decltype(detail::to_styled_text(std::forward<T>(text))) {
  return tjg::underlay(tjg::strike(), std::forward<T>(text));
}

}  // namespace tjg

FMT_BEGIN_NAMESPACE
// Adapt extension-owned trees to the existing formatting engine.
template <typename Char>
struct formatter<tjg::basic_styled_text<Char>, Char>
    : tjg::detail::styled_text_formatter<Char> {};
FMT_END_NAMESPACE

#endif  // FMT_STYLED_TEXT_H_
