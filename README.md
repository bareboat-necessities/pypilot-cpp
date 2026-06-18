# pypilot-cpp

Umbrella repository for the modular C++ pypilot port.

This repository does not duplicate module code. It pins the current module set as git submodules, documents the build graph, and runs Linux and Arduino builds across the full dependency chain.

## Clone

```bash
git clone --recurse-submodules https://github.com/bareboat-necessities/pypilot-cpp.git
cd pypilot-cpp
```

For an already-cloned tree:

```bash
git submodule update --init --recursive
```

## Updating modules to latest main

Each submodule tracks `main` in `.gitmodules`, but this root repository still pins exact commits for reproducible builds.

```bash
git submodule update --remote --recursive
git add modules
git commit -m "Update pypilot module pins"
```

## Modules

| Module | Role |
| --- | --- |
| `pypilot-syslib` | shared system helpers and logging |
| `pypilot-data-model` | shared pypilot data model |
| `pypilot-servo-protocol` | low-level servo packet protocol |
| `pypilot-client-protocol` | pypilot client/server protocol |
| `pypilot-algorithms` | pure math, filters, GPSFilter, WMM, pilot math |
| `pypilot-nmea0183-connector` | NMEA 0183 parsing and formatting |
| `pypilot-signalk-connector` | Signal K conversion layer |
| `pypilot-sensors` | sensor ingestion, source/device arbitration, data-model writes |
| `pypilot-gps-adapter` | gpsd/raw GPS adapter and GPS-to-sensor bridge |
| `pypilot-pilots-logic` | autopilot mode selection and pilot command logic |
| `pypilot-steering-signaling` | rudder/servo runtime, steering safety, servo signaling |

## Linux build

```bash
./scripts/build-linux-all.sh
```

## Arduino build

```bash
./scripts/build-arduino-all.sh
```

The Arduino build script targets `arduino:avr:mega` by default because it is a broad AVR compile target for CI.

## Dependency order

```text
pypilot-syslib
pypilot-data-model
pypilot-servo-protocol
pypilot-client-protocol
pypilot-algorithms
pypilot-nmea0183-connector
pypilot-signalk-connector
pypilot-sensors
pypilot-gps-adapter
pypilot-pilots-logic
pypilot-steering-signaling
```

## Current project status

Completed module groups include GPS adapter, WMM/GPSFilter, sensor arbitration, APB/NAV command handling, steering signaling, servo runtime, syslib logging, and cross-module logging integration.

Next mainline work is Phase 3: pilot parity and availability.
