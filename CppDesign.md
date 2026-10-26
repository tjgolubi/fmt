# C++ design

[Document map](README.md) · [Workflow](Workflow.md) · [Build and test](BuildAndTest.md) · [Website](Website.md)

Updated October 26, 2026. Implemented behavior is separated below from future
design. A proposal is not permission to implement it.

## Goals and boundaries

Provide useful color and style values, nested styled text, terminal rendering,
and textual/binary style serialization. User themes and future Taskwarrior
integration motivate the design; application-specific restrictions belong in
applications.

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

`FmtColorDev` at `257cdac1` integrates the ostream bridge on the foundation,
Fluent, ShortNames, VarColor, and Palette. Exact history and baseline are in
[README.md](README.md#current-baseline). Current source and tests establish
implemented behavior; private numeric representations are not stable contracts.

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
- `gray_color` has `value_type = uint8_t`, indices 0–23, and equal RGB channel
  levels `8 + 10 * index`. Indexed SGR uses palette index `232 + index`.
- `cube_color` has `value_type = uint8_t`, channels 0–5 and cube index
  `36*r + 6*g + b` in 0–215. RGB levels are `{0, 95, 135, 175, 215, 255}`;
  indexed SGR uses `16 + index`.

Cube and gray constructors validate the original unsigned values before
narrowing, using `FMT_THROW(std::out_of_range(...))` and upstream's disabled-
exception machinery. Both implicitly convert to `rgb`, as do terminal colors.
Terminal conversion uses this fixed Xterm-default mapping, in index order:

|Indices | RGB values |
| ------ | --- |
|  0– 3  | `(0,0,0)`, `(205,0,0)`, `(0,205,0)`, `(205,205,0)` |
|  4– 7  | `(0,0,238)`, `(205,0,205)`, `(0,205,205)`, `(229,229,229)` |
|  8–11  | `(127,127,127)`, `(255,0,0)`, `(0,255,0)`, `(255,255,0)` |
| 12–15  | `(92,92,255)`, `(255,0,255)`, `(0,255,255)`, `(255,255,255)` |

This mapping cannot discover a user's terminal theme. `underlying_type<T>` and
`to_underlying()` expose numeric values: `uint8_t` for terminal, cube, gray,
and emphasis; `uint32_t` for RGB and `color_t`; `unsigned` zero for `no_color_t`;
and `uint64_t` for styles. `narrow_cast` is unchecked; `enum_cast` expresses
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
implementations use concepts and loops. The mutating `rgb::blend()` member is
removed. No alpha compositing or nearest-palette demotion is implemented.

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
C++14+ paths. The earlier strict-rejection proposal is superseded.

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
`rsvd_color` is removed. Default construction is unspecified. Construction and
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
The old addition/overflow composition trick is gone. `to_underlying` and
internal `detail::unpack_color` reflect these changed representations; object
size does not establish ABI or persisted-data compatibility.

### Compiler constraints

ANSI generation uses append helpers, visitation, and an emphasis-to-SGR table.
RGB and indexed-palette parameters retain three-digit padding. Packed colors
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

`test/UpstreamCompatibility` at `21eba01b` verifies this header with upstream
tests and workflows. Six terminal-overlap exception assertions become exact
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

`<fmt/color-ostream.h>` provides `print(os, style, format, args...)`,
`println(os, style, format, args...)`, and corresponding `vprint`/`vprintln`
overloads with `format_args`. The destination is `std::ostream&`; no styled
`std::wostream` overload is provided. The bridge includes `color.h` and
`ostream.h`, leaving both existing headers independent.

The style applies to the entire formatted message. Nonempty styles add SGR
sequences and a reset to terminal defaults; empty styles emit plain formatted
text. A newline follows the reset. Reset does not restore an enclosing style.
The bridge formats into a buffer and delegates output to the existing ostream
printer, retaining its stream behavior and Windows console path. It supports
C++11, header-only use, installation, and C++20 module export.

The `21eba01b` compatibility branch still verifies the same `color.h`, unchanged
by Ostream; its 42-job result does not cover the new bridge. The final
`FmtColorDev` 43-job result covers the integrated bridge and tests separately.

## Pending TaskWarrior branch

The local `TaskWarrior` branch at
`e46006c9bc2e62c9bc3d0b5dd40e5644ab9bc5fe` contains application-driven work
above Palette. These features are implemented on that branch, with tests and
guide changes, but are not part of the accepted `FmtColorDev` baseline:

- `FMT_COLOR_COMPACT_SGR=1` selects one combined SGR sequence per style with
  unpadded decimal parameters. Undefined or `0` preserves legacy output.
- `to_sgr<Char>(style)` returns an owning `std::basic_string<Char>` containing
  opening style sequences; `Char` defaults to `char`. It replaces the removed
  `text_style::str()`. Empty styles return an empty string; no reset is appended.
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

Use fmt string views for C++11 support. In this version they have `size()` but
no `empty()`. Explicit character template arguments are needed when deduction
cannot convert a literal to `basic_string_view<Char>`. SGR conversion allocates
and is not noexcept. Keep style values independent of their rendering.

The reserved single-sequence bound is 56 characters, excluding a null terminator;
current combined output needs at most 52, leaving room for a future `000;`
prefix. The reset length is separately four. Special extended-SGR semantics are
not implemented by reserving that space.

Review and reconcile TaskWarrior with the current baseline before integrating;
do not overwrite the accepted ostream bridge with the older branch tree. The
final ostream CI and website publication do not validate or publish these
pending features. Their acceptance still requires the normal candidate gate.

## Planned work

Palette, RGB conversion, color/style blending, Fluent, ShortNames, and public
variants are implemented. They are not pending ConvertColors or Blend branches.
`feat/StyledText` remains a proposal for nested text, absolute boundaries, and
lazy blending. Its ownership, rendering, and first increment still need review.
Parsing, display width, and serialization remain separate design work.

The later independent library requires C++23 and `std::variant`; migration
details remain open. The {fmt} release retains C++11 throughout. Proposed
StyledText representations below do not settle a C++11 implementation path.
Do not restore a C++17 variant gate or the superseded strict style-OR policy.
Experimental branches such as `refactor/Crazy` need explicit approval for
implementation or website inclusion.

## Structured styled text

StyledText is future work. Its proposed representation is a `text_style` plus:

```cpp
std::variant<std::string_view, const styled_text*, std::vector<styled_text>>
```

Views borrow characters, pointers borrow subtrees, and vectors own child nodes.
Copying owned vectors copies nodes recursively, but views and pointers still
borrow their targets. Moving should transfer owned storage. If subtree
borrowing proves impractical, fall back to a view-or-owned-vector representation;
that fallback copies an lvalue's nodes when wrapping it.

### Lifetimes and construction

Characters must outlive all uses even when nodes are owned. Borrowed subtrees
must outlive wrappers and stay at stable addresses; moving or destroying a
target can invalidate the pointer. Const borrowing does not provide ownership
or prevent mutation elsewhere. Borrowed relationships must not form cycles.

Plan default/copy/move construction, an explicit plain `string_view` constructor
with optional style, and wrapping constructors taking an lvalue or rvalue child
with a required style. Lvalue wrapping borrows the child's address; rvalue
wrapping owns and moves the child. Never retain a pointer to a temporary or
constructor parameter that will be destroyed. Exact overloads and exception
specifications remain subject to review. Keep ordinary `styled()` formatting
support for a `styled_text` value.

Appending is the primary use case. Appending to a view turns the root into an
owned vector containing the original fragment and the new fragment; later
appends extend that sequence while retaining fragment styles. Move temporary
nodes efficiently rather than eagerly rewriting children or embedding SGR as
the normal representation. Parser-created nodes are owned; character views may
borrow the input without SGR sequences. Transformed character storage, if needed,
requires a separate lifetime solution.

Plain-text construction does not scan for or reject SGR. Call the explicit
parser when input can contain escapes; no separate unchecked constructor is
needed merely to bypass a validation scan.

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
