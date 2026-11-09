#!/usr/bin/env bash
# Native helper checks only: this does not exercise the D3D/Vulkan guard on a GPU.
set -euo pipefail
[[ $# == 0 ]] || { echo "Usage: $0" >&2; exit 2; }
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
work=$(mktemp -d)
trap 'rm -rf -- "$work"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -I"$source_dir" -I"$source_dir/include/vulkan/include" \
  "$source_dir/tools/hd620-wow/probes/guard-predicate-test.cpp" -o "$work/guard-predicate-test"
"$work/guard-predicate-test"
bash -n "$source_dir/tools/hd620-wow/launch-test.sh" \
  "$source_dir/tools/hd620-wow/build.sh" "$source_dir/tools/hd620-wow/verify.sh"
git -C "$source_dir" diff --check HEAD
echo 'Native predicates, shell syntax and diff whitespace passed; NOT GPU integration coverage.'
