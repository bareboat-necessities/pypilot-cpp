#!/usr/bin/env bash
set -euo pipefail

git submodule status --recursive

test -d modules/pypilot-event-loop
test -d modules/pypilot-syslib
test -d modules/pypilot-data-model
test -d modules/pypilot-servo-protocol
test -d modules/pypilot-client-protocol
test -d modules/pypilot-algorithms
test -d modules/pypilot-boatimu
test -d modules/pypilot-nmea0183-connector
test -d modules/pypilot-signalk-connector
test -d modules/pypilot-sensors
test -d modules/pypilot-gps-adapter
test -d modules/pypilot-pilots-logic
test -d modules/pypilot-steering-signaling
