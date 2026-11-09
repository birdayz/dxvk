# Per-user guarded HD620 / DXVK 2.7 launcher

This installer creates a separate, explicitly **EXPERIMENTAL** application entry
and optional desktop shortcut. It preserves the stock Battle.net command/icon,
original Proton runtime and existing game prefix. It does not build a renderer,
copy DLLs into a runtime/prefix, supply credentials or start an application.
Gameplay has **no timeout**. This remains an unsupported guarded graphics build,
not an official compatibility fix or an all-scene safety guarantee.

The installer reuses the single ordinary helper [../launch.sh](../launch.sh).
Keep that file and this directory together; there is no duplicate helper here.
After installation, all dependencies are copied to persistent per-user locations,
so the source checkout or installation media can be removed.

## Prerequisites and install

Use the normal desktop account, not root. Quit the game and Battle.net from its
tray menu before installing or switching runtimes. The existing prepared runtime
must contain the matching **measured guarded DXVK 2.7** DLL pair, not the stock
pair or separate 3.1 baseline. No installation packages are downloaded.

From the repository root:

```sh
RUNTIME_27=/absolute/path/to/the/existing/measured-2.7-runtime-clone
bash tools/hd620-wow/installer/install.sh \
  "$RUNTIME_27" "$HOME/Games/battlenet-proton"
```

The prefix argument is optional; it defaults to the path shown. Runtime, prefix
and installed dependencies must not live under `/tmp`, `/var/tmp` or `/run`.
The installer checks both application paths and these existing runtime files:

```text
files/lib/wine/dxvk/x86_64-windows/d3d11.dll
files/lib/wine/dxvk/x86_64-windows/dxgi.dll
```

[DLL-SHA256SUMS](DLL-SHA256SUMS) pins the frozen matching pair:

```text
87fa406ac066ca3033198a4e0e46103a5ae86967e6dfaa9f86e4042dba8e7638  d3d11.dll
a7f9bb3511d7e3947f5281bdd2141c8d9f162370f77ec383e4fc4bbcb47849a9  dxgi.dll
```

A mismatch, missing application, DLL symlink outside the runtime, or existing
installation destination aborts the operation. Existing files are not silently
overwritten: back up and review an earlier experimental installation before an
explicit replacement. Do not delete or overwrite stock launchers to make room.

Dependencies are Bash, standard coreutils, `flock`, `pgrep`, and `umu-run` for
actual launches. `desktop-file-validate` is used during installation when present;
`notify-send` supplies optional failure notifications. Missing `umu-run` produces
an installation warning and is a hard failure when launching.

## Installed paths and launch choices

With default XDG locations:

```text
~/.local/bin/wow-hd620-experimental
~/.local/libexec/wow-hd620-experimental/launch.sh
~/.local/libexec/wow-hd620-experimental/DLL-SHA256SUMS
~/.config/wow-hd620-experimental/runtime.conf
~/.local/share/applications/wow-hd620-experimental.desktop
~/.local/share/icons/hicolor/scalable/apps/wow-hd620-experimental.svg
```

The configuration contains canonical local paths, not credentials. XDG_CONFIG_HOME
and XDG_DATA_HOME are respected. The installed wrapper pins the selected runtime
and prefix, checks the DLL pair on every launch, and uses the measured XWayland /
legacy-descriptor choice with compatibility and guard flags enabled. Ambient
runtime, WineD3D or SIMD/ANV diagnostic settings must not silently replace that
ordinary configuration. The original generic helper still supports explicit
`WOW_PROTON` overrides when used separately for experiments.

```sh
"$HOME/.local/bin/wow-hd620-experimental" bnet
"$HOME/.local/bin/wow-hd620-experimental" wow
```

- **Main icon/default:** Battle.net. Use normal interactive login/update/play.
  Its child game relies on the existing D3D11 game setting; the installer does
  not edit that setting or authentication configuration.
- **Secondary desktop action:** WoW direct login, passing `-d3d11`. It does not
  depend on a temporary session helper or supply login information.
- Ordinary mode is **FPS-only**, toggled with right Shift+F12. Use the separate
  [bounded diagnostic helper](../launch-test.sh) for an intentional MangoHud
  measurement; do not enable auto-capture or raw profiling on the permanent icon.
- Busy checks may also block unrelated user-owned Wine applications. They never
  kill them. Quit existing Wine applications before switching runtimes.
- Check a fresh `KBLWOW 2.7.1` warning on the first actual handover launch. Runtime
  checksums alone do not prove which managed prefix DLLs Wine loaded. Investigate
  any mismatch; never overwrite prefix symlinks or put DLLs in the game directory.
- The private ordinary log directory is
  `~/.cache/battlenet/hd620-experimental/`. One previous launch log is retained.
  Do not publish raw launcher/account logs. An error notification includes the
  underlying process status, but the ordinary helper itself returns status 1.

For a literal desktop shortcut, if the desktop folder is enabled:

```sh
APP="${XDG_DATA_HOME:-$HOME/.local/share}/applications/wow-hd620-experimental.desktop"
DESKTOP=$(xdg-user-dir DESKTOP)
if test -d "$DESKTOP" && test "$DESKTOP" != "$HOME" && \
   test ! -e "$DESKTOP/wow-hd620-experimental.desktop" && \
   test ! -L "$DESKTOP/wow-hd620-experimental.desktop"; then
  install -m 755 "$APP" "$DESKTOP/wow-hd620-experimental.desktop"
else
  printf 'Desktop folder unavailable or shortcut already exists; nothing copied.\n' >&2
fi
```

Mark only this new shortcut trusted if KDE asks, or drag the new menu entry onto
the desktop. Leave the existing stock icon alone. The included SVG is original
generic artwork, not a game screenshot or extracted resource.

## Validation and limits

Per-user installation on the target system verified the frozen DLL pair,
unchanged stock-launcher checksum and a valid Desktop Entry. A separate shortcut
was also installed in the XDG desktop folder, without replacing the stock icon.
A real post-install test invoked the installed wrapper's `bnet` mode through an
untimed user unit (`RuntimeMaxUSec=infinity`). Battle.net startup and game-world
entry succeeded, with a fresh `KBLWOW 2.7.1` warning and both guard/compatibility
flags enabled. Filtered child-environment checks confirmed the pinned 2.7 runtime,
XWayland, FPS-only MangoHud and no SIMD/ANV/Intel-debug or u_trace overrides.
The actual desktop icon was not double-clicked: this validates its installed
command path, not the complete GUI click path. Startup success is not a new FPS
benchmark or an all-scene compatibility guarantee.

Local fixture tests also checked paths containing spaces, argument forwarding,
resistance to ambient runtime/backend overrides, refusal of changed DLLs and
external symlinks, preservation of stock files, and refusal of existing targets.
No real application was launched by those fixture tests.

There is no game configuration, prefix, binary, shader dump or credential in
this installer. Do not bypass a guard abort. See the
[profiling notes](../../../docs/hd620-wow/PROFILING.md) for performance evidence,
image-quality tradeoffs and incomplete GPU validation boundaries.
