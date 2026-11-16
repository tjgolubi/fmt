#!/bin/bash
set -euo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_dir"

if (( $# != 0 )); then
  echo "Usage: $0" >&2
  exit 2
fi

build_dir=build

set -x
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
  -DTJG_TEST=ON -DTJG_WERROR=ON
cmake --build "$build_dir" --parallel 4
ctest --test-dir "$build_dir" --output-on-failure
