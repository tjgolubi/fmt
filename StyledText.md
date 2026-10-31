# Structured styled text experiment

Updated October 31, 2026.

## Scope and state

Active library branch: `feat/Formatter`, committed and pushed at
`eff67861708d8863e3865b7d3d573ff71fc4282e`. Local and GitHub full SHAs matched; the
library working tree is clean. It starts from `feat/StyledText2` at
`08a9d3b7a828392e1d64281a6bcc4a6e0212b73d`. It has not been squash-integrated
into `color-dev`. Committed, pushed, tested, and accepted are distinct states.
Terry explicitly authorized committing and pushing the separate coordination
handoff on `docs/Coord`.

The formatter branch adds the GNU make demo (`25d9f36d`), user-guide description
(`769891e9`), and formatter plus regression tests (`eff67861`).

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
`feat/StyledText`; the owning baseline for formatter work is `feat/StyledText2`.
The borrowing `StringView` experiment is separate. Its lifetime and constexpr
properties are not those of the owning implementation.

Push CI is deliberately excluded for `feat/StyledText2` in linux, windows, macos,
and doc; the branch spelling now matches the exclusions. PR triggers remain
preserved. Do not restore push CI without instruction. Use compact SGR and
focused local checks for experiments. The new `feat/Formatter` branch does not
match that exclusion, so its authorized push triggered all four push workflows.
See BuildAndTest.md for the latest checked state; no exclusion was modified.

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
`color-dev` still follows the squash policy in Workflow.md. Preserve exact
commits in future authorized bundle deliveries. Coordination documents are
maintained on their own branch and are not included in that historical bundle.

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
authorization. The matching formatter is implemented as a friend with private tree access;
it does not add public iteration or structural APIs.
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

## Nested formatter

`formatter<basic_styled_text<Char>, Char>` is implemented in `styled-text.h`.
Its parse function accepts only the empty format specification (`{}` or `{:}`)
and reports unsupported width, alignment, precision, and other specifications
through the normal format-string checking mechanism. It works with ordinary
and compiled fmt format strings and with output iterators. Wide entry points
use `<fmt/xchar.h>`. This is a fmt formatter, not a `std::formatter`.

Each replacement field assumes terminal defaults on entry. Relative nodes
inherit missing foreground/background and OR emphasis with the enclosing
style; specified colors replace the matching channel without promotion.
Absolute nodes use their own complete style and force a reset even when the
attributes match the enclosing state. After entering a node, the effective
state's absolute flag is cleared before passing it to descendants: entering
an absolute ancestor must not force every relative child to reset.

The formatter walks the private tree directly, using `to_sgr(effective,
enclosing)` on entry and `to_sgr(enclosing, effective)` on exit. Each child
returns to the enclosing state, and the root returns to defaults. It does not
build an extended-SGR intermediate or use push/pop markers. Empty nodes emit
nothing. Text, braces, embedded nulls, and raw escapes are copied verbatim.
Raw escapes are not interpreted or tracked, so they can invalidate assumptions
about terminal state. The formatter does not capture an external terminal
style or restore one established by an enclosing ordinary `fmt::styled` call.
The tree is not modified.

## Demonstration

`demo/FmtColorDemo2.cpp` adapts Terry's newer uploaded demo to current names:
`styled-text.h`, `term_color`, `overlay`, and `fmt::println`. An absolute text
span uses `overlay(text_style{}.absolute(), underline(...))`. There is no new
free `absolute(text)` helper. The older uploaded `FmtColorDemo.cpp` is not the
active demo and contains obsolete API spellings and syntax errors.

The outer line must remain unstyled: appending to a branch retains its root
style. The demo starts with two separately styled children so red does not
leak into plain separators or later spans. Nested bold remains red. The second
line carries a gray background and strikethrough except for the absolute
underline-only span; both enclosing properties resume afterward.

Run `make -C demo run` with GNU make and a C++23-capable compiler. The Makefile
uses `FMT_DIR=..`, header-only mode, and `FMT_COLOR_COMPACT_SGR=1`; it needs no
CMake or prebuilt library. Override `FMT_DIR` or `CXX` on the make command line.
Executable output is ignored locally. The Makefile is tracked despite the
repository's general Makefile ignore rule. A standalone tarball was produced,
but the voice/task interface did not expose a usable download for Terry; the
repository demo is the current delivery method.

## Next work and validation limits

Review and remote-CI follow-up remain. GCC 14.2 produced a Release/C++11 blend
warning treated as an error; its cause is unconfirmed. Deferred blending needs
a separate interface/semantics decision. Width/alignment/precision, parsing,
visible width, serialization, module export, and public iteration remain
unimplemented. No website update is requested or performed.

Current formatter tests pass for narrow/wide inheritance, absolute groups,
equal-state absolute entry, sibling restoration, empty/binary/long/raw text,
compiled format strings, output iterators, input preservation, rejected format
specifications, and all 65,536 parent/child emphasis pairs. Focused builds used
Debian GCC 14.2.0; Clang was not tested. See
[BuildAndTest.md](BuildAndTest.md#cloud-styledtext-validation) for exact modes,
commands, the blend warning, and the remote-CI snapshot. Historical GCC 13.3
results for `08a9d3b7` do not establish a current full-suite pass.
