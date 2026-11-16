# {color} C++23 handoff

Updated November 15, 2026. This document tracks the standalone {color} library.
The other coordination documents record the original C++11 {fmt} extension and
the published site. The filename predates the decision to target C++23.

## Current baseline

- Source is on `ref/Reorg` in the [FmtColor fork](https://github.com/tjgolubi/fmt).
  The PC checkout and its local `origin/ref/Reorg` tracking ref match
  `db14a2bee42173c1217213f47c3d3b7df6604bfa`.
- The project is called **{color}**. It uses namespace `tjg`, headers under
  `include/tjg/`, and the exported CMake target `tjg::tjg`.
- **C++23 is the supported target.** Consider a C++20 backport if someone asks
  for it. The current library uses standard `<format>`, `<print>`, and
  `std::to_underlying`.
- All 10 GitHub Actions jobs passed on attempt 1 for this commit.
  All 22 local CTest suites passed with MSYS2 GCC 16.2.0 at the earlier
  `601029e69` baseline. See
  [Build, platforms, and verified results](#build-platforms-and-verified-results).
  Tablet results for this commit are unverified.
- Terry plans to start a fresh repository at `tjg/color` with the current
  refactor and archive the fork. See [Repository and website](#repository-and-website).

The PC checkout is `C:/msys64/home/tjgolubi/prj/fmt`, with the `docs/Coord`
worktree under `coord/`. This worktree uses a Git metadata path in MSYS2
syntax. From MSYS2, use `git -C coord` for its Git operations. Under native
Windows Git, use `git --git-dir=.git/worktrees/coord --work-tree=coord` from
the library root to inspect it. Check current worktree state before changing
branches or synchronizing.

## Implemented independent library

- The root CMake project installs the header-only library and a package for
  `find_package(tjg CONFIG REQUIRED)`. The installed-package test builds a
  consumer after relocation.
- The original `include/fmt/` headers are gone from this branch. The
  standalone regressions live in `test/` and use the GoogleTest sources in
  `ext/gtest/`. The library needs only the standard library. Two comparison
  demos use an external {fmt} build.
- The temporary split headers include `colors.hpp`, `text_style.hpp`,
  `text_style-format.hpp`, `styled_text.hpp`, `styled_text-format.hpp`, and
  `color-ostream.hpp`. The guide source is in `doc/`, and the root changelog
  is `CHANGELOG.md`.
- Owning styled text has separate leaf and child members. An empty child
  vector denotes a leaf. Formatting lives in the separate formatter header,
  and streams share its renderer. The parser and ANSI stripper have their own
  headers.
- Ordinary tree formatting supports narrow and wide output, padding, and
  static or dynamic width and precision. Debug tree output uses `?` and
  rejects width and precision. An absolute group emits its transition before
  visiting children, so a very small precision can leave SGR with no visible
  text. Terry accepted that result.
- Width and precision are stored as `std::size_t`. The format-spec parser
  and dynamic argument visitor use checked `int` values for format numbers
  and IDs. Use signed types where subtraction makes sense and keep
  `std::size_t` where the existing size convention applies.
- Styled ostream insertion uses a sentry and renders directly to an
  `std::ostreambuf_iterator`. It handles width, fill, and alignment and
  consumes the pending width. A write failure sets `badbit`. The exception
  regression checks the stream's mask, preservation of the original write
  exception, and restoration of the tracked style.
- Styled ostream printing follows the active standard library's
  `std::print` behavior for pending width, padding, and width consumption.
  Regression tests compare output and stream width against `std::print`,
  including left and right alignment with an empty style.
- `inline_buffer.hpp` provides fixed-capacity storage for bounded output.
  Runtime construction starts empty without filling the array. Copy and
  assignment copy only the populated characters. It supports a fill value for
  `resize` and a `resize_and_overwrite` operation. The former
  `memory_buffer` is gone.
- `unicode.hpp` consumes the generated `UnicodeData.hpp` and
  `EastAsianWidth.hpp` tables. The C++23 generators and Unicode 18.0.0 inputs
  live in `support/unicode/` and run separately from the normal build.
  Preserve the generated headers as generator output.

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
the components and still work. Equality and three-way comparison are defaulted. Color bitwise operators
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
- `<tjg/hash_support.hpp>` provides `tjg::hash_combine(seed, value)`, adapted from
  Boost.ContainerHash 1.85.0 using `std::hash<T>` and Boost's 32/64-bit mixers.
  It propagates exceptions from a throwing hasher and is otherwise conditionally
  noexcept. The Boost attribution and `tjg/LICENSE-Boost` are included; CMake
  installs the license. Tests check both mixers, combining vectors, exception
  propagation, and use of colors/styles in unordered containers.

### Compiler compatibility cleanup

The earlier pre-C++20 cleanup removed the pre-C++17 reset storage definition,
unused C++11 blend helpers, the legacy MSVC stream alignment branch, and test
gates for three-way comparison, concepts, and constexpr strings. Keep the
targeted MSVC workaround below and the supported no-exceptions path.

Earlier MSVC fixes renamed the buffer member `_inline` to `_inline_storage`
to avoid a compiler extension, used `'\xff'` and `Char{255}` for narrow and
wide backslash tests respectively, and removed unreachable code from the
deliberately throwing test formatter.

The indexed cube constructor explicitly converts validated channel values to
`chan_type` for MSVC. To address GCC Release diagnostics, blending zero
specified colors returns `no_color` directly. Styled-text constructors take
the supplied style by `const text_style&`, and unspecified `text_style` color
parameters default-initialize their `color_t` variants. Format parsing omits the
unreachable return after reporting invalid fill, and the formatter exception
tests keep both paths reachable to the compiler.

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

### Completed cleanup and retained interfaces

The refactor removed the legacy noncompact SGR option, compatibility aliases,
and C++11-only template scaffolding. Style ordering is defaulted and follows
member order: absolute, emphasis, foreground, background. Retain the targeted
MSVC workaround and supported no-exceptions path.

For color blending, preserve equal weighting and ignore unspecified colors.
Return `no_color` when none are specified, preserve the kind and value of a
sole specified color, and average two or more as RGB. Color bitwise
composition remains available.

Retain both styling interfaces. Style-taking `format`, `format_to`,
`print`, and `println` overloads style the entire result, including format
string literals. The style follows the output destination where applicable.
`styled(value, text_style)` styles an individual replacement argument.

### Delegate to <format> and <print>

Use existing standard facilities wherever they simplify the implementation.
Prefer standard formatters and standard specification parsing for ordinary values
and strings rather than recreating their behavior. Keep custom machinery for the
actual styling and styled-tree work. Standard C++ exposes no general public
Unicode display-width or grapheme-boundary helper, so some supporting code may
still be necessary for trees spanning multiple text leaves.

The whole-result wrappers delegate to the standard library. A writer-backed
`sgr_scope` saves the tracked style, emits a transition, and restores it at
scope exit. Nested styled arguments inherit the current style. Absolute
styles rebuild terminal attributes explicitly. During unwinding, closing SGR
is best effort and the scope restores logical state.

The tracked style is a shared function-local static returned by
`tjg::detail::get_style_state`. The outermost color-aware call assumes
terminal defaults. Callers that emit their own SGR through a stream manage
their terminal state and should restore defaults before a color-aware
print or format call.

Relevant standard facilities are `std::format`, `std::format_to`, `std::vformat`,
and `std::vformat_to` in `<format>`, and `std::print`, `std::println`,
`std::vprint_unicode`, and `std::vprint_nonunicode` in `<print>`. There is no
function named exactly `std::vprint`. Choose delegation that preserves standard
encoding and output behavior; the name similarity alone is insufficient. Keep
our applicable ostream conveniences as well.

The direct print and ostream paths can write incrementally.
String-returning formatting uses `std::basic_string`. Threading and
synchronization are outside the current design.

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

### Attribution

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

`<tjg/error_support.hpp>` supplies six configurable macros:

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
Use `std::get`, `std::get_if`, `std::holds_alternative`, and `std::visit` for
access. With exceptions enabled, `std::get` on the wrong alternative throws
`std::bad_variant_access`.

MSVC rejects valid constexpr transitions back to an empty `no_color_t` alternative
with C2131, reporting inactive union-member access inside its `std::variant`.
This failed at compile time in all four Windows jobs for `2d185922f`.
The [Debug compact job](https://github.com/tjgolubi/fmt/actions/runs/42466322805/job/128686579461)
identified `check_color_constexpr_assignment`. Linux, macOS, and the guide
passed at that SHA.
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

Use this order when adding or touching includes. `.clang-format` has
explicit include categories and sort priorities for the project and standard
headers, and regroups their includes accordingly. Extend those categories when
new headers require deliberate ordering. The earlier Clang-format 21.1.8 check
verified this ordering.

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

Run `./build.bash` from the repository root. It configures a C++23 Debug
build with `g++`, enables `TJG_TEST` and `TJG_WERROR`, builds in
`build`, and runs CTest. For Clang, configure a separate build
directory and select `clang++` with `CMAKE_CXX_COMPILER`. Visual Studio
multi-configuration builds also need a configuration for build and CTest.

All 10 GitHub Actions jobs passed on attempt 1 for the November 15 push of
`db14a2bee42173c1217213f47c3d3b7df6604bfa`. All four run head SHAs match.

| Workflow                                                                           | Result   | Coverage                                          |
| ---------------------------------------------------------------------------------- | -------- | ------------------------------------------------- |
| [TJG Linux](https://github.com/tjgolubi/fmt/actions/runs/43794522270/attempts/1)   | 5 passed | GCC 14 and Clang 20 Debug/Release, GCC ASan/UBSan |
| [TJG macOS](https://github.com/tjgolubi/fmt/actions/runs/43794522246/attempts/1)   | 2 passed | LLVM 20 libc++ Debug/Release                      |
| [TJG Windows](https://github.com/tjgolubi/fmt/actions/runs/43794522262/attempts/1) | 2 passed | MSVC v143 x64 Debug/Release                       |
| [Color guide](https://github.com/tjgolubi/fmt/actions/runs/43794522268/attempts/1) | 1 passed | MkDocs build with pinned dependencies             |

The guide's C++ handler creates `build/doxyxml` before invoking Doxygen.
The Pages deployment workflow still targets `color-dev` in `tjgolubi/fmt`.
Review its routing as part of the move to the new repository.

All 22 local suites, including the re-enabled styled-text checks,
no-exceptions coverage, and package relocation, passed on November 15
with MSYS2 GCC 16.2.0 at `601029e69`.

The older GCC 14.2 and Clang 21.1.8 runs at `f93e4159` passed 22 suites
in both former SGR modes. The historical cross-platform results below
cover their recorded commit.

On the earlier PC baseline `2d185922f`, MSYS2 GCC 16.2 passed all 17 suites in
each SGR mode and the guide built locally. The installed-package test consumer
defines `_DEFAULT_SOURCE` only when built with GCC on MSYS2, so strict C++23
mode exposes the stdio declarations required by libstdc++ `<print>`. The fix was
present during local testing and was committed in `b6fc79da4` after `fd8dfa256`
omitted it.

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

## Demos and comparison material

- `FmtColorDemo1.cpp` uses whole-result printing with styled arguments and
  the same surrounding white foreground, gray background, and strikethrough
  as `FmtColorDemo2.cpp`.
- `FmtColorDemo2.cpp` demonstrates underlay and overlay. `TableDemo.cpp`
  demonstrates styled columns, Unicode width, and truncation.
  `OstreamDemo.cpp` uses the stream API for the same dashboard.
- `Cpp23Demo.cpp` uses standard formatting for plain-text layout and emits
  SGR explicitly. `FmtColorDemo0.cpp` uses upstream {fmt} for the same table.
- `NestedDemo.cpp` shows two plain C++23 examples, the direct {fmt} version
  that loses the underline after the red argument, the {fmt} version that
  reapplies the underline, and the {color} version that restores it.
  `demo/NestedDemo.png` captures all five outputs. The user guide uses a
  separate terminal screenshot for its TableDemo walkthrough.
- `make -C demo -B run` rebuilds and runs all seven demos. The earlier PC run
  at `601029e69` covered the six demos configured at that commit.
  `FmtColorDemo0` and `NestedDemo` use `FMT_ROOT` and `FMT_LIB`, which
  default to `$HOME/App/fmt` and its `build/libfmtd.a`.

## Repository and website

Terry plans to create `tjg/color` from a fresh snapshot of the refactor.
The current root README already introduces the standalone library. When the
new repository is available, check its package, CI, guide publication, and
source links. Keep the original extension's source available in the fork
archive with links pinned to its contribution.

Keep the existing public OpenAI site. Lead with the standalone C++23 library
and retain the original {fmt} extension on its contribution page. Move the
independent library out of Future work and replace the proposed sample there
with current, tested examples. Put the nested-style comparison and the table
comparison on the project site. The generated user guide explains library use.
Keep the OpenAI site source in its separate checkout. See
[Website.md](Website.md) for the existing publication and update procedure.

## Working rules

Terry requires imperative instructions before edits. Check the current source
and tests when older coordination notes disagree with the implementation.
Commits, pushes, history rewrites, repository creation, and site publication
require authorization for the operation. Check CI on demand and report
validation against the commit actually tested.
