# Build and test

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Website](Website.md)

Updated October 29, 2026. Commands run from the library checkout root unless
stated otherwise. Use `color-dev` or an authorized feature branch, not the
`coord/` documentation worktree on `docs/Coord`. Run library commands from
`..` when starting in `coord/`. `/doc/` means repository-relative.

## Cloud StyledText validation

The cloud experiment runs on Debian 13 x86-64 with native GCC 14.2.0 and
CMake 4.4.4. Terry explicitly authorized native Linux tools; no MSYS2 emulation
is used. All 23 original CTest targets passed in a Release/C++23/default-SGR
build at the original `5b3f49ff` baseline, with documentation disabled.

The SGR transition change at `cff2fc5fa0793fb476b94441aa2241c895423a2b`
passed `color-test` in C++23/default, C++23/compact, and C++11/compact builds,
including all 65,536 emphasis-state pairs checked with an independent interpreter.
Strict C++11 header and color-test syntax checks passed. This commit is pushed;
GitHub CI results have not been checked, and no green remote matrix is claimed.

The `styled-text.h` foundation at pushed commit `7b0e07ce` passes its new
`styled-text-test` target in Release/C++23 and Release/C++11, both with
`FMT_COLOR_COMPACT_SGR=1`. Tests cover default/string/vector construction,
implicit literals and strings, alternative access and wrong-kind errors,
concatenation/nesting, exact equality, independent copies, moves, alternative
changes, self-assignment, and assignment from a descendant. Strict C++11 test
compilation with `-pedantic-errors -Wall -Wextra -Werror` passed.
An additional C++11/header-only probe ran 1,000 ownership/copy/move/descendant-
assignment cycles under AddressSanitizer and UndefinedBehaviorSanitizer with
no reported errors. LeakSanitizer cannot run under this environment's tracing;
`ASAN_OPTIONS=detect_leaks=0` was required, so leak detection is not established.

The current experimental build is `build/taskwarrior`, Release/C++23, with
compact SGR enabled through the compiler flags, not merely a named CMake option.
The C++11 compact build is `build/transition-cxx11-compact`. Reproduce after
restoring the source using a locally available CMake:

```bash
cmake -S . -B build/taskwarrior -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_STANDARD=23 -DFMT_TEST=ON -DFMT_DOC=OFF \
  -DFMTCOLOR_SKIP=OFF -DCMAKE_CXX_FLAGS=-DFMT_COLOR_COMPACT_SGR=1
cmake --build build/taskwarrior --target styled-text-test color-test --parallel 8
ctest --test-dir build/taskwarrior -R '^(styled-text|color)-test$' --output-on-failure
```

Use a separate build directory with `CMAKE_CXX_STANDARD=11` for the fallback.
No full-suite result is claimed for the styled-text foundation, and no guide
build, module export, old-compiler CI, or website publication was performed.
The backup includes source snapshots and Git history, not platform build caches.
See [StyledText.md](StyledText.md) for design, scope, and current local status.

## Platforms and tools

Primary development platforms are Windows/MSYS2 and Android/Termux; GitHub CI
adds Linux, Windows, and macOS coverage. Local development uses C++23; the
complete {fmt} contribution must retain C++11 support. C++26 is a future target, not a current
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
From PowerShell, a nonlogin `bash --noprofile --norc -c` preserves the selected
checkout and avoids user startup commands changing directory. Put MSYS tools
and `build/venv/bin` on that process's PATH; do not use unrelated native Python
for the MSYS documentation environment.
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
component/presence accessors and the two strike aliases. Compatibility notes
describe the original retained names; strike member aliases are described
without implying that they existed in the original upstream header. Check that
`term_color`, `strike()`,
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

`.github/workflows/fmtcolor-pages.yml` targets pushes to `color-dev`, and
`support/mkdocs-pages.yml` uses `edit/color-dev/doc/`. The instructions match.
The `color-dev` name changes are local and uncommitted. The preceding `dev`
configuration was committed and pushed at
`a18689e1a0aced47e96db18b02aab577a853168a`, with these unchanged full-SHA pins:

| Action                    | Release | Commit SHA                                 | Runtime    |
| ------------------------- | ------- | ------------------------------------------ | ---------- |
| `actions/configure-pages` | v6.0.0  | `45bfe0192ca1faeb007ade9deae92b16b8254a0d` | Node.js 24 |
| `actions/deploy-pages`    | v5.0.1  | `368f82528645a54fb793d4d04e342629a3f51346` | Node.js 24 |

The release tags were resolved through GitHub and both exact `action.yml`
files verified to declare `runs.using: node24`. This replaces the Node.js 20
pins that caused runner deprecation warnings. Workflow YAML parsing, the
renamed `color-dev` filter, both pins, and whitespace checks passed locally.
The guide rebuilt successfully; commit/push and publication verification for
the renamed filter remain pending.

For `a18689e1`, [FmtColor documentation](https://github.com/tjgolubi/fmt/actions/runs/40680365434)
completed successfully: both `build` and `deploy` jobs passed, including
Configure Pages and Publish documentation. The [ordinary doc workflow](https://github.com/tjgolubi/fmt/actions/runs/40680365328)
also passed. The platform runs were still in progress when checked:
[Linux](https://github.com/tjgolubi/fmt/actions/runs/40680365266),
[Windows](https://github.com/tjgolubi/fmt/actions/runs/40680365275), and
[macOS](https://github.com/tjgolubi/fmt/actions/runs/40680365376).
Do not claim a full green matrix for this new SHA from the earlier 41-job result.

Publication from the former `dev` branch succeeded, establishing that the
environment permitted that deployment. Verify that its selected-branch rule
also permits `color-dev`; the rename does not establish that permission.
Its exact branch-policy setting was not inspected: the connector does not expose the endpoint and the available browser was signed
out. The guide was also rebuilt locally with the new edit URI. The ordinary
`doc` workflow builds documentation without deploying it.
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

`color-dev` color tests cover construction, all 216 cube RGB conversions
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

### Compact SGR build mode

On `color-dev`, `./build.bash taskwarrior` configures
`build/taskwarrior` with compiler definition `FMT_COLOR_COMPACT_SGR=1`, builds
the library and tests, then runs CTest. It passes `FMT_DOC=OFF` and skips the
documentation target; it still prepares the documentation virtual environment
if missing. The no-argument script uses `build` with documentation enabled.
The mode and compact-SGR API are part of the accepted `color-dev` contribution.

The setting must also be defined when compiling an application: SGR generation
is in the header. A CMake cache argument named `FMT_COLOR_COMPACT_SGR` alone
does not define a compiler macro. Use `target_compile_definitions` or compiler
flags, consistently across the application's translation units and libraries.
The same installed compiled library supports either header-selected mode.
`FmtColorIntegration.md` gives Makefile and CMake examples.

## Current validation

### Integrated styling helpers

Verified October 28, 2026 for the consolidated `color-dev` commit
[`2c456778d9ff63cdf284a7d85c8dc8eb384cf3d3`](https://github.com/tjgolubi/fmt/commit/2c456778d9ff63cdf284a7d85c8dc8eb384cf3d3),
whose parent and upstream baseline is `f87a75e15262614ce8aa0e6a0664df9d0823fb92`:

| Workflow            | Successful jobs | Evidence                                                        |
| ------------------- | --------------: | --------------------------------------------------------------- |
| Linux               |              18 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40670919249) |
| Windows             |              13 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40670919152) |
| macOS               |               9 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40670919144) |
| Documentation build |               1 | [Run](https://github.com/tjgolubi/fmt/actions/runs/40670919387) |

All **41 jobs** completed successfully, with none failed, cancelled, or pending.
The local commit, GitHub branch (then named `dev`), and run SHAs matched. Every job was
inspected. This SHA already includes Terry's deletion of `FmtColorChangeLog.md`
and amended force-with-lease push; no post-verification amendment is implied.
The four push workflows cover the complete contribution. The guide-publication
workflow did not run for that commit because its branch filter was still
`FmtColorDev`. The filter switched to `dev` in `a18689e1`, whose successful publication is
recorded above; this does not retroactively add a run to `2c456778`.

The current user-guide documents built successfully with the pinned MkDocs
configuration and were served from `build/doc-html` at
`http://localhost:8000/color/index.html`. The server was running at the handoff;
check availability before restarting it. Firefox can use this URL directly.
Rendered checks covered API Reference entries, the requested section links,
factory/fluent examples, and expected output. The known upstream `syntax.md`
missing-anchor notice and Material future-version notice remain nonfatal.
A local build and successful `doc` CI alone do not establish publication; the
separate successful Pages run at `a18689e1` is recorded above.

A focused MSYS2 GCC probe verified the website's style-copy factory example
with `-std=c++14` and `-std=c++23`, `-pedantic-errors -Wall -Wextra -Werror`,
`-Iinclude`, and `-fsyntax-only`. Static assertions verify that
`constexpr auto second = fmt::underline(fmt::fg(fmt::gray_color{12}));`
blends with the bold terminal-red foreground to RGB `{166, 64, 64}`, with
unspecified background, bold, and underline. The `const auto` form was also
checked in C++11; the style-copy overload is not C++11 constexpr. This is a
focused syntax/constant-evaluation check, not an additional runtime-suite pass.
`build/website-blend-probe.cc` is disposable ignored build output.

Historical local checks covered both SGR modes, assertion-enabled boundaries,
header-only exceptions-disabled paths, and a 23-target compact C++23 suite.
Those checks predate the rebased contribution and are not fresh full-suite
results for `color-dev`. GitHub CI uses default SGR output; compact-mode
cross-platform CI remains a coverage gap. Terry's reported TaskWarrior tests
are application-team evidence, not a freshly executed application run here.
No new tablet run or performance benchmark is claimed.

`color-test` covers exhaustive emphasis OR-assignment masks, returned reference,
chaining and constexpr behavior; signed palette/RGB inputs; mixed byte/int RGB
arguments; cube getter types; and release clamping at negative/oversized
boundaries including integer limits. Cube index tests check clamping before
decomposition. Assertion-enabled invalid input belongs in `assert-test`;
release fallback tests are gated by `NDEBUG` so debug runs do not abort.

`color-ostream-test` additionally covers normal narrow/wide insertion, chaining,
empty styles/text, embedded nulls/braces, stream formatting and stream-only
values, plus reset recovery preserving the original exception. Recovery tests
use `FMT_USE_EXCEPTIONS`, matching the current core macro. The generalized print
functions have wide tests for all four APIs, SGR modes, reset/newline order,
stream format preservation, formatting failures before output, and I/O failures.

Reproduce the focused assertion-enabled build from the library root:

```bash
cmake -S . -B build/clamp-debug -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=11 \
  -DFMT_TEST=ON -DFMT_DOC=OFF
cmake --build build/clamp-debug --target assert-test color-test --parallel 4
ctest --test-dir build/clamp-debug -R '^(assert|color)-test$' --output-on-failure
```

### Upstream compatibility branch

Verified October 23, 2026 for `test/UpstreamCompatibility` commit
[`21eba01b33becd9f94cd34a1a8758f64ecbc53c3`](https://github.com/tjgolubi/fmt/commit/21eba01b33becd9f94cd34a1a8758f64ecbc53c3). It is based on
upstream `0fc31633`, with `color.h` identical to `FmtColorDev` at both
`d9d64d36` and `257cdac1`. It does not verify the current consolidated `color-dev`
header, its rebased upstream baseline, or the ostream bridge. A header refresh and fresh compatibility CI are
needed before extending that evidence to the later features.

| Workflow              | Successful jobs | Evidence                                                        |
| --------------------- | --------------- | --------------------------------------------------------------- |
| Linux                 | 19              | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194916) |
| Windows               | 13              | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194937) |
| macOS                 | 9               | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194887) |
| Documentation build   | 1               | [Run](https://github.com/tjgolubi/fmt/actions/runs/40049194931) |

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
This is historical evidence separate from the current `color-dev` regression suite.

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

Matrix fail-fast is permanently disabled across Linux, Windows, and macOS.
GCC 16/C++26 reflection is still commented out on `color-dev`. The upstream compatibility matrix includes and passed that
job. [Issue #2](https://github.com/tjgolubi/fmt/issues/2) tracks the workflow
exclusion; do not claim it is restored on `color-dev` or close the issue without
authorization. Upstream package caching remains dependent on downloads.

Push results do not establish passes for separate PR-only lint/fuzz workflows.
Individual test skips remain distinct from successful jobs. No new tablet run,
ABI verification, or performance benchmark is established by these checks.
After every amendment, header refresh, or rebase, verify the exact new commit
and every applicable job; never cite a baseline-only run for uncommitted work.

### Website validation

Website version **36** presents the contribution under its former `dev` name at `2c456778` as one contribution and
links the four exact 41-job CI runs. It omits intermediate development history,
obsolete compatibility claims, and links to the removed changelog. Validation
of the revised three-page site covered 40 local links/anchors and unique IDs;
the overview and contribution layout were inspected in the browser. Subsequent
small example corrections were checked against the source, and each native
publication returned `succeeded`. The final foreground-blend expression and
result were compiler-checked as described above. Downloads were preserved.
The site preview server on port 8875 was stopped; the separate guide server on
8000 was left running. See [Website.md](Website.md#current-publication) for
final source, version, archive, and deployment identifiers.

For new test results, record branch/full SHA, toolchain, mode, CMake options,
commands, results, and skips. Distinguish observed checks from Terry reports.
`build/Testing/Temporary/LastTest.log` is test output, not compiler diagnostics.
Follow [Workflow.md](Workflow.md#ci-gate-and-acceptance) for integration gates.
