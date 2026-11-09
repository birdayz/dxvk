# Build provenance and reproduction

## Frozen measured inputs

- Upstream tag: `v2.7.1`.
- Annotated tag object: `ab3631f80d9cd493676d4342bf614028c72e7484`.
- Peeled upstream commit: `c3dd74be6baec53786d4e064a572185b70347a17`.
- Source-only patch SHA256 (the branch's `src/` diff against that base):
  `a99ebed34e73b004b7d6cf729494cdd99e8064311b419e69754176c5916f3ccd`.

Pinned upstream gitlinks, unchanged by this branch:

| Path | Commit |
| --- | --- |
| `include/native/directx` | `9df86f2341616ef1888ae59919feaa6d4fad693d` |
| `include/spirv` | `8b246ff75c6615ba4532fe4fde20f1be090c3764` |
| `include/vulkan` | `234c4b7370a8ea3239a214c9e871e4b17c89f4ab` |
| `subprojects/libdisplay-info` | `275e6459c7ab1ddd4b125f28d0440716e4888078` |

Toolchain: LLVM-MinGW **20261104 UCRT**, Ubuntu 22.04 x86-64 host archive,
Clang/LLD **23.1.5**. Download:
<https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz>

Archive SHA256:
`919380403de87dca3c581c2053230cd298af5e891286628ba0613b7fed370926`.

Host tools: Python **3.14.9**, Meson **1.12.1**, Python package ninja **1.13.2**
(binary reports `1.13.2.git.kitware.jobserver-pipe-1`), native glslang **16.6.0**.
Python tooling was installed with `uv` in an isolated virtual environment.

The build used release/O3, C++17 and the project's static C++ runtime link flags;
LTO and debug information were off. Only x86-64 D3D11/DXGI were built; D3D8/9/10
were disabled. No install step was performed. Generated `version.h` reported
`DXVK_VERSION "v2.7.1+"` because the measured checkout was dirty.

## Reproduce the configuration

Use Linux x86-64 with `uv`, Git, curl, tar, a C/C++ host toolchain, and native
glslang 16.6.0 available. The commands below are instructions, not a publication
hook. They use generic local paths, not the original build machine's directories.

```sh
git clone --branch hd620-wow-dxvk27 https://github.com/birdayz/dxvk.git dxvk27
cd dxvk27
# Pin this checkout to the published branch commit being reproduced.
git rev-parse HEAD
git submodule update --init --recursive

uv venv --python 3.14.9 .build-tools
uv pip install --python .build-tools/bin/python meson==1.12.1 ninja==1.13.2
export PATH="$PWD/.build-tools/bin:$PATH"

mkdir -p .toolchains
cd .toolchains
curl -fLO https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz
printf '%s  %s\n' \
  919380403de87dca3c581c2053230cd298af5e891286628ba0613b7fed370926 \
  llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz | sha256sum -c -
tar -xf llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz
cd ..
export PATH="$PWD/.toolchains/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64/bin:$PATH"

cat > build-win64-clang.ini <<'CROSS'
[binaries]
c = 'x86_64-w64-mingw32-clang'
cpp = 'x86_64-w64-mingw32-clang++'
ar = 'llvm-ar'
strip = 'llvm-strip'
windres = 'x86_64-w64-mingw32-windres'

[properties]
needs_exe_wrapper = true

[host_machine]
system = 'windows'
cpu_family = 'x86_64'
cpu = 'x86_64'
endian = 'little'
CROSS

meson setup build64 . --cross-file build-win64-clang.ini \
  --buildtype=release --prefix "$PWD/stage" \
  -Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false
ninja -C build64 -j8 src/d3d11/d3d11.dll src/dxgi/dxgi.dll
sha256sum build64/src/d3d11/d3d11.dll build64/src/dxgi/dxgi.dll
```

The original cross-file used absolute paths to these same toolchain binaries.
No `meson install` or deployment is required to produce the DLLs. Do not commit
build directories, tooling, generated cross-files or `subprojects/.wraplock`.

A clean committed checkout can generate a different DXVK version string, PE
metadata or other build bytes. These instructions reproduce the configuration,
not a promise of bit-identical output. The following hashes identify the actual
frozen pair that was built and measured, not every subsequent rebuild.

## Frozen matching x86-64 DLL hashes

```text
87fa406ac066ca3033198a4e0e46103a5ae86967e6dfaa9f86e4042dba8e7638  d3d11.dll
a7f9bb3511d7e3947f5281bdd2141c8d9f162370f77ec383e4fc4bbcb47849a9  dxgi.dll
```

To verify the published source-only diff independently:

```sh
git diff --no-ext-diff --no-color \
  c3dd74be6baec53786d4e064a572185b70347a17 HEAD -- src/ | sha256sum
```

This branch publishes source and provenance only, not a binary release. See
[README.md](README.md) for opt-in scope, runtime evidence and safety limitations.
