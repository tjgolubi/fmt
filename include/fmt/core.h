// Formatting library for C++23, based on {fmt}.
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors.
// Copyright (c) 2026 Terry Golubiewski.
// Distributed under the MIT license; see LICENSE.

#ifndef FMT_CORE_H_
#define FMT_CORE_H_

#if !defined(__GNUC__) && !defined(__clang__)
#  error "This fmt fork requires GCC or Clang"
#endif
#if __cplusplus <= 202002L
#  error "This fmt fork requires C++23"
#endif

#include <algorithm>
#include <concepts>
#include <cstdio>
#include <format>
#include <iterator>
#include <locale>
#include <print>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <version>

#if !defined(__cpp_lib_format) || __cpp_lib_format < 202110L
#  error "This fmt fork requires C++23 std::format support"
#endif
#if !defined(__cpp_lib_print) || __cpp_lib_print < 202207L
#  error "This fmt fork requires C++23 std::print support"
#endif

#define FMT_VERSION 120201
// Source compatibility for literal format-string annotations. These no longer
// select a separate parser or a compiled-format engine.
#define FMT_STRING(s) s

namespace fmt {
inline namespace v12 {
using std::basic_format_arg;
using std::basic_format_args;
using std::basic_format_context;
using std::basic_format_parse_context;
using std::basic_format_string;
using std::basic_string_view;
using std::format;
using std::format_args;
using std::format_context;
using std::format_error;
using std::format_parse_context;
using std::format_string;
using std::format_to;
using std::format_to_n;
using std::format_to_n_result;
using std::formatted_size;
using std::formatter;
using std::make_format_args;
using std::make_wformat_args;
using std::string_view;
using std::vformat;
using std::vformat_to;
using std::wformat_args;
using std::wformat_context;
using std::wformat_parse_context;
using std::wformat_string;

// Kept as public type aliases, implemented by the standard library.
template <typename T> using remove_cvref_t = std::remove_cvref_t<T>;
template <typename T, typename Char = char>
using is_formattable = std::bool_constant<std::formattable<T, Char>>;
template <typename T, typename Char = char>
concept formattable = std::formattable<T, Char>;

template <typename Char = char> struct runtime_format_string {
  std::basic_string_view<Char> str;
};
inline auto runtime(string_view s) -> runtime_format_string<> { return {s}; }
inline auto runtime(std::wstring_view s) -> runtime_format_string<wchar_t> {
  return {s};
}

template <typename... T>
auto format(runtime_format_string<> s, T&&... args) -> std::string {
  return std::vformat(s.str, std::make_format_args(args...));
}
template <typename... T>
auto format(runtime_format_string<wchar_t> s, T&&... args) -> std::wstring {
  return std::vformat(s.str, std::make_wformat_args(args...));
}
template <typename... T>
auto format(const std::locale& loc, runtime_format_string<> s, T&&... args)
    -> std::string {
  return std::vformat(loc, s.str, std::make_format_args(args...));
}
template <typename OutputIt, typename... T>
auto format_to(OutputIt out, runtime_format_string<> s, T&&... args)
    -> OutputIt {
  return std::vformat_to(out, s.str, std::make_format_args(args...));
}
template <typename OutputIt, typename... T>
auto format_to(OutputIt out, runtime_format_string<wchar_t> s, T&&... args)
    -> OutputIt {
  return std::vformat_to(out, s.str, std::make_wformat_args(args...));
}

template <typename OutputIt>
auto vformat_to_n(OutputIt out, size_t n, string_view s, format_args args)
    -> format_to_n_result<OutputIt> {
  const auto text = std::vformat(s, args);
  out = std::copy_n(text.begin(), std::min(n, text.size()), out);
  return {out, static_cast<std::iter_difference_t<OutputIt>>(text.size())};
}
template <typename OutputIt, typename... T>
auto format_to_n(OutputIt out, size_t n, runtime_format_string<> s, T&&... args)
    -> format_to_n_result<OutputIt> {
  return fmt::vformat_to_n(out, n, s.str, std::make_format_args(args...));
}
template <typename... T>
auto formatted_size(runtime_format_string<> s, T&&... args) -> size_t {
  return fmt::format(s, std::forward<T>(args)...).size();
}
struct format_to_result {
  char* out;
  bool truncated;
  operator char*() const {
    if (truncated) throw format_error("output is truncated");
    return out;
  }
};
template <size_t N, typename... T>
auto format_to(char (&out)[N], format_string<T...> s, T&&... args)
    -> format_to_result {
  const auto result = std::format_to_n(out, N, s, std::forward<T>(args)...);
  return {result.out, result.size > static_cast<std::ptrdiff_t>(N)};
}
template <size_t N, typename... T>
auto format_to(char (&out)[N], runtime_format_string<> s, T&&... args)
    -> format_to_result {
  const auto result = fmt::vformat_to_n(out, N, s.str, std::make_format_args(args...));
  return {result.out, result.size > static_cast<std::ptrdiff_t>(N)};
}

inline void vprint(FILE* file, string_view s, format_args args) {
  std::vprint_unicode(file, s, args);
}
inline void vprint(string_view s, format_args args) {
  fmt::vprint(stdout, s, args);
}
inline void vprintln(FILE* file, string_view s, format_args args) {
  std::print(file, "{}\n", std::vformat(s, args));
}
template <typename... T>
void print(FILE* file, format_string<T...> s, T&&... args) {
  std::print(file, s, std::forward<T>(args)...);
}
template <typename... T>
void print(format_string<T...> s, T&&... args) {
  std::print(s, std::forward<T>(args)...);
}
template <typename... T>
void print(FILE* file, runtime_format_string<> s, T&&... args) {
  fmt::vprint(file, s.str, std::make_format_args(args...));
}
template <typename... T>
void print(runtime_format_string<> s, T&&... args) {
  fmt::print(stdout, s, std::forward<T>(args)...);
}
template <typename... T>
void println(FILE* file, format_string<T...> s, T&&... args) {
  std::println(file, s, std::forward<T>(args)...);
}
template <typename... T>
void println(format_string<T...> s, T&&... args) {
  std::println(s, std::forward<T>(args)...);
}
template <typename... T>
void println(FILE* file, runtime_format_string<> s, T&&... args) {
  fmt::vprintln(file, s.str, std::make_format_args(args...));
}
template <typename... T>
void println(runtime_format_string<> s, T&&... args) {
  fmt::println(stdout, s, std::forward<T>(args)...);
}

// Preserve the commonly used growable buffer interface with standard storage.
// SIZE is a reservation hint; this implementation has no inline allocation.
template <typename T, size_t SIZE = 500, typename Allocator = std::allocator<T>>
class basic_memory_buffer {
  std::vector<T, Allocator> data_;
 public:
  using value_type = T;
  explicit basic_memory_buffer(const Allocator& alloc = Allocator())
      : data_(alloc) { data_.reserve(SIZE); }
  basic_memory_buffer(const basic_memory_buffer&) = delete;
  auto operator=(const basic_memory_buffer&) -> basic_memory_buffer& = delete;
  basic_memory_buffer(basic_memory_buffer&&) = default;
  auto operator=(basic_memory_buffer&&) -> basic_memory_buffer& = default;
  auto data() noexcept -> T* { return data_.data(); }
  auto data() const noexcept -> const T* { return data_.data(); }
  auto begin() noexcept -> T* { return data(); }
  auto begin() const noexcept -> const T* { return data(); }
  auto end() noexcept -> T* { return size() ? data() + size() : data(); }
  auto end() const noexcept -> const T* {
    return size() ? data() + size() : data();
  }
  auto size() const noexcept -> size_t { return data_.size(); }
  auto capacity() const noexcept -> size_t { return data_.capacity(); }
  auto get_allocator() const -> Allocator { return data_.get_allocator(); }
  void clear() noexcept { data_.clear(); }
  void reserve(size_t n) { data_.reserve(n); }
  void resize(size_t n) { data_.resize(n); }
  void push_back(const T& value) { data_.push_back(value); }
  template <std::input_iterator It> void append(It first, It last) {
    data_.insert(data_.end(), first, last);
  }
  auto operator[](size_t n) -> T& { return data_[n]; }
  auto operator[](size_t n) const -> const T& { return data_[n]; }
};
using memory_buffer = basic_memory_buffer<char>;
using wmemory_buffer = basic_memory_buffer<wchar_t>;
using appender = std::back_insert_iterator<memory_buffer>;
template <typename Char, size_t SIZE, typename Allocator>
auto to_string(const basic_memory_buffer<Char, SIZE, Allocator>& buffer)
    -> std::basic_string<Char> {
  return buffer.size() ? std::basic_string<Char>(buffer.data(), buffer.size())
                       : std::basic_string<Char>();
}
template <typename T> auto to_string(const T& value) -> std::string {
  return fmt::format("{}", value);
}
template <typename T> constexpr auto ptr(T* value) -> const void* {
  return static_cast<const void*>(value);
}
template <typename T> requires std::is_enum_v<T>
constexpr auto underlying(T value) -> std::underlying_type_t<T> {
  return std::to_underlying(value);
}
}  // namespace v12
}  // namespace fmt
#endif
