#!/usr/bin/env bash
# Rebuild the source baseline. Exact frozen release DLLs remain the measurement reference.
set -euo pipefail
[[ $# == 2 ]] || { echo "Usage: $0 TOOLCHAIN_DIR BUILD_DIR" >&2; exit 2; }
source_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
toolchain=$(realpath -e -- "$1")
build_dir=$(realpath -m -- "$2")
# Meson string literals below cannot represent quotes/newlines in a toolchain path.
[[ "$toolchain" != *"'"* && "$toolchain" != *$'\n'* && "$toolchain" != *'\'* ]]
[[ ! -e "$build_dir" ]] || { echo 'Use a new build directory.' >&2; exit 1; }
for binary in x86_64-w64-mingw32-clang x86_64-w64-mingw32-clang++ llvm-ar llvm-strip x86_64-w64-mingw32-windres; do
  [[ -x "$toolchain/bin/$binary" ]] || { echo "Missing toolchain binary: $binary" >&2; exit 1; }
done
command -v uv >/dev/null
command -v glslangValidator >/dev/null || command -v glslang >/dev/null
if git -C "$source_dir" submodule status --recursive | grep -qE '^[-+U]'; then
  echo 'Initialize the pinned submodules: git submodule update --init --recursive' >&2
  exit 1
fi
mkdir -p -- "$build_dir"
cat > "$build_dir/cross.ini" <<EOF
[binaries]
c = '$toolchain/bin/x86_64-w64-mingw32-clang'
cpp = '$toolchain/bin/x86_64-w64-mingw32-clang++'
ar = '$toolchain/bin/llvm-ar'
strip = '$toolchain/bin/llvm-strip'
windres = '$toolchain/bin/x86_64-w64-mingw32-windres'

[properties]
needs_exe_wrapper = true

[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
EOF
export PATH="$toolchain/bin:$PATH"
uv run --no-project --with meson==1.12.1 --with ninja==1.13.2 \
  meson setup "$build_dir" "$source_dir" --cross-file "$build_dir/cross.ini" \
  --buildtype=release -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false
uv run --no-project --with meson==1.12.1 --with ninja==1.13.2 \
  ninja -C "$build_dir" -j"${JOBS:-8}" src/d3d11/d3d11.dll src/dxgi/dxgi.dll
sha256sum "$build_dir/src/d3d11/d3d11.dll" "$build_dir/src/dxgi/dxgi.dll"
echo 'Built only; nothing installed. See docs/hd620-wow/BUILD.md for reproducibility limits.'
