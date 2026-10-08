# {fmt}: C++23 standard-engine fork

This is Terry Golubiewski's experimental C++23 fork of
[fmtlib/fmt](https://github.com/fmtlib/fmt), retaining the `fmt` name and namespace.
It uses `std::format`, `std::formatter`, and `std::print`, with adapters for
upstream styling and common formatting call syntax.

The `Refactor` branch deliberately prioritizes the standard engine. It is not
yet a full source or binary drop-in replacement. See [CXX23.md](CXX23.md) for the
API inventory, unsupported features, toolchain requirements, and validation.

```cpp
#include <fmt/color.h>

int main() {
  fmt::println("The answer is {}", 42);
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

Requires GCC or Clang in C++23 mode and a standard library supplying C++23
formatting, printing, and range formatting. Use `fmt::fmt` or
`fmt::fmt-header-only` from CMake.

Upstream history and [MIT license](LICENSE) are preserved. Original {fmt} code
is copyright Victor Zverovich and the {fmt} contributors.
