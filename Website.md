# Website

[Document map](README.md) · [Workflow](Workflow.md) · [C++ design](CppDesign.md) · [Build and test](BuildAndTest.md)

Updated October 28, 2026.

## Purpose and content

The public [FmtColor website](https://fmtcolor.ardentokapi66.chatgpt.site)
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

Current pages:

- [Overview](https://fmtcolor.ardentokapi66.chatgpt.site/): implemented features,
  the 41-job CI result, two deliverables, design, and the future nested-text demo.
- [The contribution](https://fmtcolor.ardentokapi66.chatgpt.site/integrated-changes.html):
  current baseline, color/style APIs, palette conversion and blending,
  factories, SGR helpers, stream output, original-API changes, and exact CI links.
- [Future work](https://fmtcolor.ardentokapi66.chatgpt.site/work-in-progress.html):
  structured styled text, absolute boundaries, lazy blending, parsing,
  display width, and serialization. These remain proposals.

The styling contribution is commit `2c456778d9ff63cdf284a7d85c8dc8eb384cf3d3`
on `color-dev`,
directly above upstream `f87a75e15262614ce8aa0e6a0664df9d0823fb92`.
It supports C++11 throughout and has not been accepted into fmtlib/fmt.
All 41 push jobs passed: Linux 18, Windows 13, macOS 9, documentation build 1.
Do not promote the old 42-job compatibility-branch result as evidence for
this contribution. Its historical scope is retained in BuildAndTest.md only.

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
and aligned Markdown tables. All three Styled Arguments examples show expected
output, including padding and newline behavior. Its ostream example contains
only the statements inside `main`. The shared styled-value example uses
`color::midnight_blue` (`#191970`) as background. The Overview's nested-reset
example uses `term_color::bright_red` and displays its fixed RGB equivalent,
`{255, 0, 0}` (`#FF0000`). A terminal theme may render palette colors differently.

The Colors and Named Colors pages include RGB/terminal/gray/cube swatches.
Preserve cube red-row/green-column orientation, six blue values per cell,
three-digit padding, contrast, and responsive normal/bright terminal grids.
`doc/api.md` lists `fmt/color-ostream.h` at the top and links to the existing
styled-ostream guide. It has no separate added API Reference section.

### Guide build versus publication

The current `color-dev` guide builds locally into `build/doc-html` and is served at
`http://localhost:8000/color/index.html` while its server runs. The server was
left running for Firefox. The `doc` CI workflow also passed for `2c456778`.
The known upstream syntax-anchor and Material future-version notices remain
nonfatal. See BuildAndTest.md for commands and validation scope.

The guide-publication workflow targets `color-dev`, with edit URI
`edit/color-dev/doc/` and matching publication instructions in local uncommitted
changes. The preceding configuration targeted `dev` and was committed and
pushed at `a18689e1`.
Its Node.js 24 action pins are `configure-pages` v6.0.0 and `deploy-pages`
v5.0.1; BuildAndTest.md records the exact SHAs. The [guide publication run](https://github.com/tjgolubi/fmt/actions/runs/40680365434)
passed both build and deploy jobs for `a18689e1`, including Configure Pages
and Publish documentation. Deployment permissions allowed that `dev` run;
they have not been verified for `color-dev`. Check the selected-branch rule and the next publication run.
The exact environment policy was not inspected. The separate
ordinary documentation workflow also passed; platform CI was still running
when checked. The website's 41-job result remains scoped to `2c456778`.
The library README still has old branch text/badges and a link to the
intentionally removed `FmtColorChangeLog.md`; do not recreate that file.

## Demonstration and future design

Preserve the downloadable `FmtColorDemo.cpp` and `AnsiRenderDemo.txt`, the
displayed sample, and its styled output together. The demo is a future
acceptance target, not implemented/tested functionality. Preserve nested
emphasis/colors, absolute underline, the enclosing background, and
`gray_color{12}`. The caption describes representative appearance without
narrating implementation history.

The sample targets the later independent C++23 implementation, using
`std::println` while retaining `<fmt/color.h>` and namespace `fmt` through the
selected installation. It requires planned structured text and standard
formatter support. The displayed and downloadable C++ must agree. Keeping the
header spelling does not imply a dependency on fmtlib in that implementation.

Keep implemented value operations distinct from proposed nested rendering:
constructors/fluent setters replace components, `blend` averages specified
channels, and bitwise OR composes color bits. Sequential ANSI SGR updates are
terminal state changes. Bold/faint remain independent. The {fmt} contribution
retains conceal; the independent design replaces it with flash. Absolute
boundaries, borrowed-subtree lifetimes, lazy blending, parsing, display width,
and serialization still need design decisions. Consult CppDesign.md.

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

The repository branch is now `color-dev`. Site version 36 still displays and
links `dev`; updating the public website for the rename requires a separate
website instruction. The current task changes repository documentation only.

**Public Site version 36** is live at
[fmtcolor.ardentokapi66.chatgpt.site](https://fmtcolor.ardentokapi66.chatgpt.site).
Native deployment returned `succeeded`. The site presents the complete
contribution using the former `dev` branch name and its four exact CI runs, with no intermediate development
history. The final blend example preserves the original foreground-averaging
semantics and uses the compiler-checked expression above.

| Item                     | Current value                                                                       |
| ------------------------ | ----------------------------------------------------------------------------------- |
| Site project ID          | `appgprj_6ad129c3e8408191b873a4e06e601c63`                                          |
| Source commit            | `77d338f6f690591e6f16d7dba664b787154693d9`                                          |
| Saved version ID         | `appgprj_6ad129c3e8408191b873a4e06e601c63~appgver_9122a83ba5f8819187be73fcb480fe9c` |
| Successful deployment ID | `appgdep_6ae1d597e62c819189af14945791e4ab`                                          |
| Audience                 | Public                                                                              |

Source checkout:
`C:/Users/tjgolubi/.codex/visualizations/2026/10/28/01a18165-1e45-76c1-bf94-4c3b5db40aab/fmtcolor-site`.
Exact saved archive:
`C:/Users/tjgolubi/.codex/visualizations/2026/10/28/01a18165-1e45-76c1-bf94-4c3b5db40aab/fmtcolor-foreground-blend.tar.gz`.
The source helper opened `0e70c98c027a7e895e04d6cd346267a1733dfa86` for the
final correction and pushed the source SHA above before packaging.

The revised three-page site passed checks for 40 local links/anchors, unique
IDs, current commit links, absence of intermediate records, and all four exact
CI links. Overview and contribution layouts were inspected in the browser;
small subsequent example changes were checked against source. The final `fg`
blend expression and resulting style passed C++14/C++23 static assertions.
Downloads and the proposed nested-text sample were preserved. Native Sites
confirmed each publication; these checks do not establish new library runtime
or tablet coverage. The site preview server on 8875 was stopped.

Refresh Site metadata and source credentials before further updates; do not
treat the retained checkout as authoritative until synchronized. Preserve the
opening result, source commit, archive, version, and deployment IDs if work is
interrupted. Keep screenshots outside the source checkout. Future migration to
MkDocs/Material is a separate task.
