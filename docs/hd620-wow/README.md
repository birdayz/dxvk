# Intel HD 620 / WoW: frozen experimental baseline

This is an **altered, unsupported DXVK build**, not an upstream compatibility fix. It gets one tested WoW build past an overly broad capability gate while retaining a fail-closed check for potentially unsupported typed UAV reads. It does **not** implement missing normalized-format UAV reads or provide full D3D11 feature conformance.

**Measured result: 19.3 average FPS, not stable 40 FPS.** Preserve this baseline before making further performance changes.

See [the separate follow-up profiling notes](PROFILING.md) for guarded DXVK 2.7 measurements, quality-setting tradeoffs and the untimed launcher prototype. They do not replace this frozen baseline.

## Scope and mechanism

Upstream base: [`601930949d111edbbcf9dd463948426d9f8f6ddd`](https://github.com/doitsujin/dxvk/commit/601930949d111edbbcf9dd463948426d9f8f6ddd), the DXVK revision shipped in GE-Proton11-7 (v3.1-19). This branch changes eight source files: 244 insertions and six deletions before documentation and tooling.

Both environment switches default **off**:

| Switch | Effect |
| --- | --- |
| `DXVK_KBL_WOW_COMPAT=1` | Override only `D3D11_OPTIONS2.TypedUAVLoadAdditionalFormats`; automatically enable the guard. |
| `DXVK_KBL_WOW_UAV_GUARD=1` | Enable the guard independently, without changing capability reporting. |

The override is restricted to executable basename `Wow.exe`, the real Vulkan adapter ID **8086:5916**, and genuine RG16F/RGBA16F typed-load support. The code does **not** check the game build number: a future update is unvalidated, even if the switch still takes effect. No vendor/device spoofing is required. Individual format capability reports remain unchanged. The aggregate answer is intentionally inaccurate: do not describe it as newly implemented hardware support.

The guard combines shader-static typed-read metadata with the currently bound valid image/texel-buffer views before each user draw/dispatch. It uses actual image-tiling or buffer-specific Vulkan format features, preserves alias read-access unions and maximum descriptor counts, and retains DXVK's genuine explicit-format R32 fallback. An unsupported potential read **aborts the process before recording that command**; it does not silently skip rendering. Dynamic branches and query-only metadata can cause conservative rejection.

Two optimizations retain those checks: immutable format properties are cached per context, and immutable pipeline layouts precompute their typed-read descriptor list. Current bindings are still checked every time; there is no pointer/dirty-state shortcut. This reduces guard work but is not, by itself, a measured FPS improvement.

No game executable, loader, authentication, DRM, or anti-cheat code is changed. Nevertheless, custom graphics libraries in online games carry compatibility/account-policy risks; see the upstream [online multiplayer warning](../../README.md#online-multi-player-games). This project cannot guarantee acceptance by the game operator.

## Recorded measurements

Hardware/software: ThinkPad T470s, Intel HD 620 (8086:5916), Mesa 26.2.6, GE-Proton11-7 with the paired experimental x86-64 DXVK DLLs, and MangoHud 0.8.4. Game: **1.60.1.70862**. The laptop was on AC, with a 1920×1080 desktop; the game was maximized. Do not equate desktop mode with a verified fixed render-target size.

Both captures are 60-second **stationary in-world Dolanaar** samples, not cinematics, combat, raids, or battery tests:

| Capture | Average FPS | 1% low FPS | Average GPU load | Configuration |
| --- | ---: | ---: | ---: | --- |
| Primary baseline | **19.3** | **14.788** | **96.1%** | Native Wayland, default descriptor-heap binding, compatibility CVars below set to 0. |
| Secondary observation | 19.6 | 12.79 | 95.9% | Native Wayland, legacy descriptor binding **and** compatibility CVars restored to defaults (1). |

Primary raw data: [baseline.csv](measurements/baseline.csv), [baseline-summary.csv](measurements/baseline-summary.csv). Secondary: [legacy-defaults.csv](measurements/legacy-defaults.csv), [legacy-defaults-summary.csv](measurements/legacy-defaults-summary.csv).

**The secondary capture is not a strict single-variable comparison.** Binding mode and three CVars changed together; the small difference cannot establish an optimization. Neither capture establishes a stable FPS floor or a causal performance gain over an unguarded working game.

Primary settings:

- D3D11; `PROTON_ENABLE_WAYLAND=1`; default DXVK binding selection, with no forced legacy-binding override.
- `GxPrismEnabled=0`, `GxCompatOptionalGpuFeatures=0`, `GxCompatAllowSM6=0`. These are the measured primary settings, **not requirements for the aggregate workaround**.
- `graphicsQuality=1` (proxy value; displayed quality level 2), `renderScale=0.666667`, `maxFPS=40`, `maxFPSBk=15`, `vsync=0`.
- Preserve the remaining settings from [baseline.wtf](../../tools/hd620-wow/config/baseline.wtf); a quality proxy alone does not reconstruct all individual settings.
- MangoHud sampling interval 100 ms, capture duration 60 seconds, uploads disabled. Exclude login/loading and wait for shader compilation and clocks to settle.

The secondary restored `GxPrismEnabled`, `GxCompatOptionalGpuFeatures`, and `GxCompatAllowSM6` to 1 and disabled both descriptor heap and descriptor buffer. It is retained as provenance, not as a recommended performance win.

## What has actually been verified

- Release build, source/static review, whitespace checks, and **32 native guard-predicate cases passed**.
- The actual game reached the world and completed the reported captures. In that observed scene the guard reported supported **`VK_FORMAT_R16G16B16A16_SFLOAT` (97)** image reads, with no abort.
- An earlier standalone positive half-float GPU diagnostic passed. That is not a final-build negative-guard validation.
- **The comprehensive final-build GPU positive/negative suite has not been run.** In particular, do not claim that final-build unsupported image/buffer reads have been demonstrated to abort on hardware.
- This is not proof for every zone, effect, shader, future game update, or descriptor-binding path. A later scene may deliberately terminate the game through the guard.

The separate diagnostics under [tools/hd620-wow/probes](../../tools/hd620-wow/probes/) contain no game code. A diagnostic named `Wow.exe` is a purpose-built capability/guard test, not a copy of the game. Unsupported-read tests require fresh positive evidence that the intended guard build is active; an environment variable or arbitrary crash alone is insufficient. See [BUILD.md](BUILD.md) for validation boundaries.

## Reproduce without replacing the original runtime

1. Obtain this branch, initialize its pinned submodules, and follow [BUILD.md](BUILD.md), or use the verified paired-DLL artifact. The reference prerelease tag is **`hd620-wow-baseline-2026-11-09`**; check its [release page](https://github.com/birdayz/dxvk/releases/tag/hd620-wow-baseline-2026-11-09) for publication status and assets. Verify the frozen hashes below before use.
2. Stop the test game and its launcher before replacing any runtime DLLs. Never run multiple Proton/umu instances concurrently against this prefix.
3. Keep the original GE-Proton11-7 installation intact. Make a separate full runtime clone named `GE-Proton11-7-kbl-experimental`. Install **both matching x86-64** `d3d11.dll` and `dxgi.dll` only into that clone's `files/lib/wine/dxvk/x86_64-windows/` directory. Reject unexpected symlink destinations rather than following them into the original runtime. Do not place these files in the game directory or mix build pairs.
4. Back up the game's `WTF/Config.wtf` before applying the sanitized baseline settings. The settings fragment is not an account/configuration backup: retain unrelated preferences locally. Never publish the original file.
5. Use [launch-test.sh](../../tools/hd620-wow/launch-test.sh) with the cloned runtime; it supplies the compatibility/guard switches. The interface is `bash tools/hd620-wow/launch-test.sh LABEL wow|bnet`. For the primary capture use `BRUH_MANGOHUD=1 BRUH_WAYLAND=1` and leave `BRUH_DXVK_CONFIG` empty. Check fresh `KBLWOW`/`KBLGUARD` startup evidence and the chosen adapter; do not assume the intended renderer was loaded. Use normal game login yourself; these tools do not supply credentials or automate authentication.
6. Warm up the same stationary scene, then capture with [mango-log.py](../../tools/hd620-wow/mango-log.py). Keep raw CSV, summary, renderer hashes, backend/binding choices and settings together. MangoHud's `control=mangohud-%p` abstract socket permits external start/stop when Wayland hotkeys do not work; the helper uses `:logging=1;` / `:logging=0;`, not the unrelated mangoapp control interface.

The measured frozen DLL pair is:

```text
11045e3e90be0542dfb8cdbf152c6d316694ed77841aaf9fc4559e7b0fc1daf1  d3d11.dll
8f94b23a91971a4568972cea6943075a51827666d910397b31bf8597762896b9  dxgi.dll
```

The helper owns `bruh-wow-test.service` with a **45-minute runtime limit**: this is a bounded experiment, not a permanent launcher. Quit normally before the limit; expiry can terminate the test. `BRUH_LOG_DELAY` is optional, but login/loading samples must not be mistaken for the stationary baseline.

Proton normally manages the corresponding prefix DLLs. Verify the deployed pair after launch; never overwrite files through prefix symlinks. Keep diagnostics private by default and publish only reviewed, sanitized data—not prefixes, account names, credentials, authentication/session logs or game files.

**Rollback:** quit the experiment, launch through the unchanged original GE-Proton11-7 runtime, let Proton restore its managed prefix DLLs, and verify that the stock pair is in use. Restore the saved graphics settings if wanted. The original runtime's GPU eligibility rejection is expected to return: rollback removes this workaround, not the original compatibility problem. Deleting the clone is optional and should only happen while it is unused.

## Performance hill-climb after the frozen baseline

Keep the baseline commit/artifacts unchanged. Put each experiment on a separate commit/branch, and change **one variable** at a time:

1. Use the same game build, position, camera, visible scene, AC state, desktop/window mode and quality settings. Record thermal state and shader-cache/warmup conditions.
2. Warm up each condition, then use **A/B/A** ordering with at least **three 60-second captures per condition**. Recheck A after B to detect thermal or scene drift. Do not include login/loading in a sample.
3. Compare average FPS, 1% lows and frame-time behavior across repeats, not one favorable screenshot. **Keep repeatable gains, including 1–2 FPS**: 2 FPS is about 10% at this baseline. There is no 20% minimum improvement threshold. Use matched return comparisons to distinguish small gains from noise, check for low-percentile/frame-time or stability regressions, and explicitly label any image-quality tradeoff. Stable 40 FPS remains the goal, not a prerequisite for retaining an improvement.
4. Record all settings and rejected experiments. Any reduction in render scale, detail, resolution, view distance or visual features must be labeled as a quality trade-off, not a same-quality renderer optimization.
5. Retain guard coverage and investigate any fail-closed report; never remove the guard to improve a benchmark. A driver/device error or an unexplained exit is not a successful negative test.

Candidate directions include native Wayland versus XWayland, a separately built guarded DXVK 2.7.x compiler path, and D3D12 through a **separate vkd3d-proton experiment**. None has a measured result in this frozen baseline. DXVK itself does not translate D3D12. No stable-40-FPS claim is made.
