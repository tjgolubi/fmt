# C++ design

[Document map](README.md) · [Workflow](Workflow.md) · [Build and test](BuildAndTest.md) · [Website](Website.md)

Updated November 7, 2026. Implemented behavior is separated below from future
design. A proposal is not permission to implement it.

## Goals and boundaries

Provide useful color and style values, nested styled text, terminal rendering,
and textual/binary style serialization. User themes and future Taskwarrior
integration motivate the design; application-specific restrictions belong in
applications.

Convenience is a major library design goal. Constructors, fluent members, and
free functions deliberately overlap so callers can choose a concise form that
fits their expression or configuration-driven code. Document those choices.

The two sequential deliverables are described in [README.md](README.md). The
first release extends {fmt} and must support C++11, including the public color
variant API. C++23 is the current development mode, not its minimum requirement.
The later independent release requires C++23 and uses standard formatting and
`std::variant`. It excludes {fmt}'s formatting engine, unrelated extensions, and
pre-C++23 compatibility machinery.

Both retain `<fmt/color.h>` and the styling API in namespace `fmt`; users select
the installation through their include path. Shared-source intent covers common
styling operations, not arbitrary fmtlib APIs, identical integer representations,
or binary compatibility. Standard formatting requires the corresponding
`std::formatter` support; a {fmt} formatter alone does not supply it.

During the {fmt} stage, preserve upstream compatibility provisions, formatting
machinery, and unrelated code. Keep production styling changes in `color.h`
where practical, extending that boundary only for a concrete need. Follow the
existing source style; the later independent library follows standard-library
naming and API conventions.

## Implemented foundation

The current `color-dev` tip is `10c4cd72d015380410d73fe6fcae84b58d0dc3d5`.
It includes the foundational styling contribution, owning styled-text formatter,
format-context inheritance/restoration, Backslash utilities, underlay defaults,
recursive overlays, and the table demo with its user-guide walkthrough. The complete
fmt release supports C++11. The upstream baseline is `4ed35350`; see
[README.md](README.md#current-baseline). Source and tests establish behavior;
private numeric representations are not stable contracts.

Use `term_color`, `emphasis::strike`, `strike()`, and `is_strike()` in new
examples and tests. `terminal_color`, `emphasis::strikethrough`,
`strikethrough()`, and `is_strikethrough()` remain compatible aliases or
forwarders. Preserve their use in older upstream tests and compatibility checks.

### Colors and numeric representations

- `fmt::color` is a namespace of 141 `constexpr rgb` constants retaining the
  former names and RGB values, plus 16 `term_*`/`term_bright_*` RGB constants
  matching the fixed terminal conversion palette.
- `rgb` stores three channels. Explicit packed construction takes the low
  24 bits as `0xRRGGBB`; explicit conversion returns `0x00RRGGBB`.
- `no_color_t`/`no_color` denote unspecified color, distinct from specified
  black. They do not themselves store a terminal-reset instruction; rendering
  context determines whether a missing channel inherits or means terminal default.
- `term_color` has zero-based indices 0–15. Normal foreground/background SGR
  codes are 30–37/40–47; bright codes are 90–97/100–107.
- `gray_color` accepts an `int` index, stores it privately as a signed byte,
  exposes `value_type = uint8_t`, and has indices 0–23 and equal RGB channel
  levels `8 + 10 * index`. Indexed SGR uses palette index `232 + index`.
- `cube_color` exposes `value_type = uint8_t`, stores signed-byte channels
  privately, accepts/returns `int` channel values, and has channels 0–5 and
  cube index
  `36*r + 6*g + b` in 0–215. RGB levels are `{0, 95, 135, 175, 215, 255}`;
  indexed SGR uses `16 + index`.

The numeric constructors and cube setters assert on invalid inputs and
clamp when assertions are disabled; they are noexcept and do not throw. The
combined cube-index constructor still accepts `unsigned` and clamps before
decomposition. See the integrated-helper section for exact ranges. Both palette
colors implicitly convert to `rgb`, as do terminal colors.
Terminal conversion uses this fixed Xterm-default mapping, in index order:

| Indices | RGB values                                                   |
| ------- | ------------------------------------------------------------ |
| 0– 3    | `(0,0,0)`, `(205,0,0)`, `(0,205,0)`, `(205,205,0)`           |
| 4– 7    | `(0,0,238)`, `(205,0,205)`, `(0,205,205)`, `(229,229,229)`   |
| 8–11    | `(127,127,127)`, `(255,0,0)`, `(0,255,0)`, `(255,255,0)`     |
| 12–15   | `(92,92,255)`, `(255,0,255)`, `(0,255,255)`, `(255,255,255)` |

This mapping cannot discover a user's terminal theme. `underlying_type<T>` and
`to_underlying()` expose numeric values: `int8_t` for terminal; `uint8_t`
for cube, gray, and emphasis; `uint32_t` for RGB and `color_t`; `unsigned` zero
for `no_color_t`; and `uint64_t` for styles. `narrow_cast` is unchecked; `enum_cast` expresses
enum-related casts. Numeric helpers are not archive interfaces.

### RGB conversion and blending

Free `to_rgb(T)` returns `rgb` for the four plain color types, constrained by
`detail::is_color` or the C++23 `Color` concept. `to_rgb(no_color_t)` preserves
the sentinel. `to_rgb(color_t)` returns `color_t`, holding RGB for specified
input and unspecified otherwise. Inputs are unchanged; all are `noexcept`
and support C++11 constant evaluation. Implementation uses `rgb(color)` rather
than braced conversion to satisfy old compiler paths.

Public `blend` overloads require at least two arguments (integrated on `color-dev`). Internal recursive
accumulators retain their base cases. Free variadic `blend` supports:

- Plain colors only: return an equal-weight RGB channel average.
- A `color_t` or `no_color` argument: return `color_t`, ignore unspecified
  inputs, return unspecified for none specified, preserve the kind of one
  specified color, and return averaged RGB for two or more.
- Styles only: return `text_style`, averaging foreground and background
  independently with those same unspecified/kind rules, and ORing emphasis bits.

All channels are summed before division; fractional results round down. Inputs
are unchanged. Overloads reject non-colors and mixed raw-color/style arguments.
They are `noexcept` and constexpr in C++11 via recursive helpers; gated C++23
implementations use concepts and loops. On `color-dev`, every absolute
style input must specify both colors; `FMT_ASSERT` checks that precondition.
Unknown terminal-default colors cannot be averaged. With assertions disabled,
missing channels contribute nothing. The result is relative, clearing absolute.
No alpha compositing or nearest-palette demotion is implemented.

For example, terminal red converts to `rgb{205, 0, 0}` and `gray_color{12}`
to `rgb{128, 128, 128}`. Blending a bold terminal-red foreground with
`fmt::underline(fmt::fg(fmt::gray_color{12}))` yields foreground RGB
`{166, 64, 64}`, unspecified background, bold, and underline. This style-copy
factory expression is constexpr in C++14 and later; use `const auto` for the
second and combined values in a C++11 example.

{fmt}'s existing `underlying` converts enums through `std::underlying_type`.
Our `to_underlying` overloads have a different name and also handle color/style
classes, so there is no declaration conflict. C++23 `std::to_underlying` is an
enum-only facility in `<utility>`.

### Styles, composition, and containers

The non-explicit constructors are:

```cpp
text_style(emphasis emp = emphasis{}, color_t fg = no_color,
           color_t bg = no_color) noexcept;
text_style(color_t fg, color_t bg = no_color,
           emphasis emp = emphasis{}) noexcept;
```

They support foreground-first, emphasis-first, background-only, and default
empty construction. All plain colors and the unspecified sentinel implicitly
convert to `color_t`. Free `fg(color_t)`/`bg(color_t)` are convenience factories;
ADL finds them for color arguments from namespace `fmt`.

`emphasis | emphasis` returns `emphasis`. RGB OR combines corresponding bits.
Color/style `|` and `|=` use unspecified colors as identity, preserving the
other kind. Two specified colors promote to RGB and OR corresponding channel
bits, even for identical palette values; terminal overlaps do not throw.
Style channels are handled independently and emphasis bits are ORed. Operators
are `noexcept`; mutating style/color operations are constexpr in supported
C++14+ paths. Strict rejection is not the chosen policy.

Prefer constructors and fluent setters to choose colors, and free `blend()`
to average them. OR is cheaper arithmetic but retains bright channel bits.
No performance benchmark has established an end-to-end comparison with upstream.
Compilers can fold constant fluent operations; constexpr contexts guarantee
constant evaluation where supported.

RGB, `color_t`, and `text_style` support comparisons and `std::hash`. Ordering
uses unsigned packed values; RGB orders red, green, then blue, not brightness.
Hashes are not persistent encodings. Suitable C++20 libraries provide `<=>`;
fallbacks provide six ordinary comparisons. A union needs explicit packed-value
comparison rather than defaulted memberwise comparison.

### Fluent component and attribute access

Const getters `fg()`, `bg()`, and `emph()` return values. Empty color channels
return unspecified, empty emphasis returns `emphasis{}`, and each returned
`color_t` is independent of the style. Setters `fg(color)`, `bg(color)`, and
`emph(attributes)` replace their whole component and preserve the others.
Passing `no_color` clears a channel; `emphasis{}` clears all attributes.

`has_fg()`, `has_bg()`, and `has_emph()` distinguish unspecified from specified
black and detect any emphasis bit. Const `empty` is true only when both
colors are unspecified, all emphasis is unset, and absolute is disabled.
An absolute-only style is nonempty. Fluent `clear` also removes absolute and restores
the default style, returns `text_style&`, and supports repeated clearing and
reuse. It does not emit ANSI output.

All eight attributes have `is_*()` queries and setters taking
`bool enable = true`. Setters return `text_style&`, change only their own bit,
and support chaining. Bold and faint are independent and both render when set;
their appearance depends on the terminal. Retain conceal and omit flash in
{fmt}; the later independent library replaces conceal with flash.

All accessors are `noexcept`. Getters, presence queries, and `empty()` are
C++11 constexpr; mutating setters and `clear()` use `FMT_CONSTEXPR` for C++14+
where supported. Older component/presence names remain forwarding aliases.

### Public color variants and internal representation

`color_t` uses a discriminator and union in every supported mode, including
C++11. Its alternatives/kinds are `no_color_t`/`none` (0), `rgb`/`rgb` (1),
`term_color`/`term` (2), `cube_color`/`cube` (3), and `gray_color`/`gray` (4).
Default construction is unspecified. Construction and
assignment from alternatives are implicit and `noexcept`.

Type/index `get` preserves all four const/reference categories and reports
`format_error` on a mismatch. `get_if` returns a pointer or null for a wrong
kind/null input; `holds_alternative` classifies by type. Single-color `visit`
forwards visitor and color category and requires the same result type for all
five alternatives. Values, references, void, and propagated exceptions work.
Access/classification/visitation support C++11 constant evaluation; assignment
uses supported C++14+ paths.

References and pointers must not outlive the owner or a change of alternative.
Copy an alternative extracted directly from a temporary style getter. Public
reference access is distinct from returning a reference to a temporary copy.

Styles pack each color as 24 payload bits plus three discriminator bits:
foreground bits 0–26, background 27–53, emphasis 54–61. Bit 62 is
absolute; bit 63 remains unused.
`to_underlying` and internal `detail::unpack_color` reflect this encoding; object
size does not establish ABI or persisted-data compatibility.

### Compiler constraints

ANSI generation uses append helpers, visitation, and an emphasis-to-SGR table.
RGB and indexed-palette parameters retain three-digit padding in default
output; compact output omits padding and combines components into one sequence. Packed colors
and styles have changed from upstream; object size alone cannot establish ABI
or persisted-data compatibility.

Preserve these targeted compiler provisions:

- Initialize union alternatives with parentheses: older GCC/Clang reject
  braced copying of the empty alternative. Keep the user-provided empty default
  `color_t` constructor; the defaulted form failed on an older compiler.
- Keep test workarounds restricted to affected compilers: GCC before 5 lacks
  the tested trivial-copy trait and needs static constexpr objects for the
  constexpr pointer checks. Native MSVC 19.29 through 19.44 uses a const-reference
  empty-sentinel visitor overload to avoid its constexpr empty-union copy bug.
  Outside those gates, tests retain the original by-value visitor and
  automatic objects.
- Defaulted comparisons locally suppress Clang's spurious
  `-Wzero-as-null-pointer-constant` warning; surrounding code retains the warning.
- Fallback comparisons call `to_underlying` **unqualified**, so argument-dependent
  lookup finds the text-style friend overload. Qualified template lookup at that
  point selects an invalid generic cast for `text_style`.
- Keep unsigned emphasis/SGR arithmetic, correctly sized masks, and deliberate
  narrow casts. Local compilation without CI's strict warnings is insufficient
  evidence of compatibility.
- Keep `rgb(_data.terminal)`, `rgb(_data.cube)`, and `rgb(_data.gray)`
  parenthesized during promotion: Clang 3.6 rejects the equivalent braced
  conversion. Preserve the gated terminal-palette initialization/conversion
  paths, C++11 conditional implementations of visit/unpack, and C++14 switch
  implementations. Literal `constexpr` is used where C++11 permits it.
- C++11 assertions must not invoke release-mode `FMT_ASSERT` on valid paths.
  Test self-composition uses `self.operator|=(self)` to avoid Clang's
  self-assignment warning. Disabled-exception and C++20 module paths matter.

### Compatibility with upstream

The core styling changes are in `include/fmt/color.h`; the ostream addition is
in `include/fmt/color-ostream.h`, with module registration in `src/fmt.cc`.
Deliberate changes include RGB constants replacing the named-color enum,
explicit packed RGB construction, emphasis OR's return type, terminal renumbering, the public
variant API, automatic terminal promotion, and changed packed representations.
The guide records behavior changes without treating every one as a defect.

`test/UpstreamCompatibility` at `21eba01b` verifies the earlier Palette-era
header from `d9d64d36`, not the current `color-dev` header, with upstream tests
and workflows. Six terminal-overlap exception assertions become exact
RGB-result checks, four discarded nodiscard results are explicit, and two
documentation signatures use `color_t`. All other upstream tests are unchanged.
All 42 CI jobs passed; see [BuildAndTest.md](BuildAndTest.md#upstream-compatibility-branch).
Do not claim every original test passes unchanged or infer ABI/performance
compatibility from passing source tests.

Terminal renumbering remains negotiable during upstream review. If maintainers
require SGR-valued enum constants, retain that representation in the {fmt}
deliverable and defer renumbering to the independent library. This remains a
contingency, not a request received or accepted.

### Styled ostream output

`<fmt/color-ostream.h>` provides styled `print`, `println`, `vprint`, and
`vprintln` for narrow and wide streams, plus ADL-found insertion of
`styled(value, style)`. `print`/`println` have separate `std::ostream&` and
`std::wostream&` overloads using direct `format_string`/`wformat_string` types;
this avoids Clang 3.6 dependent-alias deduction failures. The `v` overloads use
`std::basic_ostream<Char>&` and matching argument packs. The bridge includes
`color.h`, `ostream.h`, and `xchar.h`, leaving the existing headers independent.

Whole-message formatting adds a reset for nonempty styles and places a newline
after it. Empty styles produce plain output. Reset restores terminal defaults,
not an enclosing style. Narrow formatting retains the existing Windows console
path; wide formatting uses the existing buffer writer. Insertion uses ordinary
stream formatting and preserves field width while writing SGR. Detailed lifetime
and exception-recovery rules are recorded below.

The bridge supports C++11, header-only use, installation, and C++20 module
export. All 41 current push-CI jobs and both guide-publication jobs passed
for `10c4cd72`; see BuildAndTest.md for exact evidence. The separate historical compatibility result
at `21eba01b` does not validate the current header or bridge.

## Integrated styling helpers

The following features are implemented in the consolidated `color-dev` contribution,
with regression tests and guide documentation. They are not proposals.

- `FMT_COLOR_COMPACT_SGR=1` selects one combined SGR sequence per style with
  unpadded decimal parameters. Undefined or `0` preserves legacy output.
- `to_sgr<Char>(style)` returns an owning `std::basic_string<Char>` containing
  opening style sequences; `Char` defaults to `char`. Style values remain
  independent of rendering; no `text_style::str()` member is provided. Empty styles return an empty string; no reset is appended.
- `sgr_reset<Char>()` returns a constexpr, noexcept fmt string view into static
  storage. The local-static implementation is gated by
  `__cpp_constexpr >= 202211L`; the fallback supports C++11 constant evaluation.
- `wrap_sgr(style, text)` copies text between opening SGR and reset, returning
  plain text for an empty style and an empty string for empty text. The template
  takes `basic_string_view<Char>`; a narrow convenience overload accepts literals
  and strings through conversion. Text and embedded nulls/escapes are preserved.
- Member `overlay(other)` mutates a style; free `overlay(base, overrides)`
  returns a copy. Specified incoming colors replace matching channels without
  promotion, unspecified channels leave existing colors, and emphasis is ORed.
- Member `underlay(defaults)` fills unspecified style colors, preserves explicit
  colors, and ORs emphasis. Free `underlay(base, defaults)` returns a copy;
  for style values it equals `overlay(defaults, base)`. Either absolute input
  makes the result absolute. These style operations are noexcept and constexpr
  where C++14 mutation is supported, with runtime C++11 use.
- Free `bold`, `faint`, `italic`, `underline`, `blink`, `reverse`, `conceal`,
  and `strike` take a style by value and return a changed copy. Their Boolean
  defaults to `true`; `false` clears only that bit. They are noexcept and use
  `FMT_CONSTEXPR` for supported C++14+ constant evaluation, retaining C++11 use.

- `operator|=(emphasis&, emphasis)` modifies its left operand, returns
  `emphasis&`, and is noexcept. `FMT_CONSTEXPR` preserves C++14+ constant evaluation; nonmutating
  `operator|` remains usable in C++11 constant expressions.
- `rgb(int, int, int)`, `gray_color(int)`, cube channel construction and setters
  accept signed numeric inputs. RGB channels remain public `uint8_t`; gray and
  cube use private signed-byte storage. Cube getters return `int`.
  The single-index cube constructor still accepts `unsigned`.
- Invalid RGB/gray/cube numeric arguments trigger `FMT_ASSERT`. If assertions
  are disabled, inputs clamp to their nearest legal endpoints: RGB `0..255`,
  gray `0..23`, cube channels `0..5`, and combined cube indices `0..215`.
  Clamp a combined index before decomposition. These constructors and setters
  are noexcept and no longer throw or use the disabled-exception termination
  path. Valid construction remains constexpr in C++11.
- `term_color` and its `underlying_type`/`to_underlying` use `int8_t`; named
  indices stay `0..15`. RGB conversion asserts both lower and upper bounds.
  Packed RGB, color/style representations, emphasis masks, and the public
  gray/cube numeric representations remain unsigned. Preserve `std::size_t`
  at standard-library size interfaces; do not mechanically replace bit types.
- `<fmt/color-ostream.h>` provides ADL-found insertion of `styled(value, style)`
  into narrow and wide streams. It uses the value's normal stream insertion,
  including flags, precision, padding, and custom stream-only types. Opening
  SGR and reset use `write()` so they do not consume field width. Empty styles
  emit no reset; nonempty styles around empty text still emit SGR and reset.
  The wrapper borrows its value; stored wrappers require a surviving referent.
- Insertion attempts a reset if the value throws, suppresses reset-recovery
  failures, and rethrows the original exception. It cannot guarantee a reset
  on a failed stream. The normal trailing reset is outside the recovery block.
  Gate recovery with the current `FMT_USE_EXCEPTIONS` macro.
- Styled `print` and `println` have separate narrow/wide overloads; `vprint`
  and `vprintln` take `std::basic_ostream<Char>&`, with matching packs.
  The bridge includes `xchar.h`; narrow output retains the existing console
  path, wide output uses the existing buffer writer. Formatting functions use
  fmt specifications and preserve stream format settings, unlike insertion.

These public signature/type changes can affect callers: unsigned variables may
need a checked conversion before signed brace construction; the old public cube
`chan_type` alias is now private. Do not claim numeric or source compatibility
for all changed interfaces.

Use fmt string views for C++11 support. In this version they have `size()` but
no `empty()`. Explicit character template arguments are needed when deduction
cannot convert a literal to `basic_string_view<Char>`. SGR conversion allocates
and is not noexcept. Keep style values independent of their rendering.

On `color-dev`, `sgr_max_length` is 60 characters, excluding a null terminator, for maximal noncompact output including the leading reset. Maximal
compact absolute output is 54 characters. The reset alone is four characters;
extended push/pop markers are not implemented.

Compatibility provisions found during candidate CI are retained: default the
compact macro to zero; cast signed payloads explicitly; perform terminal SGR
arithmetic as `int`; emit the reset-storage out-of-class definition only before
C++17; and avoid copying the empty union member in the SGR visitor during MSVC
constant evaluation. Tests explicitly initialize const stream-only values and
avoid shadowing and self-assignment warnings.

Current validation is scoped to `color-dev` at `10c4cd72`: all 41 push-CI
jobs and both guide-publication jobs passed on attempt 2. All 41 pr/color
push-CI jobs passed on attempt 1 at the same commit. Upstream PR #5015 is
closed and unmerged with recorded head `dd7180a8`.
A full remote compact-SGR matrix is not claimed. Public Site version 49 shows
the current semantics, both demos, and exact source/CI evidence. See
[BuildAndTest.md](BuildAndTest.md#current-color-dev-validation)
and [Website.md](Website.md#current-publication).

### Conditional attribute factories

The free factories `bold`, `faint`, `italic`, `underline`,
`blink`, `reverse`, `conceal`, and `strike` take `bool enable = true`. Each
returns a `text_style` with only its attribute set
and both colors unspecified; false returns an empty style. They are nodiscard,
noexcept, and constexpr in C++11. Use `styled(value, fmt::bold())` or
`fmt::bold(userWantsBold)` for concise unconditional or conditional styling.

The existing `bold(text_style, bool = true)` and equivalent overloads still
derive a copy, preserving colors and unrelated attributes. Their mutating
implementation supports constant evaluation in supported C++14+ paths and
runtime use in C++11. Doxygen and the guide distinguish the two overload sets.
Tests cover all eight factories with default, true, and false arguments,
unspecified colors, return types, noexcept, C++11 constexpr, and styled output.

Terry reported that the TaskWarrior team liked the factories, used them where
beneficial, and retained passing application tests. This is reported evidence,
not a freshly executed application test for `color-dev`. The guide explains
convenience as a major design goal.

## Integrated structured text and planned work

Owning text, the corrected formatter, context inheritance/restoration, parser,
and stripper form the foundational extension `35de08cc`; current `color-dev`
at `10c4cd72` also includes formatting, Backslash, underlay, and table changes above
upstream `4ed35350`. Continue from `color-dev`; earlier integration commits are
historical, not dependencies to replay. See StyledText.md for invariants and
BuildAndTest.md for exact-SHA validation and remaining limits.

The owning tree uses a custom union in all language modes. Do not restore a
C++17 variant gate or strict style-OR policy. Experimental alternatives still
need authorization for implementation or website inclusion.

### Format context state

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

### Structured styled text

`basic_styled_text<Char>` owns a node-local style and either shares immutable
`basic_string<Char>` storage or owns a vector of same-character nodes. The
aliases are `styled_text` and `wstyled_text`. Empty leaves use null shared
pointers. Sharing is limited to strings: no copy-on-write, borrowed subtrees,
or shared vectors. Direct union leaf initialization is implemented.

Concatenation alone creates branches; empty branches are prohibited. Moves
reset the source to an empty, unstyled leaf. `+=` extends only an unstyled left branch; otherwise it creates an unstyled
parent holding both complete operands. Styled groups retain their scope, and
right-hand groups remain nested. Use `underlay(style, expression)` for group defaults;
concatenation does not merge equal styles or extend a styled group. Equality compares exact
structure, styles, and text, independently of shared-pointer identity.
`plain_text` concatenates code units without interpreting escapes.

Public structural access and iteration remain deferred. The matching formatter
is implemented as a friend with private access to the tree. `style` gets or
replaces the root style;
member `underlay(text_style)` fills unspecified root channels and ORs emphasis
without changing descendants. Member `overlay(text_style)` applies overrides
recursively, skipping existing absolute nodes and complete subtrees. Both
return a reference, are noexcept, and preserve shared leaf strings. Use
`text.style(text.style().overlay(overrides))` for a root-only override.
Free style-first underlay/overlay and eight emphasis helpers accept owning text,
character strings, and `std::basic_string`. Named values copy structure;
rvalues move. Emphasis and absolute helpers use root underlay. Free helpers can
allocate. No individual fluent emphasis members were added to styled text.
The borrowing `StringView` experiment is a separate alternative,
not the active owning baseline or a guarantee of shared constexpr capabilities.

### Absolute style values and transitions

Absolute is implemented on `text_style`, not as a styled-text member. It describes
a complete terminal style: absent colors are terminal defaults, unset emphasis
is off. `.absolute(bool = true)` and `.is_absolute()` manipulate/query bit 62.
Explicit setters and overlay still work. Overlay and style OR preserve absolute
if either operand has it; clear removes it. Blend follows the assertion contract
above and returns a relative style.

`to_sgr(style)` emits a leading reset for absolute styles, including absolute-only
styles. Compact output combines reset and attributes; noncompact output emits
the reset separately. `to_sgr(final, initial)` treats both values as complete
terminal states, with no inheritance. The initial absolute bit is irrelevant.
Compact transitions honor absolute targets with a reset; relative targets choose
the shorter selective or reset-and-rebuild encoding, preferring selective on a
tie. Missing target colors can require SGR 39/49. Output is combined/unpadded.
Noncompact transitions reset and fully specify changed states with separate
sequences/padded colors, but omit a redundant reset from known defaults,
for relative targets. Absolute targets always reset in both modes, even from
defaults or equal attribute states. Equal relative states emit nothing. One-argument
`to_sgr` continues to honor explicit absolute resets in either mode.

The formatter now resolves relative inheritance and enforces absolute tree
boundaries before calling the complete-state transition function. Relative
styles inherit colors and OR emphasis; absolute styles replace the entire
inherited state. Reset emission follows the mode-specific rules above. Each
replacement field inherits its format-context state. Only a new independent
operation starts from assumed defaults; external state is not queried. Descendants inherit from an absolute node without
repeating its boundary reset. Logical styles are tracked separately from the
last emitted state: relative transitions wait until text or padding; an absolute
node emits its reset/style immediately when visited unless there is no output.
Clear its absolute flag after emission so relative descendants do not repeat it.
Returning from a group emits nothing; restore the saved enclosing context at
field end. Traversal carries the terminal state by reference, separately from
logical inheritance and precision state. Narrow/wide fill,
alignment, width, precision, optional `s`, and dynamic width/precision are
implemented. Padding receives the root style. Precision stops the whole traversal
when the next code point will not fit; unwind without intermediate transitions.
No public traversal or extended-SGR interchange rendering is introduced.

The free `absolute(text_style)` returns an absolute copy; `absolute(text)` returns
owned styled text with only its root marked absolute. Named inputs copy and
rvalues move, matching overlay's ownership rules. The style overload is noexcept
and conditionally constexpr like its member counterpart.

`display_width()` reuses fmt UTF-8 decoding and width lookup per narrow leaf;
wide strings count code units. Precision and padding match those ordinary string
rules, with one shared budget across the tree. Keep UTF-8 code points within a
leaf; raw escapes are not interpreted. A private noinline branch-width helper
avoids GCC 13/14 recursive-inlining union warnings while leaf calculation stays
inline. Shared-pointer access prevents useful constexpr tree measurement.

### Deferred blending and other future work

Deferred styled-text blend remains separate from underlay and overlay. Its nested
semantics and interface need agreement before implementation. Earlier
`styled_text& blend()` proposals do not authorize code. Debug string presentation,
readable style parsing, extended push/pop parsing, full grapheme/terminal width,
serialization, and module export remain
future work. `fmt::styled` keeps its borrowing wrapper type but now inherits
and restores the format-context style; accepting that wrapper in concatenation
is deferred until requested.

For the later independent library, reconsider factory names alongside
`using enum fmt::emphasis`: imported enum names can hide unqualified factories;
qualified `fmt::bold()` remains clear.

## Integrated SGR parser and ANSI stripping

The consolidated `color-dev` includes `<fmt/parse_styled_text.h>` and
character-generic `parse_styled_text`. It produces owned
relative spans before the first reset and one absolute group for the suffix,
applies standard SGR state sequentially, and preserves non-SGR/malformed escapes literally. Unknown
valid parameters are ignored. No extended push/pop parsing is implemented;
leading-zero reset codes are ordinary resets. See StyledText.md for exact API,
color forms, malformed-input policy, and tree shape, and BuildAndTest.md for the
current exact-SHA CI state. Parsed `plain_text()` removes decoded SGR only.
A reset before any text makes the result itself absolute; repeated resets clear
styles within the same group, without creating new groups. No reset in the input
means no absolute flags are invented. Selective clears before any reset cannot
cancel unknown inherited attributes; relative styles lack that representation.
The guide and regression expectations follow this reset-group model.

The vector-move workaround in `basic_styled_text` is gated to GCC 13 only. It
isolates the branch move in a noinline helper to avoid the reproduced optimized
union warning, preserving other compilers' original path and inline leaf moves.
The earlier parser candidate passed 42 focused tests in eight configurations
and its 41-job push matrix. Those results do not replace validation of the
rewritten `color-dev`; see BuildAndTest.md. No warning is suppressed or ABI/performance result inferred.

`<fmt/strip_ansi.h>` now implements the separate character-generic stripper.
It removes syntactically valid ESC-prefixed sequences regardless of semantic
validity or private/unknown commands. Standalone C0, single-byte C1, and
malformed/incomplete prefixes remain. Control-string payloads and terminators
are removed together; incomplete strings remain opaque and are preserved whole.
String input directly produces an owning string without building a tree. The
styled-text overload returns a new tree, preserving styles and structure while
stripping leaves independently, including leaves that become empty. It never
recognizes escape sequences spanning leaves. Flatten before stripping to scan
across those boundaries, or call `plain_text` afterward to discard explicit styles.
It neither simulates terminal editing nor sanitizes text. Both APIs are integrated
in `color-dev` at `dd7180a8`.

### Independent-library buffer discussion

The next phase ports the styling layer, not fmt's unrelated formatting features.
C++20 supplies most language facilities; the planned minimum remains C++23,
including standard print/println and simpler standard utilities. Do not copy or
rebuild fmt's whole formatting engine or modify standard format contexts.

Terry favors a small own memory buffer with a limited append/copy interface for
SGR construction. His existing fixed-capacity static string is a possible
reference, but a complete string interface is unnecessary. No implementation or
final buffer capacity has been authorized. In this fmt version, memory buffers
default to 500 inline elements; SGR code already uses its explicit 60-element
bound. Inline storage lives in the object, on the stack only when the object is
stack-local. Standard string SSO is implementation-dependent and is not enough
to guarantee allocation-free SGR construction. A returned owning string may
still allocate even when the temporary SGR buffer does not.

Styled-text leaves continue to use immutable shared basic strings, and rendering
writes to output iterators; do not impose a fixed capacity on arbitrary text.
Audit retained buffer operations before replacing them. Other fmt dependencies
include string views, formatter/parse contexts, string-spec parsing and dynamic
arguments, padding/copying, Unicode decoding/width estimates, assertions/errors,
and compatibility macros. Standard views, algorithms, concepts, and utilities
replace many; Unicode measurement and specification handling need a separate
plan. Standard-library facilities expose no general public UTF-8 decoder or
width-measurement helper. The agreed standard-context boundary remains unchanged.

## Backslash escaping and unescaping

Implemented in `<fmt/backslash.h>` and integrated into `color-dev` at `dd7180a8`:

- `backslash_escape(text)` and `backslash_unescape(text)` return owning
  `std::basic_string<Char>` values.
- `backslash_escape_to(out, text)` and `backslash_unescape_to(out, text)` accept
  output iterators and return the advanced iterator, without an intermediate
  string. Input views, literals, and strings have narrow/wide convenience overloads.
- The common implementation is character-generic and C++11-compatible. It uses
  {fmt}'s `FMT_CONSTEXPR` for C++14-capable iterator algorithms and
  `FMT_CONSTEXPR_STRING` for owning strings where C++20 support is available.
  Length-bearing views are needed for pre-C++20 constant evaluation because of
  {fmt}'s null-terminated view constructor. Use `std::basic_string_view` in the
  future independent implementation; current `fmt::basic_string_view` is a
  distinct class, not an alias to the standard type.

Encoding replaces ASCII C0 controls and DEL with readable escapes. Named forms
are `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, and `\v`; remaining controls use
exactly two lowercase hex digits after `\x`. Keep NUL as `\x00` and DEL as
`\x7f`. Emitting `\0` would need lookahead to avoid consuming a following octal
digit, so Terry retained `\x00`. Backslashes become `\\` and double quotes
become `\"`. Ordinary apostrophes, question marks, and non-ASCII code units,
including UTF-8 bytes, remain unchanged. No surrounding quotes are added.

Decoding recognizes all these forms, plus `\'`, `\?`, two-digit hex in either
case, and one to three octal digits with values up to 255. Further digits are
ordinary text. `\e` is an intentional ESC extension, and hex decoding is
fixed-width rather than C++ source-literal maximal munch. Unicode escapes,
C++23 braced numeric escapes, unknown escapes, malformed hex, octal overflow,
and trailing backslashes throw `fmt::format_error`. The earlier preservation
policy is superseded. Decoding is one pass; decoded backslashes are not reparsed.

The `_to` overloads retain output written before an error; the offending escape
writes nothing. Output must not invalidate or overwrite unread input. In
particular, appending to the input string can invalidate its view. Input-iterator,
destination-string, and in-place overloads remain deferred. No styled-tree
codec overload is implemented; pass `plain_text()` or formatted output as needed.

For valid input, unescaping escaped text recovers the original code units,
including NULs. The guide has a compact Markdown escape table; website version
49 presents the functions as implemented and links to that reference. CSV/TSV
conversion belongs elsewhere. {fmt} debug presentation and `std::quoted` do not
supply this codec's strict general-purpose inverse.

## Rendering, parsing, and serialization

Keep three responsibilities distinct:

1. `render()` produces an interchange string with extended SGR push/pop markers.
2. A terminal formatter emits ordinary ANSI SGR directly from the tree, resolving
   inheritance without calling `render()` or building an extended-SGR intermediate.
3. The implemented ANSI parser consumes ordinary SGR into owned spans and a
   reset-protecting absolute suffix group; extended push/pop parsing remains
   future work.

The legacy markers are `ESC[00m` for push and `ESC[000m` for pop, distinct from
`ESC[0m` reset. Verify behavior against `AnsiColor.hpp` and `AnsiRender.hpp` before
implementing compatibility; they are references, not wholesale implementation
instructions. Sequential SGR parsing follows terminal state changes: reset
clears state, foreground reset clears foreground, and intensity reset clears
bold/faint. Do not use style OR composition to interpret those commands.
Push/pop save and restore state. Visually equivalent output is sufficient;
identical SGR byte sequences are unnecessary.

Display-width measurement and non-mutating truncation are implemented for the
owning tree using ordinary fmt string rules. Generated SGR contributes no width;
raw embedded escapes are not interpreted. Padding is root-styled. Complete
grapheme and terminal-control handling remain future work. For lazy
`styled_arg<T>`, retain the underlying formatter's type-specific behavior and
place its padding inside the styled span.

Prefer constexpr operations where practical. C++23 vector/constant-evaluation
support does not imply a populated dynamic tree can persist as a constexpr
object; borrowed lifetimes and possible bounded storage need further design.

The proposed human-readable grammar has canonical order: `absolute`, attributes
in fixed order, foreground, `on`, background. Attributes may occur anywhere;
duplicates are harmless. The first `on` selects background assignment, later
ones do not toggle it, and the last color per selected channel wins. Recognize
`absolute` anywhere. No blend operator is planned in this grammar. Aim for exact
parse/serialize round trips while preserving independent attribute bits.
Vocabulary, errors, and private rendering-policy serialization remain open.

Plan an archive-style binary interface, initially eight little-endian bytes,
rather than treating the current packed integer helpers as a permanent format.
Reserved bits, invalid data, versioning, and Boost versus independent integration
remain open. Do not implement general numeric archive machinery before needed.
Serialization is not a prerequisite for VarColor or Palette.

Taskwarrior persistence has not been investigated. Preserve valid application
configuration and keep restrictions such as bright-foreground-only colors in
the application. Review actual representation-dependent code when integrating.
