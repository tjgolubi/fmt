#ifndef TJG_STANDARD_FORMAT_TEST_H_
#define TJG_STANDARD_FORMAT_TEST_H_
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
template <typename S, typename... Args>
auto runtime_format(const S& pattern, Args&&... args) {
  auto view = std::basic_string_view{pattern};
  using Char = typename decltype(view)::value_type;
  using Context = std::conditional_t<std::is_same_v<Char, char>,
                                     std::format_context, std::wformat_context>;
  return std::vformat(view, std::make_format_args<Context>(args...));
}
#endif
