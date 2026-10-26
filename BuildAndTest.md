# Build and test

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Website](Website.md)

Updated October 26, 2026. Commands run from the library checkout root unless
stated otherwise. Use `FmtColorDev` or the relevant candidate branch, not the
documentation-only `docs/Coord` branch. `/doc/` means repository-relative.

## Platforms and tools

Primary development platforms are Windows/MSYS2 and Android/Termux; GitHub CI
adds Linux, Windows, and macOS coverage. New work targets C++23 while retaining
{fmt}'s supported older-language paths. C++26 is a future target, not a current
local-build prerequisite.

On the PC use MSYS `/usr/bin` GCC, CMake, Python, and Doxygen. UCRT64 was removed
at Terry's request; do not use or reinstall it. Use MSYS2 Git for both project
repositories. Before diagnosing tool failures, check:

```bash
command -v git python cmake g++ doxygen
```

The latest ostream checks used MSYS GCC 16.2.0 and CMake 4.4.4. Other recorded
tools are Python 3.12.15, MkDocs 1.6.1, and Doxygen 1.18.0.
These are observed versions, not instructions
to install arbitrary latest releases. Keep CMake caches and virtual environments
specific to the selected platform/toolchain. See
[Workflow.md](Workflow.md#repositories-and-tools) for Git, SSH, and PATH rules.

## Build and tests

The shared script configures Release/C++23 with tests and documentation,
builds with eight jobs, runs CTest serially, then builds the guide:

```bash
./build.bash
```

Run it from the checkout root; do not source it. It stops at a failing test
before building documentation. For separate control:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=23 -DFMT_TEST=ON -DFMTCOLOR_SKIP=OFF
cmake --build build --parallel 8
ctest --test-dir build --output-on-failure
```

For a focused color change:

```bash
cmake --build build --target color-test --parallel 8
ctest --test-dir build -R '^color-test$' --output-on-failure
```

A focused pass does not establish a full-suite pass. Include wide-character,
compile-time-format, or other tests when affected; do not broaden or repeat
checks without a relevant change, failure, or unresolved concern.

### Termux descriptor skips

`FMTCOLOR_SKIP` is a test-only option, OFF by default. Seven tests deliberately
close descriptors owned by live C `FILE` objects and conflict with Android
`fdsan`: four in `gtest-extra-test`, three in `os-test`. ON produces explicit
GoogleTest skips; it does not disable fdsan or alter production code.
`build.bash` detects Termux and selects ON there, OFF elsewhere.

```bash
cmake -S . -B build -DCMAKE_CXX_STANDARD=23 \
  -DFMT_TEST=ON -DFMTCOLOR_SKIP=ON
cmake --build build --parallel 8
ctest --test-dir build --output-on-failure
ctest --test-dir build -R '^(gtest-extra-test|os-test)$' -V
```

Use OFF on ordinary PC/Linux runs; explicitly reset cached options when changing
purpose. An unused-option warning on an older branch means skips were not
enabled. Passing executables can contain individual skips, so record both.
Missing locales can also limit locale-dependent tests.

## Documentation

Use the checked-out, hash-pinned `support/doc-requirements.txt` and Doxygen.
Follow the virtual-environment technique in `support/release.py`, but do not run
that release script merely to build documentation. Reuse the environment unless
an interpreter/toolchain change makes it incompatible; preserve unrelated files.

```bash
(
  set -euo pipefail
  if [[ ! -d build/venv ]]; then
    python -m venv build/venv
  fi
  doc_bin="$(pwd)/build/venv/bin"
  "$doc_bin/python" -m pip install --require-hashes \
    -r support/doc-requirements.txt
  export PATH="$doc_bin:$PATH"
  cmake -S . -B build -U MKDOCS -DFMT_DOC=ON
  cmake --build build --target doc
)
```

`build.bash` uses this environment and installs its pinned dependencies when
creating it. Reinstall from the requirements if dependency inputs change.
MSYS and Termux environments use `bin/`; native Windows Python may differ.
`-U MKDOCS` clears an old cached launcher path. A stale Python launcher after an
upgrade may require recreating the environment, not replacing the whole toolchain.
Do not mix native Windows path conventions with incompatible MSYS tools.

The normal configuration is `support/mkdocs.yml`; output is `build/doc-html`.
The exact GitHub Pages build is:

```bash
(
  export PATH="$(pwd)/build/venv/bin:$PATH"
  PYTHONPATH=support/python python -m mkdocs build \
    --config-file support/mkdocs-pages.yml \
    --site-dir "$(pwd)/build/doc-html" --no-directory-urls
)
```

Preview locally, then stop with Ctrl-C:

```bash
python -m http.server 8000 --bind 127.0.0.1 --directory build/doc-html
```

Open `http://127.0.0.1:8000/` on that device. A known missing anchor in upstream
`syntax.md` and a Material notice about future MkDocs changes are nonfatal; keep
pinned dependencies and do not turn color work into an upstream-link cleanup.

### Reference rendering and publication

Doxygen comments in `include/fmt/color.h` supply shared API descriptions.
Markdown in `doc/color/` supplies organization and examples. The C++ handler at
`support/python/mkdocstrings_handlers/cxx/__init__.py` invokes Doxygen and renders
the reference, named RGB constants, explicit conversions, and swatches.
Public members need Doxygen comments or the handler omits their entries.
The text-style reference uses `exclude_members` to hide the six longer
component/presence accessors and the two former strike methods. Compatibility
notes describe the retained names. Check that `term_color`, `strike()`,
`is_strike()`, and the short getters, setters, and presence queries remain
rendered. The emphasis reference retains the documented enum alias.

The `::: name` blocks are **MkDocs directives**, not C++ or portable GitHub
Markdown. `!!! note "Changed behavior"` needs the configured `admonition`
extension. Check generated HTML for rendered API declarations and
`<div class="admonition note">`, rather than judging raw `.md` pages on GitHub.
The named-color page includes 141 named RGB swatches, 16 fixed terminal RGB
swatches in separate normal/bright grids, 24 gray swatches, and all 216 cube
colors. Cube rows vary red, columns green, and the six three-character fields
inside each cell vary blue. Check values and indexed-palette offsets against
independent expectations.

`.github/workflows/fmtcolor-pages.yml` builds and publishes after pushes to
`FmtColorDev`. The `github-pages` environment permits that branch. The separate
`doc` workflow verifies the build; it does not itself publish this guide.
See [Website.md](Website.md#user-guide) and `support/fmtcolor-pages.md` for setup.

## Compatibility checks

For color changes intended to preserve older modes, check relevant C++11,
C++14, C++17, C++20, and C++23 paths. For example:

```bash
g++ -std=c++11 -pedantic-errors -Wall -Wextra \
  -Wconversion -Wshadow -Werror -Iinclude \
  -fsyntax-only -x c++ include/fmt/color.h

g++ -D_GNU_SOURCE -std=c++11 -pedantic-errors -Wall -Wextra -Werror \
  -Iinclude -Itest -Itest/gtest -fsyntax-only test/color-test.cc
```

Repeat with affected language modes. MSYS GoogleTest needs `_GNU_SOURCE` for
POSIX declarations. The test-source command omits conversion/shadow warnings
for bundled GoogleTest; the header command and CI provide stricter coverage.
A modern compiler in C++11 mode is not evidence about the oldest compiler.

CI has verified old GCC/Clang releases as well as modern standard libraries.
When an exact release is unavailable locally, an authorized Compiler Explorer
check can isolate a diagnostic. Supply actual headers as separate files;
concatenating them can produce misleading errors. Include a negative check when
testing a rejection path and verify scoped warning suppression does not leak.

### Regression coverage

`FmtColorDev` color tests cover construction, all 216 cube RGB conversions
against independent Xterm values, terminal mappings, gray/cube boundaries,
range failures, type/index access and assignment, all five visitor alternatives,
reference lifetimes, mixed-kind OR promotion, ANSI output, comparisons, hashes,
plain/variant/style blending, `to_rgb()`, and `empty()`/fluent `clear()`.
Check unspecified versus specified black, kind preservation for a lone blended
color, independent style channels, and constexpr/noexcept contracts.

Fluent checks exercise all 256 emphasis masks, each attribute's enable/disable
and idempotence, reference returns, component replacement/clearing, presence
queries and aliases, and independent bold/faint rendering. ShortNames checks
retain aliases and all 16 terminal colors. Existing upstream spellings remain
in older tests. Targeted old GCC/native MSVC workarounds stay gated as described
in [CppDesign.md](CppDesign.md#compiler-constraints).

`color-ostream-test` covers styled `print`, `println`, `vprint`, and `vprintln`
with whole-message styling, argument formatting, newline/reset order, empty
styles and text, embedded nulls/braces, long text, stream-state preservation,
formatting errors before output, and stream failures and exceptions. Keep the
existing `ostream-test` and `color-test` when checking bridge changes.

### Pending TaskWarrior build mode

On the **TaskWarrior branch only**, `./build.bash taskwarrior` configures
`build/taskwarrior` with compiler definition `FMT_COLOR_COMPACT_SGR=1`, builds
the library and tests, then runs CTest. It passes `FMT_DOC=OFF` and skips the
documentation target; it still prepares the documentation virtual environment
if missing. The no-argument script uses `build` with documentation enabled.
Neither this build mode nor compact-SGR output is in the accepted baseline yet.

The setting must also be defined when compiling an application: SGR generation
is in the header. A CMake cache argument named `FMT_COLOR_COMPACT_SGR` alone
does not define a compiler macro. Use `target_compile_definitions` or compiler
flags, consistently across the application's translation units and libraries.
The same installed compiled library supports either header-selected mode.
`FmtColorIntegration.md` on TaskWarrior gives Makefile and CMake examples.

## Current validation

### Integrated ostream output

Verified October 26, 2026 for final amended `FmtColorDev` commit
[`257cdac170506770ef3c807abc3ddd9928ef8191`](https://github.com/tjgolubi/fmt/commit/257cdac170506770ef3c807abc3ddd9928ef8191):

| Workflow      | Successful jobs | Evidence |
|---------------------|----| ---- |
| Linux               | 18 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40305543116) |
| Windows             | 13 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40305543049) |
| macOS               |  9 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40305543144) |
| Documentation build |  1 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40305543000) |
| Guide publication   |  2 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40305542983) |

*Guide publication includes deployment*

All **43 jobs** passed, with exact local/GitHub/run SHAs matched and every job
independently checked. The deployed guide includes the styled ostream section,
all four bridge overloads, and the generated changelog alongside the existing
palette, conversion, blending, and style documentation.

The candidate `a80377d5c215bf77dda7b75f1af9991205397272` passed all 41 jobs in
the four push workflows before squash preparation. The staged squash tree
matched that candidate exactly; the generated changelog was included by
amendment before the final push. These final 43-job results supersede the
candidate's evidence for acceptance.

Local checks used an isolated Release/C++23 build in `build/ostream` with
tests enabled and documentation disabled. All three affected CTest targets
(`color-ostream-test`, `ostream-test`, and `color-test`) passed, including after
squash preparation. Strict C++11 header/test-source compilation, header-only
C++11 execution, both header include orders, disabled-exception compilation,
C++20 module compilation and an importing consumer, and installation passed.
MSYS module compilation required `-D_GNU_SOURCE` for POSIX declarations.
The guide built and all four generated ostream declarations were inspected.
No new local full-suite or Termux run is claimed; exact final-SHA CI supplies
the broader coverage.

### Upstream compatibility branch

Verified October 23, 2026 for `test/UpstreamCompatibility` commit
[`21eba01b33becd9f94cd34a1a8758f64ecbc53c3`](https://github.com/tjgolubi/fmt/commit/21eba01b33becd9f94cd34a1a8758f64ecbc53c3). It is based on
upstream `0fc31633`, with `color.h` identical to `FmtColorDev` at both
`d9d64d36` and current `257cdac1`. This verification does not include the new
ostream bridge; the final integration CI above tests that addition.

| Workflow        | Successful jobs | Evidence |
|---------------------|----| --- |
| Linux               | 19 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194916) |
| Windows             | 13 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194937) |
| macOS               |  9 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194887) |
| Documentation build |  1 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194931) |

All **42 jobs** passed for the matching local/GitHub/run SHA. Linux includes
GCC 4.9/Clang 3.6 C++11 and GCC 16/C++26 reflection. This branch uses upstream
CMake and workflows, without importing our added regression suite. Its only
production library change is the enhanced `color.h`. Ancillary additions are
the local build script, ignore rule, and
[`UpstreamTestCompatibility.md`](https://github.com/tjgolubi/fmt/blob/21eba01b33becd9f94cd34a1a8758f64ecbc53c3/UpstreamTestCompatibility.md).
The report's old `CheckTests` name refers to this renamed branch.

Required upstream adjustments are intentional:

- Six exception assertions become exact RGB-result checks: terminal black OR
  black yields RGB zero, black OR normal white yields RGB `229,229,229`, and
  terminal black OR named RGB black yields RGB zero, for both fg/bg.
- Four no-throw expressions explicitly discard nodiscard results with `(void)`.
- Two `doc/api.md` directives change from `fg/bg(detail::color_type)` to
  `fg/bg(color_t)`; no documentation-handler change is needed.

No other upstream tests required changes; do not say all tests pass unchanged.
Locally, all **22 configured C++23 CTest tests** passed with no compiler warnings
or reported individual skips, all **five color tests** passed strict C++11 with
`-pedantic-errors -Wall -Wextra -Werror`, and upstream documentation built.
This is separate from testing the complete FmtColorDev regression suite.

For a fresh isolated local compatibility build:

```bash
cmake -S . -B build/check-upstream-tests -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=23 \
  -DFMT_TEST=ON -DFMT_DOC=OFF
cmake --build build/check-upstream-tests --parallel 8
ctest --test-dir build/check-upstream-tests --output-on-failure
```

The report gives the standalone C++11 command. Logs are
`build/check-upstream-build.log`, `build/check-upstream-ctest.log`,
`build/check-upstream-cxx11.log`, and `build/check-upstream-doc.log`.
The separate existing documentation build requires `FMT_DOC=ON` and the pinned
environment. Switching branches may leave a configured build with other inputs;
inspect its cache and source branch before interpreting a result.

### Coverage limits and follow-up

FmtColorDev's Linux fail-fast is enabled; GCC 16/C++26 reflection is still
commented out there. The upstream compatibility matrix includes and passed that
job. [Issue #2](https://github.com/tjgolubi/fmt/issues/2) tracks the workflow
exclusion; do not claim it is restored on FmtColorDev or close the issue without
authorization. Upstream package caching remains dependent on downloads.

Push results do not establish passes for separate PR-only lint/fuzz workflows.
Individual test skips remain distinct from successful jobs. No new tablet run,
ABI verification, or performance benchmark is established by these checks.
After every amendment, header refresh, or rebase, verify the exact new commit
and every applicable job; never cite a baseline-only run for uncommitted work.

### Website validation

Website version **27** adds the ostream overview callout, implementation and
usage section, and exact final CI links for `257cdac1`, while preserving the
upstream-compatibility evidence. Three pages and 39 local links/anchors were
validated, with unique IDs and unchanged downloads, demo, design, and rendering
sections. The new section was inspected locally and live after native Sites
deployment succeeded. See
[Website.md](Website.md#current-publication) for exact publication provenance.

For new test results, record branch/full SHA, toolchain, mode, CMake options,
commands, results, and skips. Distinguish observed checks from Terry reports.
`build/Testing/Temporary/LastTest.log` is test output, not compiler diagnostics.
Follow [Workflow.md](Workflow.md#ci-gate-and-acceptance) for integration gates.
