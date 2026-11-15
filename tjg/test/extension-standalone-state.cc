#include "tjg/text_style-format.hpp"
auto terminal_state_from_other_translation_unit() -> tjg::text_style& {
  return tjg::detail::get_style_state();
}
