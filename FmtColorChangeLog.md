---
title: Change Log
copyright: 2026 Terry Golubiewski, all rights reserved.
---

# Change Log

## Unreleased

### 🚀 Features

- *(color)* [**breaking**] Simplify and extend colors and text styles — fmt::color is now a namespace of constexpr rgb values,
not an enum type. Packed integer construction of rgb is explicit.
Combining emphasis values returns emphasis rather than text_style.
terminal_color uses palette indices 0-15 instead of SGR codes.
Packed color and text_style representations have changed, affecting
binary compatibility and stored representations.

- *(color)* Add fluent text-style accessors

- *(color)* Shorten terminal color and strike names

- *(color)* [**breaking**] Add public `color_t` with C++11 variant access — Replace `detail::color_type` with `fmt::color_t`.
Remove packed-integer construction and integral conversion.


### 🐛 Bug Fixes

- *(docs)* Publish the user guide from FmtColorDev

