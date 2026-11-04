# Website

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Build and test](BuildAndTest.md)

Updated November 4, 2026.

## Purpose and content

The public [FmtColor website](https://fmtcolor.tjgolubi.chatgpt.site)
explains the contribution, design rationale, demonstration, and future work.
Its audience, in priority order, is Terry and collaborating assistants, the
{fmt} developers, technical friends, and nontechnical family. Use brief
plain-language introductions followed by useful technical detail.

Present the complete `color-dev` contribution as one coherent change. The website
must contain no intermediate development history: omit feature-branch
integration records, superseded commits, old API alternatives, and historical
compatibility-run narratives. Source links refer to the current contribution
and its upstream baseline. Explain behavior and differences from the original
upstream API without ABI/rebuild reminders; the ABI change is accepted.

Current published pages (version 39):

- [Overview](https://fmtcolor.tjgolubi.chatgpt.site/): integrated owning text,
  design, two deliverables, complete modernized `FmtColorDemo2.cpp`, and its
  verified styled display.
- [The contribution](https://fmtcolor.tjgolubi.chatgpt.site/integrated-changes.html):
  `color-dev` at `2fa0bd92`, value APIs, nested formatting and context restoration,
  absolute styles, nested precision/padding and C++20 pi examples with expected
  displays, and exact links to all passing CI jobs.
- [Future work](https://fmtcolor.tjgolubi.chatgpt.site/work-in-progress.html):
  deferred blending, debug presentation, parsing, full terminal-width handling,
  serialization, and the independent C++23 library. The retained independent
  sample, output, and downloads are here, distinct from the working overview demo.

All 41 push-CI jobs and both guide-publication jobs passed for `2fa0bd92`.
The contribution remains based on upstream `f87a75e1` and has not been accepted
into fmtlib/fmt. The historical 42-job compatibility branch is separate evidence.

## User guide

The Sites website explains rationale; the library's generated
[user guide](https://tjgolubi.github.io/fmt/color/index.html) explains use.
Do not maintain a competing guide in Sites or put Site source in the library
repository. Link guide buttons to generated HTML, not raw Markdown containing
MkDocs directives. Explicitly labeled source links may point to GitHub files.

Use function names without `()` in ordinary prose. Retain parentheses in calls,
examples, and signatures. Prefer factories and fluent operations for ordinary
examples; reserve `|` for demonstrations of bitwise composition. Terry accepted
the `text_style` class-comment construction/comparison examples using `|`.
The overview's opening sketch and “Small values. Clear semantics.” example both
use `fg(red).bg(blue)`.

Use `term_color`, `emphasis::strike`, and short component/attribute operations
in examples. Describe the original long getters and presence queries in one
compatibility note; generated member listings omit those aliases and the
strike member aliases. The latter are current API aliases, not methods from
the original upstream header. Preserve upstream spellings in original tests.

Named colors are RGB values. The website puts
`auto blended = fmt::blend(fmt::color::red, fmt::color::blue);`
before its explicit bitwise-OR example. The style-blend example uses:

```cpp
constexpr auto first = fmt::text_style{fmt::term_color::red, fmt::no_color,
                                      fmt::emphasis::bold};
constexpr auto second = fmt::underline(fmt::fg(fmt::gray_color{12}));
constexpr auto combined = fmt::blend(first, second);
// Foreground RGB {166, 64, 64}; emphasis bold and underline.
```

`fg` is intentional; using `bg` changes the example's meaning. Terminal red
maps to `{205, 0, 0}` and gray level 12 to `{128, 128, 128}`. The result has
unspecified background. The style-copy `underline` overload supports constexpr
in C++14 and later; use `const auto` for `second` and `combined` in C++11.
Boolean-only factories such as `bold()` are constexpr in C++11.

The guide has links from introductory identifiers to their matching sections
and aligned Markdown tables. Styled Arguments examples show expected displays
and newline behavior. Nested precision/padding uses `{:*>12.9}` on bold
`"hello " + overlay(fg(blue), "world")`: `***hello wor`, all bold with `wor` blue.
The ostream example includes `<numbers>` marked C++20 and uses
`std::numbers::pi`, displaying `Pi: 3.14` with only `3.14` bold green. The shared styled-value example uses
`color::midnight_blue` (`#191970`) as background. The Overview's nested-reset
example uses `term_color::bright_red` and displays its fixed RGB equivalent,
`{255, 0, 0}` (`#FF0000`). A terminal theme may render palette colors differently.

The Colors and Named Colors pages include RGB/terminal/gray/cube swatches.
Preserve cube red-row/green-column orientation, six blue values per cell,
three-digit padding, contrast, and responsive normal/bright terminal grids.
`doc/api.md` lists `fmt/color-ostream.h` at the top and links to the existing
styled-ostream guide. It has no separate added API Reference section.

### Guide build versus publication

The latest guide preview builds into `build/guide-output-preview` and was
served at `http://localhost:8875/color/styled.html`. Check server availability
before relying on it; older previews used `build/doc-html` and port 8000. The known
upstream syntax-anchor and Material future-version notices remain nonfatal.
The guide now documents format-context inheritance and restoration for
`styled` and `styled_text`, including the whole-sentence underline example.

The [guide publication](https://github.com/tjgolubi/fmt/actions/runs/41556114426)
passed build and deployment for current `2fa0bd92` from `color-dev`; the
[ordinary doc build](https://github.com/tjgolubi/fmt/actions/runs/41556114169)
also passed. All platform jobs passed for that same SHA. The workflow filter
and edit URI target `color-dev`; Node.js 24 action pins are recorded in
BuildAndTest.md. Successful publication verifies that this branch was permitted,
although the exact environment-policy configuration was not inspected.

The root library README still uses `FmtColorDev` in branch text/badges and the
older website URL. The changelog is tracked again, so its relative changelog
link is no longer a deleted-file link. README cleanup is a separate follow-up.

## Demonstration and future design

Preserve the downloadable `FmtColorDemo.cpp` and `AnsiRenderDemo.txt`, the
displayed independent sample, and its styled output together on Future work.
That independent demo is a future
acceptance target, not implemented/tested functionality. Preserve nested
emphasis/colors, absolute underline, the enclosing background, and
`gray_color{12}`. The caption describes representative appearance without
narrating implementation history.

The sample targets the later independent C++23 implementation, using
`std::println` while retaining `<fmt/color.h>` and namespace `fmt` through the
selected installation. It requires the planned independent implementation and standard formatter
support; the existing fmt formatter alone does not provide `std::formatter`. The displayed and downloadable C++ must agree. Keeping the
header spelling does not imply a dependency on fmtlib in that implementation.

Keep implemented value operations distinct from proposed nested rendering:
constructors/fluent setters replace components, `blend` averages specified
channels, and bitwise OR composes color bits. Sequential ANSI SGR updates are
terminal state changes. Bold/faint remain independent. The {fmt} contribution
retains conceal; the independent design replaces it with flash. The owning tree and nested formatter are integrated on `color-dev` at
`1e8fc387`. ColorCtx squash `b8bf6293` integrates context state, so `styled`
and `styled_text` inherit and restore outer message styles. Absolute spans
temporarily replace them. The modernized `demo/FmtColorDemo2.cpp` is committed
and pushed, using `absolute(underline(...))`. The former deferred `.tjg.cpp`
file is no longer present. The overview embeds the complete working source,
a download, and both output lines; the source and download match.

The Site's downloadable sample remains the distinct future independent-library
target. Preserve its displayed source, output, and downloads together. The
independent library will omit whole-message style-taking output overloads and
use ordinary standard formatting with styled replacement values/trees; this
design is described on the Site. Debug formatting,
deferred blending, parsing, full grapheme/terminal width, serialization, and
styled-text module export remain future work. Consult CppDesign.md and
StyledText.md for current implementation and boundaries.

## Unpublished parser progress

The separate `feat/parser` candidate adds standard SGR-to-owned-styled-text
parsing in `<fmt/parse_styled_text.h>` and `strip_ansi` in `<fmt/strip_ansi.h>`
at `76fd8ba6`. All 41 push-CI jobs pass, including macOS; the candidate is not
yet integrated. Published version 39 predates it and remains unchanged. Do not silently relabel
these as published or accepted features; a website update requires instruction.

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

Prefer native Windows Git for the Sites Node helper; MSYS2 Git remains
required for the library. If native Git is unavailable, the MSYS helper needs
Windows drive-path normalization (including uppercase drive letters) and
compatible commit/tree revision handling. Sites 1.1.2 failed on these mismatches
in this session; a checkout-external helper copy was corrected without changing
plugin or library files. Keep credentials in hidden stdin and do not log them.

Use Windows `C:/Windows/System32/tar.exe` ahead of MSYS tar when packaging
absolute Windows paths; alternatively GNU tar needs `TAR_OPTIONS=--force-local`.
Use forward slashes in archive arguments. The original package helper was run
after source push so its bundled preparation script resolved correctly. Do not
alter global Git, SSH, or toolchain settings. Preserve successful source/package
results and continue saving/deploying that same state after a helper failure.

## Current publication

**Public Site version 39** is live at
[fmtcolor.tjgolubi.chatgpt.site](https://fmtcolor.tjgolubi.chatgpt.site).
Native deployment returned `succeeded`; the existing public audience was preserved.
It presents the complete modernized demo, context restoration, revised guide
examples, and current green `2fa0bd92` matrix.

| Item                     | Current value                                                                       |
| ------------------------ | ----------------------------------------------------------------------------------- |
| Site project ID          | `appgprj_6ad129c3e8408191b873a4e06e601c63`                                          |
| Source commit            | `c176a0d76d5452ab9f41378788783287adc0f37c`                                          |
| Saved version ID         | `appgprj_6ad129c3e8408191b873a4e06e601c63~appgver_4059a0cac90481919368c406c53b8314` |
| Successful deployment ID | `appgdep_6ae934985b148191a65a3f3308be86f0`                                          |
| Audience                 | Public                                                                              |

Source checkout:
`C:/Users/tjgolubi/.codex/visualizations/2026/11/02/01a19d5b-62f8-7e32-9bf2-8f3bf72731c9/fmtcolor-site`.
Exact saved archive:
`C:/Users/tjgolubi/.codex/visualizations/2026/11/02/01a19d5b-62f8-7e32-9bf2-8f3bf72731c9/fmtcolor-guide-update.tar.gz`.
The helper synchronized source `9c4cc17fc77f2d6516fc2526badb196ebda218a2`,
then committed/pushed the current source. The archive was packaged from that
pushed state and accepted by Sites as version 39.

The pi example was inspected in the local browser, including its bold-green
expected display. Whitespace checks passed. The preceding demo publication
verified source/download equality, 42 local links/anchors, and the demo's
ANSI display character by character; those checks were not rerun for version 39.
No new tablet or full library test is claimed by website QA. The Site preview
on port 8874 was stopped; the guide preview on port 8875 is separate.

Refresh Site metadata and source credentials before further updates; do not
treat the retained checkout as authoritative until synchronized. Preserve the
opening result, source commit, archive, version, and deployment IDs if work is
interrupted. Keep screenshots outside the source checkout. Future migration to
MkDocs/Material is a separate task.
