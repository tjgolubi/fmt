#!/usr/bin/env python3
"""Check standalone public headers and intentional compile-time diagnostics."""
import subprocess
import sys

compiler, include, *flags = sys.argv[1:]
base = [compiler, *flags, '-I' + include, '-x', 'c++', '-fsyntax-only', '-']

def compile_source(source, mode='c++23'):
    return subprocess.run(base + ['-std=' + mode], input=source, text=True,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE)

for header in ('color',):
    result = compile_source(f'#include <fmt/{header}.h>\nint main() {{}}\n')
    if result.returncode:
        sys.exit(f'Standalone header {header} failed:\n{result.stderr}')

bad_format = compile_source('#include <fmt/color.h>\nint main() { fmt::format(fmt::emphasis::bold, "{:d}", "text"); }\n')
if bad_format.returncode == 0:
    sys.exit('Invalid literal format was not rejected at compile time')
old_mode = compile_source('#include <fmt/color.h>\n', mode='c++20')
if old_mode.returncode == 0 or 'requires C++23' not in old_mode.stderr:
    sys.exit('C++20 must fail with the C++23 requirement diagnostic')
# Ordinary formatting and its types belong to std, not fmt.
for expression in ('fmt::format("{}", 42)', 'fmt::print("{}", 42)',
                   'fmt::formatter<int>{}', 'fmt::format_error("error")',
                   'fmt::memory_buffer{}', 'fmt::runtime("{}")'):
    result = compile_source('#include <fmt/color.h>\nint main() { (void)(' + expression + '); }\n')
    if result.returncode == 0:
        sys.exit(f'Out-of-scope public entry point still exists: {expression}')
print('Standalone headers and compile-time rejection checks passed')
