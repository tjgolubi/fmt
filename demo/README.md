# FmtColor demo

From this directory, run `make run` using GNU make, a C++23-capable compiler,
and a built copy of {fmt} for the comparison demos. From the repository root,
run `make -C demo run`.

The {color} demos include `<tjg/color.hpp>`. The Makefile adds
`$(TJG_ROOT)/include` to the compiler's include path, with `TJG_ROOT=..` by
default. To build a single demo using only {color} and the C++ standard library,
run `make FmtColorDemo1` and then `./FmtColorDemo1`.

`FmtColorDemo0` and `NestedDemo` use {fmt}. `FMT_ROOT` defaults to
`$HOME/App/fmt` and `FMT_LIB` defaults to `$FMT_ROOT/build/libfmtd.a`.
For a different {fmt} build, set both paths, for example:

```sh
make run FMT_ROOT=/path/to/fmt FMT_LIB=/path/to/fmt/build/libfmt.a
```

Run `make clean` before changing the header paths, compiler, or build flags.

`FmtColorDemo2.cpp` prints the same styled line with underlay first and overlay
second, using white foreground, gray background, and strikethrough. Underlay
supplies defaults while preserving explicit colors. Overlay replaces the
affected colors throughout relative nodes. Both preserve the absolute
underline-only span. Use an ANSI-capable terminal to see the styling.
