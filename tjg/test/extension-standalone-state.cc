#include "tjg/color.h"
auto terminal_state_from_other_translation_unit() -> tjg::text_style& {
  return tjg::get_style_state();
}
