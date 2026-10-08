# {fmt}: C++23 color and styling

This is Terry Golubiewski's header-only source fork of
[fmtlib/fmt](https://github.com/fmtlib/fmt), retaining its color/style interface
and using the C++23 standard formatting and printing engine.

The `RefactorMin` branch provides `<fmt/color.h>` only. Use `std::format`,
`std::print`, and `std::formatter` for ordinary formatting. The `fmt` namespace
contains colors, emphasis, `text_style`, `styled`, and style-aware operations;
it does not re-export standard formatting names or provide unstyled adapters.

```cpp
#include <fmt/color.h>
#include <format>
#include <print>

int main() {
  std::println("The answer is {}", 42);
  std::println("Status: {}", fmt::styled("ready", fmt::fg(fmt::color::green)));
  fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::green),
             "Elapsed time: {:.2f} seconds\n", 1.23);
}
```

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix ~/.local
```

Requires GCC or Clang in C++23 mode with standard formatting and printing
support. No compiled fmt library is built or required. CMake targets `fmt::fmt`
and `fmt::fmt-header-only` supply include paths and the C++23 requirement.
See [CXX23.md](CXX23.md) for the public interface and validation limits.

Upstream history and [MIT license](LICENSE) are preserved. Original {fmt} code
is copyright Victor Zverovich and the {fmt} contributors.
