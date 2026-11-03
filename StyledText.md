# Structured styled text and format contexts

Updated November 3, 2026.

## Scope and state

Owning styled text and its formatter are integrated in `color-dev` at
`1e8fc387`; context inheritance and restoration are integrated in the accepted
ColorCtx squash `b8bf6293`. Current tip is
`2fa0bd9269a54d1fe155ff7ee95e96dfb9d0b6c3`. Local and GitHub full SHAs
matched. All 41 push-CI jobs and both guide-publication jobs passed for that
final tip. See [BuildAndTest.md](BuildAndTest.md#current-color-dev-validation).

The library's tracked tree is clean. The modernized demo is committed and
pushed; the former deferred `.tjg.cpp` file is no longer present. The active
parser implementation is separately committed/pushed on `feat/parser` at
`be665d942d0352911ea6da8678c948ed49c20970`, based on `2fa0bd92`. `feat/Formatter` and `feat/ColorCtx` are integrated
source branches, not the next development baseline. The borrowing StringView
experiment remains a separate ownership alternative. The old transition-fix
bundle and dependency history are retained in Git history; they are not needed
to continue from the accepted baseline.

Push exclusions still apply only to `feat/StyledText2`; PR triggers remain
enabled. Do not modify exclusions or cancel runs without instruction. The
current website is version 39 and documents context integration, the complete
modernized demo/display, and the current passing CI matrix.

## Representation and public API

`<fmt/styled_text.h>` defines `basic_styled_text<Char>`, with aliases
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
temporaries move, and self-concatenation is safe. An unstyled left branch extends
its vector. Otherwise concatenation creates a new unstyled parent containing
both complete operands, preserving their styles and group boundaries. A
right-hand branch remains nested. `operator+` delegates to `+=`. Neither
operator hoists identical styles or merges leaf strings.

Use `overlay(style, lhs + rhs)` to style a group. Appending to a styled group
preserves that group as a child; its style does not leak into the appended span.
The formatter accepts nested trees and matching parent/child styles without
requiring structural simplification.

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

No individual fluent emphasis members were added to styled text.
The free `absolute(text)` helper accepts the same text inputs and marks only the
returned root absolute, preserving its existing colors, emphasis, and children.
Lvalues copy; rvalues move. No styled-text member `absolute` was added.
Deferred `blend` remains a separate decision.

## Absolute styles and blend contract

Absolute is a `text_style` property using bit 62. `.absolute(bool = true)` sets
or clears it and `.is_absolute()` queries it. It specifies a complete terminal
style: missing colors mean terminal defaults and absent emphasis means off.
The free `absolute(text_style)` helper returns an absolute copy without
modifying its input. It is noexcept and supports constant evaluation where the
member setter does. An absolute-only style is nonempty; one-argument SGR
emission produces a reset. `clear` removes absolute
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

Behavior depends on `FMT_COLOR_COMPACT_SGR`:

- Compact (`1`): an absolute target uses the same emission as one-argument `to_sgr` and
  explicitly resets, including equal states. A relative target chooses the
  shorter selective or reset-and-rebuild encoding, preferring selective changes
  on a tie. Selective transitions can use SGR 39/49 to restore default colors.
  Output is combined and unpadded. SGR 22 clears bold/faint together; a surviving
  intensity bit is reapplied when needed.
- Noncompact (`0`): equal states emit nothing unless the target is explicitly
  absolute. Otherwise reset and emit the complete target with separate sequences
  and padded color components. If the initial state is already defaults, omit
  that redundant reset even for an absolute target. An absolute-only target
  from defaults therefore emits nothing. The initial absolute flag is cleared
  before testing whether the initial state is empty.

Both narrow and wide output are supported. One-argument `to_sgr` still honors
an explicit absolute flag by emitting a reset; the known-initial-state
optimization belongs to the two-argument overload only.

The opening-output bound is now 60 characters for maximal noncompact absolute
output; maximal compact absolute output is 54. These are bounds for one-argument
style emission, not an implementation of extended push/pop markers.

Terry's earlier `AnsiColor.hpp` reference also compares both candidates; its
comment about count-first cost estimation remains an optimization proposal.
No XOR/popcount estimator has been implemented.

## Nested formatter

`formatter<basic_styled_text<Char>, Char>` is implemented in `styled_text.h`.
It reuses the ordinary string specification parser for fill, left/right/center
alignment, width, precision, optional `s`, and dynamic width/precision references.
Debug presentation (`?`) and invalid string flags are rejected. Ordinary and
compiled format strings, output iterators, bounded output, and counting output
are supported. Wide entry points use `<fmt/xchar.h>`. This is a fmt formatter,
not a `std::formatter`.

Each replacement field starts from its enclosing formatting context. Relative nodes
inherit missing foreground/background and OR emphasis with the enclosing
style; specified colors replace the matching channel without promotion.
Absolute nodes use their own complete style. Compact mode emits an explicit
reset; noncompact mode can omit it when the enclosing state is already defaults.
After entering a node, the effective
state's absolute flag is cleared before passing it to descendants: entering
an absolute ancestor must not force every relative child to reset.

The formatter walks the private tree directly. Logical enclosing styles are
passed through recursion, while the format context records the last emitted
terminal style. Transitions occur only immediately before nonempty leaf output or padding.
Returning through groups emits nothing. The field always finishes with a
transition from its last emitted style to the saved enclosing style; it can be
empty. An absolute boundary is deferred until content is printed, consumed once,
and discarded if its subtree prints nothing. No extended-SGR intermediate or
push/pop markers are used. The tree is not modified.

`format()` is const; `write_node()` remains static and receives per-call state
by reference, alongside the formatting context. Logical styles and precision
remain per-call data; emitted style is stored in the context, not mutable
formatter storage. Parsing options are separate from remaining precision.

### Width, padding, and precision

`display_width()` sums leaf widths using fmt's own UTF-8 decoder and Unicode
wide/fullwidth lookup for `char`, and code-unit counts for other character types,
matching this version's ordinary string formatter. Empty and moved-from leaves
have width zero. This is not complete grapheme or terminal measurement. UTF-8
code points must not be split between leaves. Styles add no width; embedded raw
escapes are counted as ordinary text, not stripped or interpreted.

One shared precision budget applies across the whole tree. Narrow precision
limits estimated display columns without splitting a code point; wide precision
limits code units. The first code point that does not fit stops the whole text
traversal, including later siblings; no ellipsis is added. A measurement pass
finds the truncated width for padding, followed by direct rendering without
flattening the tree. The ordinary no-width/no-precision case skips measurement.

Padding receives the root style, including for empty text or precision zero.
After truncation, right padding transitions directly to the root style, then
field completion restores the saved context style. Without right padding,
completion transitions directly to that style without intermediate group
restorations. The effective root style includes inherited context properties.

Raw escapes can invalidate terminal-state assumptions. The formatter does not
capture external terminal styling. It does inherit and restore the style
established by an outer `fmt::styled` formatter using the same context. Full Unicode grapheme handling and escape parsing
remain deferred. Shared-pointer storage prevents useful constant evaluation of
the owning tree's `display_width()`.

## Format context style state

Both narrow `context` and `generic_context` store packed `uint64_t`
`color_state_`, initially zero. Internal helpers bridge that private value to
`text_style`; it is not a public numeric/archive contract. Ordinary formatters
ignore it. The styled top-level `print`/`println`/`format`/`format_to` families,
including wide and ostream entry points, seed the formatting operation with
their style. `styled_arg` and `styled_text` inherit relative styles and restore
the enclosing context style afterward, including padding. An absolute child
temporarily replaces the enclosing state and also restores it on completion.

The top-level style retains `absolute` when emitting opening SGR. Only the
copy stored in the context clears that flag: it records resulting terminal
state rather than a reset instruction. A relative top-level style does not
automatically emit an initial reset. Unknown external terminal state and raw
escapes are not tracked.

Packed state propagates through the exact-`{}` shortcut, ordinary replacement
fields, custom-value writes used by `nested_formatter`, and its alignment
buffer contexts. Direct delegation to another formatter with the same `ctx`
preserves state. A separate `fmt::format_to(ctx.out(), ...)` operation starts a
new default context; the output iterator alone does not carry styling state.
There is no global or thread-local state. Scope guards restore context metadata
on exceptions; partial output may remain.

Transition emission is shared through constexpr-capable buffer helpers,
preserving compiled formatting and constant-evaluated size calculations.
Context layouts and the internal `vformat_to` signature change. Clang-only
`FMT_MAYBE_UNUSED` annotations prevent unused-private-field errors in non-color
translation units: C++17+ uses `[[maybe_unused]]`, older Clang uses its GNU
attribute. GCC and other compilers receive an empty macro; older GCC rejects
these attributes on the fields. Test string streams use direct construction
for GCC 4.9 compatibility.

For the independent C++23 library, omit whole-message
`print(out, text_style, ...)`/`println(out, text_style, ...)` and analogous
style-taking formatting overloads. Use ordinary standard formatting with
`styled` or `styled_text` replacement fields, expressing a whole styled message
as one value/tree. Keep inheritance inside that formatter rather than modifying
`std::basic_format_context`. The fmt extension retains its existing styled-call
overloads, which justify the context integration; upstream may reject that
feature. No standard-library proposal or prototype is authorized by this design
discussion. No performance benchmark establishes overhead claims.

## Demonstration

`demo/FmtColorDemo2.cpp` adapts Terry's newer uploaded demo to current names:
`styled_text.h`, `term_color`, `overlay`, and `fmt::println`. An absolute text
span uses the free `absolute(underline(...))` helper. The older uploaded `FmtColorDemo.cpp` is not the
active demo and contains obsolete API spellings and syntax errors.

Concatenation preserves the red group as a complete span, so red applies only
to its initial inner phrase, including nested bold, and does not leak into
plain separators or later spans. The second
line carries a gray background and strikethrough except for the absolute
underline-only span; both enclosing properties resume afterward.

Run `make -C demo run` with GNU make and a C++23-capable compiler. The Makefile
uses `FMT_DIR=..`, header-only mode, and `FMT_COLOR_COMPACT_SGR=1`; it needs no
CMake or prebuilt library. Override `FMT_DIR` or `CXX` on the make command line.
Executable output is ignored locally. The Makefile is tracked despite the
repository's general Makefile ignore rule. A standalone tarball was produced,
but the voice/task interface did not expose a usable download for Terry; the
repository demo is the current delivery method.

The guide's nested padding/precision example uses
`fmt::println("{:*>12.9}", fmt::bold("hello " + overlay(fg(blue), "world")));`.
Its display is `***hello wor`, all bold, with `wor` blue: precision spans the
tree and padding uses the root style. Expected displays also show the whole
sentence underlined around bright-red `42`, and C++20 `std::numbers::pi`
rounded by stream precision to bold-green `3.14`.

## SGR parser

`<fmt/parse_styled_text.h>` on `feat/parser` defines:

```cpp
template<typename Char>
auto parse_styled_text(basic_string_view<Char>) -> basic_styled_text<Char>;
```

Narrow and wide convenience overloads accept literals and standard strings.
The result owns the text. Plain input or a single styled run produces one leaf;
multiple runs produce a flat branch. The root is absolute: an otherwise empty
absolute parent isolates relative child styles, while a single leaf retains its
own colors/emphasis and is marked absolute. Children need no individual absolute
flags. Empty input produces an empty absolute leaf. No public structural API was
added to construct these results.

SGR parameters update one current relative style in order. Reset clears it;
empty parameters, `0`, `00`, and `000` are ordinary resets, never push/pop.
A leaf is emitted only when text is present. Consecutive sequences create no
empty spans, adjacent text with equal styles is combined, and trailing SGR
without text contributes no leaf. The parser represents appearance, not exact
escape-byte round trips or the final input terminal state after trailing SGR.

Recognized codes include the eight supported emphasis bits and their off codes,
SGR 22 clearing both bold/faint, normal/bright terminal colors, 39/49 defaults,
256-color indexed values, and RGB truecolor. Semicolon and colon color forms
are supported, including omitted/default RGB color-space fields. It handles
ASCII escape syntax in either character type and preserves text code units and
sized embedded nulls without Unicode decoding.

Only ESC-prefixed CSI sequences ending in lowercase `m` are decoded. Non-SGR
commands, including uppercase `M`, and opaque OSC/DCS/SOS/PM/APC payloads remain
literal. Malformed SGR, incomplete/out-of-range supported colors, and unsigned
parameter overflow are preserved atomically without changing style. Unknown valid
parameters and unsupported colon groups are ignored; an unknown semicolon color
mode discards the remainder of that SGR because its payload boundary is unknown.
Preserved raw terminal commands can affect state outside the model. This is not
sanitization. The old `AnsiColor.hpp::Apply` was read as a reference, not copied
as the implementation or treated as authoritative policy.

### ANSI stripping

`<fmt/strip_ansi.h>` implements character-generic
`strip_ansi(basic_string_view<Char>) -> std::basic_string<Char>`, with narrow/wide
convenience overloads for literals and standard strings. It makes an owning
result directly without constructing a styled tree or decoding Unicode.

It removes syntactically complete ESC-prefixed CSI, OSC, DCS, SOS, PM, APC, and
ordinary ESC sequences, including private/unknown commands and numerically invalid
parameters whose syntax is well formed. CSI requires parameter bytes, then
intermediates, then a final byte. OSC accepts BEL or ESC-backslash termination;
other control strings require ESC-backslash. Payloads and terminators are removed
together; visible hyperlink labels outside OSC markers remain.

Standalone C0 controls (including tabs/newlines/NUL), single-byte C1 forms, and
malformed/incomplete escape prefixes remain. Scanning resumes at an interrupting
character, allowing a later independent escape to be stripped. An unterminated
control string is preserved whole, including embedded apparent escapes. CAN/SUB
cancel control-string recognition and are retained. This function does not
simulate cursor movement or erase commands and is not a security sanitizer.
Parser `plain_text()` removes decoded SGR only and retains other escapes.

## Next work and validation limits

The integrated tree/context code and guide are accepted on `color-dev`; the 41-job push matrix and
guide build/deployment passed for exact current tip `2fa0bd92`. Branch candidate results
and earlier cancelled macOS jobs are not the final acceptance evidence.

Local MSYS GCC 16.2 checks covered C++11/C++23 and both SGR modes, context
restoration, compiled formatting, and a header-only no-exceptions probe. Earlier
cloud width-helper and exhaustive inheritance checks remain useful historical
evidence, not new local results. The GCC 13/14 noinline branch-width workaround
is retained. Older strict local Release blend/union diagnostics remain open;
see BuildAndTest.md for exact validation limits. No PR-only lint/fuzz, tablet,
performance, or ABI-validation result is inferred from passing push CI.

The combined parser/stripper candidate at `be665d94` passes all 41 push-CI jobs,
including all nine macOS jobs. Local strip tests pass C++11/C++23; CMake parser,
stripper, styled-text, and formatter suites pass. Sanitizer and no-exceptions
probes pass. Candidate validation does not authorize squash integration.
See BuildAndTest.md for exact evidence and limits.

Debug formatting, deferred blending, extended push/pop parsing, full grapheme/terminal width,
serialization, styled-text module export, and public iteration remain deferred.
Appending `detail::styled_arg` to a tree is also deferred. `fmt::styled` keeps
its borrowing wrapper type while inheriting/restoring context when formatted.
