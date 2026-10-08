# Handover: C++23-only `{fmt}` fork

**For:** GPT-6 Sol Medium  
**Prepared:** 2026-10-08  
**Upstream:** [fmtlib/fmt](https://github.com/fmtlib/fmt)

## Project goal

Create a C++23-only fork of `{fmt}` that uses the C++ standard formatting library as cleanly as possible. Compatibility with standard C++23 formatting interfaces matters more than preserving `{fmt}`'s performance advantage.

The intended result is a drop-in replacement for `{fmt}`, retaining the `fmt` library name, `fmt` namespace, and existing interface to the greatest extent practical. Do not rename it `tjgfmt`.

This is a fork in code lineage and should retain upstream history and attribution. The public project/package/repository naming should remain consistent with the user's intent to retain `fmt`, unless a concrete distribution issue requires a separate decision.

## Target toolchains

- Require C++23.
- Support the latest stable GCC and Clang releases at the time of development.
- Specify and test the associated standard-library implementations. GCC normally uses libstdc++; Clang may use libstdc++ or libc++. Decide which Clang/library combinations are in scope.
- Do not support older compiler releases or older C++ language modes.
- If a supported compiler/library combination lacks a required C++23 facility, treat it as unsupported rather than adding a fallback for older implementations.

## Implementation direction

- Use `std::format`, `std::formatter`, and `std::print` as the formatting foundation.
- Prefer standard C++23 facilities over corresponding `{fmt}` implementation machinery, even where the upstream implementation is faster.
- Remove compatibility macros and fallback branches that exist only for pre-C++23 modes or older compilers.
- Before removing a macro, identify whether it instead handles a platform, build configuration, or supported compiler/library difference. Keep only what is genuinely needed for the declared target matrix.
- Reuse `{fmt}`'s color and style interface and useful implementation ideas where they fit the standard formatter model. Do not assume its formatting engine can be retained unchanged when the goal is to use the standard engine.
- Preserve the `fmt` namespace and public names as far as possible. Avoid depending on upstream `fmt::detail` internals as public API.

## Styling scope

The intended styling surface is based on `{fmt}`'s `text_style`, named `fmt::color` values, `fmt::rgb`, the 16 `fmt::terminal_color` values, foreground/background helpers, emphasis, `styled(value, style)`, and style-aware print/format overloads.

For the initial C++23 refactor, defer redesigning nested-style semantics, including relative versus absolute rendering. Keep the initial scope focused on adapting the existing interface to the standard formatting foundation. Any later nested-style redesign should be a separate, explicit phase.

## First work requested

Do not begin code edits until Terry asks for implementation. Start with a source audit and a short, evidence-based plan:

1. Inspect the current upstream repository and identify the exact baseline revision.
2. Inventory public headers, functions, types, macros, and tests relevant to formatting and styling.
3. Classify compatibility code into pre-C++23 support, compiler/library workarounds, platform behavior, and build/configuration features.
4. Map which APIs can use the standard library directly, which styling pieces can be adapted, and which upstream APIs would be lost or changed.
5. Assess the meaning of “drop-in replacement.” List any `{fmt}` APIs not covered by C++23 or the styling goal, and flag compatibility gaps instead of silently dropping them.
6. Propose a minimal supported compiler/standard-library test matrix and phase boundaries.

After Terry requests implementation, work in small reviewable steps, preserve upstream attribution and license notices, and run the relevant tests on the declared toolchain matrix.

## Decisions already made

- Project is a source fork of `{fmt}`, not a clean-room reimplementation.
- The library and namespace should remain `fmt`; `tjgfmt` is not the chosen public name.
- C++23 standard formatting compatibility is more important than formatter efficiency.
- Only latest stable GCC and Clang releases are targets; older compiler and language-mode compatibility is out of scope.
- Initial work is the C++23 refactor. Relative/absolute nested-style behavior is deferred.
- GPT-6 Sol Medium is the preferred working model for this effort because it offers a better cost/capability balance for iterative analysis and implementation.
- Terry previously asked not to generate code until explicitly requested. Treat this handover request as authorization to create this document only; wait for an implementation request before editing the library source.
