# Using TJG styling from Make and CMake

FmtColor is a standalone C++23 header library. It uses `std::format` and
`std::print`, with public APIs in namespace `tjg`. Headers live under
`include/tjg/` and are included as `<tjg/...>`. Use a C++23 compiler and
standard library with working `<format>` and `<print>`.

## Select the headers

| Header | Use |
| --- | --- |
| `<tjg/colors.hpp>` | RGB, terminal, cube, and grayscale colors, plus conversions and blending. |
| `<tjg/emphasis.hpp>` | Text attributes such as bold and underline. |
| `<tjg/text_style.hpp>` | Combine colors and emphasis into a style. |
| `<tjg/text_style-format.hpp>` | Styled arguments, narrow and wide formatting, and printing to standard output or a `FILE*`. |
| `<tjg/color-ostream.hpp>` | Styled printing to `std::ostream` and styled insertion into narrow and wide streams. |
| `<tjg/sgr_support.hpp>` | Convert styles to SGR sequences, obtain a reset sequence, or wrap text with a style. |
| `<tjg/strip_ansi.hpp>` | Strip ANSI escape sequences from strings. |
| `<tjg/backslash.hpp>` | Escape control characters or decode backslash escapes. |
| `<tjg/inline_buffer.hpp>` | Fixed-capacity character buffers with inline storage. |

Owning `styled_text` trees and SGR parsing into those trees are still being
refactored.

## Example application

```cpp
#include <tjg/strip_ansi.hpp>
#include <tjg/text_style-format.hpp>

#include <print>

int main() {
  const auto green = tjg::fg(tjg::term_color::green);
  tjg::println(tjg::bold(), "Hello, {:>8}!", tjg::styled("world", green));

  const auto message = tjg::format(green, "Result: {}", 42);
  std::println("{}", tjg::strip_ansi(message));
}
```

The first line is bold, with the aligned field in green. The second line is
plain text. `tjg::styled(value, style)` uses the wrapped value's formatter,
so width, alignment, and precision work as they do for the value on its own.
It borrows the value, which must remain alive whenever the argument is
formatted.

Style-taking `tjg::format`, `format_to`, `print`, and `println` accept
standard format strings. The runtime variants take argument packs from
`std::make_format_args` or, for wide formatting, `std::make_wformat_args`.
Keep the referenced arguments alive while using the pack. Use positional
arguments for dynamic width and precision, as in `"{0:>{1}.{2}}"`.

Invalid runtime format strings throw `std::format_error`. Backslash decoding
throws `std::invalid_argument` for malformed escapes and `std::out_of_range`
for octal escapes above 255.

## SGR output

An ANSI-capable terminal interprets the generated SGR sequences. Styled
output also includes those sequences when redirected to a file or pipe.
Use `tjg::strip_ansi` to get plain text from an already formatted string.

Relative styled arguments inherit their enclosing style and restore it
after formatting. An absolute style uses terminal defaults for unspecified
colors and emphasis. A style-taking `println` completes the styled span
before writing the newline.

The formatting and printing functions track a shared style state and
assume terminal defaults at the start of the outermost call. If you write
raw SGR, restore terminal defaults before calling those functions. Styled
stream insertion emits its own reset for a nonempty style. Serialize
concurrent calls that use the shared style state.

## Use Make

Save the example as `hello.cpp` and point `TJG_ROOT` at this checkout.

```makefile
CXX = g++
TJG_ROOT ?= /path/to/checkout
CPPFLAGS += -I$(TJG_ROOT)/include
CXXFLAGS += -O2 -std=c++23

hello: hello.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@
```

On MSYS, use `make CXXFLAGS='-O2 -std=gnu++23'` with GCC so libstdc++'s
`<print>` has the stdio declarations it needs. From the checkout, run
`make -C demo run` to build and run [FmtColorDemo1](demo/FmtColorDemo1.cpp).

## Build and install the standalone package

Use the repository root as the CMake source directory.

```bash
cmake -S . -B build/color-install \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/tjg"
cmake --install build/color-install
```

The package installs the `include/tjg/` headers, license notices, and package
configuration under `lib/cmake/tjg/` or the platform's configured library
directory. `TJG_INSTALL=OFF` disables installation rules.

## Use the installed package with CMake

Save the example as `hello.cpp` next to this `CMakeLists.txt`.

```cmake
cmake_minimum_required(VERSION 3.20)
project(tjg_hello LANGUAGES CXX)
find_package(tjg CONFIG REQUIRED)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE tjg::tjg)
```

Build against the installed headers and package.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local/tjg"
cmake --build build
./build/hello
```

`tjg::tjg` supplies the include directory and C++23 requirement. Use
`PUBLIC` when a library exposes TJG through its public headers. The
installed package can be moved to a different prefix without rebuilding it.

## Use TJG directly from a checkout

A parent CMake project can add the standalone project as a subdirectory.

```cmake
add_subdirectory(/path/to/checkout color-build)
add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE tjg::tjg)
```

With FetchContent, use `FetchContent_MakeAvailable` and link the same target.

## Run the tests

Run `./build.bash` from the checkout to build with GCC in Debug mode and run
the tests with warnings treated as errors. It uses `build/color/`.

To select a different compiler or generator, configure the project directly.

```bash
cmake -S . -B build/color-tests \
  -DTJG_TEST=ON -DTJG_WERROR=ON \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/color-tests --parallel 4
ctest --test-dir build/color-tests --output-on-failure
```

`TJG_TEST` defaults to `OFF`. The test sources are in `test/`.
They use the vendored Google Test in `ext/gtest/` or an installed
`GTest` CMake package when the vendored sources are unavailable. The package
test installs and relocates TJG, then builds a separate consumer with
`find_package(tjg)`.

The Linux, Windows, and macOS workflows build the standalone project in
C++23 Debug and Release configurations. Linux also runs an address and
undefined-behavior sanitizer build. Test configuration checks support for
`<format>`, `std::formattable`, and `<print>` by compiling a small program.

## User guide sources

The [color user guide source](doc/index.md) lives in `doc/`. MkDocs builds
the guide directly from that directory and uses its index as the site's home
page.
