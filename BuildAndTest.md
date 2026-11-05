# Build and test

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Website](Website.md)

Updated November 4, 2026. Commands run from the library checkout root unless
stated otherwise. Use `color-dev` or an authorized feature branch, not the
`coord/` documentation worktree on `docs/Coord`. Run library commands from
`..` when starting in `coord/`. `/doc/` means repository-relative.

## Current color-dev validation

Verified November 4, 2026 for `color-dev` and `pr/color` at the same amended
commit `35de08cc2de5b25100d02e51bdc41d8b595ccb01`, directly above upstream
`4ed3535092a96ac9f56fab80df83416d51bcc78e`. Local and GitHub branch SHAs match;
all nine workflow runs below target this exact SHA. Every job was inspected:
**all 41 push-CI jobs passed on each branch**. The separate `color-dev` guide
build and deployment also passed (**43 successful jobs on color-dev**).
All runs succeeded on attempt 1; no failed, cancelled, or pending jobs remain.

| Workflow          | color-dev evidence                                                                      | pr/color evidence                                                     |
| ----------------- | --------------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| Linux             | [18 passed](https://github.com/tjgolubi/fmt/actions/runs/41950172424)                   | [18 passed](https://github.com/tjgolubi/fmt/actions/runs/41950252811) |
| Windows           | [13 passed](https://github.com/tjgolubi/fmt/actions/runs/41950172367)                   | [13 passed](https://github.com/tjgolubi/fmt/actions/runs/41950252814) |
| macOS             | [9 passed](https://github.com/tjgolubi/fmt/actions/runs/41950172402)                    | [9 passed](https://github.com/tjgolubi/fmt/actions/runs/41950252945)  |
| Doc build         | [1 passed](https://github.com/tjgolubi/fmt/actions/runs/41950172372)                    | [1 passed](https://github.com/tjgolubi/fmt/actions/runs/41950252845)  |
| Guide publication | [Build and deployment passed](https://github.com/tjgolubi/fmt/actions/runs/41950172506) | Published from color-dev.                                             |

The amended tree removes the obsolete `feat/StyledText2` push exclusions from
the four ordinary workflows. These new results supersede the previous SHA's
validation. No rerun was initiated by the assistant. Check on demand only.

The remote matrix uses default/noncompact SGR. It includes old GCC 4.9 and
Clang 3.6 C++11, modern compilers, shared builds, and sanitizer jobs. Push CI
does not establish separate PR-only lint/fuzz passes, individual test-skip
counts, performance, ABI validation, or tablet results. GCC 16/C++26 reflection
remains excluded; issue #2 stays open. Check CI on demand, not periodically.

### Historical ColorCtx local validation

These earlier feature checks predate `35de08cc`; they were not rerun during the
website/coordination update. MSYS GCC 16.2 verified strict Debug C++11/C++23 color, styled-text-format, and
color-ostream suites in default SGR mode. C++23 core, format, and compile suites
also passed. The wide suite passed with an existing dangling-else warning and
missing `es_ES.UTF-8` locale notice. Compact direct runs passed **92 tests** in
C++11 and **132 tests** in C++23 (the latter includes compiled formatting).
Direct builds used `-pedantic-errors -Wall -Wextra -Werror`, treating bundled
GoogleTest as a system include where needed. Compiled formatting retains the
existing constant-evaluated styled `formatted_size` regression.

Changed-header syntax checks passed C++11/C++23 in both SGR modes with
`-Wconversion -Wshadow -Werror`. A C++11 compact/header-only no-exceptions
probe built and ran. The final stream-construction correction additionally
passed strict C++11 syntax checking with `-fno-elide-constructors` and the
existing Release/C++23 formatter test. Attribute corrections passed strict
GCC C++11/17/23 header checks with attribute warnings as errors. No local Clang
or GCC 4.9/11 compiler was available; the passing final remote matrix supplies
those checks.

Tests cover plain `{}` shortcuts, explicit/dynamic specifications, independent
contexts, nested styled wrappers and trees, absolute-only spans, inherited root
padding, empty/precision-zero text, direct and buffered nested formatters,
pointer output, wide/ostream/FILE entry points, and metadata restoration on
exceptions. The independent ANSI interpreter checks visible text/style state.

Local temporary `build/colorctx11` and `build/colorctx23` configurations were
removed later; do not assume their binaries or logs survive. Recreate an
isolated build rather than relying on stale caches:

```bash
cmake -S . -B build/colorctx11 -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_STANDARD=11 -DFMT_TEST=ON -DFMT_DOC=OFF \
  -DFMT_PEDANTIC=OFF -DFMT_WERROR=ON \
  -DCMAKE_CXX_FLAGS='-pedantic-errors -Wall -Wextra -Werror'
cmake --build build/colorctx11 \
  --target styled-text-format-test color-test color-ostream-test --parallel 8
ctest --test-dir build/colorctx11 \
  -R '^(styled-text-format|color|color-ostream)-test$' --output-on-failure
```

Use separate directories for C++23, optimization, and SGR mode. For compact
mode, add `-DFMT_COLOR_COMPACT_SGR=1` to compiler flags consistently. Defining
only a CMake cache variable of that name does not define the macro.

### Remaining local diagnostics and older evidence

Strict optimized MSYS GCC 16.2 validation encountered conversion diagnostics
in the existing file-output code under full fmt pedantic flags, and a
`-Wmaybe-uninitialized` diagnostic in the ANSI test interpreter's color-union
assignment under `-O3 -Werror`. No source warning suppression was introduced;
the exact strict Release configuration is not established passing.

The older Debian GCC 14.2 strict Release/C++11 blend-path warning remains
unresolved and is not established to be a false positive. Remote green jobs
do not reproduce that exact local configuration. The GCC 13/14 recursive-width
warning is distinct: its private noinline branch-width helper is retained and
the final shared CI passes.

Earlier Formatter checks covered 29 owning/formatter tests in four modes and
all 65,536 parent/child emphasis combinations, plus 65,536 transition pairs in
the color suite. Earlier cloud ASan/UBSan checks passed with leak detection
disabled because of ptrace restrictions; no successful leak check is claimed.
Those are historical focused results, not fresh local tests of `b8bf6293`.

### Allocation and demo coverage

Leaf copies/assignment and in-place root overlay require zero allocations.
MSVC iterator-debug builds alone permit bounded measured bookkeeping; larger
allocations must match the branch-copy baseline. An explicit `operator new`
control avoids old string copy-on-write and optimized-away allocations. The
final Windows matrix validates this policy. Other builds remain strict.

`make -C demo run` builds the checked-in C++23/header-only compact-SGR demo
without CMake or a prebuilt library. Earlier character-by-character appearance
checks covered nested colors, emphasis, absolute boundaries, and restoration.
The modernized demo is committed and pushed, using `absolute(underline(...))`.
Its helper cleanup was built with strict C++23 warnings and retained identical
ANSI output (350 bytes, two lines). The website embeds its complete source and
verified display. The deferred `.tjg.cpp` file is no longer present.

The updated guide built locally and its rendered inheritance/restoration
example was checked. The ordinary doc job and separate Pages build/deploy
passed for the former baseline `2fa0bd92`. The revised guide rebuilt locally, including expected
displays and the C++20 pi example. The localhost preview uses port 8875 and
`build/guide-output-preview`; check availability before relying on it.

## Historical parser candidate validation

`feat/parser` at `76fd8ba61e501ffa48a0eb1945475a67252f0d0e` is committed and
pushed, based on the former `color-dev` at `2fa0bd92`. The results in this section
are historical feature evidence. Parser/stripper code is now integrated into the
rewritten `color-dev`; only the current section above validates that new SHA.

At that candidate, the parser had six regression groups. Those runs passed **42 tests in
four focused suites** (owning text, formatter, parser, stripper) in each of eight
configurations: GCC 13.3/14.2 crossed with C++11/C++23 and compact/noncompact SGR.
All use header-only mode, `-pedantic-errors -Wall -Wextra -Werror`; GCC 13 uses
`-O3 -DNDEBUG`, while the usual GCC 14 checks are unoptimized. The host GCC 14
C++ runtime and prebuilt support/gtest libraries were used for the local GCC 13
runs; this is not an exact CI-image reproduction.

Parser regressions cover both character types, all indexed colors/emphasis masks,
color forms, reset-created groups, repeated resets without extra nesting,
relative prefixes, trailing resets, malformed/literal escapes, ownership, round
trips, and nested context behavior. The candidate guide now describes the reset
group rather than an unconditional absolute root; its example passed C++11/compact
and C++23/noncompact probes. No new full guide build was run locally because
Doxygen is unavailable; the remote doc build below passed.

### GCC 13 branch-move workaround

The reset-group update exposed `-Werror=maybe-uninitialized` in the GCC 13/C++23
Release shared CI compile, inlining the vector move through a concatenation chain
in the parser round-trip test. The exact warning was reproduced with GCC 13.3 at
`-O3 -DNDEBUG -DFMT_SHARED -fPIC`. Inspection found no initialization defect; the
vector move is guarded by the initialized discriminator. Original tests passed
ASan/UBSan (leak detection disabled); that is supporting evidence, not proof that
all uninitialized reads are absent.

A private noinline vector-move helper is gated by
`1300 <= FMT_GCC_VERSION && FMT_GCC_VERSION < 1400`. Other compilers retain the
original move path; leaf moves stay inline everywhere. No warning is suppressed.
The previously failing shared compile passes with strict warnings, the eight local
runs above pass, and the exact final CI shared job passes. A direct-link setup
error during testing was corrected by including the OS support used by stream
regressions. Those cloud logs were under
`/workspace/regression-checks/gated-gcc{13,14}-{11,23}-{0,1}.log`.
This workaround is separate from the older GCC 13/14 branch-width workaround.

A fresh Debug/C++23 CMake build in `build/parser23` passed parser, styled-text,
and styled-text-format targets through CTest. The header is registered for
installation and its test target is registered in CMake. A header-only C++11
no-exceptions probe passed. C++23 ASan/UBSan passed a deterministic 20,000-input
random-escape probe with leak detection disabled for the existing ptrace limit.
No new full local suite or old-compiler run is claimed.

All **41 push-CI jobs passed** for `76fd8ba6`. All workflow head SHAs matched
that full commit and every job's conclusion was inspected. No job failed,
was cancelled, or remained pending. This supersedes the earlier failed GCC 13
parser compile and older incomplete macOS matrix; their results do not validate
this final commit.

| Workflow | Passed jobs | Evidence                                                        |
| -------- | ----------- | --------------------------------------------------------------- |
| Linux    | 18          | [Run](https://github.com/tjgolubi/fmt/actions/runs/41795136165) |
| Windows  | 13          | [Run](https://github.com/tjgolubi/fmt/actions/runs/41795136298) |
| macOS    | 9           | [Run](https://github.com/tjgolubi/fmt/actions/runs/41795136527) |
| Doc      | 1           | [Run](https://github.com/tjgolubi/fmt/actions/runs/41795136829) |

No runs were cancelled or rerun by the assistant. Check on demand; no periodic
polling. These candidate results do not establish squash acceptance, PR-only
lint/fuzz, guide publication, or a full remote compact-SGR matrix.

The candidate `strip-ansi-test` passed all three test groups in strict header-only C++11/C++23.
It checks both character types, private/unknown CSI, numeric values irrelevant
to grammar, OSC hyperlinks, opaque control strings and both terminators,
malformed/incomplete sequences, C0/C1/NUL preservation, Unicode, and ownership.
The four focused Debug/C++23 CMake targets pass through CTest: stripper, parser,
styled-text, and styled-text-format. CMake registers both new installed headers
and test targets. A C++11 no-exceptions probe passed, including explicit
`char16_t`/`char32_t` calls. C++23 ASan/UBSan passed a 30,000-input deterministic
random-escape probe, checking output is a subsequence and never grows. Leak
detection was disabled for the known ptrace restriction; no leak-check pass is
claimed. Local strip logs are `/workspace/regression-checks/strip-{11,23}.log`.

Reproduce the focused CMake checks:

```bash
cmake -S . -B build/parser23 -DFMT_DOC=OFF -DFMT_TEST=ON \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_BUILD_TYPE=Debug \
  -DFMT_PEDANTIC=OFF -DFMT_WERROR=ON
cmake --build build/parser23 --target parse-styled-text-test \
  styled-text-test styled-text-format-test strip-ansi-test --parallel 4
ctest --test-dir build/parser23 \
  -R '^(strip-ansi|parse-styled-text|styled-text|styled-text-format)-test$' --output-on-failure
```

Direct four-mode parser logs are under
`/workspace/regression-checks/parser-{11,23}-{0,1}.log`. These are local artifacts.
The older color-dev context/formatter results above remain separately scoped.

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
The consolidated publication configuration retains these full-SHA action pins:

| Action                    | Release | Commit SHA                                 | Runtime    |
| ------------------------- | ------- | ------------------------------------------ | ---------- |
| `actions/configure-pages` | v6.0.0  | `45bfe0192ca1faeb007ade9deae92b16b8254a0d` | Node.js 24 |
| `actions/deploy-pages`    | v5.0.1  | `368f82528645a54fb793d4d04e342629a3f51346` | Node.js 24 |

The release tags were resolved through GitHub and both exact `action.yml`
files verified to declare `runs.using: node24`. This replaces the Node.js 20
pins that caused runner deprecation warnings. Workflow YAML parsing, the
renamed `color-dev` filter, both pins, and whitespace checks passed locally.
The guide rebuilt successfully during that work. Current `color-dev`
publication is verified at `35de08cc` as recorded above.

For `35de08cc`, the [guide publication](https://github.com/tjgolubi/fmt/actions/runs/41950172506)
passed both build and deployment from `color-dev`, establishing that the
environment permitted this branch. The precise environment policy was not
inspected. The [ordinary doc workflow](https://github.com/tjgolubi/fmt/actions/runs/41950172372)
also passed; it builds without deploying. See the current CI snapshot above,
[Website.md](Website.md#user-guide), and `support/fmtcolor-pages.md`.

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

The consolidated styling/tree/context/parser/stripper code is on `color-dev`
at `35de08cc`. Use the [current exact-SHA snapshot](#current-color-dev-validation),
including passing macOS and guide publication on attempt 1. Earlier feature
matrices do not establish a complete pass for the rewritten baseline.

Numeric bounds and cube/gray construction, variant access/visitation, blending,
fluent styles, ostream output, context inheritance, absolute spans, and SGR
transitions are covered by the integrated suites. C++11 remains required.
`FmtColorChangeLog.md` was regenerated for the consolidated history before the
documentation amendment captured in `35de08cc`.
The prior compiler-checked foreground blend example still uses `fg`, not `bg`:
terminal red and `gray_color{12}` produce RGB `{166, 64, 64}`, unspecified
background, bold and underline. Its style-copy factory form is constexpr only
from C++14; use `const auto` in C++11. No fresh application or performance
benchmark is claimed.

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

### Current integration-document examples

The refreshed `FmtColorIntegration.md` examples target C++17. A fresh local
MSYS GCC 16.2 static library was built and installed under
`build/docs-integration-check/prefix` with tests/docs OFF, installation ON,
Release, and shared libraries OFF. Extracted Make/CMake examples built and ran
against that installation in both SGR modes; the tree/parser/stripper example
also built and ran header-only in both modes. An independent ANSI interpreter
confirmed all displayed text/styles and the final terminal state. This was
example validation, not a full local regression run. Root upstream README
content was verified unchanged; local links and whitespace checks passed.

### Separate string-view comparison fix

`fix/StringViewCompare` at `f9b5ccae427403158e3695a8a94b3d1f1c2180ff` is based on
`main` at `4ed35350`; it is not part of `color-dev`. `detail::compare` now returns
zero immediately for `n == 0`, avoiding a zero-length `memcmp` call with null
pointers. Explicit braces in `test/xchar-test.cc` eliminate the dangling-else
warning around an existing GoogleTest macro.

The rejected `s1 == s2` shortcut broke C++20 constant evaluation of wide named
arguments in GCC 11 and Clang 14. The initial C++11/C++17 header-only syntax checks
did not instantiate that path. After removing it, existing `xchar-test.cc` and
`core-test.cc` passed GCC 16.2 C++20 syntax checks with `-D_GNU_SOURCE`,
`-pedantic-errors -Wall -Wextra -Werror`, `-Iinclude -Itest`, and GoogleTest as a
system include. The C++11 header check also passed. After adding the braces,
the wide test compiled with no warning exemption. No full local suite was run.

Latest inspected CI for that corrected SHA: [Linux](https://github.com/tjgolubi/fmt/actions/runs/41896720428)
19 passed (including the upstream reflection job),
[Windows](https://github.com/tjgolubi/fmt/actions/runs/41896720473) 13 passed,
[doc](https://github.com/tjgolubi/fmt/actions/runs/41896720612) 1 passed, and
[macOS](https://github.com/tjgolubi/fmt/actions/runs/41896720492/attempts/2) all
9 passed on attempt 2. All **42 jobs passed** for this exact SHA; no failed,
cancelled, or pending jobs remain in the latest attempts. Local and GitHub branch
SHAs match. These counts must not be used for the separate `color-dev` baseline. Upstream PR #5014 is open and unmerged; upstream CI was awaiting maintainer
approval at the last workflow inspection, distinct from these passing fork jobs.

### Website validation

Version **44** is the confirmed public Site publication for `35de08cc` source
links, passing CI on both `color-dev` and `pr/color`, and links labeled
“Proposed contribution / pr/color.” The large PR has not yet been submitted;
the site states this and has no upstream PR link for that contribution. The
planned backslash utilities and comparison-fix exclusion remain intact.

QA passed 46 local links/anchors, unique IDs, demo source/download/exact-commit
equality, and independent comparison of 214 rendered characters and their styles.
The two-branch CI table was inspected in the local browser; whitespace and
excluded-branch checks passed. Native deployment returned `succeeded`, the live
table was verified, and the owned port 8874 preview stopped. See Website.md for
publication IDs. No fresh full local library, tablet, performance, or ABI test
is claimed by website QA.

For new test results, record branch/full SHA, toolchain, mode, CMake options,
commands, results, and skips. Distinguish observed checks from Terry reports.
`build/Testing/Temporary/LastTest.log` is test output, not compiler diagnostics.
Follow [Workflow.md](Workflow.md#ci-gate-and-acceptance) for integration gates.
