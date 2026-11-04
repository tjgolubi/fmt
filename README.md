# FmtColor coordination

Updated November 4, 2026.

FmtColor develops color and text-style extensions for C++. We plan two
sequential deliverables:

1. A focused extension to [{fmt}](https://github.com/fmtlib/fmt), offered
   upstream while preserving its formatting engine and compatibility machinery.
2. An independent styling library built on standard C++23 formatting and
   `std::variant`, without {fmt}'s unrelated extensions or older-compiler compatibility machinery.

The entire {fmt}-based release must support C++11, including every feature
and the styling API in namespace `fmt`; the include path selects the
installation. Shared-source intent covers the common styling API, not arbitrary
fmtlib-specific application code or identical binary representations.

## Documents

| Document                           | Purpose                                                       |
| ---------------------------------- | ------------------------------------------------------------- |
| [Workflow.md](Workflow.md)         | Authorization, Git, integration, and release rules.           |
| [CppDesign.md](CppDesign.md)       | Implemented foundation, future design, and open decisions.    |
| [BuildAndTest.md](BuildAndTest.md) | Build commands, compatibility checks, and current validation. |
| [Website.md](Website.md)           | Website content, publication, and maintenance.                |
| [StyledText.md](StyledText.md)     | Owning styled text, context state, and future work.           |

Maintain these files in the `coord/` Git worktree on `docs/Coord`, separate
from the library checkout. `cd coord` enters the documentation worktree;
`cd ..` returns to the library branch. `/coord/` is excluded locally through
`.git/info/exclude`; this setup is local Git metadata, not a library commit.
The coordination documents live at the root of the documentation branch. Earlier
coordination history is in the
[FmtColorGpt repository](https://github.com/tjgolubi/FmtColorGpt). These are
handoff documents for the next task conversation: current
baseline, accepted behavior, working rules, design vision, and open decisions.
Keep them current rather than appending an event log. Source and tests establish
implemented behavior; newer explicit instructions from Terry override older
wording. Proposals are not implementation authorization. Earlier development
records remain available in Git history.

## Current baseline

The development baseline is `color-dev` at `fecee9480923c787d2ce05cd57424ab767642119`,
amended and force-pushed by Terry to include the refreshed changelog,
integration instructions, and root README after consolidating `color-dev5`. It is one
extension commit directly above upstream/main at `4ed3535092a96ac9f56fab80df83416d51bcc78e`.
The library's tracked tree is clean. Local and GitHub branch SHAs matched during
the website update, and all five inspected workflow runs target that exact SHA.
Use this rewritten `color-dev` for subsequent work; older formatter/parser branch
commits are historical references, not prerequisites to replay.

The consolidated contribution includes color values, fluent styles, SGR helpers,
ostream support, owning styled text, the corrected formatter and context state,
standard SGR parsing, ANSI stripping, the demo, and user documentation.

The November 4, 2026 CI snapshot for the amended commit shows **32 of 41
push-CI jobs passed and all 9 macOS jobs cancelled**. None of the macOS jobs
received a runner or executed steps; the workflow reports failure and macOS
validation remains incomplete. Linux (18), Windows (13), and documentation (1)
passed, as did the separate guide build and deployment. See
[BuildAndTest.md](BuildAndTest.md#current-color-dev-validation).

`FmtColorChangeLog.md` was regenerated with git-cliff 2.14.2, `--offline --no-exec`, and the explicit upstream-to-consolidated-commit range. It now reflects
the single accepted extension commit and breaking-change footer. Do not regenerate
it automatically. No fork release number or tag has been assigned; the contribution
has not been accepted into fmtlib/fmt. Library and coordination commits/pushes
still need explicit authorization.

## Separate upstream comparison fix

The active library checkout is `fix/StringViewCompare`, branched from `main` at
`4ed35350`, committed and pushed at `f9b5ccae427403158e3695a8a94b3d1f1c2180ff`.
It adds only `if (n == 0) return 0;` in `detail::compare` and braces around the
wide-character test's conditional to eliminate a dangling-else warning.
It is separate from the FmtColor contribution and must not appear on the website.
The earlier pointer-equality shortcut broke C++20 constant evaluation and was
removed. Linux, Windows, and doc CI passed for the corrected SHA; macOS has one
passed job and eight queued in the latest inspected snapshot. See BuildAndTest.md.

No upstream PR has been created. The connector rejected creation in `fmtlib/fmt`
with HTTP 403, the GitHub CLI was unavailable, and Firefox automation stopped
because browser URL-policy enforcement was unsupported. A subsequent API check
found no PR for this branch. Terry has a prepared compare link; submission remains
outstanding. Do not mistake the earlier attempt for a published PR.

## Integrated parser and ANSI stripping

`<fmt/parse_styled_text.h>` provides character-generic standard SGR parsing into
owned narrow/wide styled text. Text before the first reset is relative; the first
reset starts one absolute group protecting all subsequent text from inherited
styles. Later resets clear styles within that group. Non-SGR and malformed
escapes stay literal; unknown valid parameters are ignored. Extended push/pop
parsing remains deferred. See [StyledText.md](StyledText.md#sgr-parser).

`<fmt/strip_ansi.h>` removes syntactically complete ESC-prefixed sequences from
strings. Its styled-text overload returns a new tree, preserving every node's
style and structure while stripping each leaf independently. It does not remove
sequences spanning leaves. Use `strip_ansi(x.plain_text())` to scan flattened text
or `strip_ansi(x).plain_text()` to extract text after leaf-local stripping; both
discard explicit styles in the returned string, with different leaf-boundary
behavior. Standalone C0, single-byte C1, and malformed/incomplete sequences remain.
Neither parser nor stripper simulates terminal editing or sanitizes text.

## Implemented styled text and format contexts

The complete release remains C++11-compatible. `<fmt/styled_text.h>` supplies
owning narrow/wide trees, immutable shared leaf strings, value-owned private
children, concatenation, plain-text extraction, root overlays/emphasis helpers,
absolute styles, and nested formatting. `+` and `+=` preserve complete styled
spans; use `overlay(style, expression)` to style a group.

`styled_arg` and `styled_text` now inherit and restore the enclosing formatting
context style. An outer underline therefore survives a red replacement field.
Absolute spans replace inherited state temporarily. Ordinary operations start
at defaults; a relative top-level style does not reset unknown terminal state.
See [StyledText.md](StyledText.md#format-context-style-state).

Narrow/wide fill, alignment, width, precision, optional `s`, and dynamic
specifications are implemented. Width follows ordinary fmt string rules per
leaf; one precision budget spans the tree, and padding uses the effective root
style. Generated SGR adds no width; raw escapes are not interpreted. Keep each
UTF-8 code point within a leaf.

`to_sgr(final, initial)` compares complete terminal states. Compact mode chooses
the shorter selective or reset-and-rebuild sequence, preferring selective on a
tie. Absolute targets explicitly reset in both modes, even from defaults or
equal attribute states. Noncompact relative transitions reset and fully specify
changed targets, omitting a redundant reset from known defaults.
An absolute styled-text node emits an SGR reset unless there is no output;
relative transitions wait until text or padding is emitted.

Run `make -C demo run` for the checked-in header-only C++23 compact-SGR demo.
Debug formatting, deferred blending, extended push/pop parsing, full grapheme/terminal width,
serialization, styled-text module export, and public iteration remain deferred.
Appending `detail::styled_arg` to a tree remains deferred; `fmt::styled` retains
its borrowing return type, with updated formatting behavior.

For the independent C++23 library, omit whole-message
`print(out, text_style, ...)`/`println(out, text_style, ...)` and analogous
style-taking formatting overloads. Use ordinary standard formatting with
`styled` or `styled_text` replacement fields, expressing a whole styled message
as one value/tree. Keep inheritance inside that formatter rather than modifying
`std::basic_format_context`. The fmt extension retains its existing styled-call
overloads, which justify the context integration; upstream may reject that
feature. No standard-library proposal or prototype is authorized by this design
discussion. No performance benchmark establishes overhead claims.

Older strict local Release blend/union warnings remain validation limits;
passing remote CI does not establish that those exact local configurations are
fixed. See BuildAndTest.md. Check CI on demand; do not start periodic polling,
change workflow exclusions, or cancel runs without instruction.

## Upstream compatibility

The separate `test/UpstreamCompatibility` result at `21eba01b` is historical:
it tests an older header on upstream `0fc31633`, not the consolidated `color-dev`
contribution. Its 42 passing jobs are not current `color-dev` compatibility evidence.
The required upstream adjustments and exact scope remain in
[BuildAndTest.md](BuildAndTest.md#upstream-compatibility-branch).
Do not claim that all original upstream tests pass unchanged.

## Public documentation and next work

- [Project website](https://fmtcolor.tjgolubi.chatgpt.site): public version
  **41**, including parser/stripper examples, current reset behavior, retained
  demos/downloads, exact `fecee948` source links, and the incomplete CI snapshot.
- [Generated user guide](https://tjgolubi.github.io/fmt/color/index.html):
  build and deployment passed for `fecee948` from `color-dev`.
- [Issues](https://github.com/tjgolubi/fmt/issues): bugs and follow-up work.

The `color-dev` root README now uses current branch badges and the correct
website URL and lists owning text, context restoration, parser, and stripper.
`FmtColorIntegration.md` includes header selection, current reset behavior,
C++17 application examples, and Make/CMake/header-only integration. The library
minimum remains C++11; the complete working demo remains C++23.

The complete C++11 API includes named RGB constants, terminal/cube/gray colors,
public five-alternative `color_t` access and visitation, comparisons and hashing,
RGB conversion and color/style blending, fluent components and attributes,
style clearing and overlays, conditional emphasis factories, SGR helpers and
compact output, and styled narrow/wide ostream formatting and insertion.
Include `<fmt/color-ostream.h>` for the stream bridge; it includes `color.h`,
`ostream.h`, and `xchar.h` while the original headers remain independent.

Use factories and fluent operations in ordinary examples. Keep `|` for examples
specifically explaining that operator, and preserve the class-comment examples
Terry explicitly accepted. Use function names without `()` in prose, retaining
parentheses for calls, examples, and signatures. Readers are moving directly
from the original upstream header to these APIs; omit intermediate API history
and ABI/rebuild reminders. The ABI change is accepted.

The guide has aligned Markdown tables, links from introductory identifiers to
their sections, and expected displays for the Styled Arguments examples. The nested field
padding/precision example prints `***hello wor`, all bold with `wor` blue. The
stream example uses C++20 `std::numbers::pi` and displays bold-green `3.14`.
The API Reference header list includes `fmt/color-ostream.h`, linking to its
existing guide section; it has no separate new API Reference section.

The owning tree, nested formatter, context inheritance, parser, and stripper are
integrated in the consolidated baseline; see [StyledText.md](StyledText.md). The independent
library requires C++23 and `std::variant`; the fmt release retains C++11.
