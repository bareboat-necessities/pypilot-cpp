#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"
B="$ROOT/build"

refresh_event_loop_module() {
  git -C "$ROOT" submodule update --init --remote modules/pypilot-event-loop
}

cmake_build_test() {
  local name="$1"
  shift
  cmake -S "$M/$name" -B "$B/$name" "$@"
  cmake --build "$B/$name" --parallel
  ctest --test-dir "$B/$name" --output-on-failure
}

refresh_event_loop_module

cmake_build_test pypilot-event-loop
cmake_build_test pypilot-syslib
cmake_build_test pypilot-data-model
cmake_build_test pypilot-servo-protocol
cmake_build_test pypilot-client-protocol

cmake_build_test pypilot-algorithms \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib/src"

cmake_build_test pypilot-boatimu \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src"

cmake_build_test pypilot-nmea0183-connector \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src"

cmake_build_test pypilot-signalk-connector \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src"

cmake_build_test pypilot-sensors \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src" \
  -DPYPILOT_ALGORITHMS_DIR="$M/pypilot-algorithms/src" \
  -DPYPILOT_NMEA0183_CONNECTOR_DIR="$M/pypilot-nmea0183-connector/src" \
  -DPYPILOT_SIGNALK_CONNECTOR_DIR="$M/pypilot-signalk-connector/src" \
  -DPYPILOT_SERVO_PROTOCOL_DIR="$M/pypilot-servo-protocol/src" \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib/src"

cmake_build_test pypilot-gps-adapter \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src" \
  -DPYPILOT_ALGORITHMS_DIR="$M/pypilot-algorithms/src" \
  -DPYPILOT_SENSORS_DIR="$M/pypilot-sensors/src" \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib/src"

cmake_build_test pypilot-pilots-logic \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src" \
  -DPYPILOT_ALGORITHMS_DIR="$M/pypilot-algorithms/src" \
  -DPYPILOT_SERVO_PROTOCOL_DIR="$M/pypilot-servo-protocol/src" \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib/src"

cmake_build_test pypilot-steering-signaling \
  -DPYPILOT_SERVO_PROTOCOL_DIR="$M/pypilot-servo-protocol" \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib"
