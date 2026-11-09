# Opt-in BRW compute scheduling experiment — rejected, default OFF

**No performance gain was observed. The experiment was rejected and must remain OFF.** This is a reproducible research record, not a recommended driver, deployment instruction or claimed game fix.

The 55-line patch targets official Mesa **26.2.6**. On compute shaders and pre-Xe3 hardware, the explicit `INTEL_DEBUG=cs-schedule-eval` flag allows the existing first pre-register-allocation loop to evaluate its five existing scheduling candidates. Unset retains the stock scheduling path. The flag is included in the compiler configuration/cache key.

The patch adds no scheduling algorithm, recursive retry, application/shader-hash matching, SIMD forcing, math changes, capability overrides, barrier disabling or guard bypass. Existing dependency analysis, register allocation and fallback machinery remain in place. Native SIMD policy is unchanged, although different allocation success could in principle change an automatically selected width; record widths rather than assume they remain equal. They did remain equal in the measured three hot kernels.

## Recorded result

Same patched driver, same guarded experimental DXVK 2.7.1+ runtime, stationary same-view live-scene A/B/A intrusive GPU traces. **Online player activity varies; this is not deterministic replay and the workload is not immutable.** Driver/debug configuration was controlled, not the full live application workload:

| Run | Experiment | Complete frames | Mean frame time |
|---|---|---:|---:|
| A1 | OFF | 415 | 44.977907 ms |
| B | ON | 418 | 45.182857 ms |
| A2 | OFF | 421 | 45.227752 ms |

B was **0.177% slower than the arithmetic midpoint of the off runs**. Off endpoints drifted by 0.555%. This is no observed gain, not evidence of a statistically significant regression. These are trace-capture times, **not ordinary FPS benchmark results**.

One hot kernel changed top-down -> latency-sensitive -> top-down. Its selected width, instruction/send counts, register count and zero spills/fills were unchanged. The other two retained their schedules/statistics. There was no observed default spilling to fix.

A separate original **integer-only** synthetic kernel passed six exact CPU-golden readbacks per mode across off/on/off (18 readbacks total). That is limited integer coverage, not floating-point, typed-image, game shader or full compiler correctness coverage, and not a performance measurement.

## Contents

- `cs-schedule-eval.patch`: unchanged public-source patch.
- `REPRODUCING.md`: source provenance, build configuration and bounded validation protocol.
- `RESULTS.md`: complete aggregate interpretation and native-stat table.
- `aggregates.json`: all per-hash trace aggregates for the three captures, frame/event summaries, three hot native compiler summaries and limited synthetic results.
- `LICENSE-NOTICES.md`: retained upstream copyright/license notices.
- `SHA256SUMS`: bundle integrity checks.

Only opaque shader/source hashes and statistical metadata are supplied. **No shader inputs/bodies, SPIR-V, native assembly, raw logs/traces, frame timestamp streams, executables, screenshots, usernames, account data, host paths, device UUIDs or device identity paths are included.** Hashes do not permit reproducing the private application workload; its inputs are intentionally absent. The public Mesa build is reproducible independently.

The measured runtime used an explicitly experimental aggregate D3D11 typed-UAV compatibility override with a fail-closed per-command guard. Normalized typed-UAV reads were **not implemented**; real per-format capabilities remained truthful. This bundle neither implements that compatibility layer nor claims full/conformant normalized typed-UAV support. No baseline fork branch/tag is modified or superseded by this record.

Retain `LICENSE-NOTICES.md` when copying the patch/docs. Respect the destination repository's documentation/data licensing policy and the source files' MIT terms. Publication itself requires the repository owner's separate review; this bundle performs no commit, push or fork modification.
