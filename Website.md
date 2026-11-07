# Website

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Build and test](BuildAndTest.md)

Updated November 7, 2026.

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

Current published pages (version 49):

- [Overview](https://fmtcolor.tjgolubi.chatgpt.site/): implemented features,
  parser/stripper introduction, design, two deliverables, complete working
  `FmtColorDemo2.cpp` underlay/overlay comparison, styled `TableDemo.cpp` display,
  source/ANSI downloads, and guide walkthrough link.
- [The contribution](https://fmtcolor.tjgolubi.chatgpt.site/integrated-changes.html):
  current `color-dev` at `10c4cd72`, value APIs, nested formatting/context
  restoration, absolute resets unless no output, parser and leaf-local stripping
  examples, guide links, and the complete passing CI results.
- [Future work](https://fmtcolor.tjgolubi.chatgpt.site/work-in-progress.html):
  deferred blending, debug presentation, readable style parsing, extended push/pop
  parsing, terminal-width handling, serialization, and the independent C++23
  library. The independent sample, output, and downloads remain distinct from
  the working overview demo. Backslash utilities are implemented and linked from
  the contribution page; the old future-work anchor directs readers there.

Source links identify `10c4cd72d015380410d73fe6fcae84b58d0dc3d5`, based on
upstream `4ed3535092a96ac9f56fab80df83416d51bcc78e`. The contribution has not been
accepted into fmtlib/fmt. `pr/color` now matches color-dev at `10c4cd72`.
[Upstream PR #5015](https://github.com/fmtlib/fmt/pull/5015) is closed and
unmerged, with recorded head `dd7180a8`, verified November 7, 2026.
The Site distinguishes that PR record from the updated source branch.
All 41 push-CI jobs passed on each branch: color-dev attempt 2, pr/color
attempt 1. Both color-dev guide jobs passed on attempt 2 (43 total).
No failed, cancelled, or pending jobs remain in the latest attempts.
The separate `fix/StringViewCompare` branch must not be mentioned on the Site.

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
`"hello " + fmt::styled_text{"world", fmt::fg(fmt::color::blue)}`: `***hello wor`, all bold with `wor` blue.
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

A prior guide preview built into `build/guide-output-preview` and was
served at `http://localhost:8875/color/styled.html`. Check server availability
before relying on it; older previews used `build/doc-html` and port 8000. The previously observed
upstream syntax-anchor and Material future-version notices remain nonfatal.
The guide now documents format-context inheritance and restoration for
`styled` and `styled_text`, including the whole-sentence underline example.

The [guide publication](https://github.com/tjgolubi/fmt/actions/runs/42313730769/attempts/2)
passed build and deployment for `10c4cd72` from `color-dev`; the
[ordinary doc build](https://github.com/tjgolubi/fmt/actions/runs/42313730724/attempts/2)
also passed. The workflow filter and edit URI target `color-dev`; Node.js 24
action pins are recorded in BuildAndTest.md. The exact environment policy was
not inspected. The guide includes the parser, string/tree stripping, and the
absolute-node reset guarantee unless there is no output.

The amended `color-dev` README uses current branch badges and the correct
website URL and documents owning text, parser, and stripper. The refreshed
integration document uses C++17 examples and explains header selection,
inheritance/restoration, and leaf-local stripping. Its changelog was regenerated
from the consolidated accepted history.

The guide table now uses colored HTML output. Terry approved one extra display
space after `東京 NAS` and removal of the paragraph beginning “Rendered from
the demo's ANSI output.” Those guide edits are in `10c4cd72`; the website's
existing ANSI-derived table remains unchanged. The page was built locally and
a CSS-embedded `styled.html` download was provided for Terry's review.

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
retains conceal; the independent design replaces it with flash. The consolidated
`color-dev` includes owning text, its corrected formatter, and context state;
`styled` and `styled_text` inherit and restore outer message styles. Absolute spans
temporarily replace them. The modernized `demo/FmtColorDemo2.cpp` is committed
and pushed, using `absolute(underline(...))`. The former deferred `.tjg.cpp`
file is no longer present. The overview embeds the complete working source,
a download, and the original/underlay/overlay output; the source and download match.
The table section displays compiled styled output with UTF-8 alignment and
truncation, build/run instructions, source/ANSI downloads, and the guide link.

The Site's downloadable sample remains the distinct future independent-library
target. Preserve its displayed source, output, and downloads together. The
independent library will omit whole-message style-taking output overloads and
use ordinary standard formatting with styled replacement values/trees; this
design is described on the Site. Debug formatting,
deferred blending, readable style parsing, extended push/pop parsing, full
grapheme/terminal width, serialization, and
styled-text module export remain future work. Consult CppDesign.md and
StyledText.md for current implementation and boundaries.

## Published parsing and stripping

The current Site presents standard SGR parsing as implemented, including the first-reset
absolute group protecting its suffix. It explains preserved malformed/non-SGR
escapes and appearance rather than byte-for-byte round trips. The new stripping
example distinguishes raw escapes from explicit tree styles. Tree stripping
preserves styles/structure, processes leaves separately, and never joins escape
sequences across leaves. Both features link to the guide and exact source.
Extended push/pop and readable style parsing remain future work.

## Implemented Backslash utilities

[Backslash escaping and unescaping](https://fmtcolor.tjgolubi.chatgpt.site/integrated-changes.html#backslash-escapes)
is presented as implemented, with the four public functions, a C++17-compatible
example, source/test links at `10c4cd72`, and a link to the guide's Markdown
escape reference table. It is no longer listed as planned work. Preserve the
legacy future-work anchor as a link to the current contribution.

Describe the strict codec: ASCII controls and DEL are escaped, ESC uses `\e`,
NUL stays `\x00`, and unknown/malformed decoding input throws
`fmt::format_error`. The iterator variants retain output written before an
error. Narrow/wide support and available constexpr evaluation are implemented.
Do not restore the superseded malformed-escape preservation policy. Keep the
website concise; the complete grammar and table belong in the guide, with the
accepted design in CppDesign.md. CSV/TSV conversion remains outside scope.

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

Prefer native Windows Git for the Sites Node helper; MSYS2 Git remains required
for the library. The installed Sites 1.1.3 helper opened, committed/pushed, and
packaged this update without custom helper changes. A sandbox source-preparation
failure was resolved by running the helper with approved elevated execution.
Keep credentials in memory and pass them through hidden stdin only.

Use Windows `C:/Windows/System32/tar.exe` ahead of MSYS tar when packaging
absolute Windows paths; use forward slashes in archive arguments. Do not alter
global Git, SSH, or toolchain settings. Preserve successful source/package
results and continue saving/deploying that same state after a helper failure.

## Current publication

**Public Site version 49** is live at
[fmtcolor.tjgolubi.chatgpt.site](https://fmtcolor.tjgolubi.chatgpt.site).
Native deployment returned `succeeded` November 7, 2026 at 00:17:21 UTC,
preserving public access. It presents `color-dev` at `10c4cd72`, current
underlay defaults and recursive overlay overrides, protected absolute scopes,
the revised FmtColorDemo2, the styled table demo, source/ANSI downloads, exact
source links, passing results on both branches with exact attempt links, and
the closed/unmerged PR #5015 status. Color-dev has 43 successful jobs on
attempt 2; pr/color has 41 on attempt 1. Guide publication is color-dev only.

| Item                     | Current value                                                                       |
| ------------------------ | ----------------------------------------------------------------------------------- |
| Site project ID          | `appgprj_6ad129c3e8408191b873a4e06e601c63`                                          |
| Source commit            | `d7405bdf11675041ddb74de5c030bf0749ee3e82`                                          |
| Saved version ID         | `appgprj_6ad129c3e8408191b873a4e06e601c63~appgver_a4f4ffcb063081919e6da4fb9a0bbe16` |
| Successful deployment ID | `appgdep_6aee6e0a80108191b511fb8e37c14fff`                                          |
| Audience                 | Public                                                                              |

Cloud source checkout: `/workspace/sites/fmtcolor`.
Exact saved archive: `/workspace/sites/fmtcolor-pr-ci-update.tar.gz`.
The installed cloud skill was readable, but the Sites source helper was absent
and its script resource could not be retrieved. Source was opened and pushed
with ordinary Git using fresh Site credentials through hidden stdin and
process-local headers. No token was persisted in source/configuration.
The archive contains `.openai/hosting.json` and `dist/` from the exact pushed
source commit; Sites saved it as version 49 and confirmed deployment success.
Use the supported helper when available in subsequent environments.

The current update passed 50 local links/anchors, unique IDs, and checks of all
four pr/color evidence links. Demo source/download equality and rendered
text/padding comparisons passed when the unchanged displays were introduced. Displays were generated from that output. Blink is illustrated as
steady text and terminal palette/theme limits are disclosed. Browser tools were
unavailable, so no visual browser QA is claimed for version 49. No preview
server was started. No new full local library, tablet, performance, or ABI result
is inferred from website QA.

Refresh Site metadata and credentials before another update. Keep successful
source/archive/version/deployment identifiers if interrupted. Do not resave or
redeploy a successful version unnecessarily. Site source stays separate from
the library. Future migration to MkDocs/Material remains a separate task.
