// Assertion, contract, and exception policies for {color}.
#ifndef TJG_ERROR_SUPPORT_H_
#define TJG_ERROR_SUPPORT_H_

#include <cstdio>
#include <cstdlib>
#include <source_location>

namespace tjg::detail {
// Avoid formatting or allocating while reporting a failure.
[[noreturn]] inline void assert_fail(
    const char* message,
    std::source_location location = std::source_location::current()) noexcept {
  std::fprintf(stderr, "%s:%u: assertion failed: %s", location.file_name(),
               static_cast<unsigned>(location.line()), message);
  std::fflush(stderr);
  std::abort();
}
}  // namespace tjg::detail

// Define any policy macro before including a {color} header to override it.
// Use the same definitions throughout the program.
#ifndef TJG_ENFORCE
#  define TJG_ENFORCE(condition, message) \
    ((condition) ? (void)0 : ::tjg::detail::assert_fail(message))
#endif

#ifndef TJG_ASSERT
#  ifdef NDEBUG
// Like standard assert, do not evaluate the condition or message in Release.
#    define TJG_ASSERT(condition, message) ((void)0)
#  else
#    define TJG_ASSERT(condition, message) TJG_ENFORCE(condition, message)
#  endif
#endif

// Preconditions and postconditions remain active regardless of NDEBUG.
#ifndef TJG_EXPECTS
#  define TJG_EXPECTS(condition, message) TJG_ENFORCE(condition, message)
#endif
#ifndef TJG_ENSURES
#  define TJG_ENSURES(condition, message) TJG_ENFORCE(condition, message)
#endif

#ifndef TJG_THROW
#  if defined(__cpp_exceptions) || defined(_CPPUNWIND)
#    define TJG_THROW(exception) throw exception
#  else
#    define TJG_THROW(exception) ::tjg::detail::assert_fail((exception).what())
#  endif
#endif

// May be defined as noexcept when allocation failure terminates. Apply only
// where allocation failure is the sole intended escaping exception.
#ifndef TJG_NOEXCEPT_UNLESS_BAD_ALLOC
#  define TJG_NOEXCEPT_UNLESS_BAD_ALLOC
#endif

#endif  // TJG_ERROR_SUPPORT_H_
