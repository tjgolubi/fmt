// Common formatting failure handling for the TJG extension.
#ifndef TJG_FORMAT_SUPPORT_H_
#define TJG_FORMAT_SUPPORT_H_
#include <cstdlib>
#include <format>
namespace tjg::detail {
[[noreturn]] inline void standard_format_error(const char* message) {
#if defined(__cpp_exceptions) || defined(_CPPUNWIND)
  throw std::format_error(message);
#else
  (void)message;
  std::abort();
#endif
}
}  // namespace tjg::detail
#endif  // TJG_FORMAT_SUPPORT_H_
