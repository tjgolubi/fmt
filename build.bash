#!/bin/bash
set -euo pipefail

repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$repo_dir"

# Keep TJG separate from any existing top-level FMT build cache.
build_dir=build/tjg
if (( $# > 1 )); then
  echo "Usage: $0 [taskwarrior]" >&2
  exit 2
fi
case "${1:-}" in
  "") ;;
  taskwarrior) build_dir=build/tjg-taskwarrior ;;
  *)
    echo "Usage: $0 [taskwarrior]" >&2
    exit 2
    ;;
esac

set -x
for mode in ON OFF; do
  mode_build_dir="$build_dir/$mode"
  cmake -S tjg -B "$mode_build_dir" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=23 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DTJG_TEST=ON -DTJG_WERROR=ON -DTJG_COLOR_COMPACT_SGR="$mode"
  cmake --build "$mode_build_dir" --parallel 4
  ctest --test-dir "$mode_build_dir" --output-on-failure
done
