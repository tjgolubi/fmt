// Formatting library for C++23, based on {fmt}.
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors.
// Distributed under the MIT license; see LICENSE.
#ifndef FMT_RANGES_H_
#define FMT_RANGES_H_
#include "format.h"
#include <ranges>
#if !defined(__cpp_lib_format_ranges) || __cpp_lib_format_ranges < 202207L
#  error "fmt/ranges.h requires C++23 standard range formatting; no fallback is provided"
#endif

namespace fmt { inline namespace v12 {
using std::range_format;
using std::range_formatter;
using std::format_kind;
} }
#endif
