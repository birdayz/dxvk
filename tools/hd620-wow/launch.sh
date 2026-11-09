#!/usr/bin/env bash
# Explicit experimental launcher. Unlike launch-test.sh, sessions have no time limit.
set -euo pipefail
umask 077
entry=${1:-bnet}
[[ $# == 0 ]] || shift
if [[ "$entry" != bnet && "$entry" != wow ]]; then
  printf 'Usage: %s [bnet|wow] [application arguments...]\n' "$0" >&2
  exit 2
fi
fail() {
  printf '%s\n' "$1" >&2
  if command -v notify-send >/dev/null; then
    notify-send 'WoW HD620 experimental' "$1" || true
  fi
  exit 1
}
export WINEPREFIX="${WOW_PREFIX:-$HOME/Games/battlenet-proton}"
export PROTONPATH="${WOW_PROTON:-$HOME/.local/share/battlenet/GE-Proton11-7-kbl-experimental}"
[[ -d "$WINEPREFIX/drive_c" && -f "$PROTONPATH/proton" ]] || fail 'Prepared experimental runtime or game prefix is missing.'
[[ -x /usr/bin/umu-run ]] || fail 'umu-run is missing or not executable.'
for tool in flock pgrep; do
  command -v "$tool" >/dev/null || fail "Required tool is missing: $tool"
done
cache="$HOME/.cache/battlenet/hd620-experimental"
mkdir -p "$cache"
# Pass the lock to umu; the process check also protects against a detached session.
# Never kill an existing application.
exec 9>"${XDG_RUNTIME_DIR:-$cache}/wow-hd620-launch.lock"
flock -n 9 || fail 'An experimental game/launcher session is already running.'
if pgrep -u "$UID" -x 'Wow.exe|Battle.net.exe|Agent.exe|wineserver' >/dev/null; then
  fail 'Close existing Wine applications and quit Battle.net from its tray menu before switching runtimes.'
else
  status=$?
  [[ "$status" == 1 ]] || fail "Could not check running Wine processes (pgrep exit $status)."
fi
export GAMEID=umu-default STORE=battlenet
export PROTON_ENABLE_WAYLAND="${WOW_WAYLAND:-0}" PROTON_USE_XALIA=0
export PROTON_LOG=0 WINEDEBUG=-all
export WINEDLLOVERRIDES='winemenubuilder.exe=d;locationapi=d'
export DXVK_KBL_WOW_COMPAT=1 DXVK_KBL_WOW_UAV_GUARD=1
export DXVK_CONFIG='dxvk.enableDescriptorHeap = False; dxvk.enableDescriptorBuffer = False'
export DXVK_LOG_LEVEL=warn DXVK_LOG_PATH="$cache"
export MANGOHUD=1 MANGOHUD_CONFIG='fps_only,position=top-left,toggle_hud=Shift_R+F12,permit_upload=0'
# Clear diagnostic overrides; the game's own frame cap remains in effect.
unset INTEL_MEASURE INTEL_DEBUG INTEL_SHADER_DUMP_FILTER INTEL_GPU_TRACEPOINT
unset INTEL_SHADER_ASM_READ_PATH INTEL_SHADER_OPTIMIZER_PATH INTEL_SHADER_BIN_DUMP_PATH
unset MESA_GPU_TRACES MESA_GPU_TRACEFILE MESA_SPIRV_DUMP_PATH MESA_SPIRV_READ_PATH
unset MESA_SHADER_CACHE_DISABLE MESA_SHADER_CACHE_DIR DXVK_SHADER_DUMP_PATH
unset DXVK_FRAME_RATE DXVK_CONFIG_FILE DXVK_DEBUG MANGOHUD_CONFIGFILE
unset MESA_VK_WSI_PRESENT_MODE vblank_mode
ulimit -c 0
if [[ "$entry" == bnet ]]; then
  app="$WINEPREFIX/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe"
else
  app="$WINEPREFIX/drive_c/Program Files (x86)/World of Warcraft/_forever_/Wow.exe"
  set -- -d3d11 "$@"
fi
[[ -f "$app" ]] || fail 'Game or Battle.net executable is missing.'
cd "$(dirname "$app")"
# Keep one previous log rather than growing the ordinary-launch log indefinitely.
[[ ! -f "$cache/launcher.log" ]] || mv -f "$cache/launcher.log" "$cache/launcher.previous.log"
if /usr/bin/umu-run "$app" "$@" >"$cache/launcher.log" 2>&1; then
  exit 0
else
  status=$?
  fail "The experimental session exited with code $status. See $cache/launcher.log."
fi
