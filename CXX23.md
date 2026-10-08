# C++23 color/style fork

`RefactorMin` is a header-only source fork of {fmt}, limited to its color/style
interface on the standard C++23 formatting engine. It is not an upstream
source or binary drop-in replacement. Upstream history, LICENSE and attribution
remain; the original baseline reported {fmt} version 12.2.1.

## Public interface

The only public header is `<fmt/color.h>`. It defines `fmt::color`, `fmt::rgb`,
`fmt::terminal_color` (all 16 terminal colors), `fmt::emphasis`, `fmt::text_style`,
`fmt::fg`, `fmt::bg`, and `fmt::styled`.

`terminal_color` has default consecutive values 0-15: black through white,
then bright black through bright white. These values identify palette entries;
the packed style stores and retrieves those indices. The SGR encoder maps them
to foreground parameters 30-37/90-97 and background parameters 40-47/100-107.
The underlying numbers have changed from upstream; code or serialized data
using the former 30-37/90-97 values must be migrated. Named color calls produce
the same escape sequences. There is no SGR text parser/decoder in this package.

| Operation | Interface |
| --- | --- |
| Individual styled value | `std::format("{}", fmt::styled(value, style))`; also works with standard output-iterator and printing functions |
| Entire formatted output | `fmt::format(style, format_string, args...)` |
| Entire output to an iterator | `fmt::format_to(out, style, format_string, args...)` |
| Entire printed output | `fmt::print(style, format_string, args...)` and `fmt::println(style, format_string, args...)`; FILE overloads put the file first |
| Runtime formatted output | `fmt::vformat(style, text, std::format_args)` and `fmt::vformat_to(out, style, text, std::format_args)` |
| Runtime printed output | `fmt::vprint(file, style, text, std::format_args)` and `fmt::vprintln(file, style, text, std::format_args)` |

Style-aware whole-output functions use narrow strings. Wide styled values work
through standard wide formatting functions. Specialize `std::formatter<T>` for
custom types; the `styled` formatter inherits that standard formatter, including
its format-spec parsing.

There are no common aliases or unstyled adapters in `fmt`: use standard
formatting functions, types, argument stores and exceptions directly. The former
core/buffer/runtime-string implementation and forwarding headers have been
removed. In particular, there is no `fmt::formatter`, `fmt::format_error`,
`fmt::runtime`, `fmt::memory_buffer`, `fmt::ptr`, or `FMT_STRING`.
Other fmt-only extensions such as named arguments, joins, ostream adapters,
compiled formatting, printf and OS helpers are outside the project scope.

Runtime formatting uses the standard argument-store lifetime rules:

```cpp
int value = 42;
auto arguments = std::make_format_args(value);
std::string text = "{:04}";
auto result = fmt::vformat(fmt::emphasis::bold, text, arguments);
```

Keep both the argument store and referenced values alive for the call. `styled`
also borrows its value; wrapping a temporary in the formatting call is valid,
but do not retain the wrapper past that temporary's lifetime. Output iterators
must have sufficient writable space; use a back inserter for growable output.

The upstream ANSI encoding and reset behavior are preserved. An inner reset
does not restore an outer style; nested-style redesign remains deferred.
The standard library determines formatting grammar, locale behavior, diagnostics,
rounding and which underlying types are formattable.

## Build and validation

The intended release matrix is latest stable GCC/libstdc++, Clang/libstdc++,
and Clang/libc++; exact latest-stable release validation remains outstanding.
Require C++23, `__cpp_lib_format >= 202110L` and `__cpp_lib_print >= 202207L`.
No legacy engine or fallback is provided. Range-format support is not required
by this package; styled ranges can be used if the selected standard library
supports formatting the underlying range.

Both `fmt::fmt` and `fmt::fmt-header-only` are INTERFACE targets. Installation
supplies the header and CMake/pkg-config metadata; there is no compiled fmt
library and pkg-config has no fmt link flags. Use a clean installation prefix
when removing obsolete headers or libraries from a previous installation.

The active test directory contains `color-test.cc`, `cxx23-test.cc`, and
`compile-checks.py`. CMake runs the adapted upstream color assertions, style
integration tests through both target names, and compile-time checks for the
supported header, literal validation and intentional removal of common aliases.
Unused upstream tests and support files remain available in git history.

Local validation of this narrow interface: Clang 19.1.1/libc++ 19 Release with
warnings treated as errors passes all four configured checks. A clean installed
package consumer uses standard calls plus `styled` and style-aware calls.
These are diagnostic checks, not certification of the latest-stable matrix.
The diagnostic GitHub workflow now targets `RefactorMin` in Debug and Release.
