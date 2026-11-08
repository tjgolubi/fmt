// Common formatting failure handling for the TJG extension.
#ifndef TJG_FORMAT_SUPPORT_H_
#define TJG_FORMAT_SUPPORT_H_
#include "error_support.h"
#include <format>
namespace tjg::detail {
[[noreturn]] inline void standard_format_error(const char* message) {
  TJG_THROW(std::format_error(message));
}
}  // namespace tjg::detail
#endif  // TJG_FORMAT_SUPPORT_H_
