#!/usr/bin/env bash
# Install only new per-user launcher files. Never modify runtimes, DLLs or prefixes.
set -euo pipefail
umask 077
fail() { printf '%s\n' "$1" >&2; exit 1; }
[[ "$EUID" != 0 ]] || fail 'Run this installer as the desktop user, not root.'
[[ $# == 1 || $# == 2 ]] || fail "Usage: $0 PREPARED_DXVK27_RUNTIME [EXISTING_GAME_PREFIX]"
for tool in realpath sha256sum install mktemp; do
  command -v "$tool" >/dev/null || fail "Required tool is missing: $tool"
done
here=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
helper="$here/../launch.sh"
[[ -f "$helper" ]] || fail 'The sibling ordinary launch.sh helper is missing.'
for file in pinned-launcher.sh DLL-SHA256SUMS wow-hd620-experimental.svg; do
  [[ -f "$here/$file" ]] || fail "Installation payload is missing: $file"
done
runtime=$(realpath -e -- "$1") || fail 'The prepared runtime does not exist.'
prefix=$(realpath -e -- "${2:-$HOME/Games/battlenet-proton}") || fail 'The game prefix does not exist.'
home=$(realpath -e -- "$HOME") || fail 'HOME cannot be resolved.'
config_dir=$(realpath -m -- "${XDG_CONFIG_HOME:-$home/.config}/wow-hd620-experimental")
data_home=$(realpath -m -- "${XDG_DATA_HOME:-$home/.local/share}")
bin_dir="$home/.local/bin"
libexec="$home/.local/libexec/wow-hd620-experimental"
wrapper="$bin_dir/wow-hd620-experimental"
manifest="$libexec/DLL-SHA256SUMS"
config="$config_dir/runtime.conf"
icon="$data_home/icons/hicolor/scalable/apps/wow-hd620-experimental.svg"
desktop="$data_home/applications/wow-hd620-experimental.desktop"
for item in "$runtime" "$prefix" "$config_dir" "$data_home" "$bin_dir" "$libexec"; do
  case "$item" in
    /tmp|/tmp/*|/var/tmp|/var/tmp/*|/run|/run/*)
      fail 'Installed launcher dependencies must be persistent, not under /tmp, /var/tmp or /run.' ;;
  esac
done
# These are literal Desktop Entry paths, not shell expressions. Support spaces,
# but reject uncommon metacharacters rather than generate ambiguous Exec lines.
for item in "$wrapper" "$icon"; do
  case "$item" in
    *$'\n'*|*$'\r'*|*'\'*|*'"'*|*'`'*|*'$'*|*'%'*)
      fail 'The installation home/data path contains unsupported Desktop Entry metacharacters.' ;;
  esac
done
[[ -f "$runtime/proton" ]] || fail 'The selected runtime has no Proton entrypoint.'
[[ -d "$prefix/drive_c" ]] || fail 'The selected prefix has no drive_c directory.'
[[ -f "$prefix/drive_c/Program Files (x86)/Battle.net/Battle.net Launcher.exe" ]] || fail 'Battle.net Launcher.exe is missing from the prefix.'
[[ -f "$prefix/drive_c/Program Files (x86)/World of Warcraft/_forever_/Wow.exe" ]] || fail 'The expected WoW executable is missing from the prefix.'
dll_dir="$runtime/files/lib/wine/dxvk/x86_64-windows"
for dll in d3d11.dll dxgi.dll; do
  [[ -f "$dll_dir/$dll" ]] || fail "The selected runtime is missing $dll."
  resolved=$(realpath -e -- "$dll_dir/$dll") || fail "Cannot resolve $dll."
  [[ "$resolved" == "$runtime/"* ]] || fail "$dll resolves outside the selected runtime. Use an independent prepared clone."
done
if ! (cd "$dll_dir" && sha256sum --status -c "$here/DLL-SHA256SUMS"); then
  fail 'The runtime does not contain the frozen matching guarded DXVK 2.7 DLL pair. Nothing was installed.'
fi
for target in "$libexec/launch.sh" "$manifest" "$wrapper" "$config" "$icon" "$desktop"; do
  [[ ! -e "$target" && ! -L "$target" ]] || fail "Refusing to overwrite an existing file: $target"
done
mkdir -p "$libexec" "$bin_dir" "$config_dir" "$(dirname -- "$icon")" "$(dirname -- "$desktop")"
work=$(mktemp -d "$config_dir/.install.XXXXXX")
trap 'rm -rf -- "$work"' EXIT
{
  printf '# Generated local paths, not credentials. This file pins the runtime.\n'
  printf 'readonly HD620_RUNTIME=%q\n' "$runtime"
  printf 'readonly HD620_PREFIX=%q\n' "$prefix"
  printf 'readonly HD620_LIBEXEC=%q\n' "$libexec"
} >"$work/runtime.conf"
cat >"$work/wow-hd620-experimental.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=Battle.net - HD620 experimental (DXVK 2.7)
Comment=Unsupported guarded HD620 build; stock launcher remains separate
Exec="$wrapper" bnet
Icon=$icon
Terminal=false
StartupNotify=true
Categories=Game;
Actions=WoW;

[Desktop Action WoW]
Name=World of Warcraft - direct login (HD620 experimental)
Exec="$wrapper" wow
Icon=$icon
DESKTOP
if command -v desktop-file-validate >/dev/null; then
  desktop-file-validate "$work/wow-hd620-experimental.desktop"
fi
install -m 700 "$helper" "$libexec/launch.sh"
install -m 600 "$here/DLL-SHA256SUMS" "$manifest"
install -m 700 "$here/pinned-launcher.sh" "$wrapper"
install -m 600 "$work/runtime.conf" "$config"
install -m 644 "$here/wow-hd620-experimental.svg" "$icon"
# Publish the menu entry last, after all of its dependencies are installed.
install -m 644 "$work/wow-hd620-experimental.desktop" "$desktop"
printf 'Verified and pinned the frozen guarded DXVK 2.7 DLL pair.\n'
printf 'Launcher: %s\nMenu entry: %s\nConfiguration: %s\n' "$wrapper" "$desktop" "$config"
printf 'Default action opens Battle.net; the secondary action starts WoW with -d3d11.\n'
printf 'No application was started. No stock launcher, runtime or prefix was modified.\n'
if [[ ! -x /usr/bin/umu-run ]]; then
  printf 'Note: /usr/bin/umu-run is not present here; the launcher will refuse to run without it.\n' >&2
fi
