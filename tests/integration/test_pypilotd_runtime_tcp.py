#!/usr/bin/env python3
import os
import re
import signal
import socket
import subprocess
import sys
import time
from typing import List


PORT_RE = re.compile(r"runtime server listening on port (\d+)")


def read_startup_line(proc: subprocess.Popen, timeout_s: float = 10.0) -> str:
    deadline = time.monotonic() + timeout_s
    lines: List[str] = []
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            remaining = proc.stderr.read() if proc.stderr else ""
            if remaining:
                lines.append(remaining)
            raise RuntimeError(
                f"pypilotd exited early with code {proc.returncode}; stderr:\n{''.join(lines)}"
            )
        if not proc.stderr:
            raise RuntimeError("pypilotd stderr pipe is missing")
        line = proc.stderr.readline()
        if line:
            lines.append(line)
            if PORT_RE.search(line):
                return line
        else:
            time.sleep(0.05)
    raise TimeoutError(f"timed out waiting for pypilotd startup; stderr:\n{''.join(lines)}")


def recv_until(sock: socket.socket, needle: bytes, timeout_s: float = 5.0) -> bytes:
    deadline = time.monotonic() + timeout_s
    data = bytearray()
    while time.monotonic() < deadline:
        sock.settimeout(max(0.1, deadline - time.monotonic()))
        try:
            chunk = sock.recv(4096)
        except socket.timeout:
            continue
        if not chunk:
            break
        data.extend(chunk)
        if needle in data:
            return bytes(data)
    raise TimeoutError(f"did not receive {needle!r}; received {bytes(data)!r}")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: test_pypilotd_runtime_tcp.py /path/to/pypilotd", file=sys.stderr)
        return 2

    pypilotd = sys.argv[1]
    env = os.environ.copy()
    env["PYPILOTD_RUNTIME_HOST"] = "127.0.0.1"
    env["PYPILOTD_RUNTIME_PORT"] = "0"

    proc = subprocess.Popen(
        [pypilotd],
        stdin=subprocess.DEVNULL,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        env=env,
    )

    try:
        line = read_startup_line(proc)
        match = PORT_RE.search(line)
        if not match:
            raise RuntimeError(f"startup line did not include runtime port: {line!r}")
        port = int(match.group(1))
        if port <= 0:
            raise RuntimeError(f"invalid assigned runtime port: {port}")

        with socket.create_connection(("127.0.0.1", port), timeout=5.0) as sock:
            sock.sendall(b"values\n")
            values_reply = recv_until(sock, b"values={")
            if b"server.version" not in values_reply or b"ap.enabled" not in values_reply:
                raise AssertionError(f"values catalog missing expected entries: {values_reply!r}")

            sock.sendall(b"watch={\"server.version\":0}\n")
            watch_reply = recv_until(sock, b"server.version=")
            if b"pypilot-cpp" not in watch_reply:
                raise AssertionError(f"server.version watch did not report pypilot-cpp: {watch_reply!r}")

            sock.sendall(b"ap.enabled=false\n")

    finally:
        if proc.poll() is None:
            proc.send_signal(signal.SIGTERM)
            try:
                proc.wait(timeout=5.0)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=5.0)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
