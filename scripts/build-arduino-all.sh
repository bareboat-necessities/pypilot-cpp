#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"
FQBN="${PYPILOT_ARDUINO_FQBN:-arduino:avr:mega}"

COMMON_LIBS=(
  --libraries "$M/pypilot-event-loop"
  --libraries "$M/pypilot-syslib"
  --libraries "$M/pypilot-data-model"
  --libraries "$M/pypilot-servo-protocol"
  --libraries "$M/pypilot-client-protocol"
  --libraries "$M/pypilot-algorithms"
  --libraries "$M/pypilot-boatimu"
  --libraries "$M/pypilot-nmea0183-connector"
  --libraries "$M/pypilot-signalk-connector"
  --libraries "$M/pypilot-sensors"
  --libraries "$M/pypilot-gps-adapter"
  --libraries "$M/pypilot-pilots-logic"
  --libraries "$M/pypilot-steering-signaling"
)

arduino_compile() {
  local sketch="$1"
  if [ -d "$sketch" ]; then
    arduino-cli compile --fqbn "$FQBN" "${COMMON_LIBS[@]}" "$sketch"
  else
    echo "Skipping missing Arduino sketch: $sketch"
  fi
}

arduino_compile "$M/pypilot-event-loop/examples/arduino/EventLoopSmoke"
arduino_compile "$M/pypilot-event-loop/examples/arduino/SerialByteStreamEcho"
arduino_compile "$M/pypilot-event-loop/examples/arduino/DatagramStreamExample"
arduino_compile "$M/pypilot-event-loop/examples/arduino/PinEventExample"
arduino_compile "$M/pypilot-event-loop/examples/arduino/InterruptPinEventExample"
arduino_compile "$M/pypilot-syslib/examples/arduino/SyslibLoggingExample"
arduino_compile "$M/pypilot-data-model/examples/arduino/DataModelExample"
arduino_compile "$M/pypilot-algorithms/examples/arduino/AlgorithmsExample"
arduino_compile "$M/pypilot-boatimu/examples/arduino/BoatImuSamplesExample"
arduino_compile "$M/pypilot-sensors/examples/arduino/SensorsExample"
arduino_compile "$M/pypilot-gps-adapter/examples/arduino/GpsAdapterExample"
arduino_compile "$M/pypilot-pilots-logic/examples/arduino/PilotsLogicExample"
arduino_compile "$M/pypilot-steering-signaling/examples/arduino/SteeringExample"
