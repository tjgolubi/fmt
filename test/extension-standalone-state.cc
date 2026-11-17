#include "tjg/color.hpp"
auto terminal_state_from_other_translation_unit() -> tjg::color::text_style& {
  return tjg::color::detail::get_style_state();
}
