#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"
B="$ROOT/build"
TOTAL_TESTS=0

cmake_build_test() {
  local name="$1"
  shift
  local source_dir="$M/$name"
  local build_dir="$B/$name"

  echo "::group::Build and test $name"
  test -d "$source_dir"
  test -f "$source_dir/CMakeLists.txt"

  cmake -S "$source_dir" -B "$build_dir" "$@"
  cmake --build "$build_dir" --parallel

  echo "Registered tests for $name:"
  ctest --test-dir "$build_dir" -N

  local test_count
  test_count="$(ctest --test-dir "$build_dir" -N | awk '/Total Tests:/ {print $3}')"
  if [ -z "$test_count" ]; then
    echo "Could not determine CTest count for $name" >&2
    exit 1
  fi
  if [ "$test_count" -le 0 ]; then
    echo "$name registered zero CTest tests" >&2
    exit 1
  fi

  TOTAL_TESTS=$((TOTAL_TESTS + test_count))
  ctest --test-dir "$build_dir" --output-on-failure
  echo "::endgroup::"
}

bash "$ROOT/scripts/bootstrap-modules.sh"
bash "$ROOT/scripts/check-submodules.sh"

cmake_build_test pypilot-event-loop
cmake_build_test pypilot-settings
cmake_build_test pypilot-mdns
cmake_build_test pypilot-syslib
cmake_build_test pypilot-data-model
cmake_build_test pypilot-servo-protocol
cmake_build_test pypilot-client-protocol

cmake_build_test pypilot-runtime \
  -DPYPILOT_EVENT_LOOP_DIR="$M/pypilot-event-loop" \
  -DPYPILOT_SETTINGS_DIR="$M/pypilot-settings" \
  -DPYPILOT_MDNS_DIR="$M/pypilot-mdns"

cmake_build_test pypilot-algorithms \
  -DPYPILOT_SYSLIB_DIR="$M/pypilot-syslib/src"

cmake_build_test pypilot-boatimu \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src"

cmake_build_test pypilot-nmea0183-connector \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src"

cmake_build_test pypilot-signalk-connector \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src" \
  -DPYPILOT_MDNS_DIR="$M/pypilot-mdns"

cmake_build_test pypilot-sensors \
  -DPYPILOT_DATA_MODEL_DIR="$M/pypilot-data-model/src" \
  -DPYPILOT_ALGORITHMS_DIR="$M/pypilot-algorithms/src" \
  -DPYPILOT_NMEA0183_CONNECTOR_DIR="$M/pypilot-nmea0183-connector/src" \
  -DPYPILOT_SIGNALK_CONNECTOR_DIR="$M/pypilot-signalk-connector/src" \
  -DPYPILOT_MDNS_DIR="$M/pypilot-mdns/src" \
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

echo "Total registered module CTest tests: $TOTAL_TESTS"
