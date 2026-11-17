# FmtColor

FmtColor is a standalone C++23 header library for terminal colors and text
styles. It uses `std::format` and `std::print`, with APIs in namespace `tjg::color`.
The project began as an extension to [{fmt}](https://github.com/fmtlib/fmt).
It now depends only on the C++ standard library.

## Quick start

Add this checkout's `include/` directory to your compiler's include path.
Use a C++23 compiler and standard library with `<format>` and `<print>`.

```cpp
#include <tjg/color.hpp>

int main() {
  const auto green = tjg::color::fg(tjg::color::term_color::green);
  tjg::color::println(tjg::color::bold(), "Build {}", tjg::color::styled("passed", green));
}
```

The whole line is bold, and `passed` is green. Styled arguments inherit and
restore their enclosing style. An ANSI-capable terminal displays the colors.

Run `make -C demo FmtColorDemo1` and then `./demo/FmtColorDemo1` to try the
[first demo](demo/FmtColorDemo1.cpp). See the [demo README](demo/README.md) for
the full set of examples and the {fmt} comparison builds.

## Headers

| Header                  | Provides                                                             |
| ----------------------- | -------------------------------------------------------------------- |
| `<tjg/color.hpp>`       | Full library, including formatting, printing, streams, and SGR tools |
| `<tjg/color/colors.hpp>`      | Named RGB colors, terminal and palette colors, and `color_t`         |
| `<tjg/color/text_style.hpp>`  | Color values, emphasis, `text_style`, and style composition          |
| `<tjg/color/styled_text.hpp>` | Owning styled-text trees and their composition                       |
| `<tjg/backslash.hpp>`   | Backslash escaping and unescaping                                    |
| `<tjg/strip_ansi.hpp>`  | ANSI escape sequence removal from strings                            |

Use `<tjg/color.hpp>` for formatted output, SGR generation and parsing, or
ANSI stripping on a styled-text tree. The narrower headers support code
that only needs the value types or string utilities. Headers in `tjg/color/bits/`
are implementation headers.

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

With Python 3 and Doxygen available, configure CMake and build the guide from
the repository root.

```sh
cmake -S . -B build
cmake --build build --target doc
```

Open `build/doc-html/index.html`. The first documentation build installs the
pinned Python packages into a virtual environment under `build/`.

- [Color user guide](https://tjgolubi.github.io/fmt/)
- [Color user guide source](doc/index.md)
- [Change log](CHANGELOG.md)
- [Project website and design rationale](https://fmtcolor.tjgolubi.chatgpt.site)
- [License](LICENSE) and [Boost Software License](https://www.boost.org/LICENSE_1_0.txt)
