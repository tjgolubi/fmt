# {color}: handoff to the next work session

Updated November 10, 2026. Read this document first when continuing the independent
{color} library, probably on Terry's PC. The other coordination documents describe
the separate FMT-based development line; do not apply their C++11 requirement to
this branch or update them as part of this handoff.

## Current baseline and checkout state

- Repository: https://github.com/tjgolubi/fmt
- Development branch: `ref/CPP20`, originally split from `color-dev`.
- Latest committed and pushed source: `f93e4159548809d0017d0d32403fa6b95312f9dc`.
- GitHub's branch tip was verified to match the cloud checkout at that commit.
- The project is called **{color}**. Namespace `tjg`, headers under `include/tjg/`,
  and CMake target `tjg::tjg` retain their existing names.
- **C++23 is the current target.** C++20 support will be added later through
  separate patches; it is not a current compatibility requirement.
- All 22 local test suites pass on this source with GCC 14.2 and Clang 21.1.8,
  in both compact and noncompact SGR modes (C++23 Debug, warnings as errors,
  CMake 4.4.5).
- CI and tablet results for this latest SHA have not been checked in this
  session. The successful CI/tablet results recorded below concern the earlier
  `7253096c` baseline, not the current source.

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
`text_support.h`, `tree_format_specs.h`, `format_support.h`, `error_support.h`,
and `hash_support.h`.

The own memory buffer replaces FMT buffers. UTF-8 decoding, width estimates,
truncation, and escaping live in `text_support.h`, with attribution. Audit these
against the standard formatting rules described below; current width handling
must not be presumed to implement full grapheme segmentation or the newly
clarified policy already.

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

## Code changes completed since the previous handoff

All three integration commits below are committed and pushed to `ref/CPP20`:

| Commit | Change |
| ------ | ------ |
| `66d9775b` | Squash integration of `ref/MemBuf`: user-reworked buffer, reserve/resize, inline default 256. |
| `86124394` | Squash integration of `ref/TextStyle`: separate style components and removal of packed style APIs. |
| `f93e4159` | Remove custom underlying conversions; add palette index members and Boost-derived hashing. |

The feature branches originated at `7253096c`. `ref/MemBuf` contains original
commit `51c7fb5f` and was pushed. `ref/TextStyle` contains original commit
`7cffae7e`; that feature branch was committed locally, with its changes subsequently
pushed through the squash integration on `ref/CPP20`.

### Buffer

`basic_memory_buffer<Char, InitSize = 256>` now has `reserve(size)` and
`resize(size)`; growth through resize does not initialize added characters.
The private growth helper is constexpr and uses `TJG_NOEXCEPT_UNLESS_BAD_ALLOC`.
The attached user revision had a missing helper argument and a missing constexpr
qualification; these were corrected. Appended source ranges must not refer to the
buffer's own storage. Self-append support was deliberately not added, and no
aliasing precondition check was added. Tests now use external source ranges and
cover reserve/resize and constant evaluation.

### Style representation

`text_style` stores exactly these private components:

```cpp
bool _absolute = false;
emphasis _emph{};
color_t _fg;
color_t _bg;
```

The fluent interface remains. Style `operator|` and `operator|=` are removed;
use overlay/underlay or blend as appropriate. There is no packed-integer style
constructor, `to_underlying(text_style)`, `text_style::value_type`, custom
underlying-type specialization, or serialization API. Diagnostic formatters use
the components and still work. Equality is defaulted; ordering currently preserves
the old packed order, which is a cleanup candidate below. Color bitwise operators
remain deliberately available; removing style operators did not remove color OR.

### Numeric access and hashing

- All `tjg::to_underlying()` overloads and `tjg::underlying_type` machinery are
  removed, including the numeric conversion for `no_color_t`.
- `emphasis` and `term_color` use C++23 `std::to_underlying()`.
- `gray_color::index() const` and `cube_color::index() const` return `int`,
  constexpr and noexcept. They return relative palette indices: 0..23 and
  0..215. SGR palette entries are respectively index + 232 and index + 16.
  Their single-index constructors also accept these relative indices.
- `rgb` retains its explicit `uint32_t` conversion and explicit packed-integer
  constructor, using numeric `0x00RRGGBB` (RGB888); the constructor ignores the
  high byte. No `to_packed()` or `pack()` API is needed. Numeric layout does not
  prescribe memory byte order.
- Packed `color_t` conversion, `color_t_value_type`, payload packing, and the
  internal unpack helper are removed. Its hasher combines the alternative index
  with the active value; palette alternatives use their indices. Styles hash
  their components. No packed variant representation or serialization is offered.
- `<tjg/hash_support.h>` provides `tjg::hash_combine(seed, value)`, adapted from
  Boost.ContainerHash 1.85.0 using `std::hash<T>` and Boost's 32/64-bit mixers.
  It propagates exceptions from a throwing hasher and is otherwise conditionally
  noexcept. The Boost attribution and `tjg/LICENSE-Boost` are included; CMake
  installs the license. Tests check both mixers, combining vectors, exception
  propagation, and use of colors/styles in unordered containers.
- `emphasis::none = 0` was discussed but has not been added.

## Design policy going forward

**Start from standard C++ formatting and printing, extended with explicit
styling. Do not treat today's implementation or {fmt} compatibility as the design
we must preserve.** Existing machinery can remain temporarily during a gradual
migration; its presence is not an argument for retaining it permanently.

{color} is unreleased and has never had external users. There are no compatibility
obligations or "breaking change" concerns. Terry intends to start a new Git
repository without this history when the port is complete. Prior commit messages
containing `!` or `BREAKING CHANGE` record the old framing; do not keep using that
framing or retain aliases merely to protect nonexistent users. This does not
itself authorize creating the new repository or rewriting current history.

### Four agreed cleanup candidates

These are agreed directions, not completed changes or authorization to implement
all of them as part of this documentation update:

1. **Remove legacy SGR output.** Standardize on compact output and remove the
   noncompact path, zero-padded RGB fields, and the dual-mode option/test burden.
   Both modes still exist today and must be tested until this cleanup is done.
2. **Remove compatibility aliases.** Candidates are `terminal_color` for
   `term_color`; `strikethrough` for `strike` (enum and style setter/query);
   `get_foreground/get_background/get_emphasis` for `fg/bg/emph`; and
   `has_foreground/has_background/has_emphasis` for `has_fg/has_bg/has_emph`.
   Update uses, tests, and guide together; no deprecation period is needed.
3. **Simplify style ordering.** The manual comparison preserves the former
   packed ordering: absolute, emphasis, background, foreground. It can be
   defaulted to follow member order: absolute, emphasis, foreground, background.
4. **Remove old template scaffolding.** Use C++23 concepts, modern traits, and
   fold expressions instead of recursive C++11-era traits. `all_colors` and
   `all_text_styles` appear unused. Review uses before deleting helpers. Genuine
   MSVC compiler-bug and exception-support gates remain intentional.

The fifth suggested candidate, removal of whole-result formatting entry points,
was rejected. **Retain both kinds of interface:**

- `format(text_style, format_string, ...)`, `format_to`, `print`, `println`,
  and applicable runtime-argument and stream overloads style the entire result,
  including literal text in the format string. For printing, the style is before
  the format string (and after the output destination where applicable).
- `styled(value, text_style)` styles an individual argument inside that result.

These serve different purposes. Neither replaces the other. Retain bitwise color
composition, which Terry explicitly allowed; blend remains available for averaging.

### Delegate to <format> and <print>

Use existing standard facilities wherever they simplify the implementation.
Prefer standard formatters and standard specification parsing for ordinary values
and strings rather than recreating their behavior. Keep custom machinery for the
actual styling and styled-tree work. Standard C++ exposes no general public
Unicode display-width or grapheme-boundary helper, so some supporting code may
still be necessary for trees spanning multiple text leaves.

The intended whole-result operation is: save our tracked style, emit the SGR
transition to the requested style, call the corresponding standard formatting or
printing operation, then emit restoration and restore tracked state before exit.
Handle exception paths so tracked state is restored; design output restoration
carefully during unwinding. The current whole-result functions generally append
a reset to terminal defaults, so they have not yet implemented this target model.
Track only the state managed by our library; do not assume it queries the terminal.

Relevant standard facilities are `std::format`, `std::format_to`, `std::vformat`,
and `std::vformat_to` in `<format>`, and `std::print`, `std::println`,
`std::vprint_unicode`, and `std::vprint_nonunicode` in `<print>`. There is no
function named exactly `std::vprint`. Choose delegation that preserves standard
encoding and output behavior; the name similarity alone is insufficient. Keep
our applicable ostream conveniences as well.

Printing need not first buffer the entire formatted result merely because the
current implementation does so. String-returning formatting can use
`std::basic_string`. **Threading is explicitly out of scope.** Do not repeatedly
raise interleaving/synchronization objections or add threading machinery.

### Width, precision, encoding, and embedded escapes

Follow the principle of least surprise for `std::format` users: match its encoding,
estimated-width, alignment/padding, and string-precision semantics. String width
is a minimum for padding; string precision limits estimated width for truncation.
Follow the standard's whole-extended-grapheme-cluster prefix rules, not byte-based
truncation or an independently chosen terminal-width convention. Where the
standard permits implementation choices, prefer behavior consistent with the
active standard library. Audit the current code; this policy is not a claim that
its existing decoder/tables/truncation already conform.

`char` does not imply ASCII or one column per byte. Ordinary literal encoding can
be UTF-8 (as on Termux). `char8_t` is a distinct type for UTF-8 code units; a
`std::string` can also contain UTF-8, but the type does not itself promise that
encoding. Neither string type guarantees valid contents. Avoid an unconditional
UTF-8 assumption for all `char` text or an unconditional one-column-per-char rule.
Share decoded-text width logic where possible and choose decoding according to
standard formatting semantics. Standard formatting does not directly accept
`char8_t` format strings or `std::u8string` arguments; do not claim such integration
or new styled-tree specializations already exist.

**Neither styled text nor our formatting/printing functions should parse or give
special width treatment to embedded SGR/ANSI escapes.** Embedded escape bytes are
ordinary text for formatting purposes. Tree style metadata contributes no text
width; rendering generates the corresponding SGR output. This is distinct from
examining escape sequences inside leaf strings. We are extending standard
formatting, not replacing its treatment of strings.

SGR parsing and ANSI stripping remain separate, explicitly invoked tools. Their
availability does not change the semantics of ordinary styled text, formatting,
or printing. Keep string truncation as a feature, aligned with standard precision.

C++26 `std::text_encoding` identifies encodings; it does not provide a general
width, grapheme, normalization, or decoding/transcoding facility. Its existence
is not a reason to claim the standard has supplied these missing helpers. The
project still targets C++23.

### Buffer direction and attribution

Keep `memory_buffer` for now while migrating gradually. It currently buffers
whole formatted results in format/print/ostream paths and some styled-tree debug
output, as well as SGR. The SGR bound is 60 code units (under 64 bytes for `char`),
not a bound on complete formatted output. Use fixed-capacity storage for bounded
SGR generation when appropriate; after delegation and replacement of other uses,
the growable buffer may disappear entirely. Do not rename it or add a new buffer
framework merely because naming was discussed.

If a general growable inline buffer remains useful, Terry favors capacity before
element type, e.g. `inline_buffer<N = 256, T = char>`. He predominantly uses `char`,
often chooses larger inline capacities for I/O, and considers 256 suitable here.
This is distinct from his fixed-capacity `static_string`, which never grows onto
the heap. C++26 `std::inplace_vector<T, N>` is fixed capacity, not an inline buffer
with heap overflow. These are design discussions, not completed API changes.

Credit {fmt} for the origins of `text_style` and `styled()`. Reference the
[CSS named-color table](https://www.w3.org/TR/css-color-4/#named-colors) for the
CSS-named RGB constants. The fixed terminal-palette RGB equivalents are separately
documented as the agreed Xterm-default mapping. Our styled overloads extending
standard `<format>` and `<print>` need no separate design attribution to {fmt}.
Terry wants unnecessary {fmt} copyright/license boilerplate removed as code is
independently rewritten. Review actual source provenance file by file; common
buffer design or a new name alone does not establish that all derived code can
lose its required notices. Retain the Boost attribution/license for the lifted
hashing code. Do not remove required notices in a blanket cleanup.

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
passed on MSVC 19.44 for historical baseline `7253096c`. No claim is made about
a future compiler fixing the issue; review the gate when testing later versions. Object
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
build and CTest. Local Linux now has 22 suites including the new hash-support
suite, policy tests, installation/relocation, and no-exceptions coverage; platform
counts can differ. All 22 passed on `f93e4159`'s source using GCC 14.2 and Clang
21.1.8, both SGR modes, C++23 Debug, warnings as errors, and CMake 4.4.5. The final
Clang compact-mode retry passed after an initial compiler crash while reading a
header; the file was intact. The finalized builds passed without new warnings.
The documentation source was updated, but MkDocs was unavailable and the guide
site was not rebuilt locally for these changes.

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

Historical baseline CI for `7253096c`, verified November 8, 2026, all on attempt 1:

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
failed, cancelled, or pending applicable jobs in those attempts. These are not
CI results for the three later integration/refactoring commits.

## Next session and authorization

Source is committed and pushed through `f93e4159`. The Termux and MSVC fixes
remain in place. The agreed cleanup and standard-delegation directions above
are the next-work policy; no implementation changes for those directions were
made in this documentation task. Terry's current imperative authorizes updating
this handoff, not implementing every discussed change. The targeted MSVC bug
workaround and exception-support gates remain intentional.

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
