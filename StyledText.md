# Structured styled text experiment

Updated October 30, 2026.

## Scope and state

Active library branch: `feat/StyledText2`, verified on GitHub at
`08a9d3b7a828392e1d64281a6bcc4a6e0212b73d`. The local `fix/SgrTransition`
worktree has the same commit and a clean working tree. The experiment has not
been squash-integrated into `color-dev`; Terry's review remains pending.
Committed, pushed, tested, and accepted are distinct states.

The dependency order above `color-dev` at `5b3f49ff` is:

| Commit     | Scope                                                     |
| ---------- | --------------------------------------------------------- |
| `ae726b28` | Require two or more public blend arguments.                |
| `d29412bb` | Absolute text-style values, contracts, and SGR generation. |
| `a8fcd97b` | Owning styled-text work rebased and branch configuration.  |
| `08a9d3b7` | Complete-state SGR transitions, regression tests, guide.   |

`a8fcd97b` identifies the rebased tip, not one implementation commit. Its
preceding series preserves the original styled-text work, including immutable
shared leaves, private character-generic structure, append fixes, plain-text
extraction, direct leaf initialization, owning-string regression tests, and
member/free overlays with free emphasis modifiers. The original branch was
`feat/StyledText`; the active owning baseline is now `feat/StyledText2`.
The borrowing `StringView` experiment is separate. Its lifetime and constexpr
properties are not those of the owning implementation.

Push CI is deliberately excluded for `feat/StyledText2` in linux, windows, macos,
and doc; the branch spelling now matches the exclusions. PR triggers remain
preserved. Do not restore push CI without instruction. Use compact SGR and
focused local checks for this experiment.

The transition fix was committed locally and delivered as `SgrTransitionFix.bundle`,
then imported and pushed by Terry. Its exact prerequisite is
`a8fcd97b608c7b20c758883acad389380d0313bf`; its exported ref is
`refs/heads/fix/SgrTransition`. Import into a checkout containing that base:

```bash
git bundle verify /path/to/SgrTransitionFix.bundle
git fetch /path/to/SgrTransitionFix.bundle fix/SgrTransition
git log --oneline HEAD..FETCH_HEAD
```

These commands inspect/import objects without switching or merging. The fix
was fast-forwarded into the experimental branch; accepted integration into
`color-dev` still follows the squash policy in Workflow.md. Preserve exact commits in future authorized bundle deliveries. Coordination changes are
separate uncommitted edits, not included in the library bundle.

## Representation and public API

`<fmt/styled-text.h>` defines `basic_styled_text<Char>`, with aliases
`styled_text` for `char` and `wstyled_text` for `wchar_t`. Public `char_type`
and `string_type` aliases describe the character and owning-string types.
The implementation supports C++11 in every language mode, using a manually
managed union, not `std::variant`.

Each node owns `_style` and a private discriminator. A leaf holds
`std::shared_ptr<const string_type>`; a branch owns a vector of nodes of the
same character type. Nonempty text uses `make_shared`; a null pointer means
empty text. Strings are immutable after construction. Copies recursively copy
vectors and node styles but share character storage. There is no copy-on-write,
shared node/vector ownership, or borrowed storage. Initial leaf construction
uses the union's leaf constructor directly; alternative changes still require
manual lifetime management.

Construction supports default, owning-string, null-terminated literal, copy,
and move forms. String/literal conversion is implicit. Embedded nulls require
owning-string input. Mixed-character conversion/concatenation is not provided.
Copying a branch can allocate and throw; node moves are noexcept. Moves leave
the source an empty, unstyled leaf. Self-assignment is valid; self-move is a no-op.
Concatenation regressions cover narrow/wide strings, empty operands, source
mutation, and temporary-string lifetimes.

The vector constructor and structural APIs remain private or absent. There is
no public `get`, `holds_alternative`, `is_branch`, `as_leaf`, `as_branch`, child
mutation, or iteration. Do not add iteration without a concrete use case and
authorization. The matching formatter is a friend but not implemented.
`style` queries or replaces only the root style.

## Concatenation and invariants

Only concatenation creates branches. A new branch contains two nonempty nodes;
there must never be an empty branch. `empty` describes text independently of
style. Empty leaves are concatenation identities, including styled empty leaves.
Appending to an empty left operand replaces it with the right operand and its
style. Appending an empty right operand does nothing.

`operator+=` takes its right operand by value: lvalues copy structure/share text,
temporaries move, and self-concatenation is safe. Appending to a nonempty leaf
creates an unstyled parent holding both original styled nodes. Appending to a
branch extends its vector and retains the branch's root style. A right-hand
branch remains nested. `operator+` delegates to `+=`; left-associated composition
extends the left sequence without flattening text.

Equality requires identical root styles, alternatives, text contents, child
order, and recursive structure. Pointer identity is only a fast path. A leaf
`"ab"` differs from `styled_text{"a"} + "b"`. No ordering or hashing is added.

## Plain-text extraction

`plain_text() const -> string_type` returns an independent owning string by
appending leaves in one traversal, without modifying the tree or styles.
It preserves whitespace, code units, embedded nulls, and ANSI escapes verbatim;
it does not parse or strip escapes. Empty and moved-from nodes return empty
strings. Allocation can throw. No preliminary sizing pass or public structural
traversal is introduced.

## Root styling helpers

Member `overlay(text_style)` updates only `_style`, returns
`basic_styled_text&`, and is noexcept. It does not copy or traverse the tree.
The free style-first form `overlay(style, text)` returns an owning styled value.
Free `bold`, `faint`, `italic`, `underline`, `blink`, `reverse`, `conceal`, and
`strike` enable the corresponding root emphasis through overlay.

The free functions accept styled text, character strings, and
`std::basic_string` of the matching character type. Named styled inputs copy
the tree structure while sharing leaf strings; rvalues move. String inputs
become owned leaves. Children retain their own styles. Existing style factory
calls such as `bold()` and `bold(false)` remain available. These helpers can
allocate and do not promise unconditional noexcept.

```cpp
auto copy = fmt::overlay(fmt::fg(fmt::term_color::red), ts);
ts.overlay(fmt::underline()); // In place, no tree copy.
auto phrase = fmt::bold("nested bold");
```

No individual fluent emphasis members were added to styled text. Styled-text
`absolute` convenience operations and deferred `blend` remain separate decisions.

## Absolute styles and blend contract

Absolute is a `text_style` property using bit 62. `.absolute(bool = true)` sets
or clears it and `.is_absolute()` queries it. It specifies a complete terminal
style: missing colors mean terminal defaults and absent emphasis means off.
An absolute-only style is nonempty and emits a reset. `clear` removes absolute
along with colors/emphasis. Explicit setters and overlays remain available;
absolute does not freeze the value. Overlay and style OR preserve the flag if
either operand has it.

Every public color/style `blend` overload requires at least two arguments;
internal accumulation base cases are unchanged. Each absolute style input must
specify both foreground and background: terminal-default RGB values are unknown.
`FMT_ASSERT` checks that precondition per input. Blending returns a relative
style. With assertions disabled, unspecified channels contribute nothing and
the absolute flag is dropped. The contract retains noexcept and valid C++11
constant evaluation. It does not implement deferred tree blending.

## SGR transition foundation

`to_sgr(final, initial)` compares complete terminal states. Empty foreground or
background means terminal default in either value; absent emphasis means off.
There is no inheritance in this overload. The initial absolute flag is ignored.

An absolute final style always resets, even when the two states have equal
attributes. It delegates to one-argument `to_sgr(final)`: compact mode combines
reset and attributes; noncompact mode emits the reset separately. An absolute-only
target emits `ESC[0m`.

For a relative final style, compare selective changes with reset-and-rebuild,
choose the shorter encoding, and prefer selective changes on a tie. A specified
initial color becoming default can use SGR 39 (foreground) or 49 (background).
If both channel values are already default, no selective color change is needed.
The reset candidate rebuilds only the final attributes. Identical complete states
emit nothing. Relative transitions use one unpadded combined sequence regardless
of the output-mode macro. SGR 22 clears bold/faint together, so reapply a surviving
intensity bit when necessary. Both narrow and wide output are supported.

The opening-output bound is now 60 characters for maximal noncompact absolute
output; maximal compact absolute output is 54. These are bounds for one-argument
style emission, not an implementation of extended push/pop markers.

Terry's earlier `AnsiColor.hpp` reference also compares both candidates; its
comment about count-first cost estimation remains an optimization proposal.
No XOR/popcount estimator has been implemented.

## Next work and validation limits

The next proposed increment is nested rendering: resolve inheritance before
calling the complete-state transition function, respect absolute boundaries,
and restore the enclosing state after each child. At an absolute boundary,
start from terminal defaults; descendants can inherit from that node normally.
Deferred blending needs a separate interface/semantics decision. Parsing,
visible width, serialization, and module export remain unimplemented. Public
iteration is still deferred. No website update is requested.

For the current source, strict C++11/compact color tests passed (61), strict
C++23/noncompact color tests passed (63), strict C++11/compact styled-text tests
passed (18), and the C++11 allocation regression passed. The color suite includes
all 65,536 emphasis-state pairs. These are direct GCC 13.3.0 executable checks,
not CMake/CTest or current remote-CI results. No fresh sanitizer, full-suite,
old-compiler, tablet, or guide-publication result is claimed for `08a9d3b7`.
Earlier overlay sanitizer checks remain historical evidence. See
[BuildAndTest.md](BuildAndTest.md#cloud-styledtext-validation) for commands.
