# HD620 follow-up: GPU bottleneck and small gains

Measurements on 2026-11-09, after the immutable `hd620-wow-baseline-2026-11-09` release. **Stable 40 FPS has not been achieved.** Repeatable 1–2 FPS gains are useful; there is no 20% minimum improvement threshold.

## Runtime and limits

These follow-up measurements use the separately built, guarded [DXVK 2.7.1 candidate](https://github.com/birdayz/dxvk/tree/hd620-wow-dxvk27), source commit `5f1bad4ff8950c86dbfc410f022aacc1e6220b53`, with XWayland and the same Proton/prefix as the baseline. This documentation branch still contains the baseline 3.1 source, **not** the 2.7 source. The scoped compatibility override and fail-closed bound-resource guard remain enabled. The 2.7 RGBA16F compute/readback smoke test passed; comprehensive negative GPU coverage remains incomplete.

Frozen candidate DLL SHA-256:

- `d3d11.dll`: `87fa406ac066ca3033198a4e0e46103a5ae86967e6dfaa9f86e4042dba8e7638`
- `dxgi.dll`: `a7f9bb3511d7e3947f5281bdd2141c8d9f162370f77ec383e4fc4bbcb47849a9`

A quiet 3.1 XWayland sample averaged 19.2 FPS; 2.7 samples averaged 20.0 and 20.3 FPS. This was **not a complete matched A/B/A**, and the second 2.7 sample may overlap an expired screen-awake inhibitor. A causal 2.7 advantage is not established. The quality-setting gains below must not be attributed to changing DXVK versions.

## Where the time goes

WoW's built-in `/console LogFps 0.1` produced approximately one row per second. Repeating `/console LogFps` stopped it. The initial [engine CSV](measurements/dxvk27-engine.csv), 13:59:44–14:04:34 CET, contains 282 rows:

| Mean | Value |
|---|---:|
| FPS | 20.135 |
| Frame time | 49.678 ms |
| CPU frame time | 13.384 ms |
| GPU render time | 46.474 ms |
| CPU wait for GPU | 36.170 ms |
| IsGpuBound | 1 in every row |

These timings overlap; do not add CPU time to GPU time. Intel PMU sampling independently showed Render/3D at 100% busy, about 977 MHz actual versus 1050 MHz maximum/requested, and approximately 10.8 W GPU / 16.4 W package power. WoW accounted for approximately 97% of GPU activity and KWin approximately 1.7%. Memory is dual-channel DDR4-2133; swap use was zero. This supports a genuine GPU-side bottleneck, rather than a hidden 20-FPS limiter or a saturated CPU producer thread. It does **not** distinguish shader ALU, texture sampling, geometry, bandwidth or GPU synchronization costs.

## Resolution and quality experiments

Stationary character in the Dolanaar inn, looking towards the bridge. This is an online scene with other players entering/leaving, not a deterministic replay. Rendering changes were followed by warm-up; command-entry and transition periods were excluded from the engine comparison windows. See the complete [tuning CSV](measurements/dxvk27-tuning.csv) and [explicit comparison windows](measurements/dxvk27-tuning-summary.json). Window filtering compares timestamps truncated to `HH:MM:SS` and includes both endpoint seconds in full.

Initially `GetPhysicalScreenSize()` returned 1280×720 while the X window and actual Vulkan swapchain were 1920×1080. Thus that getter alone was not reliable evidence of the actual output dimensions.

Setting `gxMaximize=0`, `gxWindowedResolution=1280x720`, followed by `/console gxRestart`, produced a **verified 1280×720 X window and swapchain**. At `renderScale=0.666667`, the game then reported `GetCurrentScaledResolution()` = **853×480**, window size 1280×720 and DPI scale 1. Output reduction alone barely changed FPS. Internal resolution before that restart was not independently verified; this is not a proven constant-internal-pixel-count comparison.

Other effective settings: `MSAAQuality=0`, `OutlineEngineMode=0`, `ResampleAlwaysSharpen=0`. The low quality preset had left **`ffxAntiAliasingMode=4` and `ResampleQuality=3` (FSR)** enabled.

| Same 720p output, 853×480 internal | Mean FPS | Mean GPU time |
|---|---:|---:|
| AA4 + FSR | 20.30 | 46.84 ms |
| AA0 + FSR | 21.20 | 44.80 ms |
| AA4 + FSR, return | 20.68 | 46.01 ms |
| AA0 + FSR, repeat | 21.34 | 44.60 ms |
| AA0 + bilinear (`ResampleQuality=1`) | 22.66 | 42.22 ms |
| AA0 + FSR, return | 21.25 | 44.74 ms |
| AA0 + bilinear, later repeat | 22.58 | 42.12 ms |

These short return comparisons support keeping the small improvements, with a clear **image-quality tradeoff**: disabling AA exposes edges; bilinear upscaling is softer than FSR. No same-quality speedup is claimed. Differences in scene activity and power/clock behavior limit precision.

A separate [60-second MangoHud capture](measurements/dxvk27-tuned.csv) with AA0 + bilinear averaged **22.4 FPS**, 1% low **13.77 FPS**, average GPU load **98.5%** ([summary](measurements/dxvk27-tuned-summary.csv)). A screenshot was taken just after logging started, so this capture is not perfectly undisturbed; the engine windows and return comparisons are the primary evidence for direction of change, not the screenshot. The low-percentile result is not evidence of a stable 40-FPS floor.

A temporary `FlipCameraYaw(180)` showed the inn interior at about 20.25 FPS with AA0 + FSR; `FlipCameraYaw(-180)` restored the original view. No movement or saved camera slots were used. This did not isolate a blank view, so it does not rule out scene complexity.

## Deeper profiling status

An attempted Mesa `INTEL_MEASURE` shader-timing run exited with code 3 before entering the world and produced no usable timing rows. Do not treat it as a successful GPU attribution capture or as an FPS result. The normal uninstrumented runtime was restored, and login/world rendering worked again. Such GPU timestamp instrumentation is intrusive; any resulting FPS would not be comparable to ordinary gameplay.

A later **successful Mesa u_trace capture in the actual world** identified dominant compute work. It required an explicit `MESA_GPU_TRACEFILE`: the game's stdout was `/dev/null`, so default trace output had disappeared. Configuration: `MESA_GPU_TRACES=print`, `INTEL_GPU_TRACEPOINT=-all,+frame,+render_pass,+compute,+compute_indirect,+blorp`. Independent begin/end timestamp pairing found **104 complete frames**, mean 44.7754 ms. Partial boundary frames were excluded; no complete-frame pairing errors occurred.

| Driver-input SPIR-V source hash | Mean ms/frame | Calls/frame |
|---|---:|---:|
| `0x079a7036abfc4b22` | 7.8594 | 1 direct |
| `0x0580ca7030f598e7` | 7.2212 | 1 indirect |
| `0x859e6b0efc6439f6` | 4.7795 | 1 direct |

These three jobs total **19.86 ms, about 44% of the instrumented frame**. All compute totals approximately 28.03 ms (63%). Render-pass timing averages 14.58 ms; BLORP timing is nested and must not be added as independent work. The three jobs occur together before the main 853×480 world pass, not at the final bilinear upscale. Direct group counts are 256×128×1, **not proven texture dimensions**; the indirect `0×0×0` metadata is an unknown-size placeholder, not zero work. Instrumented timings are not ordinary FPS benchmarks.

Readback-guided single-variable probes found `clusteredShading=1`, `forceClusteredShading=0`, `volumeFog=1`, `volumeFogLevel=0`, `shadowMode=0`, `shadowTextureSize=1024`, and `shadowNumCascades=1`. Setting clustered shading to 0 left the top three jobs present (45.16 ms baseline → 44.63 ms off → 45.24 ms restored); setting volume fog to 0 also left them present (44.68 ms). These short captures do not establish a useful speedup or exclude features requiring a restart. Both enable values were restored to 1. In particular, fog quality level 0 was not evidence that fog was disabled. See [aggregate diagnostic timings](measurements/dxvk27-compute-summary.json).

The remaining question is the identity and reducible cost of those compute jobs. No barrier removal, shader substitution, unsupported-read suppression, or guard bypass is justified by these timings. Raw game-derived shader dumps, screenshots and private access material are not included here.

## Ordinary experimental launcher

[`launch.sh`](../../tools/hd620-wow/launch.sh) is an explicit opt-in ordinary launcher, unlike the 45-minute diagnostic `launch-test.sh`. It has **no session timeout**, refuses an already running game/launcher/Wine server, retains the guard, and never edits the stock runtime. Its default is the prepared baseline runtime; set `WOW_PROTON` to a separately prepared candidate runtime to test 2.7. This is a prototype, not an installed desktop replacement: run `bash tools/hd620-wow/launch.sh bnet` for Battle.net or `bash tools/hd620-wow/launch.sh wow` for direct login from the repository root. The Battle.net mode relies on the game's existing D3D11 setting; only direct `wow` mode adds `-d3d11`. This does not install or validate the DLL pair, fix unsupported shader formats, or guarantee compatibility in unvisited scenes. Keep the stock launcher separately available for rollback.
