# Public source provenance and reproduction protocol

## Exact source and patch

- Official archive: <https://archive.mesa3d.org/mesa-26.2.6.tar.xz>.
- Archive SHA256: `b9436b3e1a8be76fbd4823dc884f90596ab7c6ac5e1867a305e66ffccfb31a92`.
- Tag: `mesa-26.2.6`; peeled commit `8e87a503ebd91408c88596d1b9eeb5f4c0b2088d`.
- Patch SHA256: `9a7d0a06498c376a245a763a2e85c2cbbce798f4ace6258761e02ff958744c4d`.

Pristine archive-file SHA256s, verified before applying the patch:

| Relative source file | SHA256 |
|---|---|
| src/intel/dev/intel_debug.c | `3afe32cc9789c164e29edea7c5a26c4fca4450509d6c5fa32ef82a1f1208fd1f` |
| src/intel/dev/intel_debug.h | `fe604cc2fdee38447f2d8e4eda806dc6aac2ce59cb2bff256e3d9c0f8ee58b8b` |
| src/intel/compiler/brw/brw_compiler.c | `787ef3a192776559defeb5e1fd37f402a57a3b610bf7501af05887a8bd44f4cf` |
| src/intel/compiler/brw/brw_shader.cpp | `f21dcbc68aefa935584ec8345120b93f224ca23a0fd59b1789e62886021ebbc0` |

The first two files carry full MIT-family notices; the compiler files identify MIT via SPDX. Their original notices are retained separately. Other Mesa components can have different licenses; this bundle does not relicense Mesa as a whole.

## Build environment observed

GCC 16.2.1, LLVM/Clang 23.1.4, SPIRV-Tools 2026.4.1; generic x86-64 CPU flags, no LTO. Private Python tooling was managed with `uv`: Meson 1.12.1, Ninja 1.13.2, Mako 1.4.3, MarkupSafe 3.0.4, PyYAML 6.0.3 and packaging 26.3.

Mesa 26.2.6 ANV requires the CLC build helpers, so **LLVM cannot simply be disabled**. An initial `-Dllvm=disabled` configuration failed with `CLC requires LLVM`. Matching LLVMSPIRVLib must be available; the observed helper was official Arch spirv-llvm-translator 23.1.3-1, privately extracted after detached-signature verification using the public distribution keyring, not system-installed. Its package SHA256 was `a3e838d4a916dbd19432409dd6c3887ca7e6683178d0f7837fbceb3d7890f797`; its .pc reported compatible version 23.1.0.0.

Provision normal development dependencies or verified private equivalents before configuring. These include libdrm, zlib/zstd, expat, udev, XCB/X11-xcb/xshmfence, Wayland, libdisplay-info, LLVM/clang-cpp, SPIRV-Tools and the matching translator. This ANV-only/no-OpenGL/no-Gallium/no-Intel-tools configuration does not require flex/bison. Do not modify source capabilities to work around a missing dependency.

## Recorded runtime/test context

The target hardware class was Intel Kaby Lake GT2 / HD Graphics 620. Validation used UMU 1.4.4-1, steamrt4 4.0.20261021.265997 and pressure-vessel 0.20261007.0. The actual-application comparison used a GE-Proton11-7-derived private runtime with the same experimental guarded DXVK 2.7.1+ x64 DLLs in all three runs; no runtime binaries are included here.

Observed patched driver SHA256: `36104d4e859887905f6b961209d15e34c7347f128849f555ad81c14f6f65bdf8`. Recorded x64 D3D11/DXGI SHA256s were respectively `87fa406ac066ca3033198a4e0e46103a5ae86967e6dfaa9f86e4042dba8e7638` and `a7f9bb3511d7e3947f5281bdd2141c8d9f162370f77ec383e4fc4bbcb47849a9`. They are identity metadata, not distributed binaries, source licenses or a claim of bit-reproducibility on arbitrary hosts.

Actual mappings, imported manifests/configurations and selected environments were checked rather than assuming that a version string or launch environment proved the loaded implementation. Private identity paths and raw evidence are intentionally omitted.

## Isolated build example

Choose an owned, disk-backed scratch directory in `WORK` and the directory containing this bundle in `BUNDLE`. These are operator-selected paths, not supplied host paths. Do not build/install over a system Mesa or working game runtime.

```sh
: "${WORK:?Choose an owned scratch directory}"
: "${BUNDLE:?Choose this bundle directory}"
mkdir -p "$WORK"
curl --fail --location https://archive.mesa3d.org/mesa-26.2.6.tar.xz \
  --output "$WORK/mesa-26.2.6.tar.xz"
printf '%s  %s\n' \
  b9436b3e1a8be76fbd4823dc884f90596ab7c6ac5e1867a305e66ffccfb31a92 \
  "$WORK/mesa-26.2.6.tar.xz" | sha256sum -c -
tar -xJf "$WORK/mesa-26.2.6.tar.xz" -C "$WORK"
git -C "$WORK/mesa-26.2.6" apply --check "$BUNDLE/cs-schedule-eval.patch"
git -C "$WORK/mesa-26.2.6" apply "$BUNDLE/cs-schedule-eval.patch"
git -C "$WORK/mesa-26.2.6" apply --reverse --check "$BUNDLE/cs-schedule-eval.patch"
UV_CACHE_DIR="$WORK/uv-cache" uv venv --python python3 "$WORK/tooling"
UV_CACHE_DIR="$WORK/uv-cache" uv pip install --python "$WORK/tooling/bin/python" \
  meson==1.12.1 ninja==1.13.2 mako==1.4.3 MarkupSafe==3.0.4 \
  PyYAML==6.0.3 packaging==26.3
```

If using a private translator extraction, adjust only its copied LLVMSPIRVLib.pc prefix and scope its pkg-config/library directories to the build. Do not edit system metadata or export its library path into a game. The following Bash example assumes required dependencies already resolve. If necessary, set HELPER_PREFIX to the private extraction's prefix containing lib/pkgconfig and lib; those paths are scoped to build commands only.

```bash
build_env=(env -u INTEL_DEBUG -u INTEL_SIMD_DEBUG -u ANV_DEBUG \
  -u LD_LIBRARY_PATH -u LD_PRELOAD -u LD_AUDIT \
  "PATH=$WORK/tooling/bin:$PATH" PYTHONDONTWRITEBYTECODE=1 \
  CC=cc CXX=c++ 'CFLAGS=-march=x86-64 -mtune=generic' \
  'CXXFLAGS=-march=x86-64 -mtune=generic' LDFLAGS=)
if [[ -n ${HELPER_PREFIX:-} ]]; then
  build_env+=("PKG_CONFIG_PATH=$HELPER_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}" \
    "LD_LIBRARY_PATH=$HELPER_PREFIX/lib")
fi
"${build_env[@]}" meson setup "$WORK/build" "$WORK/mesa-26.2.6" \
  --prefix="$WORK/stage" --libdir=lib --buildtype=release --wrap-mode=nofallback \
  '-Dgallium-drivers=[]' -Dvulkan-drivers=intel -Dplatforms=x11,wayland \
  -Dopengl=false -Dglx=disabled -Degl=disabled -Dgbm=disabled \
  -Dgles1=disabled -Dgles2=disabled -Dllvm=enabled -Dshared-llvm=enabled \
  -Dbuild-tests=false '-Dtools=[]' '-Dvulkan-layers=[]' -Dperfetto=false \
  -Dintel-rt=disabled -Dintel-elk=false -Db_lto=false
"${build_env[@]}" ninja -C "$WORK/build" -j2
"${build_env[@]}" meson install -C "$WORK/build" --no-rebuild
```

The observed build completed 1,093 steps at two jobs. Optional shader-stat XML schema validation was skipped because lxml/rnc2rng were absent; four upstream Expat deprecated-function warnings occurred. That is not a passed schema/shader-correctness test. No compiler/linker error occurred.

`intel-elk=false` excludes the pre-Gen9 compiler, not Kaby Lake BRW. Kaby Lake has no hardware RT; controls used the same `intel-rt=disabled` option. This ANV-only build is **not a universal full-feature distribution Mesa replacement**. Toolchain/dependency differences can change ELF output; this procedure is not a claim of bit-reproducible binaries on arbitrary hosts.

## Bounded checks and runtime protocol

Before considering an explicitly approved runtime test:

1. Inspect the patch scope. Only four files change. The flag starts unset/zero; only compute stages with generation <30 broaden first-pass evaluation. The same five candidates and fallback remain. A static model checked 70 flag/stage/generation combinations; the compiler configuration uses 60 of 64 bits after cache-key inclusion.
2. Build successfully, check standard ICD exports, and inspect the finished ELF with `readelf -dW`, `readelf -VW` and clean-environment `ldd -r`. No RPATH/RUNPATH or LLVM/clang/LLVMSPIRV runtime dependency was observed. Important runtime dependencies include unversioned SPIRV-Tools and libdisplay-info; a filename alone does not prove symbol/API compatibility.
3. Keep the original driver/runtime intact. Use a new private per-process ICD prefix, never a system ICD directory. A relocated manifest must resolve the intended library; retain the matching unmodified drirc files and select them through DRIRC_CONFIGDIR. Preserve and review existing custom configuration rather than silently discard it.
4. Verify the actual device/driver, imported closure and format properties. For pressure-vessel, supply the host ICD as input; inspect the rewritten in-container variables/manifest and actual providers. Do not overwrite the rewritten paths inside Wine or blindly inject host libraries. Native success alone does not prove container or actual-game providers.
5. For an approved actual application test, verify actual mapped driver and D3D DLL hashes, intended debug mode, no forced SIMD, matching drirc and active fail-closed guard. Do not publish process environments or raw logs. Keep identities/paths out of reports.
6. Use the same patched build for OFF/ON/OFF. Warm each mode separately and exclude compilation. The new cache-key bit separates modes; never delete shared caches. Diagnostic compilation statistics use `INTEL_DEBUG=cs` versus `cs,cs-schedule-eval`; timing instrumentation must be identical across the three captures. Ordinary FPS measurements should omit tracing/compiler dumps and must not be conflated with the recorded intrusive traces.
7. Record native SIMD widths, instructions, loops, spills/fills, sends, scheduler and GRFs for each hot kernel. A width change is a confound, not an isolated scheduler result. Estimated compiler cycles and send counts are not measured dispatch time; send counts exclude spill/fill traffic.
8. Reject corruption, validation errors, guard rejection, device loss, unexplained exits or missing identity evidence. A separate original integer-only synthetic A/B/A passed six exact readbacks per run, but it does not cover floating point, typed images or proprietary shader behavior. This bundle intentionally includes no test binaries or shader bodies.

Recorded native and native-pressure-vessel checks passed on the target hardware class, including genuine per-format RG16F/RGBA16F reads while the legacy global read-without-format feature remained false. The actual application A/B/A showed **no observed performance gain**, so the experiment remained rejected/default OFF.
