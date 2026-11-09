# Results — no observed gain; experiment rejected

The default remains OFF. This single stationary same-view live-scene A/B/A record does not justify enabling or deploying the experiment. Online player activity varies; the workload is not immutable and this is not deterministic replay. Driver/debug configuration was controlled, not the full live application workload. All timings below are intrusive GPU-trace measurements, not ordinary FPS.

## Complete-frame aggregates

| Run | Mode | Frames | Mean ms | Min ms | Max ms | Median ms | P95 ms | Sample SD ms |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| A1 | OFF | 415 | 44.977907 | 42.958000 | 49.746250 | 45.044167 | 45.983375 | 0.744190 |
| B | ON | 418 | 45.182857 | 42.695750 | 49.219167 | 45.249916 | 46.181962 | 0.784089 |
| A2 | OFF | 421 | 45.227752 | 43.336250 | 47.151000 | 45.350000 | 46.178500 | 0.674554 |

B is +0.204950 ms (+0.455668%) versus A1 and +0.0800275 ms (+0.177433%) versus the arithmetic off-run midpoint (45.1028295 ms). Off endpoints drift +0.249845 ms (+0.555484%). The midpoint is descriptive, not a confidence interval or drift correction. Consecutive frames are correlated; three captures do not establish statistical significance. No observed improvement was found, and no statistically significant regression is claimed.

Only complete frames are included. Each capture had zero complete-frame pairing errors and one incomplete trailing frame excluded. The parser uses paired timestamps and per-kind stacks; globally reversed output timestamps are not treated as elapsed-time deltas. Overlapping event-kind totals are not additive to whole-frame time.

## Hot dispatch aggregates

Shader/source hashes are opaque statistical identities only. The machine-readable file includes every per-hash aggregate: 18 identities in A1, 20 in B and 18 in A2, with counts, frame presence, totals, per-frame/per-dispatch means, min/max and event kinds. It contains no event stream, frame timestamps, shader inputs or code.

| Driver shader hash | Source SHA1 identity | A1 ms/dispatch | B ms/dispatch | A2 ms/dispatch |
|---|---|---:|---:|---:|
| 0x79a7036abfc4b22 | e2e66a6bb3d4cc162df043f02106d2699a600389 | 7.860290 | 7.975442 | 8.112572 |
| 0x580ca7030f598e7 | dbcbde0edb20ad6d58c0dc0e1c5a3b72110cd0b1 | 5.719706 | 5.695624 | 5.751351 |
| 0x859e6b0efc6439f6 | 940a0c96886f320234330b4d18cb6fa4d43f51ca | 4.938685 | 4.907489 | 4.947643 |

Each listed hot kernel appeared once in every complete frame of each capture. Dispatch timing and whole-frame results vary; do not equate compiler estimates with measured elapsed time or sum overlapping trace event totals.

## Native compiler statistics

These fields were identical across A1/B/A2 for each row:

| Driver shader hash | SIMD | Instructions | Loops | Sends | GRFs | Spills:fills | Promoted constants |
|---|---:|---:|---:|---:|---:|---|---:|
| 0x79a7036abfc4b22 | 16 | 1136 | 0 | 133 | 128 | 0:0 | 10 |
| 0x580ca7030f598e7 | 8 | 3626 | 4 | 146 | 128 | 0:0 | 20 |
| 0x859e6b0efc6439f6 | 16 | 1156 | 0 | 143 | 128 | 0:0 | 6 |

| Driver shader hash | A1 scheduler / estimated cycles | B scheduler / estimated cycles | A2 scheduler / estimated cycles |
|---|---|---|---|
| 0x79a7036abfc4b22 | non-lifo / 8360 | non-lifo / 8360 | non-lifo / 8360 |
| 0x580ca7030f598e7 | non-lifo / 524484 | non-lifo / 524484 | non-lifo / 524484 |
| 0x859e6b0efc6439f6 | top-down / 14000 | latency-sensitive / 14032 | top-down / 14000 |

The changed schedule proves that the flag was effective for one hot kernel. Its estimated cycles increased 14000 -> 14032 -> 14000, while the selected SIMD/instruction/send/register/spill fields stayed equal. No SIMD-width or spill-count confound was observed in these three kernels; this does not imply identical physical register assignment or native instruction order. Sends do not count spill/fill traffic. There was no observed default spilling to fix.

## Identity, guard and native-container checks

Actual application mappings matched the same patched Mesa library and guarded DXVK 2.7.1+ D3D11/DXGI hashes in all three runs. Runtime/descriptor settings were stable; only the experiment token changed after normalizing deliberately separate per-run logging/cache destinations. Actual imported manifests, config-file hashes and native providers were checked. No forced SIMD or runtime bypass was active. The fail-closed per-command guard remained enabled; normalized typed-UAV reads were not implemented or misreported as supported.

Separate native and native-pressure-vessel capability/closure checks passed for the patched driver and both version controls. Genuine per-format float reads were present, while the legacy global read-without-format feature was false. These checks are not rendering or private-shader numerical validation.

## Limited synthetic integer-only reference

A separate original immutable integer kernel (951 SPIR-V words, identity hash 610e6fd96e040088) ran six seed/round cases in each native OFF/ON/OFF mode. All 18 exact readbacks matched CPU goldens; inputs were unchanged, canaries intact and all three exit statuses zero. Every run produced aggregate 7d5418d86d2d49fc. No kernel body, SPIR-V, executable or raw log is included.

Coverage is limited to this one integer workload: not floating point, transcendental operations, typed images, rendering, proprietary shaders, all compiler paths, pressure-vessel dispatch/readback or performance. It does not prove which schedule or instruction stream the synthetic kernel selected.

## Decision and limitations

**Rejected / default OFF.** No observed performance gain, no default spill reduction and no deployment recommendation. The private application inputs are intentionally absent, so this is not a self-contained application benchmark. The public source/patch build and the disclosed statistical calculations can be independently reproduced; exact proprietary-workload replay cannot be reconstructed from hashes. Zero reported errors in these captures and a limited integer test are not all-scene correctness guarantees.
