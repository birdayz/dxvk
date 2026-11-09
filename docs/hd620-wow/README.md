# Experimental HD620 WoW guard for DXVK 2.7.1

This is an altered, unsupported DXVK experiment, not an official upstream release.
It does **not** implement missing normalized-format typed UAV loads or establish
that the game is safe in every scene. Both opt-ins are off by default.

Upstream: <https://github.com/doitsujin/dxvk>, tag `v2.7.1`.
Peeled base commit: `c3dd74be6baec53786d4e064a572185b70347a17`.
This independent branch is `hd620-wow-dxvk27` at
<https://github.com/birdayz/dxvk>; it does not replace the separate 3.1 baseline.
See [BUILD.md](BUILD.md) for the measured build's provenance and reproduction.

## Scope and behavior

- `DXVK_KBL_WOW_COMPAT=1` applies only to the exact executable name `Wow.exe`
  and the real Vulkan Intel HD620 identity `8086:5916`. The aggregate override
  additionally requires real D3D11 typed UAV load support for RG16F and RGBA16F.
  It changes only the successful `CheckFeatureSupport(D3D11_OPTIONS2)` returned
  copy's `TypedUAVLoadAdditionalFormats` field. This is an intentionally
  inaccurate aggregate response, not implementation of unsupported formats.
- Individual format queries, internal device features, feature levels and shader
  instructions remain unchanged. Unsupported normalized formats do not acquire
  a real implementation.
- Compatibility automatically enables the guard. The separate
  `DXVK_KBL_WOW_UAV_GUARD=1` enables guard-only diagnostics on the same exact
  executable/device, without overriding the aggregate response.
- Before recording each user draw/dispatch, the guard checks current valid bound
  storage-image and storage-texel-buffer views whose shader metadata permits a
  read. Image tiling features and buffer features are checked separately; the
  basic explicit-format R32 fallback is retained. A potentially unsupported read
  logs its binding and **aborts before the offending draw/dispatch is recorded**.
  There is no emulation, silent dropped draw or recovery promise.
- Immutable format properties are cached per context, and typed-read descriptor
  lists are flattened when pipeline bindings are built. Current resource views
  are still checked for each draw/dispatch. Flags are assumed fixed at launch.
- The existing 2.7 shader compiler already unions access analysis per UAV
  register. The separate 3.1 alias-metadata compiler change is not ported here.
  Existing class-linkage/resource-indexing limitations remain.

This is conservative metadata-based checking, not proof that a texel load will
execute: dynamic branches and query-only UAV use can produce false positives.
No full negative-path, adversarial shader, format/tiling, or all-scene GPU
coverage is claimed. Do not disable the guard to work around an abort.

Two existing build-compatibility fixes are included: an explicit `<algorithm>`
include, and copying the already empty pipeline-library key into the emplacement
constructor tuple for modern libc++. Neither adds rendering features.

## Evidence and limitations

Historical validation of the frozen build:

- Release x86-64 D3D11/DXGI build succeeded; 32 native guard predicate checks
  passed. Predicate checks are not end-to-end GPU negative-path coverage.
- The positive RGBA16F typed-UAV GPU probe passed on the target HD620.
- The game reached a rendered world at approximately 20 FPS in the tested scene.
  Similar results with 3.1 do not establish a DXVK-version performance gain.
- Later quality tradeoffs (AA disabled and bilinear resampling) yielded roughly
  22.4 FPS in the overlay and 22.6 FPS in the engine's display. Those are settings
  changes, **not evidence that switching to 2.7 caused the improvement**. They
  are not a controlled universal benchmark or a stable-40-FPS claim.

Keep the matching x86-64 DLL pair together when comparing builds. Hold scene,
camera, resolution, graphics settings, display backend and guard flags constant.
For a deliberate legacy-binding comparison, 2.7 supports
`DXVK_CONFIG="dxvk.enableDescriptorBuffer = False"`; descriptor heaps are not
implemented in 2.7. Native Wine Wayland and X11/XWayland are both possible
underlying Win32-WSI choices; this patch requires neither one.

No binaries, game resources, credentials, account data or raw runtime logs are
included in this source branch. No new build or GPU test was performed merely
to publish it. The upstream license and pinned dependency licenses continue to
apply; any future binary distribution must retain the relevant notices.
