# Standalone diagnostics (not game code)

`guard-predicate-test.cpp` is compiled and run by `../verify.sh`. Its 32 cases are native decision-logic checks, not GPU integration coverage.

`uav-selftest.c` is a standalone Windows D3D11 compute/readback diagnostic. The source is retained for follow-up validation; the comprehensive final-build GPU suite was **not run** before freezing the 19.3-FPS baseline. Do not interpret its presence as a passing test report.

The probe intentionally uses `mainCRTStartup` with direct Win32 output and no C runtime. Compile with the pinned llvm-mingw cross-compiler, `-nostdlib`, entry point `mainCRTStartup`, and import libraries `kernel32`, `d3d11`, `d3dcompiler`. Keep the resulting executable in a dedicated diagnostics directory, never in the game directory. Name that standalone executable `Wow.exe` only to activate the exact-basename guard scope; it is not the actual game.

Environment inputs:

- `BRUH_UAV_RESULT`: writable Windows output-file path (for example a Wine `Z:` path to a fresh private diagnostics directory).
- `BRUH_UAV_CASE`: `rh` (RGBA16F image read), `wh` (RGBA16F image write), `wb` (RGBA8 image write), `rb` (RGBA8 image read). Append `b` for typed-buffer equivalents (`rhb`, `whb`, `wbb`, `rbb`).

Run only with the correct paired experimental DLLs and `DXVK_KBL_WOW_UAV_GUARD=1`, an idle prefix, fresh per-case directories, disabled core dumps, and a short process timeout. Before any unsupported-read case, establish fresh positive readback plus `KBLGUARD enabled`, `KBLGUARD live checkpoint=compute`, and first-read evidence for that exact build. On the target HD 620 the unsupported RGBA8 image/buffer read cases must produce the specific `KBLGUARD FAIL-CLOSED` message **before** `Map completed result` or `PASS completed GPU readback`. An arbitrary crash, timeout or old log does not count. Do not run negative cases through stock or unverified DLLs.

The probe covers direct compute only. Even a fully passing run would not establish every graphics/indirect/alias/descriptor scenario or whole-game compatibility.
