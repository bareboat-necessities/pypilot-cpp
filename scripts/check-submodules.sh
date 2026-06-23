#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"

cmake_modules=(
  pypilot-event-loop
  pypilot-settings
  pypilot-mdns
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
  pypilot-runtime
)

checkout_only_modules=(
  ocean-imu
)

check_git_checkout() {
  local name="$1"
  local dir="$M/$name"
  test -d "$dir"
  git -C "$dir" rev-parse --is-inside-work-tree >/dev/null
  echo "$name -> $(git -C "$dir" rev-parse --short=12 HEAD)"
}

for name in "${cmake_modules[@]}"; do
  check_git_checkout "$name"
  test -f "$M/$name/CMakeLists.txt"
done

for name in "${checkout_only_modules[@]}"; do
  check_git_checkout "$name"
done
