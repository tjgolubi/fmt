/// @file
/// Remove syntactically complete ESC-prefixed ANSI control sequences from
/// `styled_text`.
/// @copyright 2026 Terry Golubiewski, all rights reserved.

#pragma once

#include <tjg/color/styled_text.hpp>
#include <tjg/strip_ansi.hpp>

#include <string>
#include <string_view>

namespace tjg::color {

namespace detail {

struct styled_text_ansi_stripper {
  template<CharType Char>
  static void strip(basic_styled_text<Char>& text) TJG_NOEXCEPT_UNLESS_BAD_ALLOC
  {
    if (!text._children.empty()) {
      for (auto& child : text._children)
        strip(child);
    } else if (text._text) {
      auto stripped =
              tjg::strip_ansi<Char>(std::basic_string_view<Char>{*text._text});
      if (stripped == *text._text)
        return;
      if (stripped.empty()) {
        text._text.reset();
      } else {
        text._text =
          std::make_shared<const std::basic_string<Char>>(std::move(stripped));
      }
    }
  }
}; // styled_text_ansi_stripper

} // detail

/**
 * Returns an owning copy with ANSI sequences removed independently from each
 * leaf. Preserves all node styles, absolute boundaries, and tree structure,
 * including leaves that become empty. Sequences spanning leaves are retained.
 * Does not modify the input; unchanged leaf strings remain shared.
 */
template<CharType Char>
[[nodiscard]]
auto strip_ansi(basic_styled_text<Char> input) TJG_NOEXCEPT_UNLESS_BAD_ALLOC
  -> basic_styled_text<Char>
{
  detail::styled_text_ansi_stripper::strip(input);
  return input;
}

} // tjg::color
