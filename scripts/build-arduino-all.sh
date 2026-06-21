#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
M="$ROOT/modules"
FQBN="${PYPILOT_ARDUINO_FQBN:-arduino:avr:mega}"

COMMON_LIBS=(
  --libraries "$M/pypilot-event-loop"
  --libraries "$M/pypilot-settings"
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
  --libraries "$M/pypilot-runtime"
)

ESP32_LIBS=(
  "${COMMON_LIBS[@]}"
  --libraries "$M/pypilot-mdns"
)

arduino_compile() {
  local sketch="$1"
  if [ -d "$sketch" ]; then
    echo "::group::Arduino compile $sketch"
    arduino-cli compile --fqbn "$FQBN" "${COMMON_LIBS[@]}" "$sketch"
    echo "::endgroup::"
  else
    echo "Skipping missing Arduino sketch: $sketch"
  fi
}

arduino_compile_esp32() {
  local sketch="$1"
  if [ -d "$sketch" ]; then
    echo "::group::Arduino compile ESP32 $sketch"
    arduino-cli compile --fqbn "$FQBN" "${ESP32_LIBS[@]}" "$sketch"
    echo "::endgroup::"
  else
    echo "Skipping missing Arduino sketch: $sketch"
  fi
}

bash "$ROOT/scripts/bootstrap-modules.sh"
bash "$ROOT/scripts/check-submodules.sh"

arduino_compile "$M/pypilot-event-loop/examples/EventLoopTimerExample"
arduino_compile "$M/pypilot-event-loop/examples/LineProtocolExample"
arduino_compile "$M/pypilot-event-loop/examples/FixedHeaderProtocolExample"
arduino_compile "$M/pypilot-event-loop/examples/PinEventExample"
arduino_compile "$M/pypilot-settings/examples/SettingsExample"
arduino_compile "$M/pypilot-syslib/examples/arduino/SyslibLoggingExample"
arduino_compile "$M/pypilot-data-model/examples/arduino/DataModelExample"
arduino_compile "$M/pypilot-algorithms/examples/arduino/AlgorithmsExample"
arduino_compile "$M/pypilot-boatimu/examples/arduino/BoatImuSamplesExample"
arduino_compile "$M/pypilot-sensors/examples/arduino/SensorsExample"
arduino_compile "$M/pypilot-gps-adapter/examples/arduino/GpsAdapterExample"
arduino_compile "$M/pypilot-pilots-logic/examples/arduino/PilotsLogicExample"
arduino_compile "$M/pypilot-steering-signaling/examples/arduino/SteeringExample"

if [[ "$FQBN" == esp32:* ]]; then
  arduino_compile_esp32 "$M/pypilot-mdns/examples/MdnsExample"
  arduino_compile_esp32 "$M/pypilot-runtime/examples/RuntimeServerExample"
else
  echo "Skipping pypilot-mdns MdnsExample for non-ESP32 target: $FQBN"
  echo "Skipping pypilot-runtime RuntimeServerExample for non-ESP32 target: $FQBN"
fi
