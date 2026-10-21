#!/bin/bash
set -euo pipefail

set -x

doc_bin="$(pwd)/build/venv/bin"
export PATH="$doc_bin:$PATH"

# Follow support/release.py: isolate the pinned documentation tools and put
# them on PATH for this script, without activating them in the calling shell.
# Reuse the environment; do not delete existing local files.
if [[ ! -d build/venv ]]; then
  python -m venv build/venv
  "$doc_bin/python" -m pip install --require-hashes -r support/doc-requirements.txt
fi

# Skip tests incompatible with Android fdsan on Termux only.
fmtcolor_skip=OFF
if [[ -n ${TERMUX_VERSION:-} || ${PREFIX:-} == */com.termux/files/usr ]]; then
  fmtcolor_skip=ON
fi

cmake -S . -B build -U MKDOCS -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_CXX_STANDARD=23 -DFMT_TEST=ON -DFMT_DOC=ON -DFMTCOLOR_SKIP="$fmtcolor_skip"
cmake --build    build --parallel 8
ctest --test-dir build --output-on-failure
cmake --build    build --target doc
#cmake --install  build --prefix ~/.local

# For Clang/libc++, add `-DCMAKE_CXX_COMPILER=clang++`
# and `-DCMAKE_CXX_FLAGS=-stdlib=libc++`.
# Consumers use `fmt::fmt` or `fmt::fmt-header-only`; both propagate `cxx_std_11`.
