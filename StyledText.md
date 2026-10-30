# Structured styled text experiment

Updated October 29, 2026.

## Scope and state

Library branch: `feat/StyledText`. Local HEAD is
`9306441b406c743df39d865dcd41f22ee0f4e86c`, with a clean library working tree.
The verified GitHub head is `54f06693bea6735b16f6ad4a8e131d8d850bc8de`.
Four local commits are not pushed:

| Commit     | Scope                                                     |
| ---------- | --------------------------------------------------------- |
| `17a87964` | Character-generic nodes, append fixes, private structure. |
| `ca36b9d1` | Shared immutable leaf strings and allocation regression.  |
| `afe46c0c` | Leading underscores and brace initialization.             |
| `9306441b` | Plain-text extraction and regression tests.               |

Push was authorized but failed: HTTPS had no Git credentials, and SSH could not
resolve `github.com`. The configured SSH remote was preserved. The GitHub
connector supports object/ref creation but was not used to recreate these
commits with different hashes. Do not claim this progress is on GitHub.
`StyledTextProgress.bundle` preserves all four exact commits and requires
`54f06693` as its existing base. Import it in a checkout containing that base:

```bash
git bundle verify /path/to/StyledTextProgress.bundle
git fetch /path/to/StyledTextProgress.bundle refs/heads/feat/StyledText
git log --oneline HEAD..FETCH_HEAD
```

Inspect local branch ancestry and work before integrating; these commands do not
switch, merge, reset, or push. The requested push remains authorized but blocked
by transport/authentication.

Commit `54f06693` excludes `feat/StyledText` from push triggers in linux, windows,
macos, and doc workflows. Pull-request triggers and other branches are unchanged;
the other push workflows already exclude this branch. Do not restore automatic
push CI without instruction. Use compact SGR and focused local checks.
Coordination updates are local and uncommitted on `docs/Coord`; they are not yet
available from GitHub. This conversation's decisions supersede older guidance.

## Representation and public API

`<fmt/styled-text.h>` defines `basic_styled_text<Char>`, with aliases
`styled_text` for `char` and `wstyled_text` for `wchar_t`. Public `char_type`
and `string_type` aliases describe the character and owning-string types.
The implementation supports C++11 in every language mode, using a manually
managed union, not `std::variant`.

Each node owns its `_style` and a private discriminator. A leaf holds
`std::shared_ptr<const string_type>`; a branch owns a vector of nodes of the
same character type. Nonempty text is constructed with `make_shared`; a null
pointer represents empty text. Strings are immutable after construction.
Copies recursively copy vectors and node styles but share the character storage.
There is no copy-on-write or shared ownership of nodes/vectors, so the storage
cannot form ownership cycles. No borrowed strings/subtrees are introduced.

Public construction supports default, owning-string, null-terminated literal,
copy, and move forms. String and literal conversion remains implicit. Embedded
nulls require owning-string input. Mixed-character conversion/concatenation is
not provided. Copying a branch can allocate and throw; node moves are noexcept.
Moves leave the source as an empty, unstyled leaf. Self-assignment is valid and
self-move assignment is a no-op.

The vector constructor and structural APIs are removed. Leaf/branch aliases and
storage are private. There is no public `get`, `holds_alternative`, `is_branch`,
`as_leaf`, `as_branch`, child mutation, or iteration. Do not restore iteration
without a concrete use case and authorization. The matching formatter
specialization is a friend; the formatter itself is not implemented yet.
`style` queries or replaces only the current node's style.

## Concatenation and invariants

Only concatenation creates branches. A new branch contains two nonempty nodes;
there must never be an empty branch. `empty` describes text, independently of
style. Empty leaves are concatenation identities, including styled empty leaves.
Appending to an empty left operand replaces it with the right operand and its
style. Appending an empty right operand does nothing.

`operator+=` takes its right operand by value: lvalues copy structure/share text,
temporaries move, and self-concatenation is safe. Appending to a nonempty leaf
creates an unstyled parent holding both original styled nodes. Appending to a
branch extends its vector and retains the branch's root style. A right-hand
branch remains a nested child. `operator+` delegates to `+=`, so left-associated
concatenation extends the left sequence. No text is flattened during composition.

Equality requires identical root styles, alternatives, string contents, child
order, and recursive structure. Shared-pointer identity is only a fast path;
separately allocated equal strings compare equal. A leaf `"ab"` differs from
`styled_text{"a"} + "b"`. Ordering and hashing are not introduced.

## Plain-text extraction

`plain_text() const -> string_type` returns an independent owning string by
appending leaves in order in one traversal. It does not modify the tree or
styles. It preserves whitespace, character code units, embedded nulls, and
embedded ANSI escapes verbatim; it does not parse or strip escapes. Empty and
moved-from nodes return empty strings. Allocation can throw. There is no
preliminary sizing pass or public structural traversal.

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

## Next work and agreed design

The next proposed increment is root-style forwarding: component and emphasis
members operate only on `_style`, and free functions such as `underline(node)`
return an independently styled value. Lvalue branches copy structure/share text;
rvalues move. Copying can allocate, so free value-returning overloads must not
promise unconditional noexcept. This increment has been discussed, not authorized
or implemented. Review ambiguous `clear`, style composition operators, and blend
operations separately; `empty` and equality retain their text/value meanings.

The primary purpose is a formatter that renders nested styles and restores the
enclosing style after children. It may access the private tree directly.
Rendering, absolute boundaries, lazy blending, parsing, visible width,
serialization, and module export remain unimplemented. Public iteration is
explicitly deferred. No website update is requested.

## Validation and preservation

See [BuildAndTest.md](BuildAndTest.md#cloud-styledtext-validation) for exact
current scope and commands. Eleven public-API tests pass in C++11 and C++23,
including plain-text extraction; the same suite passes C++11 ASan/UBSan with
leak detection disabled. The separate allocation test confirms allocation-free
large-leaf copies and structural rather than character allocation for branch
copies. Do not turn these focused results into a full-suite or remote-CI claim.
The Git bundle above preserves committed library progress; coordination edits
require separate preservation and commit/push authorization.
