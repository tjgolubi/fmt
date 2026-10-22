---
title: Change Log
copyright: 2026 Terry Golubiewski, all rights reserved.
---

# Change Log

## Unreleased

### 🚀 Features

- *(color)* Rename `terminal_color` to `term_color` and `strikethrough` to
`strike`, including the fluent setter and query. Retain the old names as
aliases and forwarding methods for backward compatibility.

- *(color)* [**breaking**] Simplify and extend colors and text styles — fmt::color is now a namespace of constexpr rgb values,
not an enum type. Packed integer construction of rgb is explicit.
Combining emphasis values returns emphasis rather than `text_style`.
`terminal_color` uses palette indices 0-15 instead of SGR codes.
Packed color and `text_style` representations have changed, affecting
binary compatibility and stored representations.

- *(color)* Add fluent component accessors and per-attribute emphasis controls
