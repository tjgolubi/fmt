# FmtColor coordination

Updated November 3, 2026.

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

The accepted development baseline is `color-dev` at
`2fa0bd9269a54d1fe155ff7ee95e96dfb9d0b6c3`, committed and pushed. Local, remote,
workflow, and job full SHAs matched. It includes the owning styled-text and
formatter squash at `1e8fc387` and the ColorCtx squash at `b8bf6293`, above
the earlier styling contribution and publication configuration. The current tip
also includes the modernized demo and its `absolute` helper cleanup, plus the
revised guide examples and expected displays. Upstream
baseline/local `main` remains `f87a75e15262614ce8aa0e6a0664df9d0823fb92`.
Use `color-dev` for new independent feature baselines, not the integrated
`feat/Formatter` or `feat/ColorCtx` branches.

All **41 push-CI jobs passed** for this final SHA: Linux 18, Windows 13,
macOS 9, and documentation build 1. The separate guide-publication build and
deployment also passed (43 successful jobs in total). Every job was inspected;
none failed, were cancelled, or remained pending. The guide is published from
`color-dev`. See [BuildAndTest.md](BuildAndTest.md#current-color-dev-validation).

`FmtColorChangeLog.md` is tracked again, regenerated at Terry's request with
git-cliff 2.14.2, `--offline --no-exec`, and the explicit upstream-to-HEAD
accepted-history range. It includes both integrated features and their breaking
changes. Do not regenerate it automatically. No fork release number or tag has
been assigned, and the contribution has not been accepted into fmtlib/fmt.

The tracked library tree is clean. The modernized `demo/FmtColorDemo2.cpp` is
committed and pushed; the earlier deferred `.tjg.cpp` file is no longer present.
Coordination documents are maintained and pushed separately on `docs/Coord`.
Future commits and pushes still require explicit authorization.

## Active parser work

Active library checkout: `feat/parser`, based on accepted `color-dev` at
`2fa0bd92`, committed and pushed at
`1cda652f459204dd552d1d06660f3c4f128b156d`. The library tree is clean. The
parser is not squash-integrated into `color-dev`; acceptance is still pending.

`<fmt/parse_styled_text.h>` adds character-generic `parse_styled_text`: standard
SGR becomes owned narrow/wide spans with an absolute root. Non-SGR and malformed
escapes stay literal; unknown valid parameters are ignored. It does not implement
push/pop extensions. See [StyledText.md](StyledText.md#sgr-parser).

Local tests pass in C++11/C++23 and both SGR modes. Latest observed push CI:
Linux 18/18, Windows 13/13, doc 1/1 passed; macOS has 5 passed, 3 queued, and
1 cancelled. No failures were reported, but this is not a complete green matrix.
Terry reports repeated macOS runner cancellations today; their cause is unknown.
Do not attribute them to parser code or claim acceptance. See BuildAndTest.md.

A separate `strip_ansi` function was discussed, not implemented: strip syntactically
valid ESC-prefixed sequences, including private/unknown codes; preserve standalone
C0 controls, single-byte C1 forms, and malformed/incomplete escapes. The parser's
`plain_text()` removes decoded SGR only and preserves other escapes.

Terry authorized committing and pushing this coordination update on `docs/Coord`.
The published website remains version 39 and predates this parser work.

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
tie; absolute targets explicitly reset. Noncompact mode resets and fully
specifies changed targets, omitting a redundant reset from known defaults.
One-argument `to_sgr` still emits an explicit absolute reset.

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
  **39**, presenting context inheritance/restoration, the complete modernized
  demo and display on the overview, revised precision/pi examples, and exact
  green CI links for `2fa0bd92`.
- [Generated user guide](https://tjgolubi.github.io/fmt/color/index.html):
  build and deployment verified for `2fa0bd92` from `color-dev`.
- [Issues](https://github.com/tjgolubi/fmt/issues): bugs and follow-up work.

The root library README still has obsolete `FmtColorDev` branch text/badges
and the previous website URL. Its changelog link now points to a tracked file;
do not describe that file as deleted. README cleanup remains separate.

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

The owning tree, nested formatter, and context inheritance are integrated on
`color-dev` at `b8bf6293`; see [StyledText.md](StyledText.md). The independent
library requires C++23 and `std::variant`; the fmt release retains C++11.
