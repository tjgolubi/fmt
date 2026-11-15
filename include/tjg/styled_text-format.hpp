/// @file
/// Owned structured styled text formatting support.
/// @copyright 2026 Terry Golubiewski, all rights reserved.
#pragma once

#include "styled_text.hpp"
#include "format_support.hpp"
#include "sgr_support.hpp"
#include "text_style-format.hpp"
#include "unicode.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

#include <cstddef>

namespace tjg::detail {

template<CharType Char>
auto styled_text_width(std::basic_string_view<Char> text) noexcept
  -> std::size_t
{
  return tjg::unicode::text_prefix(
                          text, std::numeric_limits<std::size_t>::max()).second;
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

template<CharType Char>
auto styled_text_take(std::basic_string_view<Char> text,
                      styled_text_render_state& state)
  -> styled_text_prefix
{
  const auto prefix = tjg::unicode::text_prefix(text, state.remaining);
  state.remaining -= prefix.second;
  state.stopped = (prefix.first < text.size() || state.remaining == 0);
  return {prefix.first, prefix.second};
}

/**
 * Formats an owning styled-text tree using ordinary ANSI SGR sequences.
 * Relative nodes inherit their enclosing style; absolute nodes start from
 * terminal defaults. Debug presentation (`?`) displays stored node styles and
 * quoted leaf strings, indenting children by two spaces per level.
 */

template<CharType Char>
struct styled_text_renderer {
 private:
  using render_state = detail::styled_text_render_state;

  template<std::output_iterator<Char> OutputIt>
  static auto write_debug(const basic_styled_text<Char>& node, OutputIt out,
                          std::size_t indent = 0)
    -> OutputIt
  {
    for (std::size_t i = 0; i < indent; ++i)
      *out++ = Char{' '};
    if (!node._style.empty()) {
      out = detail::write_debug_value(std::move(out), node._style);
      *out++ = Char{' '};
    }
    if (node._children.empty()) {
      const auto text = node._text
                            ? std::basic_string_view<Char>{*node._text}
                            : std::basic_string_view<Char>{};
      out = tjg::unicode::quote_text(text, out);
      if (indent != 0) *out++ = Char{'\n'};
      return out;
    }
    *out++ = Char{'{'};
    *out++ = Char{'\n'};
    for (const auto& child : node._children)
      out = write_debug(child, out, indent + 2);
    for (std::size_t i = 0; i < indent; ++i)
      *out++ = Char{' '};
    *out++ = Char{'}'};
    *out++ = Char{'\n'};
    return out;
  } // write_debug

  static auto measure(const basic_styled_text<Char>& node, render_state& state)
    -> std::size_t
  {
    if (node.empty() || state.stopped)
      return 0;
    if (node._children.empty()) {
      const auto& text = *node._text;
      return detail::styled_text_take(
                 std::basic_string_view<Char>{text.data(), text.size()}, state)
          .width;
    }
    auto width = std::size_t{0};
    for (const auto& child : node._children) {
      width += measure(child, state);
      if (state.stopped) break;
    }
    return width;
  } // measure

  template<std::output_iterator<Char> OutputIt>
  static auto transition(text_style& style, OutputIt out) -> OutputIt {
    auto& terminal = get_style_state();
    if (style == terminal)
      return out;
    auto sgr = sgr_buffer{};
    detail::append_sgr_transition(sgr, style, terminal);
    terminal = style.absolute(false);
    for (const auto ch : sgr.view())
      *out++ = static_cast<Char>(ch);
    return out;
  } // transition

  template<std::output_iterator<Char> OutputIt>
  static auto write_node(const basic_styled_text<Char>& node,
                         text_style enclosing,
                         OutputIt out, render_state& state,
                         const string_format_specs<Char>& specs,
                         std::size_t left = 0, std::size_t right = 0)
      -> OutputIt
  {
    if ((node.empty() || state.stopped) && left == 0 && right == 0)
      return out;
    auto effective = node._style;
    if (!effective.is_absolute())
      effective.underlay(enclosing);
    if (left != 0) {
      out = transition(effective, std::move(out));
      out = specs.pad(std::move(out), left);
    }
    if (!node._children.empty()) {
      if (effective.is_absolute())
        out = transition(effective, std::move(out));
      for (const auto& child : node._children) {
        if (state.stopped) break;
        out = write_node(child, effective, std::move(out), state, specs);
      }
    } else if (!node.empty() && !state.stopped) {
      const auto& text  = *node._text;
      const auto prefix = detail::styled_text_take(
          std::basic_string_view<Char>{text.data(), text.size()}, state);
      if (prefix.size != 0) {
        out = transition(effective, std::move(out));
        out = std::copy(text.data(), text.data() + prefix.size, std::move(out));
      }
    }
    if (right != 0) {
      out = transition(effective, std::move(out));
      out = specs.pad(std::move(out), right);
    }
    return out;
  } // write_node

 public:

  // Shared by the formatter and ostream bridge. Options are already resolved;
  // stream insertion never builds or parses a format-specification string.
  template<typename FormatContext>
  static auto format_with_specs(const basic_styled_text<Char>& value,
                                FormatContext& ctx,
                                string_format_specs<Char> specs)
    -> decltype(ctx.out())
  {
    const auto limit = specs.precision;
    if (specs.debug)
      return write_debug(value, ctx.out());
    auto state   = render_state{limit};
    auto padding = std::size_t{0};
    if (specs.width != 0 || limit != std::numeric_limits<std::size_t>::max()) {
      auto measuring   = render_state{limit};
      const auto width = measure(value, measuring);
      if (specs.width > width) padding = specs.width - width;
      if (width == 0 && padding == 0)
        return ctx.out();
    }
    const auto left = (specs.alignment == '>')
                    ? padding : (specs.alignment == '^')
                              ? (padding / 2) : 0;
    if (limit == 0) state.stopped = true;
    auto initial = get_style_state();
    auto scope = detail::style_scope{};
    auto out = write_node(value, initial, ctx.out(), state, specs,
                          left, padding - left);
    return transition(initial, std::move(out));
  } // format_with_specs

};  // styled_text_renderer

template<CharType Char>
struct standard_styled_text_formatter : styled_text_renderer<Char> {
  string_format_specs<Char> specs;

  constexpr auto parse(std::basic_format_parse_context<Char>& ctx)
    -> decltype(ctx.begin())
  {
    const auto end = specs.parse(ctx);
    constexpr auto max_prec = std::numeric_limits<std::size_t>::max();
    if (specs.debug
      && (   specs.width     != 0 || specs.precision     != max_prec
          || specs.width_arg >= 0 || specs.precision_arg >= 0))
    {
      standard_format_error(
                "styled_text debug format does not accept width or precision");
    }
    return end;
  }

  template<typename Context>
  auto format(const basic_styled_text<Char>& value, Context& ctx) const
    -> decltype(ctx.out())
  {
    return this->format_with_specs(value, ctx, specs.resolve(ctx));
  }
}; // standard_styled_text_formatter

} // tjg::detail

namespace std {

template<tjg::CharType Char>
struct formatter<tjg::basic_styled_text<Char>, Char>
  : tjg::detail::standard_styled_text_formatter<Char> {};

} // std
