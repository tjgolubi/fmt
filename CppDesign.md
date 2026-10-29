# C++ design

[Document map](README.md) · [Workflow](Workflow.md) · [Build and test](BuildAndTest.md) · [Website](Website.md)

Updated October 29, 2026. Implemented behavior is separated below from future
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

The styling commit `2c456778` on `color-dev` contains the complete C++11 API below
as one contribution above upstream `f87a75e1`. The current branch tip
`a18689e1` adds documentation-publication configuration. The accepted baseline is in
[README.md](README.md#current-baseline).
Current source and tests establish behavior; private numeric representations
are not stable contracts.

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
  black. They do not store or emit a terminal-reset instruction.
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

Free variadic `blend()` supports:

- Plain colors only: return an equal-weight RGB channel average.
- A `color_t` or `no_color` argument: return `color_t`, ignore unspecified
  inputs, return unspecified for none specified, preserve the kind of one
  specified color, and return averaged RGB for two or more.
- Styles only: return `text_style`, averaging foreground and background
  independently with those same unspecified/kind rules, and ORing emphasis bits.

All channels are summed before division; fractional results round down. Inputs
are unchanged. Overloads reject non-colors and mixed raw-color/style arguments.
They are `noexcept` and constexpr in C++11 via recursive helpers; gated C++23
implementations use concepts and loops. No alpha compositing or nearest-palette demotion is implemented.

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
black and detect any emphasis bit. Const `empty()` is true only when both
colors are unspecified and all emphasis is unset. Fluent `clear()` restores
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
foreground bits 0–26, background 27–53, emphasis 54–61. Bits 62–63 are unused.
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
export. All 41 `color-dev` push jobs passed at `2c456778`, including documentation
build, but not guide publication. The separate historical compatibility result
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

The public opening-output bound is 56 characters, excluding a null terminator.
Legacy output can use all 56; current compact output needs at most 52, leaving
room for a future `000;` prefix in a combined sequence. The reset length is
separately four. Special extended-SGR semantics are not implemented by
reserving that space.

Compatibility provisions found during candidate CI are retained: default the
compact macro to zero; cast signed payloads explicitly; perform terminal SGR
arithmetic as `int`; emit the reset-storage out-of-class definition only before
C++17; and avoid copying the empty union member in the SGR visitor during MSVC
constant evaluation. Tests explicitly initialize const stream-only values and
avoid shadowing and self-assignment warnings.

The 41-job `color-dev` result validates the accepted contribution in default SGR
mode. Compact mode has historical local evidence, but no current full compact
matrix is claimed. See [BuildAndTest.md](BuildAndTest.md#integrated-styling-helpers).
The public website is version 36. The last verified guide publication used
the former `dev` branch at `a18689e1`, whose documentation build and deployment passed. Platform CI for
that configuration commit was still running when checked; the complete 41-job
result above remains specific to the styling commit.

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

## Planned work

Palette colors, RGB conversion, color/style blending, fluent operations,
short names, public variants, and conditional factories are implemented.
The owned leaf/branch foundation is committed and pushed at `7b0e07ce` on `feat/StyledText`,
with the agreed model in [StyledText.md](StyledText.md). Rendering, absolute
boundaries, and lazy blending remain design work.
Parsing, display width, and serialization remain separate design work.

The later independent library requires C++23 and `std::variant`; migration
details remain open. The {fmt} release retains C++11 throughout. The current
StyledText foundation uses a custom C++11 union in all language modes.
Do not restore a C++17 variant gate or the superseded strict style-OR policy.
Experimental branches such as `refactor/Crazy` need explicit approval for
implementation or website inclusion.

### Attribute-factory design for the independent library

Revisit the convenience and naming of these factories for the independent
C++23-and-later library. `using enum fmt::emphasis` (available since C++20)
already shortens unconditional styling; imported enum names can hide or
conflict with unqualified factory names. Qualified `fmt::bold()` remains clear.

## Structured styled text

The current authorized model is fully owned. Each `styled_text` has a
`text_style` and a custom discriminated union containing either:

```cpp
using leaf = std::string;
using branch = std::vector<styled_text>;
```

The separate header is `<fmt/styled-text.h>`. The latest explicit filename
supersedes the earlier tentative `text-style.h`. A private Boolean selects the
active union member. Other flags and representation compaction can be revisited.
Do not use `std::variant`, string views, or borrowed subtree pointers in this
increment. String views may be considered later, with a new lifetime design.

Default construction gives an empty leaf and empty style. String, literal, and
child-vector constructors take an optional style defaulting to empty. Strings
and literals implicitly convert to nodes. Copying owns a recursive copy;
moving transfers storage. `operator+` creates an empty-style parent with both
operands as children, preserving styles and nesting. No `operator+=` is added.
Equality compares exact styles, alternative, strings, child order, and structure;
render-equivalent trees need not be equal. The type-based `holds_alternative`
and `get` overloads expose alternatives. See [StyledText.md](StyledText.md) for
signatures, limitations, and the working-copy status.

Plain strings are stored as supplied, without interpreting embedded escapes.
Rendering, inheritance resolution, formatting, and parsing are not implemented
by these constructors or comparisons. The rules below remain proposals.

### Absolute and lazy styles

`absolute` belongs to StyledText. At an absolute boundary, rendering starts
from terminal defaults with `ESC[0;...m`; descendants inherit that node's style
normally. Enclosing style changes and blending must not penetrate the boundary.
Surrounding styling resumes after the span. No absolute flag is implemented in
the current foundation.

Plan `styled_text& blend() noexcept` to enable the current node's lazy blending
policy and return `*this`. It does not traverse children, allocate, or copy them.
Rendering resolves blending later, respecting absolute boundaries. Storage in
a private style bit or node member is an implementation choice. Do not add the
previously rejected free `blend(styled_text, text_style)` operation.

The motivation is independently styled phrases wrapped in a paragraph's
background and then an enclosing emphasis, while preserving their structure.
Color promotion and value-level style blending are implemented. Their use
through nested inheritance and absolute boundaries still needs agreement.

## Rendering, parsing, and serialization

Keep three responsibilities distinct:

1. `render()` produces an interchange string with extended SGR push/pop markers.
2. A terminal formatter emits ordinary ANSI SGR directly from the tree, resolving
   inheritance without calling `render()` or building an extended-SGR intermediate.
3. An ANSI parser consumes ordinary SGR and extended push/pop sequences into a tree.

The legacy markers are `ESC[00m` for push and `ESC[000m` for pop, distinct from
`ESC[0m` reset. Verify behavior against `AnsiColor.hpp` and `AnsiRender.hpp` before
implementing compatibility; they are references, not wholesale implementation
instructions. Sequential SGR parsing follows terminal state changes: reset
clears state, foreground reset clears foreground, and intensity reset clears
bold/faint. Do not use style OR composition to interpret those commands.
Push/pop save and restore state. Visually equivalent output is sufficient;
identical SGR byte sequences are unnecessary.

Plan display-width measurement and non-mutating truncation that ignore escape
sequences and preserve surviving styles. Alignment/precision work on visible
text; Unicode boundaries, graphemes, and padding placement remain open. For lazy
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
