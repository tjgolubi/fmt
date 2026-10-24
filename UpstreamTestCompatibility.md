# Upstream test compatibility

This check uses branch `CheckTests`, based on upstream commit
`0fc31633e7d53ac895ae0a688b83b7d44f0b0c1f`, with only the library header
`include/fmt/color.h` imported from `FmtColorDev` commit
`d9d64d361591311901e06c662c24879128c63724`.
The imported header has not been modified for this check.

## Unchanged upstream color tests

The original five color test cases compile in C++23. Four pass unchanged;
`color_test.text_style` fails at six exception assertions. Each expects a
same-channel overlap involving a terminal color to throw `fmt::format_error`.
Our implementation intentionally promotes both specified colors to fixed
Xterm-default RGB values and combines their channel bits by OR instead.

Thus, the original tests do not all pass unchanged. The failures identify a
deliberate behavior change, rather than a compilation failure.

## Required test changes

Only `test/color-test.cc` required test changes. Existing upstream names, including
`terminal_color` and `strikethrough`, are retained.

| Original expectation                     | Replacement expectation                                     | Count |
| ---------------------------------------- | ----------------------------------------------------------- | ----- |
| Terminal black OR terminal black throws  | RGB black, separately for foreground and background             | 2 |
| Terminal black OR terminal white throws  | RGB `{229, 229, 229}`, separately for foreground and background | 2 |
| Terminal black OR named RGB black throws | RGB black, separately for foreground and background             | 2 |

The expected RGB values are explicit, rather than computed through the
conversion implementation being checked. These equality checks also verify
promotion to the RGB kind, rather than merely accepting any nonthrowing result.
Terminal white uses the normal Xterm-default value 229, not bright white's 255.

Four existing `EXPECT_NO_THROW` expressions now explicitly discard their
results with `(void)`. Our style OR operator is marked `FMT_NODISCARD`, so
discarding its result implicitly produces warnings in modern language modes.
The six replacement equality checks consume their results and eliminate the
corresponding warnings there. No warning suppression was added.

## Required documentation changes

The upstream API page refers to `fg(detail::color_type)` and
`bg(detail::color_type)`. The documentation build fails because those signatures
no longer exist: both functions now accept public `color_t`.

Update the two MkDocs directives in `doc/api.md` to `fg(color_t)` and
`bg(color_t)`. The documentation then builds successfully and renders both
function references. No handler or header changes are required.

```bash
export PATH="$PWD/build/venv/bin:$PATH"
cmake --build build --target doc
```

This uses the existing documentation environment and configured build with
`FMT_DOC=ON`. The known upstream `syntax.md` missing-anchor notice and Material
notice are nonfatal. The documentation log is `build/check-upstream-doc.log`.

## Validation

The check runs locally on Windows/MSYS2 with GCC 15.3.0, Release builds, and
the upstream test infrastructure. It does not import our added regression
tests or our modified CMake configuration.

```bash
cmake -S . -B build/check-upstream-tests -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=23 \
  -DFMT_TEST=ON -DFMT_DOC=OFF
cmake --build build/check-upstream-tests --parallel 8
ctest --test-dir build/check-upstream-tests --output-on-failure
```

The adjusted five color test cases also pass in C++11 with
`-pedantic-errors -Wall -Wextra -Werror`. The standalone C++11 executable uses
the same upstream test support libraries:

```bash
g++ -D_GNU_SOURCE -std=c++11 -pedantic-errors -Wall -Wextra -Werror -O2 \
  -Iinclude -Itest -Itest/gtest test/color-test.cc \
  build/check-upstream-tests/test/libtest-main.a \
  build/check-upstream-tests/libfmt.a \
  build/check-upstream-tests/test/gtest/libgtest.a \
  -o build/check-upstream-tests/bin/color-test-cxx11.exe
build/check-upstream-tests/bin/color-test-cxx11.exe
```

All 22 configured CTest tests pass in C++23. The completed build log contains
no compiler warnings or errors, and the CTest log reports no individual skips.
No other upstream tests required changes. The upstream documentation build also
passes after the two reference updates above. This is local validation, not
a fresh CI matrix, C++11 full-suite, or module-build result.

Build and test logs are retained locally in
`build/check-upstream-build.log`, `build/check-upstream-ctest.log`, and
`build/check-upstream-cxx11.log`.
