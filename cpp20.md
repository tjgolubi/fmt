# CPP20 branch: handoff to PC Codex

Updated November 8, 2026. Read this document first when continuing the
independent TJG migration. This is separate from the usual coordination
documents, which describe the FMT-based development line. Terry explicitly
requested that those documents remain unchanged for this handoff.

## Current baseline

- Repository: https://github.com/tjgolubi/fmt
- Development branch: `ref/CPP20`, originally split from `color-dev`.
- Latest committed and pushed source: `e1fd53ddb16fbb5fd0cc31a4dc5c86af6d26b4d9`.
- GitHub was verified to match the cloud source checkout at that commit.
- Despite the branch name, TJG requires **C++23 or later**. Terry authorized
  using C++23 freely; do not preserve C++11/C++20 support for this branch's TJG
  extension.
- All latest CI workflows completed successfully for this exact commit:
  Linux run `42426189201`, Windows `42426189164`, macOS `42426189168`,
  documentation `42426189189`.

Inspect the PC checkout's branch, changes, remotes, and worktrees before
synchronizing. Preserve local work; do not reset, stash, or rewrite history
automatically. The cloud source checkout has no tracked modifications. Its
untracked `demo/FmtColorDemo2` and `demo/TableDemo` are generated binaries,
not missing source files.

## Implemented migration

The extension now uses standard formatting and TJG utilities, without including
or linking the FMT library. The original FMT engine remains in the repository;
leave that code alone when working on TJG.

- Extension namespace: `tjg`.
- Headers: `include/tjg/`, used as `#include <tjg/color.h>` and similar.
- Standalone project: `tjg/CMakeLists.txt`; exported target: `tjg::tjg`.
- Tests and independent test harness: `tjg/test/`. Google Test is used, but FMT
  libraries and FMT test helpers are not. Vendored Google Test currently lives
  under the repository's `test/gtest/`.
- User guide sources: `tjg/doc/` (eight files moved from `doc/color/`). Existing
  published guide URL paths are preserved through the MkDocs source hook in
  `support/python/tjg_docs.py`.
- Demos remain in `demo/`. The migration checks preserved their output.
- The install/relocation test builds an independent consumer using
  `find_package(tjg)` and `tjg::tjg`, with no FMT installed.

Headers include `color.h`, `color-ostream.h`, `styled_text.h`,
`parse_styled_text.h`, `strip_ansi.h`, `backslash.h`, `memory_buffer.h`,
`text_support.h`, `tree_format_specs.h`, and `format_support.h`.

`tjg::basic_memory_buffer` and its `tjg::memory_buffer` alias replace the
extension's FMT buffers. Standard string views and standard utility helpers
replace FMT equivalents. UTF-8 decoding, width estimates, truncation, and
escaping live in `text_support.h` under `tjg::detail`, with attribution.
The width behavior preserves the earlier code-point estimates; full grapheme
cluster and terminal-specific width handling are future work.

Standard formatters support colors, styles, emphasis, styled arguments, and
owning `styled_text`/`wstyled_text`, including nesting, absolute styles, dynamic
width/precision, alignment, padding, and tree inspection with `?`.
Readable color/style values use `?` as an unquoted presentation. The shared
parser in `tree_format_specs.h` operates on the original parse context rather
than replacing it, preserving argument indices and avoiding the MSVC failure.
Errors use `std::format_error`. Named format arguments are unsupported by
`std::format`; positional dynamic width and precision work. The only prior
named-argument use was a test, now changed to check rejection.

Terminal state is no longer carried by formatting contexts.
`tjg::get_terminal_state()` returns a reference to a function-local static
`text_style`. Terry explicitly requested a single shared instance for now,
without locking or `thread_local`. Formatting scopes restore enclosing state
after nesting and exceptions. Do not add synchronization without discussion.

## Build and test policy

Build and test **only TJG**, not the original FMT library/regressions.
Use C++23 or newer and test **both compact and non-compact SGR** from now on.
The option is `TJG_COLOR_COMPACT_SGR`, not the historical FMT option.

The root `build.bash` configures the standalone project in Debug with GCC,
warnings as errors, and both modes, under `build/tjg/ON` and `build/tjg/OFF`.
Equivalent commands (from the source root):

```bash
for mode in ON OFF; do
  cmake -S tjg -B "build/tjg/$mode" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DTJG_TEST=ON -DTJG_WERROR=ON -DTJG_COLOR_COMPACT_SGR="$mode"
  cmake --build "build/tjg/$mode" --parallel 4
  ctest --test-dir "build/tjg/$mode" --output-on-failure
done
```

Use a compiler and standard library with working `<format>`, `<print>`, and
`std::formattable`; C++23 mode alone is insufficient. For Visual Studio's
multi-configuration generator, also pass the configuration to build and CTest.

There are 17 local GCC CTest suites, including installation/relocation and
no-exceptions coverage. Platform-dependent suites can differ. All 17 passed
locally in both modes after the accessor fix. After the final one-line test
fix, the affected styled-text formatting suite passed in both modes, followed
by the full successful remote CI matrix for the final commit.

## CI configuration and resolved failures

Workflows are in `.github/workflows/`. CI is enabled on `ref/CPP20` for Linux,
Windows, macOS TJG regression tests and documentation. Lint and CodeQL are
excluded for this branch; the old FMT fuzz workflow was removed. Do not
re-enable extra workflows without instruction. Leave running jobs alone;
Terry asked not to cancel them. Check CI on demand, not by periodic polling.

The regression matrices cover Debug/Release and both SGR modes. Linux includes
GCC 14, Clang 20/libc++, and Debug ASan/UBSan coverage. Windows uses VS2022
v143/MSVC 19.44 in C++23 mode. macOS uses LLVM 20. All passed for the source
baseline above, including Windows Debug and Linux sanitizers.

Recently resolved issues, useful if another checkout shows older failures:

- A stored `styled` wrapper borrowed temporary `42`; the test now borrows its
  existing named `number`. The wrapper remains borrowing, not owning.
- Buffer member `_inline` collided with an MSVC extension; renamed
  `_inline_storage`.
- The temporary parse-context workaround for `?` lost MSVC checking data;
  replaced with the shared TJG parser using the original context. The old
  ignored `next_arg_id()` return disappeared with that workaround.
- The backslash test now expresses the narrow byte as `'\xff'`, and the wide
  code unit as `Char{255}`, avoiding MSVC's constant-conversion warning.
- Both `get<T>` overloads now check the active kind with `if`, report errors,
  then return normally, removing unreachable fallback expressions.
- The deliberately throwing test formatter no longer has an unreachable
  `return ctx.out()` after its unconditional throw.

No known CI failures remain at the current baseline.

## Next task and authorization

Terry's latest pending code request was spoken as: "Move all the gates for the
old compilers, the pre C++20 compilers." This likely means **remove** obsolete
compiler compatibility gates from TJG, but the wording has not been confirmed.
Clarify that point before editing. No gate cleanup has started. Inspect only
the TJG extension and its build/test support; do not mechanically remove gates
from the original FMT code. Exception-support checks serve the supported
no-exceptions configuration and are not automatically obsolete compiler gates.

This handoff document is the only requested coordination-file addition.
Do not update the usual coordination documents or infer C++11 requirements
from their FMT-specific baseline for TJG. Preserve the user's separate branch
and scope choices. Commit and push source changes only when requested; do not
infer permission from previous completed commit/push requests.

On the PC, fetch/read this file from `docs/Coord`, then continue code work on
`ref/CPP20`. The conversation history itself is not needed once this handoff
and the actual checkout have been inspected.
