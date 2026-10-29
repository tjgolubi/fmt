# Owned styled text experiment

Updated October 29, 2026.

## Scope and state

Library branch: `feat/StyledText`, based on `color-dev` at
`5b3f49ff7599df5beb651c3ccc8a403d7a7386a1`. The SGR transition contribution is
committed and pushed as `cff2fc5fa0793fb476b94441aa2241c895423a2b`.
The owned structured-text foundation is committed and pushed as `7b0e07ce` on
`feat/StyledText`. Terry explicitly authorized committing and pushing both this
implementation and the coordination handoff. The handoff is maintained on the
separate `docs/Coord` branch; verify its remote ref after the save.
Use compact SGR and focused local checks; avoid unnecessary CI-triggering pushes.

## Agreed representation

`<fmt/styled-text.h>` defines `fmt::styled_text`: a node-local `text_style`,
a private `bool is_branch_`, and a manually managed union. The alternatives are
`styled_text::leaf` (`std::string`) and `styled_text::branch`
(`std::vector<styled_text>`). Every string and child is owned. Do not introduce
string views or borrowed subtrees in this increment. The implementation supports
C++11 in every language mode and does not depend on `std::variant`.
Develop with the eventual C++23 target in mind, using compiler/feature gates
when modern and older implementations differ; this foundation shares one path.

## Construction and access

```cpp
styled_text();
styled_text(leaf text, text_style style = {});
styled_text(const char* text, text_style style = {});
styled_text(branch children, text_style style = {});
styled_text(const styled_text&);
styled_text(styled_text&&) noexcept;
```

The default is an empty leaf with an empty style. Strings and literals convert
implicitly. By-value strings/vectors copy lvalues and move rvalues into storage.
Literal pointers must refer to valid null-terminated text; use `std::string`
for embedded nulls. The rule of five manages destruction and copy/move assignment;
copying recursively owns its contents. Assignment from a descendant is preserved
before destroying the parent. Allocation can throw; copy operations are not
`noexcept`.

`style` gets or replaces the node-local style. `is_branch`, `as_leaf`, and
`as_branch` provide direct access; wrong-alternative access throws `format_error`
(or follows the existing no-exception failure policy). Type-based `fmt::get`
preserves all four reference/const categories. `fmt::holds_alternative` accepts
`styled_text::leaf` and `styled_text::branch`. Index-based access, visitation,
and `get_if` are not added in this increment.

## Concatenation and equality

`operator+` returns an empty-style branch with two children, preserving both
operand nodes and their styles. Operands are passed by value so lvalues copy
and temporaries move. Plain strings and literals work on either side when
at least one operand is `styled_text`; adding two plain strings does not gain
an unrelated new overload. Repeated concatenation preserves nesting; it does
not flatten existing branches. No `operator+=` is included.

An empty parent style is the default, and a group can later be assigned a
style. Actual rendering/inheritance rules are future work.

`==` requires identical styles, the same alternative, and exactly matching
strings or ordered recursive child structures. `!=` negates equality.
A leaf `"ab"` differs from a branch with leaves `"a"` and `"b"`.
No ordering or hashing is introduced.

## SGR transition foundation

`to_sgr(final, initial)` already compares relative changes with reset-and-rebuild
and chooses the shorter byte encoding; ties prefer relative changes. Unspecified
final colors inherit initial colors, and a reset candidate restores them.
Emphasis is replaced, including removals; SGR 22 clears bold and faint together,
so any surviving intensity bit is reapplied. Output is one unpadded sequence.
The original one-argument overload is unchanged.

Terry supplied `AnsiColor.hpp` as a reference. Its `transition` also constructs
both candidates and chooses the shorter, with a comment proposing later cost
estimation. This supplied version contains no XOR/popcount estimation formula.
Unlike the new function, it mutates stored state and treats unspecified target
colors as defaults via SGR 39/49. A future count-first optimization is discussion,
not implemented work.

## Remaining work

Review the public API and owned union foundation before extending it. Nested
rendering, fmt formatters, C++20 module export, absolute boundaries, lazy blending,
parsing, visible width, serialization, and wide structured strings are not
implemented. CMake registers the header for installation and adds a focused
`styled-text-test` target. No website update is requested.

## Validation and backup

Focused styled-text tests pass in C++23 and C++11 compact-SGR builds. Strict
C++11 compilation and the ASan/UBSan ownership probe pass; leak detection is
unavailable in this environment. See [BuildAndTest.md](BuildAndTest.md) for
commands and limits. The downloadable backup includes both working-tree
snapshots, a Git bundle, patches, validation logs, and restore instructions.
It excludes build caches and credentials. The archive preserves the earlier uncommitted snapshot; the implementation is
now also saved on GitHub at `7b0e07ce`.
