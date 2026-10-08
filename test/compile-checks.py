#!/usr/bin/env python3
"""Check standalone public headers and intentional compile-time diagnostics."""
import pathlib
import subprocess
import sys

compiler, include, *flags = sys.argv[1:]
base = [compiler, *flags, '-I' + include, '-x', 'c++', '-fsyntax-only', '-']

def compile_source(source, mode='c++23'):
    return subprocess.run(base + ['-std=' + mode], input=source, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

for header in ('core', 'base', 'format', 'format-inl', 'chrono', 'std',
               'color', 'ranges', 'xchar'):
    result = compile_source(f'#include <fmt/{header}.h>\nint main() {{}}\n')
    if result.returncode:
        sys.exit(f'Standalone header {header} failed:\n{result.stderr}')

bad_format = compile_source('#include <fmt/format.h>\nint main() { fmt::format("{:d}", "text"); }\n')
if bad_format.returncode == 0:
    sys.exit('Invalid literal format was not rejected at compile time')
old_mode = compile_source('#include <fmt/core.h>\n', mode='c++20')
if old_mode.returncode == 0 or 'requires C++23' not in old_mode.stderr:
    sys.exit('C++20 must fail with the C++23 requirement diagnostic')
for header in ('args', 'printf', 'os', 'fmt-c', 'compile', 'enum', 'ostream'):
    result = compile_source(f'#include <fmt/{header}.h>\n')
    if result.returncode == 0 or 'not supported' not in result.stderr:
        sys.exit(f'Unsupported header {header} must report its compatibility gap')
print('Standalone headers and compile-time rejection checks passed')
