#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
B="$ROOT/build"
BUILD_DIR="$B/pypilot-cpp"

if [ ! -d "$BUILD_DIR" ]; then
  echo "pypilot-cpp build directory does not exist: $BUILD_DIR" >&2
  echo "Run scripts/build-linux-all.sh before integration tests." >&2
  exit 1
fi

if [ ! -x "$BUILD_DIR/pypilotd" ]; then
  echo "pypilotd executable does not exist or is not executable: $BUILD_DIR/pypilotd" >&2
  echo "Run scripts/build-linux-all.sh before integration tests." >&2
  exit 1
fi

echo "Registered pypilot-cpp integration tests:"
ctest --test-dir "$BUILD_DIR" -N -R '^pypilotd_runtime_tcp_integration$'

if ! ctest --test-dir "$BUILD_DIR" -N -R '^pypilotd_runtime_tcp_integration$' | grep -q 'pypilotd_runtime_tcp_integration'; then
  echo "pypilot-cpp did not register pypilotd_runtime_tcp_integration" >&2
  exit 1
fi

test_count="$(ctest --test-dir "$BUILD_DIR" -N -R '^pypilotd_runtime_tcp_integration$' | awk '/Total Tests:/ {print $3}')"
if [ -z "$test_count" ] || [ "$test_count" -ne 1 ]; then
  echo "Expected exactly one pypilotd_runtime_tcp_integration test, got: ${test_count:-unset}" >&2
  exit 1
fi

echo "Running pypilotd_runtime_tcp_integration with verbose output:"
ctest --test-dir "$BUILD_DIR" -V -R '^pypilotd_runtime_tcp_integration$'
