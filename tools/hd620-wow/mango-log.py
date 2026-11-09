#!/usr/bin/env python3
"""Control the per-process MangoHud layer over its abstract Unix socket."""
import socket
import sys
import time

if len(sys.argv) != 3:
    raise SystemExit("Usage: mango-log.py PID 0|1")
pid, enabled = sys.argv[1:]
if not pid.isdecimal() or enabled not in ("0", "1"):
    raise SystemExit("Usage: mango-log.py PID 0|1")
with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as connection:
    connection.settimeout(5)
    connection.connect("\0mangohud-" + pid)
    greeting = connection.recv(4096)
    if b"MangoHudControlVersion" not in greeting:
        raise SystemExit("Unexpected control socket greeting; no command sent")
    connection.sendall(f":logging={enabled};".encode("ascii"))
    time.sleep(1)
print("MangoHud logging " + ("started" if enabled == "1" else "stopped"))
