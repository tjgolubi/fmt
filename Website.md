# Website

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Build and test](BuildAndTest.md)

Updated October 26, 2026.

## Purpose and content

The public [FmtColor website](https://fmtcolor.ardentokapi66.chatgpt.site)
explains the contribution, design rationale, demonstration, and future work.
Its audience, in priority order, is Terry and collaborating assistants, the
{fmt} developers, technical friends, and nontechnical family. Use brief
plain-language introductions followed by useful technical detail.

The site is not a progress blog. Present the current contribution as a coherent
change, without narrating development struggles, intermediate commits, or
superseded fixes. Explain motivation, behavior, examples, tradeoffs,
compatibility, and validation. Keep implemented APIs separate from proposals,
and fork integration separate from upstream acceptance.

The current structure is:

- [Overview](https://fmtcolor.ardentokapi66.chatgpt.site/): features, two
  deliverables, a prominent upstream-compatibility callout, design, and demo.
- [The contribution](https://fmtcolor.ardentokapi66.chatgpt.site/integrated-changes.html):
  upstream compatibility evidence first, followed by implemented styling,
  palette colors, conversion, blending, styled ostream output, compatibility
  changes, and final CI.
- [Future work](https://fmtcolor.ardentokapi66.chatgpt.site/work-in-progress.html):
  the retained introduction/development summary, then `feat/StyledText` and
  inclusion policy. Repeated completed-feature sections are removed.

Fluent, ShortNames, VarColor, Palette, RGB conversion, color/style blending,
style clearing, and styled ostream output are integrated on `FmtColorDev` at
`257cdac1`. The branch contains six feature commits and a publication fix above
upstream `0fc31633`.
All 43 final integration CI jobs passed, including guide deployment. The
separate `test/UpstreamCompatibility` commit `21eba01b` passed 42 jobs using
upstream test infrastructure and the documented adjustments. Neither result
means the contribution has been accepted into fmtlib/fmt.

Keep upstream compatibility prominent and link its exact report/diff/CI.
State the six result-expectation changes, four explicit nodiscard discards,
and two documentation-signature changes; do not claim unchanged-test success.
These reflect intended features. Preserve the distinction between source-test
compatibility and numeric/binary compatibility or measured performance.

Branch inclusion needs Terry's approval; an experimental/play branch such as
Crazy does not belong merely because it exists. Describing a proposal does not
authorize implementation. See [CppDesign.md](CppDesign.md) for current behavior.

## User guide

The Sites website explains rationale; the library's generated
[user guide](https://tjgolubi.github.io/fmt/color/index.html) explains use.
Do not maintain a competing guide in Sites or put the Sites implementation into
the library repository.

Documentation buttons and user-guide links must point to the **generated HTML**,
not GitHub's raw/rendered Markdown. The source contains `:::` MkDocs reference
directives and `!!! note` admonitions that GitHub Markdown does not process.
Source links remain appropriate when explicitly labeled as source.

The guide covers colors, emphasis, text styles, styled arguments, formatting,
printing, styled ostream output, palette conversion/blending, empty/clear style
operations, and a separate named-color swatch page. Doxygen comments supply shared
reference text; Markdown provides organization and examples. Keep RGB concepts
before named RGB constants and numeric representation details after ordinary
usage. Keep emphasis discussion on its own page. Normal enum-underlying
facilities do not need generic explanation in this guide.

Keep component accessors, presence queries, attribute queries, and attribute
setters grouped on the Text styles page. Use `fg()`, `bg()`, `emph()`,
`has_fg()`, `has_bg()`, and `has_emph()` throughout examples and prose. Mention
the six older accessor names only in one backward-compatibility note; omit
their separate generated entries with the reference handler's `exclude_members`
option. Keep Doxygen comments on public members, including compatibility
wrappers; undocumented members are omitted by the handler. Use `term_color`,
`emphasis::strike`, `strike()`, and `is_strike()` in examples. The former strike
methods are also excluded from generated member entries and explained in a
compatibility note.

Use `!!! note "Changed behavior"` boxes for changed behavior. Describe the
change without telling users how to fix their code. The MkDocs configuration
enables admonitions; judge the generated HTML rather than source Markdown.

`.github/workflows/fmtcolor-pages.yml` publishes after pushes to `FmtColorDev`.
`support/mkdocs-pages.yml` supplies the fork URL/edit links and disables the
version selector. The `github-pages` environment now permits `FmtColorDev`;
existing `FmtColor` and `main` rules were retained. It uses GitHub Actions as the
Pages source and uploads generated HTML as an artifact, not committed output.
The ordinary `doc` workflow builds but does not publish this guide.
See `support/fmtcolor-pages.md` and [BuildAndTest.md](BuildAndTest.md#documentation).

Guide build and deployment succeeded for `257cdac1`. The guide includes
five-alternative public variants, `to_rgb()`, color/style blending, `empty()`
and `clear()`, and the grouped Fluent API. Constructors and fluent operations
precede the less desirable OR examples; the scoped `using enum` style example
is labeled C++20 and later. Free fg/bg docs explain ADL convenience.

The Styled output page has a
[styled ostream section](https://tjgolubi.github.io/fmt/color/print.html#styled-ostream-output)
with `<fmt/color-ostream.h>`, a usage example, reset/newline behavior, and API
references for `print`, `println`, `vprint`, and `vprintln` taking
`std::ostream&`. The new header is included in the Doxygen reference handler.
Keep the website explanation linked to this generated guide.

Pending TaskWarrior features are not part of this publication: compact SGR,
`to_sgr`, `sgr_reset`, `wrap_sgr`, overlays, and free attribute functions remain
on that branch. Their local guide changes must not be described as deployed
from the accepted `FmtColorDev` baseline.

The Colors page orders terminal, RGB, gray, cube, unspecified, then variants.
Terminal tables include RGB equivalents and link to swatches, as do gray/cube
sections. Named Colors separates normal and bright terminal grids, includes
24 gray levels, and shows all 216 cube colors with red rows, green columns,
and six blue digits padded to three monospace characters per cell. The
Overview compares blend and OR using red/bright-red against blue/bright-blue,
with aligned columns and colored input labels. Blend immediately follows
Palette on the contribution website, before comparisons.

Preserve the swatch orientation, padding, contrast, responsive grids, and
the fixed-Xterm-theme explanation. The VPS remains an unused alternative host.

## Demonstration and future design

Keep the downloadable `FmtColorDemo.cpp` and `AnsiRenderDemo.txt`, the displayed
sample, and its styled output together. The demo is a **future acceptance
target**, not implemented/tested functionality of the current contribution.
Preserve nested emphasis/colors, absolute underline, the enclosing background,
and `gray_color{12}`. The HTML output reproduces Terry's older AnsiRender output
with a stated representative terminal palette; exact SGR bytes need not match.

The sample targets the later standard-formatting deliverable using
`std::println`, retaining `<fmt/color.h>` and namespace `fmt` through the selected
installation. Both the displayed and downloadable samples use `term_color`
and `strike()`, retaining `no_color` and
`emphasis::reverse`. The sample shows color-first, background-only,
emphasis-only, emphasis-first, and default-plus-fluent construction. Keep
its downloadable and displayed C++ identical. Shared-source compatibility
also requires standard formatter
support in the {fmt}-based implementation. Do not imply that retaining the
header name introduces a dependency on fmtlib in the independent library.

Preserve these design distinctions:

- Color/style OR promotes overlapping specified colors to RGB and combines
  channel bits. Free `blend()` averages specified channels, independently for
  style fg/bg, and ORs emphasis. Unspecified inputs are ignored; one specified
  color retains its kind. Strict rejection is superseded, and `rgb::blend()`
  is removed.
- Constructors/fluent setters choose or replace state. Sequential ANSI SGR
  commands update terminal state; they are distinct from value composition.
- Bold/faint are independent. {fmt} retains conceal and omits flash; the
  independent library replaces conceal with flash.
- Absolute boundaries reset terminal state and block enclosing blending in the
  planned StyledText renderer. Lazy blending, subtree lifetimes, parsing,
  width/precision, and serialization remain design work.

Use [CppDesign.md](CppDesign.md) for the complete design and open decisions.
A documentation or website request does not authorize implementing proposals.

## Sites maintenance and publication

Only update the website when requested. An authorized website update includes
the source edits and normal publication; Terry need not manage its checkout or
build tools. It does not authorize library/coordination commits or pushes.
Follow the currently installed Sites skill and native tool access checks.

1. Identify the existing site by its URL and retained project ID. Read its
   latest metadata, access, and shared source before editing. Do not overwrite
   from an old checkout, downloaded page, captured Library copy, or chat summary,
   and do not create a replacement site.
2. Preserve the site identity, public audience, design, demonstration, assets,
   and unrelated content. Verify source, commit, and CI claims against GitHub.
   Keep credentials in memory and pass them only through the helper's hidden
   stdin, never shell arguments, repository files, logs, or handoffs.
3. Use the Sites source helper to open/synchronize the checkout, own source
   commit/push, run remaining checks, and package the exact pushed state.
   Validate affected copy, local links/anchors, unique IDs, examples, and layout
   with available tools. State meaningful verification limits honestly.
4. Save the exact source/archive as a Sites version and deploy through native
   Sites tools. Reuse successful source/package/version results after a later
   failure. A source push or archive alone is not publication; report success
   only after deployment is confirmed.
5. Return the affected live links. For a request covering both website and
   coordination, publish first, then record confirmed provenance here. Do not
   claim that local coordination edits have been committed or pushed.

Every Sites deployment URL is production. Preserve public access; do not
change it to deploy. Poll only a nonterminal deployment, stop on success/failure,
and do not resave or redeploy an already successful result unnecessarily.
Avoid concurrent edits; resynchronize after another session's update.

If tools cannot retrieve source, edit, or confirm publication, identify the
blocked step. Do not promise that switching apps or devices guarantees access.
A handoff to Sites in the ChatGPT web interface should include the existing URL,
requested change, and direction to read these documents and retrieve fresh source.
Personal-website links belong to Terry's separate personal-website context.

### Windows publishing

The Sites Node helper needs **native Windows Git** ahead of
`C:\msys64\usr\bin` on its process-local PATH. This is an exception for the
separate Sites checkout; MSYS2 Git remains required for `fmt` and `FmtColorGpt`.
Locate the current native Git/runtime rather than assuming an old plugin path.

Bash must also be available for packaging. Set process-local
`TAR_OPTIONS=--force-local` so GNU tar does not interpret the colon in an absolute
Windows archive path as a remote host. Use forward slashes in archive arguments.
Do not alter global Git, SSH, or library-toolchain settings to repair Sites.
Follow the skill's helper procedure and preserve the opening result, source SHA,
and exact archive returned for the selected site.

## Current publication

**Public Site version 27** is live at
[fmtcolor.ardentokapi66.chatgpt.site](https://fmtcolor.ardentokapi66.chatgpt.site).
The overview prominently links to
[upstream compatibility](https://fmtcolor.ardentokapi66.chatgpt.site/integrated-changes.html#upstream-compatibility).
That section documents the exact compatibility changes and 42 successful jobs
for `21eba01b`, including GCC 16/C++26 reflection. The contribution also covers
integrated Palette, conversion, color/style blending,
[styled ostream output](https://fmtcolor.ardentokapi66.chatgpt.site/integrated-changes.html#ostream),
and the separate 43-job FmtColorDev result at `257cdac1`.
The complete {fmt} release supports C++11;
the later independent library requires C++23 and `std::variant`.

| Item | Current value |
| --- | --- |
| Site project ID | `appgprj_6ad129c3e8408191b873a4e06e601c63` |
| Source commit | `2d981ac651e08933406f435db9323f960c2e0527` |
| Saved version ID | `appgprj_6ad129c3e8408191b873a4e06e601c63~appgver_998d9922111c8191b4bd981481e12095` |
| Successful deployment ID | `appgdep_6adf2b8d92f48191a8d558f966762bf7` |
| Audience | Public |

Native Sites deployment returned `succeeded`. Validation covered all three
pages, 39 local links/anchors, unique IDs, whitespace, and the exact five final
CI links. Downloads and the demo, design, and rendering sections were unchanged;
the Future Work introduction was updated without changing its proposals.
The new ostream section was inspected locally and live. Upstream compatibility
remains prominent, with its own 42-job evidence kept separate from final CI.
The demo remains a future acceptance target; nested text is not implemented.
Website checks do not establish library runtime or tablet coverage.

The successful helper result used the checkout at
`C:/Users/tjgolubi/.codex/visualizations/2026/10/25/01a17106-ee81-76c3-95ab-c8186910c8a9/fmtcolor-site`
and archive
`C:/Users/tjgolubi/.codex/visualizations/2026/10/25/01a17106-ee81-76c3-95ab-c8186910c8a9/fmtcolor-ostream.tar.gz`.
The archive was saved as version 27 and its deployment returned `succeeded`.
Refresh native Site metadata and source access before any further update.

Keep preview screenshots outside the source checkout (or in a helper-supported
ignored directory) so the helper's commit step does not add them. Package an
archive outside the website checkout; on this Windows setup the helper rejected
paths inside it. Preserve the project, opening result, source commit, archive,
saved version, and successful deployment when continuing work.

Future migration to MkDocs/Material remains a separate task.
