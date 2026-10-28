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

- *(color)* [**breaking**] Add palette colors, blending, and RGB conversion — Remove `rsvd_color` and `rgb::blend()`. Overlapping
style colors now promote to RGB and combine by bitwise OR,
including terminal colors. Use free `blend()` for channel averaging.

- *(color)* Add styled ostream output

- *(color)* [**breaking**] Add SGR helpers, style overlays, and stream insertion — RGB, gray, and cube numeric interfaces use signed inputs
with assertion and clamping behavior. term_color uses int8_t, cube channel
getters return int, and cube_color::chan_type is now private.

- *(color)* Add conditional emphasis factories


### 🐛 Bug Fixes

- *(docs)* Publish the user guide from FmtColorDev

