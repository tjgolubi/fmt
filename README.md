# FmtColor coordination

Updated October 26, 2026.

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

| Document                           | Purpose |
| ---------------------------------- | ------- |
| [Workflow.md](Workflow.md)         | Authorization, Git, integration, and release rules. |
| [CppDesign.md](CppDesign.md)       | Implemented foundation, future design, and open decisions. |
| [BuildAndTest.md](BuildAndTest.md) | Build commands, compatibility checks, and current validation. |
| [Website.md](Website.md)           | Website content, publication, and maintenance. |

In this checkout, maintain these files on the separate `docs/Coord` branch,
not on a library development branch. Earlier coordination history is in the
[FmtColorGpt repository](https://github.com/tjgolubi/FmtColorGpt). These are
handoff documents for the next task conversation: current
baseline, accepted behavior, working rules, design vision, and open decisions.
Keep them current rather than appending an event log. Source and tests establish
implemented behavior; newer explicit instructions from Terry override older
wording. Proposals are not implementation authorization. Earlier development
records remain available in Git history.

## Current baseline

The development branch is
[`FmtColorDev`](https://github.com/tjgolubi/fmt/tree/FmtColorDev). Its chosen
upstream baseline is
[`0fc31633`](https://github.com/fmtlib/fmt/commit/0fc31633e7d53ac895ae0a688b83b7d44f0b0c1f).
The fork's `main` tracks upstream; styling development belongs on `FmtColorDev`.

| Commit | Purpose |
| --- | --- |
| [`0c7dad5c`](https://github.com/tjgolubi/fmt/commit/0c7dad5c53635c474761f933a4bab5508f9bfa53) | One consolidated feature commit directly above the upstream baseline: named RGB constants, style construction, comparisons, hashing, RGB averaging, documentation, tests, and tooling. |
| [`60eed619`](https://github.com/tjgolubi/fmt/commit/60eed6190eb5fb8c8f2d79036e78442da93a57eb) | Publish the generated user guide from `FmtColorDev`, with matching edit links and instructions. |
| [`b5b14ed2`](https://github.com/tjgolubi/fmt/commit/b5b14ed27bdd2d968649385fc4e9ec4dc5ae6ff2) | Fluent component getters/setters, presence aliases, all eight attribute controls, regression tests, and guide documentation. |
| [`8e8b8132`](https://github.com/tjgolubi/fmt/commit/8e8b8132191068db7c1e2d95221aba54c05768a6) | Shorter terminal-color and strike names, backward-compatible aliases and methods, regression tests, and updated guide. |
| [`1aaba9b5`](https://github.com/tjgolubi/fmt/commit/1aaba9b5945dbf269a6a6b3f199e8f77e793be4f) | Public C++11 `color_t`, reference/pointer access, visitation (extended by Palette below), regression tests, guide documentation, and generated changelog. |
| [`d9d64d36`](https://github.com/tjgolubi/fmt/commit/d9d64d361591311901e06c662c24879128c63724) | Palette colors, fixed RGB conversion, color/style blending, C++11 constexpr improvements, style clearing, tests, guide, and generated changelog. |
| [`257cdac1`](https://github.com/tjgolubi/fmt/commit/257cdac170506770ef3c807abc3ddd9928ef8191) | Styled ostream bridge, regression tests, installation registration, C++20 module export, user guide, and generated changelog. |

The current tip is `257cdac170506770ef3c807abc3ddd9928ef8191`. The branch has
six feature commits and one documentation-publication fix above upstream
`0fc31633`. No release tag or fork version has been assigned. Integration on
`FmtColorDev` is distinct from a numbered release or acceptance into `fmtlib/fmt`.
Terry intends eventual consolidation of the complete contribution for upstream;
that does not authorize changing `main` or rewriting the accepted history now.

All **43 jobs** passed for the final amended `FmtColorDev` commit, including
guide deployment. Linux fail-fast is enabled. GCC 16/C++26 reflection remains
excluded from that branch's workflow, tracked in
[issue #2](https://github.com/tjgolubi/fmt/issues/2).

## Upstream compatibility

[`test/UpstreamCompatibility`](https://github.com/tjgolubi/fmt/tree/test/UpstreamCompatibility)
at [`21eba01b`](https://github.com/tjgolubi/fmt/commit/21eba01b33becd9f94cd34a1a8758f64ecbc53c3) applies the same
enhanced header to upstream `0fc31633`, using upstream tests, CMake, and CI.
All **42 jobs** passed, including GCC 16/C++26 reflection. Required adjustments
are six intentional terminal-overlap result expectations, four explicit
nodiscard result discards, and two documentation-reference updates. Do not
claim that all upstream tests pass unchanged.

See [BuildAndTest.md](BuildAndTest.md#upstream-compatibility-branch) for exact
evidence, scope, and the branch's compatibility report. This branch is a
separate compatibility check, not the feature-development baseline. Terry
plans to rebase it periodically; each rebase requires fresh verification.

## Public documentation and next work

- [Project website](https://fmtcolor.ardentokapi66.chatgpt.site): public version
  **27**, including styled ostream output and prominent upstream compatibility evidence.
- [Generated user guide](https://tjgolubi.github.io/fmt/color/index.html): API
  references, examples, behavior notes, and RGB/terminal/gray/cube swatches.
- [Issues](https://github.com/tjgolubi/fmt/issues): bugs and follow-up work.

Fluent, ShortNames, VarColor, Palette, and Ostream are integrated. The public variant has
five alternatives: unspecified, RGB, terminal, cube, and gray. `rsvd_color` and
the mutating `rgb::blend()` member are removed. Free `to_rgb()` and variadic
color/style `blend()` are implemented, as are `text_style::empty()` and fluent
`clear()`. OR composition promotes overlapping specified colors to RGB;
strict rejection is not the chosen policy.

Include `<fmt/color-ostream.h>` for styled `print()`, `println()`, `vprint()`,
and `vprintln()` overloads taking `std::ostream&`. The bridge includes the color
and ostream headers without making either existing header depend on the other.
It was integrated independently of the pending TaskWarrior work.

The local `TaskWarrior` tip is `e46006c9bc2e62c9bc3d0b5dd40e5644ab9bc5fe`.
It branches from Palette and has not incorporated the accepted ostream bridge.
Its compact SGR mode, conversion/wrapping helpers, overlays, and value-returning
attribute functions are implemented there but are **not integrated into
FmtColorDev**. See [CppDesign.md](CppDesign.md#pending-taskwarrior-branch).

Structured styled text, absolute boundaries, lazy nested blending, parsing,
display-width handling, and serialization remain design work. Begin new feature
work from the current `FmtColorDev` tip, not an older candidate or compatibility
branch. The complete {fmt} release requires C++11; the later independent library
requires C++23 and will use `std::variant`.
