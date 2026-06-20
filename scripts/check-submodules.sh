#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"

modules=(
  pypilot-event-loop
  pypilot-syslib
  pypilot-data-model
  pypilot-servo-protocol
  pypilot-client-protocol
  pypilot-algorithms
  pypilot-boatimu
  pypilot-nmea0183-connector
  pypilot-signalk-connector
  pypilot-sensors
  pypilot-gps-adapter
  pypilot-pilots-logic
  pypilot-steering-signaling
)

git -C "$ROOT" submodule status --recursive || true

for name in "${modules[@]}"; do
  dir="$M/$name"
  test -d "$dir"
  test -f "$dir/CMakeLists.txt"
  git -C "$dir" rev-parse --is-inside-work-tree >/dev/null
  echo "$name -> $(git -C "$dir" rev-parse --short=12 HEAD)"
done
