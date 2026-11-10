# FmtColor demo

From this directory, run `make run` using GNU make and a C++23-capable compiler.
From the repository root, run `make -C demo run`.

The Makefile uses `TJG_ROOT=..` to read this checkout's `include/tjg/` headers.
It builds in standalone mode with `std::format` and `std::print`, using compact SGR;
no CMake, {fmt} library, or `FMT_HEADER_ONLY` definition is needed.
Override the path or compiler with `make run TJG_ROOT=/path/to/fmt CXX=g++`.
Run `make clean` before changing the TJG path, compiler, or build flags.

`FmtColorDemo2.cpp` exercises nested colors/emphasis and absolute boundaries.
The second line has strikethrough and a gray background except inside the
absolute underline-only span. Output requires an ANSI-capable terminal.


`FmtColorDemo2.cpp` prints the same `line2` with underlay first and overlay
second, using the same white foreground, gray background, and strikethrough.
Underlay supplies defaults while preserving explicit colors. Overlay replaces
the affected colors throughout relative nodes. Both preserve the absolute span.
