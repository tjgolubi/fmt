# Using FmtColor from Make and CMake

FmtColor is our extension of {fmt}. Include `<fmt/color.h>` and link against
the matching {fmt} library. The styling API supports C++11 and later.

There are two SGR output modes:

`FMT_COLOR_COMPACT_SGR` effect table:

| Value | Output |
| ---   | ------ |
| `0`   | Legacy output: separate SGR sequences for emphasis, foreground, and background, with three-digit RGB and indexed-color parameters. |
| `1`   | Compact output: one SGR sequence per style, with decimal parameters that have no leading zeros. |

Both modes express the same terminal styling. Formatting styled text adds a
reset after the text. `fmt::to_sgr(style)` returns just the style's opening SGR
sequence; it returns an empty string for an empty style.

For bold text with RGB foreground `(1,2,3)` and terminal-black background,
the opening sequences are:

```text
Legacy:  \x1b[1m\x1b[38;2;001;002;003m\x1b[40m
Compact: \x1b[1;38;2;1;2;3;40m
```

Here `\x1b` denotes the actual ESC character.

If `FMT_COLOR_COMPACT_SGR` is undefined, the effect is the same as `0`.

## Build and install the library

Run these commands from this fork's source directory in a Bash shell.
On Windows, use the MSYS2 toolchain for both this library and the consuming
application. Do not mix an MSYS2 library with native MSVC objects.

The development script builds the library and runs tests in either mode.
The default build also generates documentation; the TaskWarrior build uses
`FMT_DOC=OFF` and skips documentation generation:

```bash
./build.bash              # Legacy mode, in build/
./build.bash taskwarrior  # Compact mode, in build/taskwarrior/
```

Both modes prepare the shared documentation-tool environment, even though
TaskWarrior mode does not generate the guide. The script therefore requires
the project's test and documentation tools. For an
application-only installation, build the static library without those tools:

```bash
cmake -S . -B build/install \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DFMT_TEST=OFF \
  -DFMT_DOC=OFF \
  -DFMT_INSTALL=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/fmtcolor"
cmake --build build/install --parallel 8
cmake --install build/install
```

This installs headers, the static library, and CMake package files under the
chosen prefix. The examples below use that prefix and a static installation.

**Select the SGR mode when compiling the application.** The gated code lives
in `color.h`; the same installed headers and compiled library support either
mode. The installation does not automatically enable compact output in its
consumers, even when it comes from `build/taskwarrior`.

## Example application

Save this as `hello.cpp` in a separate application directory:

```cpp
#include <fmt/color.h>

int main() {
  const auto style = fmt::text_style{
      fmt::emphasis::bold, fmt::rgb{1, 2, 3}, fmt::term_color::black};
  fmt::print(style, "Hello from FmtColor!\n");
}
```

## Use a traditional Makefile

Save the following as `Makefile` alongside `hello.cpp`. Recipe lines must
start with a tab.

```makefile
CXX = g++
FMT_PREFIX ?= $(HOME)/.local/fmtcolor
COMPACT_SGR ?= 0
BUILD_DIR := build/sgr-$(COMPACT_SGR)

CPPFLAGS += -I$(FMT_PREFIX)/include -DFMT_COLOR_COMPACT_SGR=$(COMPACT_SGR)
CXXFLAGS += -O2 -std=c++11
LDFLAGS += -L$(FMT_PREFIX)/lib
LDLIBS += -lfmt

.PHONY: all
all: $(BUILD_DIR)/hello

$(BUILD_DIR)/hello: hello.cpp Makefile
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) hello.cpp $(LDFLAGS) $(LDLIBS) -o $@
```

Build and run legacy output:

```bash
make COMPACT_SGR=0
./build/sgr-0/hello
```

Build and run compact output:

```bash
make COMPACT_SGR=1
./build/sgr-1/hello
```

For another installation location, add `FMT_PREFIX=/path/to/fmtcolor` to the
`make` command. The separate output directories prevent reusing an executable
compiled for the other mode. For a larger application, apply the definition
to every source file that uses the styling header and link `-lfmt` after the
application's object files.

## Use CMake

Save this as `CMakeLists.txt` alongside `hello.cpp`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(fmtcolor_hello LANGUAGES CXX)

option(USE_COMPACT_SGR "Use compact FmtColor SGR output" OFF)

find_package(fmt CONFIG REQUIRED)

add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE fmt::fmt)
target_compile_features(hello PRIVATE cxx_std_11)
target_compile_definitions(hello PRIVATE
  FMT_COLOR_COMPACT_SGR=$<BOOL:${USE_COMPACT_SGR}>)
```

Build and run legacy output:

```bash
cmake -S . -B build/legacy \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH="$HOME/.local/fmtcolor" \
  -DUSE_COMPACT_SGR=OFF
cmake --build build/legacy --parallel 8
./build/legacy/hello
```

Build and run compact output:

```bash
cmake -S . -B build/compact \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH="$HOME/.local/fmtcolor" \
  -DUSE_COMPACT_SGR=ON
cmake --build build/compact --parallel 8
./build/compact/hello
```

`USE_COMPACT_SGR` is an option introduced by this example. It becomes the
compiler definition `FMT_COLOR_COMPACT_SGR=0` or `1` through
`target_compile_definitions`. Passing `-DFMT_COLOR_COMPACT_SGR=1` to CMake
alone does not define the macro for the compiler.

If CMake finds another installation, use
`-Dfmt_DIR="$HOME/.local/fmtcolor/lib/cmake/fmt"` to select this package
explicitly. On multi-configuration generators, add `--config Release` to the
build command and run the executable from the generator's Release directory.

For a library target whose public headers use FmtColor, use `PUBLIC` for both
the link dependency and the compile definition so its consumers inherit them:

```cmake
target_link_libraries(my_library PUBLIC fmt::fmt)
target_compile_definitions(my_library PUBLIC
  FMT_COLOR_COMPACT_SGR=$<BOOL:${USE_COMPACT_SGR}>)
```

Use the same macro value throughout an application, including its libraries.
The macro changes inline and template definitions in the header; mixing
settings across translation units violates C++'s one-definition rule.

## Header-only use

The same two output modes also work without linking the compiled library.
With Make, define `FMT_HEADER_ONLY=1` and omit `-lfmt`. With CMake, link
`fmt::fmt-header-only` instead of `fmt::fmt`. Keep the SGR mode definition
in either case.
