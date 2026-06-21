#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"
mkdir -p "$M"

clone_or_update() {
  local name="$1"
  local url="https://github.com/bareboat-necessities/${name}.git"
  local dir="$M/$name"

  if git -C "$dir" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    echo "Updating $name"
    git -C "$dir" fetch --depth 1 origin main
    git -C "$dir" checkout main
    git -C "$dir" reset --hard origin/main
  else
    echo "Cloning $name"
    rm -rf "$dir"
    git clone --depth 1 --branch main "$url" "$dir"
  fi

  echo "$name -> $(git -C "$dir" rev-parse --short=12 HEAD)"
}

clone_or_update pypilot-event-loop
clone_or_update pypilot-settings
clone_or_update pypilot-syslib
clone_or_update pypilot-data-model
clone_or_update pypilot-servo-protocol
clone_or_update pypilot-client-protocol
clone_or_update pypilot-algorithms
clone_or_update pypilot-boatimu
clone_or_update pypilot-nmea0183-connector
clone_or_update pypilot-signalk-connector
clone_or_update pypilot-sensors
clone_or_update pypilot-gps-adapter
clone_or_update pypilot-pilots-logic
clone_or_update pypilot-steering-signaling
clone_or_update pypilot-runtime

git -C "$ROOT" status --short
