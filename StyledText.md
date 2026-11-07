# Structured styled text and format contexts

Updated November 7, 2026.

## Scope and state

Owning text, formatter/context restoration, parsing, stripping, Backslash,
and underlay/overlay operations are integrated into `color-dev` at
`10c4cd72d015380410d73fe6fcae84b58d0dc3d5`. The active library checkout is
`color-dev`, clean and pushed. The baseline includes underlay `0ba4e364` and
the table/guide/changelog commit `10c4cd72` above the earlier foundation.
`pr/color` now matches color-dev at `10c4cd72`; PR #5015 is closed and
unmerged with recorded head `dd7180a8`.

Verified November 7, 2026: all 41 push-CI jobs and both guide-publication jobs
passed on color-dev attempt 2 for `10c4cd72`; all 41 pr/color push-CI jobs
passed on attempt 1 at the same SHA. See
[BuildAndTest.md](BuildAndTest.md#current-color-dev-validation).
Public Site version 49 includes both demos, current styling semantics, the
table's styled output, and exact source/CI links. Check CI on demand; do not
cancel jobs or start periodic polling without instruction.

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

Use `underlay(style, lhs + rhs)` to supply group defaults. Appending to a styled group
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

## Root defaults and recursive overrides

Member `underlay(text_style)` fills unspecified colors in the root style,
preserves explicit colors, ORs emphasis, and returns `basic_styled_text&`.
It is noexcept and does not traverse or copy children. Formatting carries
root defaults to relative descendants without changing their stored styles.

Member `overlay(text_style)` recursively overrides specified colors and ORs
emphasis, skipping existing absolute nodes and their entire subtrees. It is
noexcept and preserves text, structure, and shared leaf strings. To override
only the root, use `ts.style(ts.style().overlay(overrides))`.

Free style-first `underlay(style, text)` and `overlay(style, text)` accept
styled text, character strings, and matching `std::basic_string` inputs.
Named styled inputs copy structure while sharing leaves; rvalues move.
Strings become owned leaves. Free helpers can allocate. The eight emphasis
helpers and `absolute(text)` use underlay on the root, preserving child styles.
The latter marks only the returned root absolute. No individual fluent emphasis
members or styled-text member `absolute` were added. Deferred blend is separate.

```cpp
auto copy = fmt::underlay(fmt::fg(fmt::term_color::white), ts);
ts.overlay(fmt::fg(fmt::term_color::red)); // Recursive override.
auto phrase = fmt::bold("nested bold");
```

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
  that redundant reset for a relative target. An absolute target always emits
  a reset and its full style, even from defaults or equal attribute states.
  The initial absolute flag does not request a reset.

Both narrow and wide output are supported. One-argument `to_sgr` still honors
an explicit absolute flag by emitting a reset. The known-initial-state
optimization never removes an explicitly requested absolute reset.

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
Absolute nodes use their own complete style and emit an SGR reset before
applying it in either SGR mode, unless there is no output. That transition is
issued immediately when the absolute node is visited after the formatter's
empty/stopped-output checks. Its absolute flag is then cleared from the resulting
terminal state before passing the effective style to descendants; relative
children do not repeat the ancestor's reset. Explicitly absolute descendants
establish their own boundaries.

The formatter walks the private tree directly. `enclosing` supplies logical
inheritance; a separate per-call `text_style terminal` records the last emitted
terminal state and is passed by reference through `write_node`. Relative
transitions occur immediately before nonempty leaf output or padding. Returning
through groups emits nothing. Absolute nodes use the immediate transition rule
above. At field completion, the formatter restores the saved enclosing context
style from the last emitted terminal state, without intermediate group restores.
The tree is not modified; no extended-SGR intermediate or push/pop markers are used.

`format` is const; `write_node` remains static. Precision state and terminal
state are separate per-call values. Format-context metadata supplies the enclosing
style; it is not a substitute for the recursive traversal's terminal-state
reference. Parsing options remain separate from remaining precision.
If measurement finds zero output width and no padding, `format` returns without
emitting SGR. Empty nodes without padding and stopped traversal are skipped.

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

`demo/FmtColorDemo2.cpp` prints the same composed line underlaid and overlaid
with white foreground, gray background, and strikethrough. Underlay preserves
explicit red/blue colors; overlay replaces relative colors. Both protect the
absolute underline-only span and its complete subtree.

`demo/TableDemo.cpp` shows per-column backgrounds and defaults, nested colors,
bold/underlined headings, bold and faint rows, an absolute red-on-white warning,
UTF-8 widths, and fixed-width padding/truncation. `cell` uses underlay to keep
explicit foreground and background choices. Width equals precision per field;
the Database explanation truncates after `CRITICAL!` with column-styled padding.

`make -C demo` builds both executables, `make -C demo run` runs both, and
`make -C demo clean` removes both. The Makefile defaults to C++23 header-only
compact SGR and accepts `FMT_DIR`/`CXX` overrides. The library remains C++11.
The user guide contains a table walkthrough and colored expected output
at `color/styled.html#table-demonstration`. The guide illustration adds one space after `東京 NAS` at Terry’s request;
this is HTML display padding, not a demo/library change. Site version 49 shows the styled
display and provides source and actual ANSI output downloads for both demos.

The nested padding/precision guide example prints `***hello wor`, all bold with
`wor` blue. Precision spans the tree and padding uses the root style.

## SGR parser

`<fmt/parse_styled_text.h>` in the consolidated `color-dev` defines:

```cpp
template<typename Char>
auto parse_styled_text(basic_string_view<Char>) -> basic_styled_text<Char>;
```

Narrow and wide convenience overloads accept literals and standard strings.
The result owns the text. Plain input is a relative leaf; no initial reset is
invented. Relative spans before the first reset can inherit an enclosing style.
The first reset that has following text starts one absolute group, with relative
children describing subsequent spans from terminal defaults. If no text precedes
that reset, the result itself is absolute without a redundant relative wrapper.
The absolute group can retain a single child; no hoisting pass is required. Empty input stays
an empty relative leaf. No public structural API was added.

Later resets only clear the current style within the existing absolute group;
they create no further nested groups. This keeps defaults in effect for all
subsequent spans, avoiding accidental re-inheritance of an outer style. For
example, `ESC[m A ESC[1m B` inside an underlined context produces plain A and
bold-only B. If text preceded the reset, that prefix retains its inheritance.
The absolute group protects the whole suffix; it is not consumed by just the
first styled leaf.

SGR parameters update the current style in order. Empty parameters, `0`, `00`,
and `000` reset normally, never push/pop. Consecutive sequences produce no empty
leaves. Equal adjacent text styles coalesce within each group; trailing SGR
without text creates no span or empty group. The parser represents appearance,
not original escape bytes or a trailing command's final terminal effect.
Selective clears before the first reset cannot cancel an unknown inherited
attribute using relative `text_style`; this remains an explicit limitation.

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

`strip_ansi(basic_styled_text<Char>) -> basic_styled_text<Char>` takes its tree by
value and strips each leaf independently. It preserves node styles, absolute
boundaries, structure, and leaves that become empty. Unchanged leaf strings remain
shared; lvalue input is not modified. No sequence is recognized across leaves.
`strip_ansi(x.plain_text())` scans the flattened string; `strip_ansi(x).plain_text()`
extracts text after leaf-local stripping. Both yield strings without explicit
tree styling, but their handling of split escapes differs.

## Next work and validation limits

The consolidated tree/context/parser/stripper code is on `color-dev`; current
exact-SHA CI and guide publication passed as recorded in BuildAndTest.md. Earlier feature
matrix and local results are historical evidence, not fresh checks of the
rewritten baseline. Current guide build/deployment passed.

The prior GCC 13/14 branch-width and GCC-13-only branch-move noinline workarounds
remain. Older strict local Release blend/union diagnostics remain validation
limits. No PR-only lint/fuzz, tablet, performance, ABI, or complete remote
compact-SGR result is inferred. Website QA rebuilt both current compact demos,
not the full regression suite. Implemented Backslash codecs are separate text
utilities, not tree formatting or SGR parsing; they have no styled-tree overload.
Their strict error behavior and constexpr support are in CppDesign.md. CSV/TSV
conversion is out of scope.

Debug formatting, deferred blending, extended push/pop parsing, full grapheme/terminal width,
serialization, styled-text module export, and public iteration remain deferred.
Appending `detail::styled_arg` to a tree is also deferred. `fmt::styled` keeps
its borrowing wrapper type while inheriting/restoring context when formatted.
