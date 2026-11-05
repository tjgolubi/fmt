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

Current published pages (version 44):

- [Overview](https://fmtcolor.tjgolubi.chatgpt.site/): implemented features,
  parser/stripper introduction, design, two deliverables, complete working
  `FmtColorDemo2.cpp`, download, and verified styled display.
- [The contribution](https://fmtcolor.tjgolubi.chatgpt.site/integrated-changes.html):
  consolidated `color-dev` at `35de08cc`, value APIs, nested formatting/context
  restoration, absolute resets unless no output, parser and leaf-local stripping
  examples, guide links, and the complete passing CI results.
- [Future work](https://fmtcolor.tjgolubi.chatgpt.site/work-in-progress.html):
  deferred blending, debug presentation, readable style parsing, extended push/pop
  parsing, terminal-width handling, serialization, and the independent C++23
  library. The independent sample, output, and downloads remain distinct from
  the working overview demo. The planned `backslash_escape`/`backslash_unescape`
  pair has its own linked section and is explicitly unimplemented.

Source links identify `35de08cc2de5b25100d02e51bdc41d8b595ccb01`, directly above
upstream `4ed3535092a96ac9f56fab80df83416d51bcc78e`. The contribution has not been
accepted into fmtlib/fmt. `pr/color` identifies the same proposed contribution;
the large PR has not yet been submitted. Link the branch as “Proposed contribution
/ pr/color,” then add a separate upstream PR link after submission. All 41
push-CI jobs passed on each branch on attempt 1. The separate `color-dev` guide
build and deployment also passed, for 43 successful jobs on that branch. No failed, cancelled, or pending jobs remain in
the latest workflow attempts. The separate `fix/StringViewCompare` branch
must not be mentioned on the website; Terry explicitly excluded it from this update.

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

The [guide publication](https://github.com/tjgolubi/fmt/actions/runs/41950172506)
passed build and deployment for `35de08cc` from `color-dev`; the
[ordinary doc build](https://github.com/tjgolubi/fmt/actions/runs/41950172372)
also passed. The workflow filter and edit URI target `color-dev`; Node.js 24
action pins are recorded in BuildAndTest.md. The exact environment policy was
not inspected. The guide includes the parser, string/tree stripping, and the
absolute-node reset guarantee unless there is no output.

The amended `color-dev` README uses current branch badges and the correct
website URL and documents owning text, parser, and stripper. The refreshed
integration document uses C++17 examples and explains header selection,
inheritance/restoration, and leaf-local stripping. Its changelog was regenerated
from the consolidated accepted history.

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
a download, and both output lines; the source and download match.

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

Version 41 presents standard SGR parsing as implemented, including the first-reset
absolute group protecting its suffix. It explains preserved malformed/non-SGR
escapes and appearance rather than byte-for-byte round trips. The new stripping
example distinguishes raw escapes from explicit tree styles. Tree stripping
preserves styles/structure, processes leaves separately, and never joins escape
sequences across leaves. Both features link to the guide and exact source.
Extended push/pop and readable style parsing remain future work.

## Planned backslash utilities

[Backslash escaping and unescaping](https://fmtcolor.tjgolubi.chatgpt.site/work-in-progress.html#backslash-escapes)
is listed in the future-work table and a dedicated section. Preserve the agreed
names `backslash_escape` and `backslash_unescape` and mark them unimplemented.
Escaping covers control characters, backslash (`\\`), and double quote (`\"`).
Unescaping reverses recognized sequences while retaining unknown/malformed
sequences and trailing backslashes verbatim. CSV/TSV conversion belongs elsewhere.
Do not imply these plans are in the current library or choose additional numeric
escape grammar without agreement.

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

**Public Site version 44** is live at
[fmtcolor.tjgolubi.chatgpt.site](https://fmtcolor.tjgolubi.chatgpt.site).
Native deployment returned `succeeded` on November 4, 2026 at 19:57 CST (November 5, 01:57 UTC), preserving
the public audience. It presents `35de08cc` on both `color-dev` and `pr/color`,
with a two-branch passing CI table and links to the proposed contribution branch.
It explicitly says the large PR has not been submitted. The planned backslash
utilities, parser/stripper examples, reset behavior, and demos are retained. No comparison-fix branch reference appears in the site.

| Item                     | Current value                                                                       |
| ------------------------ | ----------------------------------------------------------------------------------- |
| Site project ID          | `appgprj_6ad129c3e8408191b873a4e06e601c63`                                          |
| Source commit            | `357fe91684cecba2a315c14fe0205719e4791bce`                                          |
| Saved version ID         | `appgprj_6ad129c3e8408191b873a4e06e601c63~appgver_5df68c4d4a4081919944717454ae9939` |
| Successful deployment ID | `appgdep_6aebe2a05f808191b2cdf68377e9f06e`                                          |
| Audience                 | Public                                                                              |

Source checkout:
`C:/Users/tjgolubi/.codex/visualizations/2026/11/04/01a1a5a3-edf0-7bb0-9962-4ccf7e94539a/fmtcolor-site`.
Exact saved archive:
`C:/Users/tjgolubi/.codex/visualizations/2026/11/04/01a1a5a3-edf0-7bb0-9962-4ccf7e94539a/fmtcolor-pr-color.tar.gz`.
The helper synchronized published source before editing, then committed/pushed
and packaged the final state above. Sites accepted that archive as version 44.

Validation passed 46 local links/anchors, unique IDs, embedded demo/download/source
equality, and independent character/style comparison of all 214 demo characters.
Demo equality was checked against the exact amended commit without switching
the active library branch. The demo remained unchanged. The two-branch CI table
and proposed-branch wording were inspected in the local browser. Existing future
work and escape examples were retained. Whitespace and excluded-branch checks
passed. Native deployment verified publication, and the live URL was opened in the existing
browser tab. The owned port 8874 preview was stopped. No new full library,
tablet, performance, or ABI test is claimed by website QA.

Refresh Site metadata and source credentials before further updates; do not
treat the retained checkout as authoritative until synchronized. Preserve the
opening result, source commit, archive, version, and deployment IDs if work is
interrupted. Keep screenshots outside the source checkout. Future migration to
MkDocs/Material is a separate task.
