# FmtColor coordination

Updated October 28, 2026.

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

Maintain these files in the `coord/` Git worktree on `docs/Coord`, separate
from the library checkout. `cd coord` enters the documentation worktree;
`cd ..` returns to the library branch. `/coord/` is excluded locally through
`.git/info/exclude`; this setup is local Git metadata, not a library commit.
The five documents live at the root of the documentation branch. Earlier
coordination history is in the
[FmtColorGpt repository](https://github.com/tjgolubi/FmtColorGpt). These are
handoff documents for the next task conversation: current
baseline, accepted behavior, working rules, design vision, and open decisions.
Keep them current rather than appending an event log. Source and tests establish
implemented behavior; newer explicit instructions from Terry override older
wording. Proposals are not implementation authorization. Earlier development
records remain available in Git history.

## Current baseline

The development branch is [`color-dev`](https://github.com/tjgolubi/fmt/tree/color-dev), at
[`a18689e1`](https://github.com/tjgolubi/fmt/commit/a18689e1a0aced47e96db18b02aab577a853168a).
Its documentation-publication configuration follows the complete styling
contribution at `2c456778`, which is one squash commit directly
above upstream [`f87a75e1`](https://github.com/fmtlib/fmt/commit/f87a75e15262614ce8aa0e6a0664df9d0823fb92),
which is also the local `main` tip. Terry created `Phase1` from `FmtColorDev`,
rebased it onto freshly updated `main`, and squash-merged it onto `dev`.
Terry renamed the GitHub branch to `color-dev`; the local checkout and tracking
branch now also use `color-dev`. The workflow filter, guide edit URI, and
publication instructions have been updated locally to the new name. Those
rename-related configuration edits are not yet committed or pushed.
Continue from `color-dev`, not the superseded development or compatibility branches.
No release tag or fork version has been assigned; the contribution has not
been accepted into fmtlib/fmt.

Terry removed `FmtColorChangeLog.md`, amended the contribution, and pushed
with `--force-with-lease` before the CI verification below. The verified SHA
already includes that removal. Do not regenerate the file without instruction.

All **41 jobs** passed for styling commit `2c456778`: Linux 18, Windows 13,
macOS 9, and documentation build 1. Local, GitHub branch, and workflow SHAs
matched; every job was inspected. There were no failed, cancelled, or pending
jobs. See [BuildAndTest.md](BuildAndTest.md#integrated-styling-helpers).
Matrix fail-fast remains disabled. GCC 16/C++26 reflection remains excluded,
tracked in [issue #2](https://github.com/tjgolubi/fmt/issues/2).

Documentation commit `a18689e1a0aced47e96db18b02aab577a853168a` is committed
and pushed. It targeted the former `dev` branch for publication and edit links
and pins Node.js 24 actions: `configure-pages` v6.0.0 and `deploy-pages` v5.0.1.
The [guide build and deployment](https://github.com/tjgolubi/fmt/actions/runs/40680365434)
both passed for that exact SHA, as did ordinary documentation CI. Platform CI
was still running when checked; the earlier 41-job result applies specifically
to `2c456778`. Publication succeeded from `dev`; this does not establish
that the environment allows the renamed `color-dev` branch. Verify its deployment rule and the next
publication run. The exact policy setting was not inspected. The root
library README still names `FmtColorDev` in text/badges and links the deleted
changelog; that cleanup remains a separate follow-up.

## Upstream compatibility

The separate `test/UpstreamCompatibility` result at `21eba01b` is historical:
it tests an older header on upstream `0fc31633`, not the consolidated `color-dev`
contribution. Its 42 passing jobs are not current `color-dev` compatibility evidence.
The required upstream adjustments and exact scope remain in
[BuildAndTest.md](BuildAndTest.md#upstream-compatibility-branch).
Do not claim that all original upstream tests pass unchanged.

## Public documentation and next work

- [Project website](https://fmtcolor.ardentokapi66.chatgpt.site): public version
  **36**, describing the complete contribution and its 41 passing jobs.
  Its displayed branch name is still `dev`; a website rename update has not
  been requested.
  It contains no intermediate development history.
- [Generated user guide](https://tjgolubi.github.io/fmt/color/index.html):
  last verified publication from the former `dev` branch at `a18689e1`. Also available locally at
  [localhost](http://localhost:8000/color/index.html) while its server runs.
- [Issues](https://github.com/tjgolubi/fmt/issues): bugs and follow-up work.

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
their sections, and expected output for all three Styled Arguments examples.
The API Reference header list includes `fmt/color-ostream.h`, linking to its
existing guide section; it has no separate new API Reference section.

Structured styled text, absolute boundaries, lazy nested blending, parsing,
display-width handling, and serialization remain design work. The later
independent library requires C++23 and uses `std::variant`; the complete
{fmt} release continues to support C++11. Revisit attribute-factory naming
for the independent library as described in
[CppDesign.md](CppDesign.md#attribute-factory-design-for-the-independent-library).
