# C++23 standard-engine refactor

This branch is an experimental source fork of {fmt}. It preserves the `fmt`
namespace, include paths, and common call syntax while using the C++23 standard
formatting engine. **It is not a complete source or binary drop-in replacement.**
Terry chose the standard engine over strict compatibility during implementation.

Baseline: fork commit `6b186b6aa13062ece0dfc05487479a32a8021a5f`; upstream parent
`10cda465`, reporting {fmt} version 12.2.1. Upstream history, LICENSE, and
attribution remain in this repository. Existing unrelated upstream tests are
retained as material for subsequent compatibility work; they are not silently
counted as passing tests of this implementation.

## Architecture and compatibility-code audit

`core.h` imports standard formatting types and functions, with small adapters
for runtime strings, printing, bounded output, and memory buffers. `format.h`
and `base.h` include this standard-backed core. There is no second parser,
floating-point formatter, or compatibility engine.

The upstream color enum, RGB/terminal color types, text-style packing, and ANSI
encoding are reused. Their language-feature macros are replaced by ordinary
`constexpr` and standard assertions. `styled`, join views, and ostream views
are program-defined types with legal `std::formatter` specializations.

| Former machinery | Result |
| --- | --- |
| Pre-C++23 language and old compiler workarounds | Removed from active library code |
| Constexpr, consteval, attribute and SFINAE compatibility macros | Removed |
| Upstream parser, format contexts, argument type erasure, number conversion | Supplied by the standard library |
| Unicode and platform-specific print machinery | Supplied by `std::print` and `std::vprint_unicode` |
| Module, C binding, fmt OS implementation sources | Removed; these interfaces have not been ported |
| Shared/static and header-only consumption | CMake targets retained; functions are standard-backed/header defined |
| Header guards and `FMT_VERSION` | Retained as metadata/build necessities |
| `FMT_STRING` and `FMT_COMPILE` | Literal identity annotations; no separate compiled-format engine |

## API inventory

| Header or API | Standard-engine behavior and gaps |
| --- | --- |
| `core.h`, `base.h`, `format.h` | Standard formatting functions, contexts, arguments, errors and compile-time format-string validation |
| `fmt::runtime` | Runtime formatting/printing adapters; narrow and wide formatting, narrow printing |
| `format_to`, `format_to_n`, `formatted_size` | Standard operations plus narrow runtime adapters; bounded narrow character-array `format_to` retains truncation reporting |
| `vformat_to_n` | Narrow adapter formats into a temporary string, copies up to the limit, reports the full size |
| `make_format_args` | Standard reference-holding argument store; keep the store and referenced values alive through the formatting call |
| `memory_buffer` | Vector storage, not upstream inline storage; common append/reserve/resize/data interface retained |
| `color.h` | Named colors, RGB, all 16 terminal colors, foreground/background, emphasis, style format/print/println/format_to and `styled` |
| `styled` | Works with `fmt::format` and directly with `std::format`; inherits the underlying standard formatter; preserves reset behavior |
| `ranges.h` | Standard range formatting plus iterator/range `join`; tuple joins and wide join convenience overloads are not implemented |
| `ostream.h` | `streamed`, ostream formatters and print/println using standard formatting; upstream FILE-extraction optimizations removed |
| `chrono.h` | Standard chrono formatting; upstream extensions such as `fmt::localtime`, `gmtime`, duration_cast helpers and nonstandard chrono specs are absent |
| `std.h` | Only types formatable by the selected standard library; upstream optional, variant, filesystem/path and other fmt-only formatters are absent unless the standard library itself supplies them |
| `xchar.h` | Standard wide format types/functions and wide runtime format adapters; wide printing and exotic character types are absent |
| `compile.h` | `FMT_COMPILE` retains literal call syntax but not constexpr result formatting or upstream compiled-format types |
| `enum.h` | C++23 `fmt::underlying` using `std::to_underlying`; C++26 reflection features are absent |
| `args.h`, `printf.h`, `os.h`, `fmt-c.h` | Explicit compile-time unsupported diagnostics |

There is no named-argument parser, dynamic argument store, `format_as` dispatch,
`fmt::detail` compatibility, or automatic conversion of arbitrary pointers.
Use standard formatter specializations and `fmt::ptr` for object pointers.
`fmt::formatter` is the standard template imported into `fmt`; existing
specializations declared in `fmt` must be migrated to `std::formatter<T>`.
There is no bridge that secretly falls back to the upstream formatter engine.
The standard engine also determines formatting grammar, diagnostics, rounding,
locale behavior, and which built-in types are formattable. The compiled library
has no upstream ABI: rebuild consumers and do not substitute it for upstream
shared-library binaries.

Nested-style behavior remains upstream's escape/reset behavior: an inner reset
does not restore an outer style. No relative/absolute-style redesign is included.
View adapters borrow values/ranges; do not retain them beyond those lifetimes.

## Toolchains and build

The intended release matrix is latest stable GCC/libstdc++, latest stable
Clang/libstdc++, and latest stable Clang/libc++. Exact versions must be recorded
when those release checks run. No older-version fallback is implemented.
Compiler/library combinations missing required standard features fail configuration.
The package requires C++23, `__cpp_lib_format >= 202110L`,
`__cpp_lib_print >= 202207L`, and `__cpp_lib_format_ranges >= 202207L`.
These are separate feature-test macros; range support is not inferred from
`__cpp_lib_format`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix ~/.local
```

For Clang/libc++, add `-DCMAKE_CXX_COMPILER=clang++`
and `-DCMAKE_CXX_FLAGS=-stdlib=libc++`. Consumers use `fmt::fmt` or
`fmt::fmt-header-only`; both propagate `cxx_std_23`.

## Validation and remaining release work

The diagnostic GitHub workflow uses Clang 19/libc++ 19 in Debug/Release and
static/shared configurations. It replaces the obsolete upstream workflows
for old language modes, MSVC, docs, fuzzing, and release packaging on this branch.
It does not certify the latest-stable target matrix.


Local validation uses available GCC 14.2/libstdc++ and Clang 19.1.1/libc++ 19.
These are diagnostic checks, not proof of the latest-stable release matrix.
Clang/libc++ runs the adapted upstream color assertions and the new standard-
engine regression suite in compiled and header-only configurations. GCC 14
checks the core and styling, but its libstdc++ lacks standard range formatting,
so package configuration correctly rejects that combination.

Before release: run the exact latest-stable three-way toolchain matrix; extend
compatibility tests to real consumers; and decide which remaining fmt-only
features should receive standard-backed adapters. Existing upstream tests that
exercise removed implementation details cannot be used unchanged to certify
this fork. Any future release claiming drop-in compatibility must close or
explicitly constrain the gaps above first.

Recorded checks for this implementation:

- Clang 19/libc++ 19: Release static/shared builds and adapted upstream color
  checks plus compiled/header-only regression tests pass, with warnings as errors.
- Clang 19/libc++ 19: Debug UBSan regression tests pass.
- Public supported headers compile independently; invalid literal format strings,
  C++20 mode, and explicitly unsupported headers are rejected as intended.
- Installed CMake package consumers build/run through both exported targets.
- GCC 14/libstdc++ 14: core formatting, runtime strings, buffers, styles, and FILE
  printing smoke checks pass. Full package is rejected for missing range support.
- ASan is blocked by this environment's libc++/libc++abi allocation mismatch; a
  standalone program that only throws `std::runtime_error` reproduces it.
- Exact latest-stable matrix and remote CI have not yet been validated.
