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

### Shader identification: semantic metadata only

A cold-cache diagnostic captured the actual driver-input SPIR-V, matched its BLAKE3 identity to the trace hash, and checked the embedded application-shader identity. The resulting semantic classification is:

| Trace source hash | Local workgroup size | Aggregate semantic evidence |
|---|---|---|
| `0x079a7036abfc4b22` | 8×8×1 | Temporal, angular-weighted RGB / probe-atlas integration. |
| `0x0580ca7030f598e7` | 32×1×1 | Indirect software traversal of packed BVH geometry, with ray/slab calculations and geometry/material sampling. |
| `0x859e6b0efc6439f6` | 8×8×1 | Octahedral-direction, exponential distance / irradiance integration with history. |

Together these strongly support a **DDGI-like dynamic diffuse global-illumination / ray-and-probe update family**. The exact engine feature name and a supported disabling control remain unproven. These are application compute jobs, not the final bilinear upscale. Work-count and task-sentinel early exits mean the fixed 256×128 direct dispatch dimensions do not imply that every group updates an active probe.

Only semantic descriptions, identities and aggregate metrics are published here. Captured modules and derived disassembly remain private; no shader contents, assembly, screenshots or private access material are included. No barrier removal, shader substitution, dispatch skipping, unsupported-read suppression or guard bypass is justified.

### Startup controls and GI quality

The normal graphics profile (index 0) was verified active, with `RAIDsettingsEnabled=0` and normal `giQuality=0`. That confirms the selected profile/quality value, **not that the GI-like work is disabled**. A read-only CVar registry name filter for `^gi`, `illum` or `probe` returned only `giQuality`; this limited search is not proof that every possible control has been exhausted, and it does not justify inventing negative or undocumented values.

A live `linear=0` change and a graphics restart did not remove the three jobs. A subsequent **full process restart with saved `linear=0`** still reported `C_VideoOptions.IsLinearEnabledOnStart()` as true, with all three hot jobs present. Thus the saved CVar did not establish that the startup rendering mode had actually changed. The graphics-restart sample was additionally contaminated by cold compilation and is not a steady-state FPS comparison. The [sanitized startup and SIMD aggregates](measurements/dxvk27-simd-linear-summary.json) retain the complete-frame timings without raw trace or shader payloads.

### SIMD compiler diagnostics: return comparison

Same-scene u_trace windows followed **default A → cs8 → cs16 → default A return**, comparing the ordinary compiler choice with forced compute SIMD widths. All values below are **instrumented GPU milliseconds per complete frame**, not normal FPS measurements:

| Compiler selection | Complete frames | Frame interval | RGB / atlas job | Indirect BVH job | Distance / history job |
|---|---:|---:|---:|---:|---:|
| Default (`simd-default-a`) | 421 | 45.3148 | 7.8683 | 7.1192 | 4.9293 |
| `INTEL_SIMD_DEBUG=cs8` (`simd8-a`) | 424 | 48.2554 | 8.4891 | 6.9340 | 6.4824 |
| `INTEL_SIMD_DEBUG=cs16` (`simd16-a`) | 420 | 56.6748 | 7.8751 | 18.9275 | 4.6732 |
| Default return (`simd-default-b`) | 423 | 45.1593 | 7.9111 | 7.2023 | 4.9484 |

Both forced-width samples had longer total frame intervals. In particular, forcing SIMD16 increased the indirect BVH interval from about 7.12 to 18.93 ms. Filtered compiler statistics for that SIMD16 shader reported 6,959 instructions, four loops, 149 spills / 511 fills and 128 GRF registers. Those are compiler statistics, not dynamic execution counts or hardware-utilization counters; they support investigating register pressure but do not by themselves establish the entire slowdown's cause.

The default return was within **0.16 ms** of the initial default mean. See the [sanitized four-capture aggregate](measurements/dxvk27-simd-linear-summary.json) for counts, ranges, event totals and the three shader intervals. This was **not a perfectly controlled causal comparison**: live-player activity varied, and Firefox was opened idle behind the game for the cs16 and return captures, but not for the initial default or cs8 captures. The return comparison supports **rejecting both forced widths**, not a precisely isolated effect size or an ordinary-gameplay FPS claim. The normal launcher clears `INTEL_SIMD_DEBUG` and `ANV_DEBUG`; the compiler's default selection remains the ordinary choice.

## Ordinary experimental launcher

[`launch.sh`](../../tools/hd620-wow/launch.sh) is an explicit opt-in ordinary launcher, unlike the 45-minute diagnostic `launch-test.sh`. It has **no session timeout**, refuses an already running game/launcher/Wine server, retains the guard, and never edits the stock runtime. Used directly, its default is the prepared baseline runtime; set `WOW_PROTON` to the intended candidate. From the repository root, use `bash tools/hd620-wow/launch.sh bnet` or `bash tools/hd620-wow/launch.sh wow`. Battle.net mode relies on the game's existing D3D11 setting; only direct `wow` mode adds `-d3d11`.

The reusable [per-user installer](../../tools/hd620-wow/installer/README.md) adds a separate experimental icon and a wrapper pinned to the verified 2.7 pair, existing prefix and measured XWayland / legacy-descriptor choice. It reuses the same helper rather than maintaining a duplicate. It verifies runtime DLL hashes at installation and each launch, refuses external DLL symlinks or existing destination files, and leaves the stock launcher/icon/runtime untouched. Per-user installation on the target system passed the pair check and Desktop Entry validation, with the stock launcher's checksum unchanged. A separate shortcut is now also installed in the XDG desktop folder, without replacing the stock icon. A real post-install handover then invoked the **installed wrapper in `bnet` mode** through a user unit with `RuntimeMaxUSec=infinity`: normal Battle.net startup and entry into the same Dolanaar world succeeded. A fresh `KBLWOW 2.7.1` warning confirmed the compatibility override and enabled guard. Filtered child-environment checks confirmed the pinned 2.7 runtime, both guard/compatibility flags set to 1, `PROTON_ENABLE_WAYLAND=0` and FPS-only MangoHud; `INTEL_SIMD_DEBUG`, `ANV_DEBUG`, `INTEL_DEBUG` and `MESA_GPU_TRACES` were absent. The desktop icon itself was **not double-clicked**; its installed command path was exercised directly, and Desktop Entry syntax was validated separately. This establishes real startup/world entry through the installed command path; incidental HUD readings are not treated as a measurement.

### Ordinary installed-wrapper capture

A subsequent [engine CSV](measurements/dxvk27-handover-engine.csv) and [numeric summary](measurements/dxvk27-handover-engine-summary.json) contain **67 approximately one-second rows over 67.475 seconds**, captured through the actual installed Battle.net flow. The character was stationary in the same scene, Battle.net was minimized, and Firefox was idle behind the game. Mesa GPU tracing and forced SIMD overrides were absent.

| Arithmetic mean of engine-reported rows | Value |
|---|---:|
| FPS | **22.028** |
| Frame time | 45.390 ms |
| CPU frame time | 15.696 ms |
| GPU render time | 43.004 ms |
| CPU wait for GPU | 29.573 ms |
| IsGpuBound | 1 in every row |

These are **per-second engine aggregates, not per-frame samples or a frame-time 1% low**. CPU/GPU timings overlap and must not be added. This ordinary handover capture is distinct from the intrusive SIMD traces; it does not establish a stable frame-rate floor, an isolated DXVK-version speedup, or stable 40 FPS. Live-scene activity and the recorded background applications limit comparisons with earlier windows.

Ordinary mode remains FPS-only; intentional 60-second MangoHud captures belong in the separate diagnostic workflow. No SIMD/ANV experiment, raw trace or shader capture is enabled by the permanent icon. Neither helper nor installer implements unsupported shader formats or guarantees unvisited scenes. Keep the stock launcher separately available for rollback.
