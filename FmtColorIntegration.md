# Using TJG styling from Make and CMake

On `ref/CPP20`, the styling extension is a standalone C++23 header library.
It uses `std::format` and `std::print`, with public APIs in namespace `tjg`.
Headers live under `include/tjg/` and are included as `<tjg/...>`. No {fmt}
engine headers, compiled library, `FMT_HEADER_ONLY` definition, or {fmt}
formatter adapters are required.
The repository retains the original {fmt} library for unrelated consumers.

A C++23 standard library with `<format>` and `<print>` is required. Development
validation uses GCC 14.2 with compact SGR enabled.

## Select the headers

| Header | Use |
| --- | --- |
| `<tjg/color.h>` | Color/style values, styled arguments, SGR helpers, narrow/wide formatting, and narrow FILE printing. |
| `<tjg/styled_text.h>` | Owning narrow/wide text trees and standard formatters. |
| `<tjg/parse_styled_text.h>` | Parse standard ANSI SGR into owned styled text. |
| `<tjg/strip_ansi.h>` | Strip ANSI sequences from strings or styled-text leaves. |
| `<tjg/memory_buffer.h>` | Inline/growable buffer in namespace `tjg`. |
| `<tjg/backslash.h>` | Escape control characters or decode backslash escapes. |
| `<tjg/color-ostream.h>` | Styled narrow/wide ostream output and insertion. |

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
#include <tjg/parse_styled_text.h>
#include <tjg/strip_ansi.h>
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

## Build and install the standalone package

From the checkout, configure only the TJG project. This does not configure,
build, or install the retained {fmt} library:

```bash
cmake -S tjg -B build/tjg-install \
  -DTJG_COLOR_COMPACT_SGR=ON \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/tjg"
cmake --build build/tjg-install
cmake --install build/tjg-install
```

The package installs `include/tjg/`, its license, and CMake configuration under
`lib/cmake/tjg/` (or the platform's configured library directory). It contains
no compiled library. `TJG_INSTALL=OFF` disables installation rules.

`TJG_COLOR_COMPACT_SGR` is a CMake option as well as the compiler definition.
The CMake target propagates its selected value, which defaults to `ON`. Consumers
should use the target's setting consistently instead of overriding the macro.
This branch is validated only with compact SGR and C++23 or later.

## Use the installed package with CMake

Save the example as `hello.cpp` with this `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(tjg_hello LANGUAGES CXX)
find_package(tjg CONFIG REQUIRED)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE tjg::tjg)
```

Then build against the installation:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local/tjg"
cmake --build build
./build/hello
```

`tjg::tjg` supplies the include directory, C++23 requirement, and SGR definition.
No `find_package(fmt)` or {fmt} link target is needed. For a library whose public
headers use TJG, use `PUBLIC` instead of `PRIVATE` so these requirements propagate.
The installed package can be moved to a different prefix without rebuilding it.

## Use TJG directly from a checkout

A parent project can use `add_subdirectory` or FetchContent's `SOURCE_SUBDIR`
without configuring the repository's {fmt} project:

```cmake
add_subdirectory(/path/to/fmt/tjg tjg-build)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE tjg::tjg)
```

The repository's top-level build also provides `tjg::tjg`, and installs TJG
alongside {fmt} when both install options are enabled. To install just TJG from
a top-level build, use `cmake --install build --component tjg`.

The extension is included directly; importing the retained `fmt` module does
not export TJG APIs. `./build.bash` and `./build.bash taskwarrior` run the full
repository tests with C++23 and compact SGR. CI stays disabled for `ref/CPP20`.
The package test copies only TJG sources/headers, installs and relocates them,
and builds a separate consumer using `find_package(tjg)`.

## Run only TJG tests

All TJG test sources, helpers, and registrations live in `tjg/test/`. Configure
that package directly to build and run its full suite without configuring,
building, or linking the FMT library:

```bash
cmake -S tjg -B build/tjg-tests \
  -DTJG_TEST=ON -DTJG_WERROR=ON -DTJG_COLOR_COMPACT_SGR=ON \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/tjg-tests --parallel 4
ctest --test-dir build/tjg-tests --output-on-failure
```

Tests use the checkout's vendored Google Test sources, or an installed
`GTest` CMake package when those sources are unavailable. Neither path needs
FMT's library or test harness. `TJG_TEST` defaults to `OFF` for the standalone
package; the top-level repository build defaults it to the `FMT_TEST` setting.
When both suites are configured, `ctest --test-dir build -L tjg` selects only
TJG tests. All validation on this branch uses C++23 or later and compact SGR.
