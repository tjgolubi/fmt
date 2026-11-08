# {color}: handoff to the next work session

Updated November 8, 2026. Read this document first when continuing the independent
{color} library, probably on Terry's PC. The other coordination documents describe
the separate FMT-based development line; do not apply their C++11 requirement to
this branch or update them as part of this handoff.

## Current baseline and checkout state

- Repository: https://github.com/tjgolubi/fmt
- Development branch: `ref/CPP20`, originally split from `color-dev`.
- Latest committed and pushed source: `7253096c43cf30ec3915ea2b53517064bdad1f3e`.
- GitHub's branch tip was verified to match the cloud checkout at that commit.
- The project is called **{color}**. Namespace `tjg`, headers under `include/tjg/`,
  and CMake target `tjg::tjg` retain their existing names.
- **C++23 is the current target.** C++20 support will be added later through
  separate patches; it is not a current compatibility requirement.
- All 19 applicable CI jobs passed on attempt 1 for this exact SHA. Terry also
  confirmed this commit passes on his Samsung Android tablet running Termux.

The cloud library checkout is `/workspace/fmt` on `ref/CPP20`, with a clean working
tree. Its separate documentation worktree is `/workspace/fmt-coord` on `docs/Coord`.
This handoff update is being prepared there; its commit/push status must be reported
separately from the already committed and pushed library source.

Inspect the PC's branch, changes, remotes, and worktrees before synchronizing.
Preserve local work; do not reset, stash, or rewrite history automatically. The
MSVC test-only commit was amended during diagnosis: `ea1ae7eb` was replaced by
`447a368e`. The final production workaround `7253096c` is a normal subsequent
commit. A checkout containing `ea1ae7eb` needs deliberate synchronization rather
than an ordinary pull. Terry deferred pulling during diagnosis; inspect his actual
PC history rather than assuming where it stopped.

## Implemented independent library

The extension uses standard formatting and TJG utilities without including or
linking the FMT library. The original FMT engine remains in the repository; leave
it alone when working on {color}.

- Public headers: `include/tjg/`, used as `#include <tjg/color.h>` and similar.
- Standalone project: `tjg/CMakeLists.txt`; exported target: `tjg::tjg`.
- Tests and independent helpers: `tjg/test/`. GoogleTest is reused from
  `test/gtest/`, without FMT libraries or FMT test helpers.
- Guide sources: `tjg/doc/`. Existing published guide paths are preserved by
  `support/python/tjg_docs.py`.
- Demos: `demo/FmtColorDemo2.cpp` and `demo/TableDemo.cpp`.
- Installation/relocation tests build a consumer with `find_package(tjg)` and
  `tjg::tjg`, without FMT installed.

Headers include `color.h`, `color-ostream.h`, `styled_text.h`,
`parse_styled_text.h`, `strip_ansi.h`, `backslash.h`, `memory_buffer.h`,
`text_support.h`, `tree_format_specs.h`, `format_support.h`, and `error_support.h`.
The own memory buffer replaces FMT buffers. UTF-8 decoding, width estimates,
truncation, and escaping live in `text_support.h`, with attribution. Full grapheme
and terminal-specific width handling remain future work.

Standard formatters support colors, styles, emphasis, borrowing styled arguments,
and owning narrow/wide styled text, including nesting, absolute styles, dynamic
width/precision, padding, and tree inspection with `?`. Readable color/style values
use `?` without quoting. The shared specification parser uses the original standard
parse context, preserving argument indices and MSVC's checking data. Named format
arguments are unsupported by standard formatting; positional dynamic specifications
work. Errors use `std::format_error` through the failure policy below.

Terminal state is separate from standard formatting contexts.
`tjg::get_terminal_state()` returns a reference to a function-local static
`text_style`. Terry explicitly requested one shared instance for now, without locks
or `thread_local`. Formatting scopes restore enclosing state after nesting and
exceptions. Do not add synchronization without discussion.

## Assertion and exception toolkit

`<tjg/error_support.h>` supplies six configurable macros:

| Macro                             | Policy                                                          |
| --------------------------------- | --------------------------------------------------------------- |
| `TJG_ASSERT(condition, message)`  | Like standard assert; disabled by `NDEBUG`, without evaluation. |
| `TJG_ENFORCE(condition, message)` | Runtime assertion, active regardless of `NDEBUG`.               |
| `TJG_EXPECTS(condition, message)` | Always-active precondition; stand-in for GSL `Expects`.         |
| `TJG_ENSURES(condition, message)` | Always-active postcondition; stand-in for GSL `Ensures`.        |
| `TJG_THROW(exception)`            | Throw with exceptions enabled; report and abort otherwise.      |
| `TJG_NOEXCEPT_UNLESS_BAD_ALLOC`   | Empty by default; application may define it as `noexcept`.      |

The default failure function uses `std::source_location`, writes and flushes a
message to stderr without formatting-library dependencies, and aborts. Policies
can be overridden before including headers, consistently across translation units.
The existing color range and absolute-blend checks use `TJG_ASSERT`, preserving
their documented Release clamping/unspecified-channel behavior. Formatting failures
and buffer length errors use `TJG_THROW`, directly or through the formatting-error
helper. Existing exception-recovery/rethrow guards remain necessary for supported
no-exceptions builds. GSL is not currently a dependency.

When generating new code, apply `TJG_NOEXCEPT_UNLESS_BAD_ALLOC` where allocation
failure is the only intended escaping exception. Audit the function and its
callees. Leave functions unannotated if length errors, format/parse errors, stream
failures, or exceptions from user-defined conversions, operations, or iterators are
plausible. Use ordinary `noexcept` for functions that cannot throw at all.

Current annotations cover owned-string styled-text construction, tree copying and
copy assignment, bounded `to_sgr` overloads, and tree-based ANSI stripping and its
recursive helper. Tests cover narrow/wide signatures with the hook empty and set
to `noexcept`, and verify that decoding errors still propagate. An application may
opt in when its allocation-failure policy terminates; this does not authorize us
to replace its allocator or process-wide allocation handler. All escaping
exceptions terminate at an enabled `noexcept` boundary, not just `std::bad_alloc`.

## MSVC constexpr variant workaround

`color_t` is now exactly
`std::variant<no_color_t, rgb, term_color, cube_color, gray_color>`.
It does not use the former custom variant or public custom `get`/`visit` machinery.

MSVC rejects valid constexpr transitions back to an empty `no_color_t` alternative
with C2131, reporting inactive union-member access inside its `std::variant`.
Assigning a temporary and using `emplace` did not solve it. The final fix is a
private `char _dummy{'\0'}` in `no_color_t`, gated by:

```cpp
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER <= 1951
```

Comparisons remain defaulted; ordinary construction and copying preserve zero in
the private member. Other compilers retain the empty class. The emptiness/size test
accounts for the gate. The original constexpr `color = tjg::no_color` test is
restored without compiler-specific substitution. Do not reintroduce the failed
test-only workarounds.

A minimized reproducer failed on MSVC 19.44, both tested 19.50 releases, and 19.51
(the newest version then offered by Compiler Explorer). The dummy member passed
those versions. The restored constexpr function with the actual separate project
headers also compiled on MSVC 19.44, 19.50, and 19.51. Full Windows CI subsequently
passed on MSVC 19.44 for the exact baseline above. No claim is made about a future
compiler fixing the issue; review the gate when testing later versions. Object
layout is compiler-dependent and not a stable archive or ABI contract.

## Header order and formatting preferences

Terry wants includes to expose missing direct dependencies:

1. The source file's corresponding header first when applicable.
2. Project headers, from most specific to most general.
3. Third-party headers.
4. Higher-level C++ standard facilities, then containers/general facilities,
   then foundational utilities.
5. C compatibility headers such as `<cstdio>`, `<cstdlib>`, `<cstdint>`, and
   `<cstddef>` last; platform C headers are also last.

The {color} headers, tests, and demos follow this order. `.clang-format` has
explicit include categories and sort priorities for the project and standard
headers, and regroups their includes accordingly. Extend those categories when
new headers require deliberate ordering. Clang-format 21.1.8 was used to verify
that it preserves the ordering. All 11 public headers compiled independently;
both demos passed syntax checks. No missing includes surfaced in these checks.
Function bodies and existing brace layouts were not reformatted.

Terry prefers an 80-column limit. A trailing return type and opening brace may stay
on the signature line when they fit. Otherwise he prefers this layout:

```cpp
auto function(arguments)
  -> return_type
{
```

This exact conditional brace/indentation preference has not been implemented in
the formatter configuration. Do not add noisy `clang-format off/on` comments to
force it. Discuss formatter changes rather than assuming upstream Google-style
indentation and brace choices are his preference.

## Build, platforms, and verified results

Build and test **only {color}/TJG**, not the original FMT library/regressions.
Use C++23 or newer and test both compact and noncompact SGR. The compiler and
standard library must implement `<format>`, `<print>`, and `std::formattable`.
The option is `TJG_COLOR_COMPACT_SGR`, not the historical FMT option.

The root `build.bash` explicitly selects `g++`, configures Debug with warnings as
errors, builds, and runs CTest for both modes in `build/tjg/ON` and `OFF`. It stops
at the first failed command, so a compact-mode failure prevents the noncompact
run. For an explicit Clang build, preserve existing GCC caches by using separate
build directories:

```bash
for mode in ON OFF; do
  cmake -S tjg -B "build/tjg-clang/$mode" \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DTJG_TEST=ON -DTJG_WERROR=ON -DTJG_COLOR_COMPACT_SGR="$mode"
  cmake --build "build/tjg-clang/$mode" --parallel 4
  ctest --test-dir "build/tjg-clang/$mode" --output-on-failure
done
```

Visual Studio multi-configuration builds also need the configuration passed to
build and CTest. Local Linux has 21 suites including policy tests,
installation/relocation, and no-exceptions coverage; platform counts can differ.
All 21 passed locally in both SGR modes for the final source tree using Clang
21.1.8, C++23 Debug, warnings as errors, and CMake 4.4.5. Earlier GCC 14.2 baselines
also passed; do not represent those as fresh GCC results for the final SHA.

Terry's current tablet toolchain is Termux Clang 21.1.8 targeting
`aarch64-unknown-linux-android24`, CMake 4.4.5, and Python 3.14.10. He permits use of
other Python versions in our environment. He confirmed `7253096c` passes on the
tablet after all fixes. The cloud uses Linux x86-64; its tools were installed under
`/workspace/tools` with user-PATH wrappers. Inspect actual tool availability in a
new environment rather than assuming those installations persist.

Two Android test-infrastructure fixes are retained:

- GoogleTest's shared `CapturedStream` in `test/gtest/gmock-gtest-all.cc` honors a
  nonempty `TMPDIR` on non-Windows platforms. Unset/empty values retain platform
  fallbacks. Termux already sets a writable `TMPDIR`; `/data/local/tmp` is not
  writable by its app user. Tests verified custom paths, trailing slashes,
  cleanup, and fallbacks.
- The shared TJG test runner uses `threadsafe` death tests and forces their
  `death_test_use_fork` flag on Android. Raw `clone()` caused death-test children
  to die with empty captured stderr even with explicit diagnostic writes. Terry
  verified `GTEST_DEATH_TEST_USE_FORK=1 ./build.bash` fixed both modes; the runner
  now selects it automatically. No environment-variable workaround or weakened
  diagnostic checks are needed. This vendored GoogleTest flag needs a local
  declaration because its public header omits it.

Vendored GoogleTest emits a Clang 21 `-Wcharacter-conversion` warning converting
`char8_t` to `char32_t`. It is nonfatal and Terry explicitly said to leave it alone.

Exact baseline CI, verified November 8, 2026, all on attempt 1:

| Workflow      | Result     | Run                                                             |
| ------------- | ---------- | --------------------------------------------------------------- |
| TJG Linux     | 10 passed  | https://github.com/tjgolubi/fmt/actions/runs/42513162971        |
| TJG Windows   | 4 passed   | https://github.com/tjgolubi/fmt/actions/runs/42513163104        |
| TJG macOS     | 4 passed   | https://github.com/tjgolubi/fmt/actions/runs/42513163051        |
| Documentation | Guide pass | https://github.com/tjgolubi/fmt/actions/runs/42513163045        |

All run head SHAs match `7253096c43cf30ec3915ea2b53517064bdad1f3e`. Windows
MSVC 19.44/v143 and macOS LLVM 20 cover Debug/Release and both SGR modes. Linux
covers GCC 14, Clang 20/libc++, and Debug ASan/UBSan. The original FMT doc job is
intentionally skipped on this branch; the TJG guide job passed. There are no
failed, cancelled, or pending applicable jobs in those attempts.

## Next session and authorization

This session is complete; the Termux and MSVC failures above are resolved. No new
implementation task is selected. The older request about removing pre-C++20
compiler gates remains an unconfirmed discussion item, not automatic permission
to perform broad cleanup. The targeted MSVC bug workaround and exception-support
gates are intentional and must not be removed as obsolete language support.

Terry requires **imperative instructions before edits**. Questions such as
"Can you fix it?" are discussion, not authorization. Explain a proposed change
before editing and keep its scope focused. Read-only investigation is allowed.
Commits, pushes, and history rewrites need authorization for the operation; do not
infer them from permission to edit. Terry temporarily allowed amend/force-with-lease
while resolving this particular MSVC issue. That issue is now fixed; this is not
standing authorization to rewrite unrelated or future work. No one else was
collaborating on the branch during this session.

Check CI on demand, never by periodic polling. Do not cancel or rerun jobs,
re-enable excluded lint/CodeQL workflows, or change workflow policies without
instruction. CI and example results for an old SHA do not validate later edits.
Only this handoff document is being updated; leave the usual FMT coordination
files unchanged. On the PC, fetch/read `docs/Coord:cpp20.md`, inspect local history,
and continue source work on `ref/CPP20` when Terry selects the next task.
