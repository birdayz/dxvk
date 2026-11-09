#!/usr/bin/env bash
# Explicit, time-limited experimental DXVK test; normal desktop launcher is unchanged.
set -euo pipefail
umask 077
tag=${1:?diagnostic label required}
entry=${2:-wow}
[[ "$tag" =~ ^[a-z0-9-]+$ ]]
[[ "$entry" == wow || "$entry" == bnet ]]
if systemctl --user is-active --quiet bruh-wow-test.service; then
  echo 'Previous owned test still running; stop it before starting another.' >&2
  exit 1
fi
# Conservatively refuse any existing game/launcher/Wine server owned by this user.
# Do not kill unrelated applications or race a second launcher into the prefix.
if pgrep -u "$UID" -x 'Wow.exe|Battle.net.exe|Agent.exe|wineserver' >/dev/null; then
  echo 'Close existing game/launcher/Wine processes before starting this test.' >&2
  exit 1
fi
base="$HOME/.cache/battlenet/diagnostics"
log="$base/$tag"
mkdir -p "$log/fps"
prefix="${WOW_PREFIX:-$HOME/Games/battlenet-proton}"
proton="${WOW_PROTON:-$HOME/.local/share/battlenet/GE-Proton11-7-kbl-experimental}"
[[ -d "$prefix/drive_c" && -f "$proton/proton" ]] || {
  echo 'Existing game prefix and prepared experimental Proton clone are required.' >&2
  exit 1
}
if [[ "$entry" == bnet ]]; then
  command=("$prefix/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe")
else
  command=("$prefix/drive_c/Program Files (x86)/World of Warcraft/_forever_/Wow.exe" -d3d11)
fi
systemd-run --user --collect --unit=bruh-wow-test --property=RuntimeMaxSec=45min \
  --property=UMask=0077 --property=LimitCORE=0 \
  --property="StandardOutput=append:$log/umu.log" \
  --property="StandardError=append:$log/umu.log" \
  --setenv="WINEPREFIX=$prefix" \
  --setenv="PROTONPATH=$proton" \
  --setenv=GAMEID=umu-default --setenv=STORE=battlenet \
  --setenv="PROTON_ENABLE_WAYLAND=${BRUH_WAYLAND:-1}" --setenv=PROTON_USE_XALIA=0 \
  --setenv=PROTON_LOG=0 --setenv=WINEDEBUG=-all \
  --setenv='WINEDLLOVERRIDES=winemenubuilder.exe=d;locationapi=d' \
  --setenv=DXVK_LOG_LEVEL=info --setenv="DXVK_LOG_PATH=$log" \
  --setenv="DXVK_CONFIG=${BRUH_DXVK_CONFIG:-}" \
  --setenv=DXVK_KBL_WOW_COMPAT=1 --setenv=DXVK_KBL_WOW_UAV_GUARD=1 \
  --setenv="MANGOHUD=${BRUH_MANGOHUD:-0}" \
  --setenv="MANGOHUD_CONFIG=fps,frametime,frame_timing,gpu_stats,cpu_stats,gpu_core_clock,cpu_mhz,position=top-left,toggle_hud=Shift_R+F12,toggle_logging=Shift_L+F2,control=mangohud-%p,autostart_log=${BRUH_LOG_DELAY:-0},log_interval=100,log_duration=60,output_folder=$log/fps,permit_upload=0" \
  /usr/bin/umu-run "${command[@]}"
printf 'Diagnostic directory: %s\n' "$log"
