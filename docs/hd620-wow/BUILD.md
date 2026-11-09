# Rebuilding and validating the HD 620 baseline

Read the [scope, measured results and runtime limitations](README.md) first. These instructions build the altered **x86-64 D3D11/DXGI pair only**. They do not install DLLs, run the game, or modify an original Proton runtime. This is a source/behavior reproduction recipe, **not a promise of bit-identical binaries**.

## 1. Source and pinned dependencies

```sh
git clone --branch hd620-wow-baseline https://github.com/birdayz/dxvk.git dxvk-hd620
cd dxvk-hd620
git submodule update --init --recursive
git rev-parse HEAD
git submodule status --recursive
```

Record the baseline branch's reviewed commit ID; a branch name alone is not immutable provenance. The upstream base is **`601930949d111edbbcf9dd463948426d9f8f6ddd`** from GE-Proton11-7 (v3.1-19). This branch already contains the full workaround and guard-performance changes: **do not apply the historical patch files again**.

Expected submodule revisions:

| Path | Commit |
| --- | --- |
| `include/native/directx` | `9df86f2341616ef1888ae59919feaa6d4fad693d` |
| `include/vulkan` | `8864cdc896bbc2a9b6eb36b3218fc9ef57908d77` |
| `include/spirv` | `04f10f650d514df88b76d25e83db360142c7b174` |
| `subprojects/dxbc-spirv` | `bf14419e5fa7eacb817b7b632f03cb61d61bbad7` |
| `subprojects/dxbc-spirv/submodules/spirv_headers` | `c8ad050fcb29e42a2f57d9f59e97488f465c436d` |
| `subprojects/libdisplay-info` | `275e6459c7ab1ddd4b125f28d0440716e4888078` |

Do not substitute current submodule heads when reproducing this baseline.

## 2. Toolchain

Linux x86-64 host prerequisites: Git, curl, tar with xz support, `sha256sum`, `uv`, Python 3, a native C++ compiler for predicate tests, and native glslang/glslangValidator. The measured artifact build used **glslang 16.6.0**, **Python 3.14.9**, **Meson 1.12.1**, and the Python **ninja 1.13.2** package (its binary reported `1.13.2.git.kitware.jobserver-pipe-1`).

Windows cross-toolchain: **llvm-mingw 20261104 UCRT / Clang and LLD 23.1.5**:

```sh
work="$HOME/.cache/dxvk-hd620-build"
mkdir -p "$work"
archive="$work/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz"
curl --fail --location --output "$archive" \
  https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz
printf '%s  %s\n' \
  919380403de87dca3c581c2053230cd298af5e891286628ba0613b7fed370926 \
  "$archive" | sha256sum --check
tar -xJf "$archive" -C "$work"
toolchain="$work/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64"
```

Verify the archive before extracting or executing its tools. Use a separate empty build directory. The build helper uses `uv --no-project` with pinned Meson/Ninja dependencies, avoiding accidental use of a surrounding Python project or unpinned global packages.

## 3. Build and native verification

From the repository root:

```sh
bash tools/hd620-wow/verify.sh
bash tools/hd620-wow/build.sh "$toolchain" "$work/build64"
sha256sum "$work/build64/src/d3d11/d3d11.dll" \
          "$work/build64/src/dxgi/dxgi.dll"
git diff --check
```

`verify.sh` takes no arguments. It compiles/runs **32 native guard-predicate checks** and checks the working-tree diff. It is **not** a GPU integration test. `build.sh TOOLCHAIN_DIR BUILD_DIR` builds release/O3 x86-64 `d3d11.dll` and `dxgi.dll` with the static C++ runtime; it does not install them or launch Wine/Proton.

The helper's crossfile selects the supplied toolchain's `x86_64-w64-mingw32-clang`, `clang++`, `windres`, `llvm-ar`, and `llvm-strip`, with Windows/x86-64/little-endian and `needs_exe_wrapper=true`. The relevant Meson configuration is:

```text
--buildtype=release
-Denable_d3d8=false -Denable_d3d9=false -Denable_d3d10=false
```

Only these targets are needed:

```text
src/d3d11/d3d11.dll
src/dxgi/dxgi.dll
```

Retain source commit/submodule IDs, tool versions, build output, generated crossfile/version header, checksums and the **matching DLL pair**. Do not use a new D3D11 DLL with an older experimental DXGI DLL.

### Exact measured artifacts versus a rebuild

The frozen measured pair has these SHA-256 hashes:

```text
11045e3e90be0542dfb8cdbf152c6d316694ed77841aaf9fc4559e7b0fc1daf1  d3d11.dll
8f94b23a91971a4568972cea6943075a51827666d910397b31bf8597762896b9  dxgi.dll
```

Its generated header said **`DXVK_VERSION "3.1.0"`**, because the shallow scratch checkout fell back to the Meson project version. A normal fork checkout may generate another version string. Compiler/build inputs, generated version strings, PE timestamps and other host details can change binary hashes. A different hash is not proof of behavioral equivalence or of a defective build; identify rebuilt artifacts separately and validate them. Use the frozen pair when reproducing the exact recorded measurement.

The reference prerelease tag is **`hd620-wow-baseline-2026-11-09`**. Check the [release page](https://github.com/birdayz/dxvk/releases/tag/hd620-wow-baseline-2026-11-09) for publication status and the frozen paired-DLL archive; a tag name alone does not prove assets are available. Verify the archive's `SHA256SUMS` and the pair above before use.

## 4. Runtime reproduction and evidence

Follow the [runtime-clone and rollback instructions](README.md#reproduce-without-replacing-the-original-runtime). Back up the existing graphics configuration, then **carefully merge** [baseline.wtf](../../tools/hd620-wow/config/baseline.wtf); do not blindly replace the entire user file.

The supplied launch helper's interface is:

```sh
BRUH_MANGOHUD=1 BRUH_WAYLAND=1 \
  bash tools/hd620-wow/launch-test.sh baseline wow
# Use "bnet" instead of "wow" to launch the ordinary Battle.net entry point.
```

Defaults match the measured setup: prefix `$HOME/Games/battlenet-proton`, runtime clone `$HOME/.local/share/battlenet/GE-Proton11-7-kbl-experimental`, native Wayland enabled, and no forced DXVK binding override. `WOW_PREFIX` and `WOW_PROTON` override those paths. The archived helper adds path checks and conservatively refuses existing user-owned game/launcher/Wine-server processes; it otherwise preserves the measured launch environment. `BRUH_DXVK_CONFIG` accepts an optional explicit DXVK override; leave it empty for the primary baseline. `BRUH_LOG_DELAY` is optional; do not use an automatically started login/loading capture as an in-world benchmark.

This is a **bounded test launcher**, not a permanent replacement: it owns the user unit **`bruh-wow-test.service`** with a **45-minute runtime limit**. Quit normally before that limit; if it expires the test process can be terminated. Never start a second launcher against the same prefix or replace loaded DLLs. Stop only the owned test unit when ending an experiment; do not stop unrelated user services.

For a warmed-up in-world capture with `control=mangohud-%p` enabled:

```sh
python3 tools/hd620-wow/mango-log.py GAME_LINUX_PID 1  # start
# Keep the same scene/camera for the 60-second capture.
python3 tools/hd620-wow/mango-log.py GAME_LINUX_PID 0  # stop, if still active
```

Use the renderer process's native Linux PID. The helper uses Python's standard library; no Python packages are required. MangoHud sends a connection greeting but no command acknowledgement, so confirm fresh CSV output. Preserve CSV and summary together, with settings/backend/binding mode and renderer hashes.

For guard evidence, require fresh startup messages confirming the intended target/build plus live graphics/compute checkpoints. Individual format reports must remain truthful. Distinguish:

1. **Native predicate tests:** passed for this source; they check decision logic without a GPU.
2. **Build/static review:** passed; not a runtime coverage proof.
3. **Observed game scene:** actual rendering and supported format-97 image reads, no abort; limited to the recorded scene.
4. **Final-build GPU positive/negative suite:** **not run at baseline freeze**. In particular, unsupported image and typed-buffer reads have not yet been demonstrated to fail closed on that final build.

The standalone [probes](../../tools/hd620-wow/probes/) exercise image and typed-buffer reads/writes with CPU readback. A negative test is successful only when a fresh positive test establishes guard activation and the intended unsupported test produces the **specific pre-command fail-closed report**. A timeout, unrelated crash, stale log or an environment variable alone is not validation. Do not submit unsupported-read probes through stock/unverified DLLs. Image-derived D3D11 per-format capability answers are not independent proof of buffer support; buffer decisions must use actual Vulkan buffer format features.

## 5. Distribution and notices

The altered build is not an official DXVK, Proton, Intel or Blizzard release. Keep source provenance and mark the modifications plainly. A source or paired-DLL archive must preserve applicable notices, including:

- DXVK's root [LICENSE](../../LICENSE) (zlib/libpng).
- `subprojects/dxbc-spirv/LICENSE` and `subprojects/libdisplay-info/LICENSE` (MIT).
- `include/vulkan/LICENSE.md` and relevant Apache-2.0/MIT SPDX notices.
- Both SPIRV-Headers trees' full `LICENSE` files, including listed exceptions.
- `include/native/directx/COPYING.MinGW-w64.txt` and applicable header notices; these are not all simply MIT.
- The LLVM toolchain's `LICENSE.TXT` (Apache-2.0 WITH LLVM-exception and included notices).
- The toolchain's `x86_64-w64-mingw32/share/mingw32/COPYING.MinGW-w64-runtime.txt`, plus `COPYING.MinGW-w64.txt` and `COPYING.winpthreads.txt` for the statically linked runtime notice bundle.

Distribute only reviewed source, tools, patches, notices, checksums, sanitized measurements and the paired renderer DLLs. **Do not include game files, Wine prefixes, personal/account identifiers, credentials, SSH details or raw authentication/session logs.**
