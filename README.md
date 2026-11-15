# FmtColor

FmtColor is a standalone C++23 header library for terminal colors and text
styles. It uses `std::format` and `std::print`, with APIs in namespace `tjg`.
The project began as an extension to [{fmt}](https://github.com/fmtlib/fmt).
It now depends only on the C++ standard library.

## Quick start

Add this checkout's `include/` directory to your compiler's include path.
Use a C++23 compiler and standard library with `<format>` and `<print>`.

```cpp
#include <tjg/text_style-format.hpp>

int main() {
  const auto green = tjg::fg(tjg::term_color::green);
  tjg::println(tjg::bold(), "Build {}", tjg::styled("passed", green));
}
```

The whole line is bold, and `passed` is green. Styled arguments inherit and
restore their enclosing style. An ANSI-capable terminal displays the colors.

Run `make -C demo run` to try [FmtColorDemo1](demo/FmtColorDemo1.cpp).

## Headers

| Header | Provides |
| --- | --- |
| `<tjg/colors.hpp>` | Named RGB colors, terminal colors, 216-color cube and grayscale palettes |
| `<tjg/text_style.hpp>` | Foreground, background, and emphasis, with style composition and absolute styles |
| `<tjg/text_style-format.hpp>` | Styled formatting and printing, and `styled(value, style)` for use with standard format strings |
| `<tjg/color-ostream.hpp>` | Styled insertion into narrow and wide streams, and styled printing to `std::ostream` |
| `<tjg/sgr_support.hpp>` | SGR conversion, reset sequences, and wrapping text with a style |
| `<tjg/strip_ansi.hpp>` | ANSI escape sequence removal from strings |
| `<tjg/backslash.hpp>` | Backslash escaping and unescaping |

Owning `styled_text` trees and SGR parsing into those trees are still being
refactored.

## CMake

Configure the project from the repository root to install the headers and the
`tjg::tjg` CMake target.

```sh
cmake -S . -B build/color-install
cmake --install build/color-install --prefix /path/to/install
```

In your application, point `CMAKE_PREFIX_PATH` at the install prefix and add
the following to its CMake file.

```cmake
find_package(tjg CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE tjg::tjg)
```

## Regression tests

Run `./build.bash` from the repository root to configure, build, and run the
TJG suite with GCC and warnings treated as errors. To use a different compiler,
configure the standalone project with `-DTJG_TEST=ON`.

Test sources live in `test/`. The suite uses the vendored Google Test in
`ext/gtest/` and checks that the installed package works after relocation.

## Documentation and licenses

- [Color user guide](https://tjgolubi.github.io/fmt/)
- [Color user guide source](doc/index.md)
- [Change log](CHANGELOG.md)
- [Project website and design rationale](https://fmtcolor.tjgolubi.chatgpt.site)
- [License](LICENSE) and [Boost Software License](https://www.boost.org/LICENSE_1_0.txt)

The guide still uses some header names from before the C++23 refactor.
