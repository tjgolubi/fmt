# Using TJG styling from Make and CMake

On `ref/CPP20`, the styling extension is a standalone C++23 header library.
It uses `std::format` and `std::print`, with public APIs in namespace `tjg`.
Header paths remain under `fmt/`, but no {fmt} engine headers, compiled library,
`FMT_HEADER_ONLY` definition, or {fmt} formatter adapters are required.
The repository retains the original {fmt} library for unrelated consumers.

A C++23 standard library with `<format>` and `<print>` is required. Development
validation uses GCC 14.2 with compact SGR enabled.

## Select the headers

| Header | Use |
| --- | --- |
| `<fmt/color.h>` | Color/style values, styled arguments, SGR helpers, narrow/wide formatting, and narrow FILE printing. |
| `<fmt/styled_text.h>` | Owning narrow/wide text trees and standard formatters. |
| `<fmt/parse_styled_text.h>` | Parse standard ANSI SGR into owned styled text. |
| `<fmt/strip_ansi.h>` | Strip ANSI sequences from strings or styled-text leaves. |
| `<fmt/memory_buffer.h>` | Inline/growable buffer in namespace `tjg`. |
| `<fmt/backslash.h>` | Escape control characters or decode backslash escapes. |
| `<fmt/color-ostream.h>` | Styled narrow/wide ostream output and insertion. |

The parser and stripper include `styled_text.h`, which includes `color.h`.
Wide styling no longer needs `<fmt/xchar.h>`. Internal utilities are in
`text_support.h` (UTF-8 decoding, width tables, and escaping),
`tree_format_specs.h` (tree specifications), and `format_support.h` (errors).

## Select the SGR output mode

Define `TJG_COLOR_COMPACT_SGR=1` to emit one SGR sequence per style, with decimal
parameters without leading zeros. This replaces `FMT_COLOR_COMPACT_SGR`.
Undefined or `0` retains the separate, zero-padded output mode, but this branch
is validated only in compact mode.

Use the same setting in every translation unit. The setting changes inline and
template definitions; mixing settings violates the one-definition rule.

## Example application

```cpp
#include <fmt/parse_styled_text.h>
#include <fmt/strip_ansi.h>
#include <print>

int main() {
  const auto style = tjg::fg(tjg::rgb{1, 2, 3}).bold();
  tjg::println(style, "Hello, {}!", "world");
  auto phrase = tjg::bold("Hello ")
      + tjg::overlay(tjg::fg(tjg::term_color::green), "world");
  std::println("{}", phrase);
  auto parsed = tjg::parse_styled_text("\x1b[31mred\x1b[0m");
  std::println("{} / {}", parsed, tjg::strip_ansi(parsed).plain_text());
}
```

Style-taking `tjg::format`, `format_to`, `print`, and `println` accept standard
format strings and use standard formatters. Their `v` counterparts take packs
from `std::make_format_args` or `std::make_wformat_args`. Arguments referenced by
packs must remain alive. Invalid specifications and malformed backslash escapes
throw `std::format_error`. The standard library does not support named arguments;
use positional dynamic width and precision, such as `"{0:>{1}.{2}}"`.

A style-taking output call appends a terminal reset for a nonempty style.
Nested `tjg::styled` arguments and owning trees inherit and restore their
enclosing style. Shared terminal state assumes single-threaded use for now.

## Use Make

Save the example as `hello.cpp`. Point `TJG_ROOT` at this checkout:

```makefile
CXX = g++
TJG_ROOT ?= /path/to/fmt
CPPFLAGS += -I$(TJG_ROOT)/include -DTJG_COLOR_COMPACT_SGR=1
CXXFLAGS += -O2 -std=c++23

hello: hello.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@
```

No `-lfmt` or header-only engine definition is needed. Run `make -C demo run`
from the checkout for the full nested-style and table demonstrations.

## Use CMake

```cmake
cmake_minimum_required(VERSION 3.20)
project(tjg_hello LANGUAGES CXX)
set(TJG_ROOT /path/to/fmt CACHE PATH "TJG checkout")
add_executable(hello hello.cpp)
target_include_directories(hello PRIVATE "${TJG_ROOT}/include")
target_compile_features(hello PRIVATE cxx_std_23)
target_compile_definitions(hello PRIVATE TJG_COLOR_COMPACT_SGR=1)
```

For a library whose public headers use TJG, propagate the include directory,
C++23 feature, and SGR definition with `PUBLIC` instead of `PRIVATE`.

The repository's existing CMake install also installs these headers. A full
repository build still builds/tests the retained {fmt} engine; that is separate
from the extension's requirements. `./build.bash` and `./build.bash taskwarrior`
select C++23 compact SGR. CI remains disabled for `ref/CPP20`.
