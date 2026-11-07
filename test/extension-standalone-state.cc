#include "fmt/color.h"
auto terminal_state_from_other_translation_unit() -> tjg::text_style& {
  return tjg::get_terminal_state();
}
